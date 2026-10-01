/*
 * ritmo.c - o ritmo do duelo, mestre a mestre, medido de ponta a ponta com o robô perfeito. make ritmo
 * (make ritmo LUTAS=500). Tudo em TEMPO REAL: o tempo do núcleo mais o que o jogo congela nos impactos (hitstop),
 * que é o que o jogador vive. Colunas:
 *   luta            duração da luta (s) e golpes por minuto
 *   abertura        do começo da luta ao primeiro aviso (s)
 *   contato→aviso   do contato de uma sequência ao aviso da seguinte (s): o tempo em que não há nada a reagir
 *   aviso→contato   do aviso do primeiro golpe de uma sequência ao contato dele (s): o aviso que o jogador tem
 *   na sequência    de um contato ao seguinte, dentro de uma sequência (s)
 *   congelado       fração da luta em que o jogo está parado no hitstop
 *
 *   ritmo_relatorio [lutas]       a tabela (Markdown)
 *   ritmo_relatorio --teste       o que tem de valer sempre (make test-ritmo, dentro do make test):
 *       - o aviso do primeiro golpe de uma sequência nunca é menor que 300 ms (a regra: aviso mínimo 300 ms; hoje o piso é 320);
 *       - dentro de uma sequência, dois contatos nunca ficam mais perto que AJ_CADEIA_MIN;
 *       - fora a pausa do selo do oboro, nunca se espera mais de 3 s por um aviso (nada de tempo morto sem fim);
 *       - o hitstop nunca passa de 20% de uma luta.
 */
#include "../src/robo.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PASSO 0.001
#define MAX_AMOSTRAS 200000
#define AVISO_MINIMO 0.300           /* a regra do aviso, em s */
#define ESPERA_MAXIMA 3.0            /* o tempo máximo sem aviso, fora a pausa do selo do oboro */
#define CONGELADO_MAXIMO 0.20

typedef struct {
    double luta, golpes, congelado;
    double abertura, aberturaMax;
    double espera[MAX_AMOSTRAS]; int nEspera;     /* contato → aviso, entre sequências */
    double aviso[MAX_AMOSTRAS]; int nAviso;       /* aviso → contato do primeiro golpe */
    double cadeia[MAX_AMOSTRAS]; int nCadeia;     /* contato → contato, dentro da sequência */
    double esperaMax, avisoMin, cadeiaMin;
    double congeladoMax;
    int lutas;
} Medida;

static int cmp(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }
static double percentil(double *v, int n, double p) {
    if (n <= 0) return 0;
    qsort(v, (size_t)n, sizeof *v, cmp);
    return v[(int)(p * (n - 1) + 0.5)];
}

static void mede(const MasterProfile *m, int vencidos, int lutas, Medida *r) {
    memset(r, 0, sizeof *r);
    r->avisoMin = r->cadeiaMin = 1e9;
    for (int k = 0; k < lutas; k++) {
        Settings s;
        settings_default(&s);
        settings_for_level(&s, vencidos);
        Duel d;
        duel_init(&d, &s, m, 1000u + (uint32_t)k);
        RoboMente mente;
        robo_iniciar(&mente, &ROBO_DO_DEMO, 1000u + (uint32_t)k);
        DuelEvent ev[MAX_EVENTS];
        double parado = 0, ultimoContato = -1, cueDoPrimeiro = -1;
        bool primeiraSequencia = true, aposSelo = false;
        while (d.phase != PH_FINISHED && d.clock < AJ_ROBO_LUTA_MAX) {
            duel_step_at(&d, PASSO, robo_aperto_em(&mente, &d, PASSO));
            int n = duel_drain(&d, ev, MAX_EVENTS);
            const double agora = d.clock + parado;      /* tempo real: o núcleo mais o que o hitstop congelou */
            for (int i = 0; i < n; i++) {
                if (ev[i].kind == EV_CUE && ev[i].i == 0) {
                    cueDoPrimeiro = agora;
                    if (primeiraSequencia) {
                        r->abertura += agora;
                        if (agora > r->aberturaMax) r->aberturaMax = agora;
                        primeiraSequencia = false;
                    } else if (ultimoContato >= 0 && r->nEspera < MAX_AMOSTRAS) {
                        double e = agora - ultimoContato;
                        r->espera[r->nEspera++] = e;
                        /* a pausa depois de quebrar um selo do oboro é uma cena (2,2 s): fica de fora do máximo */
                        if (!aposSelo && e > r->esperaMax) r->esperaMax = e;
                    }
                    aposSelo = false;
                }
                if (ev[i].kind == EV_SEAL) aposSelo = true;
                if (ev[i].kind == EV_IMPACT) {
                    r->golpes++;
                    if (cueDoPrimeiro >= 0 && r->nAviso < MAX_AMOSTRAS) {
                        double a = agora - cueDoPrimeiro;
                        r->aviso[r->nAviso++] = a;
                        if (a < r->avisoMin) r->avisoMin = a;
                        cueDoPrimeiro = -1;
                    } else if (ultimoContato >= 0 && agora - ultimoContato < 1.5 && r->nCadeia < MAX_AMOSTRAS) {
                        double c = agora - ultimoContato;
                        r->cadeia[r->nCadeia++] = c;
                        if (c < r->cadeiaMin) r->cadeiaMin = c;
                    }
                    ultimoContato = agora;      /* o contato, e não o fim do congelamento: o intervalo que o jogador vive */
                    const double h = duel_hitstop_for(&d.s, ev[i].judgement, ev[i].flag, false);
                    parado += h;
                    r->congelado += h;
                }
            }
        }
        const double total = d.clock + parado;
        r->luta += total;
        if (total > 0 && parado / total > r->congeladoMax) r->congeladoMax = parado / total;
    }
    r->lutas = lutas;
}

int main(int argc, char **argv) {
    const bool teste = argc > 1 && strcmp(argv[1], "--teste") == 0;
    int lutas = teste ? (argc > 2 ? atoi(argv[2]) : 40) : (argc > 1 ? atoi(argv[1]) : 200);
    if (lutas < 1) lutas = 40;
    static Medida r;
    int falhas = 0;
    if (!teste)
        printf("Ritmo do duelo em tempo real (núcleo + hitstop), robô perfeito, %d lutas por mestre.\n\n| # | Mestre | luta (s) | golpes/min | abertura (s) | contato→aviso (s) mediana / p95 | aviso→contato (s) mín / mediana | na sequência (s) mín / mediana | congelado |\n|---|---|---|---|---|---|---|---|---|\n",
               lutas);
    for (int i = 0; i < roster_size(); i++) {
        const MasterProfile *m = roster_get(i);
        mede(m, i, lutas, &r);
        const double luta = r.luta / lutas, minutos = r.luta / 60;
        if (!teste) {
            printf("| %d | %s | %.1f | %.1f | %.2f | %.2f / %.2f | %.2f / %.2f | ", i + 1, m->name, luta, r.golpes / minutos, r.abertura / lutas, percentil(r.espera, r.nEspera, 0.5),
                   percentil(r.espera, r.nEspera, 0.95), r.avisoMin, percentil(r.aviso, r.nAviso, 0.5));
            if (r.nCadeia) printf("%.2f / %.2f | ", r.cadeiaMin, percentil(r.cadeia, r.nCadeia, 0.5)); else printf("- | ");
            printf("%.1f%% |\n", 100 * r.congelado / r.luta);
        } else {
            if (r.avisoMin < AVISO_MINIMO) { printf("FALHA: %s: o aviso do primeiro golpe chega a %.0f ms do contato (o mínimo é %.0f)\n", m->name, 1000 * r.avisoMin, 1000 * AVISO_MINIMO); falhas++; }
            if (r.nCadeia && r.cadeiaMin < AJ_CADEIA_MIN - 0.002) { printf("FALHA: %s: dois contatos de uma sequência a %.0f ms (o mínimo é %.0f)\n", m->name, 1000 * r.cadeiaMin, 1000 * AJ_CADEIA_MIN); falhas++; }
            if (r.esperaMax > ESPERA_MAXIMA) { printf("FALHA: %s: %.1f s sem aviso\n", m->name, r.esperaMax); falhas++; }
            if (r.congeladoMax > CONGELADO_MAXIMO) { printf("FALHA: %s: o hitstop chega a %.0f%% de uma luta (o máximo é %.0f%%)\n", m->name, 100 * r.congeladoMax, 100 * CONGELADO_MAXIMO); falhas++; }
            if (r.nAviso < 5 || r.golpes < lutas) { printf("FALHA: %s: poucas medidas (%d avisos, %.0f golpes)\n", m->name, r.nAviso, r.golpes); falhas++; }
        }
    }
    if (teste) printf("%s\n", falhas ? "ritmo: FALHA" : "ritmo: o aviso do primeiro golpe nunca baixa de 300 ms, a sequência nunca aperta mais que AJ_CADEIA_MIN, nunca há mais de 3 s sem aviso e o hitstop nunca passa de 20% da luta");
    return falhas ? 1 : 0;
}
