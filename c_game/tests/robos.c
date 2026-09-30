/*
 * robos.c - curva de dificuldade: cada robô enfrenta cada mestre N vezes e a
 * tabela sai em Markdown. Rodar: make robos (ou make robos LUTAS=1000).
 * Kojiro chega em cada mestre depois de vencer os anteriores, como na trilha.
 * Os robôs decidem em ms e apertam no instante exato: a taxa de quadros não muda
 * o resultado (make robos HZ=144). Com QUADROS=1 o aperto entra no meio do quadro,
 * como no jogo, e o resultado passa a depender da taxa.
 *
 *   robos_relatorio [lutas [hz [quadros]]]     a tabela (make robos)
 *   robos_relatorio --taxas [lutas]            make robos-taxas: a mesma luta a 30, 60, 120, 144 e
 *       240 Hz, com o aperto em ms exato, tem que dar o MESMO resultado em todas as taxas, coluna a
 *       coluna: vitória, perfeitos, bons e erros iguais (a duração só varia um quadro a 30 Hz, porque
 *       a luta acaba no quadro do golpe final). Sai com erro se divergir. Exceção conhecida: o enjin.
 *       As brasas são somadas por quadro antes de o quadro julgar o golpe que caiu dentro dele, então
 *       um parry perfeito que as apaga no meio de um quadro chega até um quadro de brasa tarde; isso
 *       muda ~0,4% das lutas dele entre as taxas, e o teste só exige que fique abaixo de 2%.
 *   robos_relatorio --ordem [lutas]            make curva-ordem: o casual (ms exato, 60 Hz) do karasu ao oboro, com muitas
 *       lutas (100000 por mestre, em todos os núcleos da máquina), tem de ser decrescente com folga de 0,5 ponto de um mestre
 *       para o seguinte: o yoru nunca mais fácil que o arashi, nem o jinshi mais fácil que o yoru. Escreve a tabela com a
 *       margem de erro (95%) e sai com erro se a ordem quebrar.
 *       Mostra também o casual com o clique no meio do quadro, que é o que o jogo faz hoje, ao lado
 *       da coluna em ms exato.
 */
#include "../src/robo.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "portavel.h"

typedef struct { const char *nome; Robo r; } Coluna;

#define NCOL 7
static void colunas(Coluna *col) {
    Coluna base[NCOL] = {
        {"Perfeito", ROBO_DO_DEMO},
        {"Nunca defende", ROBO_SEM_DEFESA},
        {"Spam", ROBO_APERTA_SEM_PARAR},
        {"Reação 200", {0}},
        {"Reação 250", {0}},
        {"Reação 300", {0}},
        {"Humano casual", ROBO_HUMANO_CASUAL},
    };
    base[3].r = robo_reacao(0.200f);
    base[4].r = robo_reacao(0.250f);
    base[5].r = robo_reacao(0.300f);
    for (int c = 0; c < NCOL; c++) col[c] = base[c];
}

/* A mesma luta em todas as taxas de quadros. */
static int taxas(int n) {
    static const double HZ[] = {30, 60, 120, 144, 240};
    const int nh = (int)(sizeof HZ / sizeof HZ[0]), REF = 1;      /* a referência é 60 Hz */
    Coluna col[NCOL];
    colunas(col);
    long divergem = 0, comparacoes = 0, divergemBrasas = 0, comparacoesBrasas = 0;
    long meio[13][5], exato[13];
    memset(meio, 0, sizeof meio);
    memset(exato, 0, sizeof exato);
    for (int i = 0; i < roster_size(); i++) {
        const MasterProfile *m = roster_get(i);
        for (int c = 0; c < NCOL; c++) {
            for (int k = 0; k < n; k++) {
                uint32_t semente = 1000u + (uint32_t)k;
                RoboLuta ref = robo_lutar_hz(&col[c].r, m, i, semente, HZ[REF], false);
                for (int h = 0; h < nh; h++) {
                    if (h == REF) continue;
                    RoboLuta l = robo_lutar_hz(&col[c].r, m, i, semente, HZ[h], false);
                    bool igual = l.vitoria == ref.vitoria && l.perfeitos == ref.perfeitos && l.bons == ref.bons && l.erros == ref.erros &&
                                 fabs(l.duracao - ref.duracao) <= 1.0 / 30 + 1e-9;
                    if (m->burn > 0) { comparacoesBrasas++; divergemBrasas += !igual; }
                    else {
                        comparacoes++;
                        if (!igual && divergem++ < 5) printf("DIVERGE: %s, %s, semente %u, %.0f Hz x %.0f Hz\n", m->name, col[c].nome, semente, HZ[h], HZ[REF]);
                    }
                }
                if (c == NCOL - 1) {
                    exato[i] += ref.vitoria;
                    for (int h = 0; h < nh; h++) meio[i][h] += robo_lutar_hz(&col[c].r, m, i, semente, HZ[h], true).vitoria;
                }
            }
        }
    }
    bool brasasOk = comparacoesBrasas == 0 || 50 * divergemBrasas <= comparacoesBrasas;     /* até 2% */
    printf("Taxas de quadros, aperto em ms exato, %d lutas por mestre e coluna (a 30, 120, 144 e 240 Hz contra 60 Hz):\n", n);
    printf("- sem brasas: %ld comparações, %ld divergem %s\n", comparacoes, divergem, divergem ? "(FALHA)" : "(resultado idêntico em todas as taxas)");
    printf("- enjin (brasas somadas por quadro): %ld comparações, %ld divergem (%.2f%%) %s\n\n", comparacoesBrasas, divergemBrasas,
           comparacoesBrasas ? 100.0 * (double)divergemBrasas / (double)comparacoesBrasas : 0.0, brasasOk ? "(dentro do limite de 2%)" : "(FALHA: acima de 2%)");
    printf("Casual, vitórias (%%): o clique no meio do quadro (como o jogo hoje) x o instante exato\n\n| Mestre |");
    for (int h = 0; h < nh; h++) printf(" meio %.0f Hz |", HZ[h]);
    printf(" ms exato |\n|---|");
    for (int h = 0; h < nh + 1; h++) printf("---|");
    printf("\n");
    for (int i = 0; i < roster_size(); i++) {
        printf("| %s |", roster_get(i)->name);
        for (int h = 0; h < nh; h++) printf(" %.0f |", 100.0 * (double)meio[i][h] / n);
        printf(" %.0f |\n", 100.0 * (double)exato[i] / n);
    }
    return divergem || !brasasOk ? 1 : 0;
}

/* ---- a ordem da curva, do karasu ao oboro ---- */
#define ORDEM_DE 5                 /* karasu, o 6º mestre */
#define ORDEM_N (13 - ORDEM_DE)
#define ORDEM_FOLGA 0.5            /* pontos de vitória: cada mestre tem de ser ao menos isto mais difícil que o anterior */

static struct { int lutas; long vit[ORDEM_N]; long proximo; pthread_mutex_t trava; } ordem_estado = {0, {0}, 0, PTHREAD_MUTEX_INITIALIZER};

/* Cada luta é uma tarefa (mestre, semente): as threads pegam a próxima, e o resultado não depende de quem a fez. */
static void *ordem_trabalha(void *arg) {
    (void)arg;
    for (;;) {
        pthread_mutex_lock(&ordem_estado.trava);
        long t = ordem_estado.proximo++;
        pthread_mutex_unlock(&ordem_estado.trava);
        if (t >= (long)ORDEM_N * ordem_estado.lutas) return NULL;
        int i = ORDEM_DE + (int)(t / ordem_estado.lutas);
        uint32_t semente = 1000u + (uint32_t)(t % ordem_estado.lutas);
        bool vitoria = robo_lutar_hz(&ROBO_HUMANO_CASUAL, roster_get(i), i, semente, AJ_ROBO_HZ_PADRAO, false).vitoria;
        pthread_mutex_lock(&ordem_estado.trava);
        ordem_estado.vit[i - ORDEM_DE] += vitoria;
        pthread_mutex_unlock(&ordem_estado.trava);
    }
}

static int ordem(int lutas) {
    long nucleos = nucleos_online();
    if (nucleos < 1) nucleos = 1;
    if (nucleos > 16) nucleos = 16;
    ordem_estado.lutas = lutas;
    pthread_t th[16];
    for (long i = 0; i < nucleos; i++) pthread_create(&th[i], NULL, ordem_trabalha, NULL);
    for (long i = 0; i < nucleos; i++) pthread_join(th[i], NULL);
    printf("Casual, ms exato, %d lutas por mestre: a curva do karasu ao oboro tem de cair, com folga de %.1f ponto\n\n| Mestre | vitórias (%%) | ± (95%%) | o anterior menos este |\n|---|---|---|---|\n", lutas, ORDEM_FOLGA);
    int falhas = 0;
    double anterior = 0;
    for (int k = 0; k < ORDEM_N; k++) {
        double p = (double)ordem_estado.vit[k] / lutas, v = 100 * p, ic = 196 * sqrt(p * (1 - p) / lutas);
        if (k == 0) printf("| %s | %.2f | %.2f | |\n", roster_get(ORDEM_DE + k)->name, v, ic);
        else {
            bool ok = anterior - v >= ORDEM_FOLGA;
            printf("| %s | %.2f | %.2f | %+.2f %s |\n", roster_get(ORDEM_DE + k)->name, v, ic, anterior - v, ok ? "" : "(FALHA: a ordem quebrou)");
            falhas += !ok;
        }
        anterior = v;
    }
    printf("\n%s\n", falhas ? "ordem: a curva não cai com a folga de 0,5 ponto (FALHA)" : "ordem: a curva cai do karasu ao oboro");
    return falhas ? 1 : 0;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--ordem") == 0) return ordem(argc > 2 && atoi(argv[2]) > 0 ? atoi(argv[2]) : 100000);
    if (argc > 1 && strcmp(argv[1], "--taxas") == 0) return taxas(argc > 2 && atoi(argv[2]) > 0 ? atoi(argv[2]) : 100);
    int n = argc > 1 ? atoi(argv[1]) : 300;
    double hz = argc > 2 ? atof(argv[2]) : 60;
    bool quadros = argc > 3 && atoi(argv[3]) != 0;
    if (n < 1) n = 1;
    if (hz < 10) hz = 60;
    Coluna col[NCOL];
    colunas(col);
    const int nc = NCOL;
    printf("Vitórias em %d lutas por mestre (%%), %.0f Hz, aperto %s.\n\n| # | Mestre |", n, hz, quadros ? "no meio do quadro" : "no instante exato");
    for (int c = 0; c < nc; c++) printf(" %s |", col[c].nome);
    printf(" Casual: perf/bom/erro | Casual: duração |\n|---|---|");
    for (int c = 0; c < nc + 2; c++) printf("---|");
    printf("\n");
    for (int i = 0; i < roster_size(); i++) {
        const MasterProfile *m = roster_get(i);
        printf("| %d | %s |", i + 1, m->name);
        int P = 0, B = 0, E = 0;
        double T = 0;
        for (int c = 0; c < nc; c++) {
            int v = 0;
            for (int k = 0; k < n; k++) {
                RoboLuta l = robo_lutar_hz(&col[c].r, m, i, 1000u + (uint32_t)k, hz, quadros);
                v += l.vitoria;
                if (c == nc - 1) { P += l.perfeitos; B += l.bons; E += l.erros; T += l.duracao; }
            }
            printf(" %.0f |", 100.0 * v / n);
        }
        int tot = P + B + E > 0 ? P + B + E : 1;
        printf(" %d/%d/%d%% | %.0f s |\n", 100 * P / tot, 100 * B / tot, 100 * E / tot, T / n);
    }
    return 0;
}
