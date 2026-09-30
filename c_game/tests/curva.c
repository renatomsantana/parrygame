/*
 * curva.c - a curva de dificuldade por mestre, com os cinco robôs que a definem, a 60 e a 144 Hz.
 * Rodar: make curva (ou make curva LUTAS=10000). Só mede: escreve as tabelas em Markdown e confere o que
 * tem de valer sempre (o perfeito vence tudo, o spam e o que nunca defende perdem de todos os mestres).
 * A faixa alvo de cada robô (docs/CURVA.md) aparece na tabela como informação; ela ainda não reprova nada.
 *
 *   curva_relatorio [lutas [quadros]]     lutas por mestre e robô (2000); quadros != 0: o aperto entra no meio do quadro
 *   curva_relatorio --teste               make test-curva: o que o robô da primeira vez promete (abaixo) e o que vale sempre
 *
 * Os robôs:
 *   Primeira vez    só reage ao aviso e à lâmina, não decora nada, reação de 250 a 300 ms com variação de golpe a
 *                   golpe e 3% de apertos que falham (ROBO_PRIMEIRA_VEZ_PADRAO)
 *   Casual decora   o humano casual: decora o padrão e mede o ritmo com 8% de erro (ROBO_HUMANO_CASUAL)
 *   Reação 250      só reage ao último sinal, sempre em 250 ms (robo_reacao)
 *   Perfeito        o do demo
 *   Spam            aperta a cada 150 ms sem olhar
 */
#include "../src/robo.h"

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NROBOS 5
#define NMESTRES 13
#define NHZ 2
#define MAX_THREADS 16

enum { PRIMEIRA_VEZ, CASUAL, REACAO_250, PERFEITO, SPAM };
static const char *NOMES[NROBOS] = {"Primeira vez", "Casual decora", "Reação 250", "Perfeito", "Spam"};
static const double HZ[NHZ] = {60, 144};

static Robo robos[NROBOS];

typedef struct {
    int lutas;
    bool quadros;
    long proximo;
    long vit[NHZ][NROBOS][NMESTRES];
    pthread_mutex_t trava;
} Estado;
static Estado E = {.trava = PTHREAD_MUTEX_INITIALIZER};

/* Cada luta é uma tarefa (taxa, robô, mestre, semente): o resultado não depende de quem a fez. */
static void *trabalha(void *arg) {
    (void)arg;
    const long total = (long)NHZ * NROBOS * NMESTRES * E.lutas;
    for (;;) {
        pthread_mutex_lock(&E.trava);
        long t = E.proximo++;
        pthread_mutex_unlock(&E.trava);
        if (t >= total) return NULL;
        uint32_t semente = 1000u + (uint32_t)(t % E.lutas);
        long r = t / E.lutas;
        int i = (int)(r % NMESTRES);
        int b = (int)((r / NMESTRES) % NROBOS);
        int h = (int)(r / NMESTRES / NROBOS);
        bool v = robo_lutar_hz(&robos[b], roster_get(i), i, semente, HZ[h], E.quadros).vitoria;
        pthread_mutex_lock(&E.trava);
        E.vit[h][b][i] += v;
        pthread_mutex_unlock(&E.trava);
    }
}

static void prepara_robos(void) {
    robos[PRIMEIRA_VEZ] = ROBO_PRIMEIRA_VEZ_PADRAO;
    robos[CASUAL] = ROBO_HUMANO_CASUAL;
    robos[REACAO_250] = robo_reacao(0.250f);
    robos[PERFEITO] = ROBO_DO_DEMO;
    robos[SPAM] = ROBO_APERTA_SEM_PARAR;
}

static void medir(int lutas, bool quadros) {
    memset(&E.vit, 0, sizeof E.vit);
    E.lutas = lutas;
    E.quadros = quadros;
    E.proximo = 0;
    long nucleos = sysconf(_SC_NPROCESSORS_ONLN);
    if (nucleos < 1) nucleos = 1;
    if (nucleos > MAX_THREADS) nucleos = MAX_THREADS;
    pthread_t th[MAX_THREADS];
    for (long i = 0; i < nucleos; i++) pthread_create(&th[i], NULL, trabalha, NULL);
    for (long i = 0; i < nucleos; i++) pthread_join(th[i], NULL);
}

/* O que vale sempre, em qualquer curva: o perfeito vence todos os mestres e o spam perde de todos. */
static int invariantes(void) {
    int falhas = 0;
    for (int h = 0; h < NHZ; h++)
        for (int i = 0; i < NMESTRES; i++) {
            if (E.vit[h][PERFEITO][i] != E.lutas) { printf("FALHA: o perfeito perdeu de %s a %.0f Hz\n", roster_get(i)->name, HZ[h]); falhas++; }
            if (E.vit[h][SPAM][i] != 0) { printf("FALHA: o spam venceu %s a %.0f Hz\n", roster_get(i)->name, HZ[h]); falhas++; }
        }
    return falhas;
}

static void imprime(void) {
    for (int h = 0; h < NHZ; h++) {
        printf("Vitórias em %d lutas por mestre (%%), %.0f Hz, aperto %s.\n\n| # | Mestre |", E.lutas, HZ[h], E.quadros ? "no meio do quadro" : "no instante exato");
        for (int b = 0; b < NROBOS; b++) printf(" %s |", NOMES[b]);
        printf("\n|---|---|");
        for (int b = 0; b < NROBOS; b++) printf("---|");
        printf("\n");
        for (int i = 0; i < NMESTRES; i++) {
            printf("| %d | %s |", i + 1, roster_get(i)->name);
            for (int b = 0; b < NROBOS; b++) printf(" %.1f |", 100.0 * (double)E.vit[h][b][i] / E.lutas);
            printf("\n");
        }
        printf("\n");
    }
}

/* ---- o robô da primeira vez ---- */

static int falhas_teste;
#define CONFERE(cond, ...) do { if (!(cond)) { printf("FALHA: " __VA_ARGS__); printf("\n"); falhas_teste++; } } while (0)

static bool luta_igual(const RoboLuta *a, const RoboLuta *b) {
    return a->vitoria == b->vitoria && a->perfeitos == b->perfeitos && a->bons == b->bons && a->erros == b->erros && a->duracao == b->duracao;
}

/* O último sinal que o robô usa: a lâmina partindo ou o aviso, o que vier depois (a mesma conta do robô). */
static double ultimo_sinal(const Duel *d) {
    double partida = d->strikeAt - duel_strike_lead(d), aviso = duel_cue_time(d);
    bool percebido = d->m->cueAudio > 0 || d->m->cueVisual > 0;
    return percebido && aviso > partida ? aviso : partida;
}

/* Uma luta do robô da primeira vez, anotando quanto ele demora depois do último sinal de cada golpe. */
typedef struct { long n; double soma, soma2, menor; } Reacoes;

static void luta_medindo(const Robo *r, const MasterProfile *m, int vencidos, uint32_t semente, Reacoes *re) {
    Settings s;
    settings_default(&s);
    settings_for_level(&s, vencidos);
    Duel d;
    duel_init(&d, &s, m, semente);
    RoboMente mente;
    robo_iniciar(&mente, r, semente);
    const double dt = 1.0 / AJ_ROBO_HZ_PADRAO;
    int visto = -1;
    double sinal = 0;
    DuelEvent ev[MAX_EVENTS];
    while (d.phase != PH_FINISHED && d.clock < AJ_ROBO_LUTA_MAX) {
        if (d.phase == PH_WINDUP && d.attacks != visto) { visto = d.attacks; sinal = ultimo_sinal(&d); }
        double quando = robo_aperto_em(&mente, &d, dt);
        if (quando >= 0 && visto >= 0) {
            double demora = d.clock + quando - sinal;
            re->n++;
            re->soma += demora;
            re->soma2 += demora * demora;
            if (demora < re->menor) re->menor = demora;
        }
        duel_step_at(&d, dt, quando);
        duel_drain(&d, ev, MAX_EVENTS);
    }
}

/* A menor e a maior partida da lâmina (s antes do contato) em uma luta do perfeito contra `m`. */
static void amostra_lamina(const MasterProfile *m, int vencidos, uint32_t semente, double *menor, double *maior) {
    Settings s;
    settings_default(&s);
    settings_for_level(&s, vencidos);
    Duel d;
    duel_init(&d, &s, m, semente);
    RoboMente mente;
    robo_iniciar(&mente, &ROBO_DO_DEMO, semente);
    const double dt = 1.0 / AJ_ROBO_HZ_PADRAO;
    int visto = -1;
    DuelEvent ev[MAX_EVENTS];
    while (d.phase != PH_FINISHED && d.clock < AJ_ROBO_LUTA_MAX) {
        if (d.phase == PH_WINDUP && d.attacks != visto) {
            visto = d.attacks;
            double lamina = duel_strike_lead(&d);
            if (lamina < *menor) *menor = lamina;
            if (lamina > *maior) *maior = lamina;
        }
        duel_step_at(&d, dt, robo_aperto_em(&mente, &d, dt));
        duel_drain(&d, ev, MAX_EVENTS);
    }
}

static int testes(int lutas) {
    prepara_robos();
    /* 1. sem variação nem falha, o robô da primeira vez é exatamente o que só reage (robo_reacao) */
    Robo liso = ROBO_PRIMEIRA_VEZ_PADRAO;
    liso.reacao = 0.250f;
    liso.mao = AJ_ROBO_REACAO_MAO;
    liso.reacaoDp = 0;
    liso.falha = 0;
    Robo reage = robo_reacao(0.250f);
    /* 2. falha de 100%: nunca aperta, como o que nunca defende */
    Robo cego = ROBO_PRIMEIRA_VEZ_PADRAO;
    cego.falha = 1;
    long iguais = 0, cegos = 0, comparadas = 0, taxas = 0, taxasComparadas = 0;
    for (int i = 0; i < NMESTRES; i++) {
        const MasterProfile *m = roster_get(i);
        for (int k = 0; k < lutas; k++) {
            uint32_t sem = 1000u + (uint32_t)k;
            RoboLuta a = robo_lutar_hz(&liso, m, i, sem, 60, false), b = robo_lutar_hz(&reage, m, i, sem, 60, false);
            iguais += luta_igual(&a, &b);
            RoboLuta c = robo_lutar_hz(&cego, m, i, sem, 60, false), n = robo_lutar_hz(&ROBO_SEM_DEFESA, m, i, sem, 60, false);
            cegos += luta_igual(&c, &n);
            comparadas++;
            /* 3. em ms exato o resultado não depende da taxa de quadros (o enjin soma brasas por quadro: fica de fora) */
            if (m->burn <= 0) {
                RoboLuta p = robo_lutar_hz(&robos[PRIMEIRA_VEZ], m, i, sem, 60, false);
                for (int h = 1; h < 4; h++) {
                    static const double OUTRAS[] = {30, 120, 144, 240};
                    RoboLuta q = robo_lutar_hz(&robos[PRIMEIRA_VEZ], m, i, sem, OUTRAS[h], false);
                    taxas += p.vitoria == q.vitoria && p.perfeitos == q.perfeitos && p.bons == q.bons && p.erros == q.erros;
                    taxasComparadas++;
                }
            }
        }
    }
    CONFERE(iguais == comparadas, "sem falha nem variação o robô da primeira vez devia ser o robo_reacao (%ld de %ld lutas iguais)", iguais, comparadas);
    CONFERE(cegos == comparadas, "com 100%% de falha o robô da primeira vez devia ser o que nunca defende (%ld de %ld lutas iguais)", cegos, comparadas);
    CONFERE(taxas == taxasComparadas, "o robô da primeira vez devia dar o mesmo resultado em qualquer taxa de quadros (%ld de %ld iguais)", taxas, taxasComparadas);
    /* 4. nunca aperta antes do sinal que está reagindo e a reação varia de golpe a golpe (o desvio vem da reação e da mão) */
    Reacoes re = {0, 0, 0, 1e9};
    for (int i = 0; i < NMESTRES; i++)
        for (int k = 0; k < lutas; k++) luta_medindo(&robos[PRIMEIRA_VEZ], roster_get(i), i, 1000u + (uint32_t)k, &re);
    double media = re.soma / (double)re.n, desvio = sqrt(re.soma2 / (double)re.n - media * media);
    CONFERE(re.n > 1000, "poucos apertos medidos (%ld)", re.n);
    CONFERE(re.menor >= 0, "o robô da primeira vez apertou %.0f ms antes do sinal que reage", -1000 * re.menor);
    CONFERE(desvio >= 0.030, "a reação do robô da primeira vez quase não varia (desvio %.0f ms)", 1000 * desvio);
    printf("robô da primeira vez: %ld apertos medidos, %.0f ms depois do último sinal em média (desvio %.0f ms, o menor %.0f ms)\n",
           re.n, 1000 * media, 1000 * desvio, 1000 * re.menor);
    /* 5. o teto da lâmina por mestre (MasterProfile.bladeMax): 0 é o global, nunca sobe o teto global, baixa a partida da lâmina
     * e quem mede pelo aviso (o casual) não sente */
    {
        const int GARFIEL = 4;      /* a lâmina variável começa nele */
        MasterProfile com = *roster_get(GARFIEL), alto = com, global = com;
        global.bladeMax = 0;
        com.bladeMax = 0.180f;
        alto.bladeMax = 0.900f;
        double menorG = 1, maiorG = 0, menorC = 1, maiorC = 0;
        long difAlto = 0, difCasual = 0, difReacao = 0;
        Robo reacao = robo_reacao(0.250f);
        for (int k = 0; k < lutas; k++) {
            uint32_t sem = 1000u + (uint32_t)k;
            amostra_lamina(&global, GARFIEL, sem, &menorG, &maiorG);
            amostra_lamina(&com, GARFIEL, sem, &menorC, &maiorC);
            RoboLuta g = robo_lutar_hz(&reacao, &global, GARFIEL, sem, 60, false), a = robo_lutar_hz(&reacao, &alto, GARFIEL, sem, 60, false);
            difAlto += !luta_igual(&g, &a);
            RoboLuta cg = robo_lutar_hz(&ROBO_HUMANO_CASUAL, &global, GARFIEL, sem, 60, false), cc = robo_lutar_hz(&ROBO_HUMANO_CASUAL, &com, GARFIEL, sem, 60, false);
            difCasual += !luta_igual(&cg, &cc);
            RoboLuta rc = robo_lutar_hz(&reacao, &com, GARFIEL, sem, 60, false);
            difReacao += !luta_igual(&g, &rc);
        }
        CONFERE(maiorG > 0.300 && menorG >= AJ_LAMINA_MIN - 1e-4, "sem teto por mestre a lâmina parte de %.0f a %.0f ms antes do contato (o global vai até %.0f)", 1000 * menorG,
              1000 * maiorG, 1000 * AJ_LAMINA_MAX);
        CONFERE(maiorC <= AJ_LAMINA_PARTE + 1e-4 && menorC >= AJ_LAMINA_MIN - 1e-4, "com teto de 180 ms a lâmina parte de %.0f a %.0f ms (nunca menos que %.0f, e o tempo fixo da investida é %.0f)",
              1000 * menorC, 1000 * maiorC, 1000 * AJ_LAMINA_MIN, 1000 * AJ_LAMINA_PARTE);
        CONFERE(difAlto == 0, "um teto acima do global devia valer o global (%ld lutas diferentes)", difAlto);
        CONFERE(difCasual == 0, "o casual não sente o teto da lâmina (%ld lutas diferentes)", difCasual);
        CONFERE(difReacao > lutas / 10, "quem só reage à lâmina devia sentir o teto (%ld de %d lutas diferentes)", difReacao, lutas);
        printf("teto da lâmina: sem teto %.0f a %.0f ms, com 180 ms %.0f a %.0f ms; o casual não muda e quem só reage muda em %ld de %d lutas\n", 1000 * menorG,
               1000 * maiorG, 1000 * menorC, 1000 * maiorC, difReacao, lutas);
    }
    /* 6. o que vale sempre */
    medir(lutas, false);
    falhas_teste += invariantes();
    printf("%s\n", falhas_teste ? "curva: FALHA" : "curva: o robô da primeira vez só reage (sem variação é o robo_reacao, com 100% de falha é o que nunca defende), não muda com a taxa, o teto da lâmina por mestre só pesa em quem reage à lâmina, e o perfeito e o spam valem sempre");
    return falhas_teste ? 1 : 0;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--teste") == 0) return testes(argc > 2 && atoi(argv[2]) > 0 ? atoi(argv[2]) : 200);
    int lutas = argc > 1 && atoi(argv[1]) > 0 ? atoi(argv[1]) : 2000;
    if (roster_size() != NMESTRES) { printf("curva: o elenco mudou de tamanho (%d mestres): ajuste NMESTRES\n", roster_size()); return 1; }
    prepara_robos();
    medir(lutas, argc > 2 && atoi(argv[2]) != 0);
    imprime();
    int falhas = invariantes();
    printf("%s\n", falhas ? "curva: o que tem de valer sempre não vale (FALHA)" : "curva: o perfeito vence todos os mestres e o spam perde de todos");
    return falhas ? 1 : 0;
}
