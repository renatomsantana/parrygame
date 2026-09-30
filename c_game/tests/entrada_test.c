/*
 * entrada_test.c - o carimbo do clique (src/entrada.c), sem janela nem raylib. make test-entrada
 * Cobre: a conversão do instante do sistema para o passo do núcleo (quadro comum, câmera lenta,
 * hitstop, relógio com offset enorme); todo carimbo inválido cai no meio do quadro, sem quebrar e sem
 * nunca sair de [0, passo]; um fuzz de um milhão de entradas; e lutas inteiras dos robôs:
 *   - com todos os carimbos inválidos a luta sai idêntica à de hoje (duel_step, o aperto no meio);
 *   - com carimbos exatos, passando pelo pipeline inteiro (quadros irregulares, câmera lenta, hitstop,
 *     relógio com offset), sai idêntica à do aperto em ms exato (duel_step_at);
 *   - com carimbos exatos e quadros regulares, o casual a 30, 60, 144 e 240 Hz sai igual, luta a luta,
 *     à coluna "ms exato" de make robos.
 */
#include "../src/entrada.h"

#include "../src/ajuste.h"
#include "../src/robo.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks = 0, failures = 0;
#define CHECK(cond, ...) do { \
    checks++; \
    if (!(cond)) { failures++; printf("FALHA %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } \
} while (0)

static bool perto(double a, double b, double tol) { return fabs(a - b) <= tol; }

/* Um gerador barato e repetível (as duas lutas de cada comparação sorteiam os mesmos quadros). */
static double sorteio(uint64_t *s) {
    *s = *s * 6364136223846793005ULL + 1442695040888963407ULL;
    return (double)(*s >> 11) / 9007199254740992.0;   /* [0, 1) */
}

/* ---- a conversão, com números que se conferem de cabeça ---- */
static void teste_conversao(void) {
    bool v;
    double t;
    const double fim = 1000.0, q = 1.0 / 60;
    /* quadro comum: o passo é o quadro */
    t = entrada_no_quadro(fim - q, fim, q, q, q, &v);
    CHECK(v && perto(t, 0, 1e-12), "clique no começo do quadro: %.9f", t);
    t = entrada_no_quadro(fim - q / 2, fim, q, q, q, &v);
    CHECK(v && perto(t, q / 2, 1e-12), "clique no meio do quadro: %.9f", t);
    t = entrada_no_quadro(fim, fim, q, q, q, &v);
    CHECK(v && perto(t, q, 1e-12), "clique no fim do quadro: %.9f", t);
    t = entrada_no_quadro(fim - 0.004, fim, q, q, q, &v);
    CHECK(v && perto(t, q - 0.004, 1e-12), "clique 4 ms antes do poll: %.9f", t);
    /* a margem: um clique até 5 ms fora do quadro é preso a ele; mais que isso, cai no meio */
    t = entrada_no_quadro(fim - q - 0.003, fim, q, q, q, &v);
    CHECK(v && t == 0, "3 ms antes do quadro: preso ao começo (%.9f)", t);
    t = entrada_no_quadro(fim + 0.003, fim, q, q, q, &v);
    CHECK(v && perto(t, q, 1e-12), "3 ms depois do poll: preso ao fim (%.9f)", t);
    t = entrada_no_quadro(fim - q - 0.006, fim, q, q, q, &v);
    CHECK(!v && perto(t, q / 2, 1e-12), "6 ms antes do quadro: meio do quadro (%.9f)", t);
    t = entrada_no_quadro(fim + 0.006, fim, q, q, q, &v);
    CHECK(!v && perto(t, q / 2, 1e-12), "6 ms depois do poll: meio do quadro (%.9f)", t);
    /* câmera lenta: 0,3 do tempo; o clique a 3/4 do quadro real fica a 3/4 do passo */
    double lento = q * 0.3;
    t = entrada_no_quadro(fim - q / 4, fim, q, q, lento, &v);
    CHECK(v && perto(t, lento * 0.75, 1e-12), "câmera lenta: %.9f (esperado %.9f)", t, lento * 0.75);
    /* hitstop: só correu a metade final do quadro; um clique congelado vale 0, um depois, na proporção */
    double corrido = q * 0.5, passo = corrido;
    t = entrada_no_quadro(fim - q * 0.9, fim, q, corrido, passo, &v);
    CHECK(v && t == 0, "clique durante o congelamento: vale no começo do passo (%.9f)", t);
    t = entrada_no_quadro(fim - corrido * 0.25, fim, q, corrido, passo, &v);
    CHECK(v && perto(t, passo * 0.75, 1e-12), "clique depois do congelamento: %.9f", t);
    /* hitstop com câmera lenta */
    t = entrada_no_quadro(fim - corrido * 0.5, fim, q, corrido, corrido * 0.3, &v);
    CHECK(v && perto(t, corrido * 0.3 * 0.5, 1e-12), "hitstop e câmera lenta: %.9f", t);
    /* o relógio do sistema pode estar em qualquer lugar: um offset enorme não custa precisão */
    for (double origem = 0; origem < 4e6; origem += 1.3e6) {
        double f = origem + 12345.678901234;
        t = entrada_no_quadro(f - q / 3, f, q, q, q, &v);
        CHECK(v && perto(t, q * 2 / 3, 1e-9), "relógio em %.0f s: %.12f (esperado %.12f)", f, t, q * 2 / 3);
    }
    /* sem ninguém para receber a resposta de "valido" */
    t = entrada_no_quadro(fim - q / 2, fim, q, q, q, NULL);
    CHECK(perto(t, q / 2, 1e-12), "valido pode ser NULL");
}

/* ---- todo carimbo inválido cai no meio do quadro, sem quebrar ---- */
static void teste_invalidos(void) {
    const double fim = 5000.0, q = 1.0 / 144;
    const double lixo[] = {NAN, INFINITY, -INFINITY, 1e300, -1e300, 1e-320, -1e-320, 1e308, 0, -1,
                           fim - 1, fim + 1, fim - q - 0.0051, fim + 0.0051, fim - 3600, fim + 3600};
    for (unsigned i = 0; i < sizeof lixo / sizeof lixo[0]; i++) {
        bool v = true;
        double t = entrada_no_quadro(lixo[i], fim, q, q, q, &v);
        CHECK(!v && t == q * 0.5, "carimbo %g: cai no meio do quadro (%.9f, valido=%d)", lixo[i], t, v);
    }
    bool v = true;
    double t = entrada_no_quadro(ENTRADA_SEM_CARIMBO, fim, q, q, q, &v);
    CHECK(!v && t == q * 0.5, "ENTRADA_SEM_CARIMBO cai no meio do quadro");
    /* o resto dos parâmetros também pode vir estragado */
    struct { double fim, quadro, corrido, passo; const char *nome; } ruins[] = {
        {NAN, q, q, q, "fim NaN"},        {INFINITY, q, q, q, "fim infinito"},
        {fim, NAN, q, q, "quadro NaN"},   {fim, 0, 0, q, "quadro zero"},
        {fim, -q, q, q, "quadro negativo"}, {fim, 0.5, 0.5, 0.5, "quadro de meio segundo (travamento)"},
        {fim, q, 0, q, "nada correu"},    {fim, q, -q, q, "corrido negativo"},
        {fim, q, 2 * q, q, "correu mais que o quadro"}, {fim, q, NAN, q, "corrido NaN"},
        {fim, q, q, -q, "passo negativo"}, {fim, q, q, NAN, "passo NaN"},
        {fim, q, q, INFINITY, "passo infinito"},
    };
    for (unsigned i = 0; i < sizeof ruins / sizeof ruins[0]; i++) {
        v = true;
        t = entrada_no_quadro(fim - q / 2, ruins[i].fim, ruins[i].quadro, ruins[i].corrido, ruins[i].passo, &v);
        bool passoBom = isfinite(ruins[i].passo) && ruins[i].passo >= 0;
        CHECK(!v && isfinite(t) && t >= 0 && t == (passoBom ? ruins[i].passo * 0.5 : 0), "%s: cai no meio do quadro (%.9f)", ruins[i].nome, t);
    }
    /* passo zero (pausa, quadro sem tempo): vale 0, válido ou não */
    t = entrada_no_quadro(fim - q / 2, fim, q, q, 0, &v);
    CHECK(isfinite(t) && t == 0, "passo zero: %.9f", t);
}

/* ---- fuzz: o invariante vale para qualquer entrada ---- */
static double numero_ruim(uint64_t *s) {
    switch ((int)(sorteio(s) * 12)) {
        case 0: return NAN;
        case 1: return INFINITY;
        case 2: return -INFINITY;
        case 3: return 1e300 * (sorteio(s) - 0.5);
        case 4: return 4.9e-324 * (sorteio(s) * 10);
        case 5: return (sorteio(s) - 0.5) * 1e-9;
        case 6: return (sorteio(s) - 0.5) * 100;
        case 7: return 1e6 * sorteio(s);
        case 8: return (sorteio(s) - 0.5) * 0.02;
        case 9: return -0.0;
        case 10: return 0.2 * sorteio(s);
        default: return sorteio(s) * 1e15;
    }
}

static void teste_fuzz(void) {
    uint64_t s = 20240607;
    long ruins = 0, validos = 0, semLimite = 0;
    for (long i = 0; i < 1000000; i++) {
        double fim = numero_ruim(&s), quadro = numero_ruim(&s), passo = numero_ruim(&s), corrido = numero_ruim(&s);
        double carimbo = sorteio(&s) < 0.5 ? fim - sorteio(&s) * quadro : numero_ruim(&s);
        if (sorteio(&s) < 0.3) corrido = quadro * sorteio(&s);
        bool v = false;
        double t = entrada_no_quadro(carimbo, fim, quadro, corrido, passo, &v);
        double meio = isfinite(passo) && passo >= 0 ? passo * 0.5 : 0;
        bool passoBom = isfinite(passo) && passo >= 0;
        if (!(isfinite(t) && t >= 0 && (!passoBom || t <= passo))) semLimite++;
        if (v) validos++;
        else if (t != meio) ruins++;
    }
    CHECK(semLimite == 0, "fuzz: %ld resultados fora de [0, passo] ou não finitos", semLimite);
    CHECK(ruins == 0, "fuzz: %ld carimbos inválidos que não caíram no meio do quadro", ruins);
    CHECK(validos > 1000, "fuzz: só %ld entradas válidas (o sorteio ficou ruim)", validos);
    printf("entrada: fuzz de 1.000.000 entradas: %ld válidas, o resto no meio do quadro\n", validos);
}

/* ---- lutas inteiras ---- */
typedef enum { REF_EXATO, REF_MEIO, PIPE_EXATO, PIPE_INVALIDO } Modo;

/* Uma luta com quadros irregulares, câmera lenta e hitstop. `regular`: quadros iguais e nada mais. As
 * lutas de uma comparação usam a mesma semente de quadros, e o jogo fictício é o mesmo. */
static RoboLuta luta(const Robo *r, const MasterProfile *m, int venc, uint32_t semente, double hz, bool regular, Modo modo, long *invalidos) {
    Settings st;
    settings_default(&st);
    settings_for_level(&st, venc);
    Duel d;
    duel_init(&d, &st, m, semente);
    RoboMente mente;
    robo_iniciar(&mente, r, semente);
    RoboLuta out;
    memset(&out, 0, sizeof out);
    DuelEvent ev[MAX_EVENTS];
    uint64_t s = 99 + semente;
    const double origem = 987654.321;       /* o relógio do sistema não começa em zero */
    double agora = origem;
    static const double LIXO[] = {NAN, INFINITY, -1e300, 1e300, 0, -1, 1e-320};
    long n = 0;
    while (d.phase != PH_FINISHED && d.clock < AJ_ROBO_LUTA_MAX) {
        double quadro = regular ? 1.0 / hz : (1.0 / hz) * (0.6 + 0.8 * sorteio(&s));
        double lento = !regular && sorteio(&s) < 0.15 ? 0.3 : 1.0;
        double corrido = !regular && sorteio(&s) < 0.10 ? quadro * 0.4 : quadro;
        double passo = corrido * lento;
        agora += quadro;                    /* o poll deste quadro */
        double t = robo_aperto_em(&mente, &d, passo);
        if (t >= 0) {
            if (modo == REF_MEIO) t = passo * 0.5;
            else if (modo == PIPE_EXATO) {
                double carimbo = (agora - corrido) + t / lento;      /* o instante de verdade do clique */
                bool v;
                t = entrada_no_quadro(carimbo, agora, quadro, corrido, passo, &v);
                if (!v && invalidos) (*invalidos)++;
            } else if (modo == PIPE_INVALIDO) {
                bool v;
                double c = n % 3 == 0 ? agora + 1 : n % 3 == 1 ? agora - quadro - 1 : LIXO[(n / 3) % (long)(sizeof LIXO / sizeof LIXO[0])];
                t = entrada_no_quadro(c, agora, quadro, corrido, passo, &v);
                if (v && invalidos) (*invalidos)++;
            }
            n++;
        }
        duel_step_at(&d, passo, t);
        int ne = duel_drain(&d, ev, MAX_EVENTS);
        for (int i = 0; i < ne; i++) {
            if (ev[i].kind == EV_IMPACT) {
                if (ev[i].judgement == J_PERFEITO) out.perfeitos++;
                else if (ev[i].judgement == J_BOM) out.bons++;
                else out.erros++;
            }
            if (ev[i].kind == EV_FINISHED) out.vitoria = ev[i].flag;
        }
    }
    out.duracao = d.clock;
    return out;
}

static bool iguais(RoboLuta a, RoboLuta b) {
    return a.vitoria == b.vitoria && a.perfeitos == b.perfeitos && a.bons == b.bons && a.erros == b.erros && fabs(a.duracao - b.duracao) < 1e-6;
}

static void teste_lutas(int lutas) {
    const Robo *robos[] = {&ROBO_HUMANO_CASUAL, &ROBO_DO_DEMO};
    long comparadas = 0, difExato = 0, difInvalido = 0, invalidosDoExato = 0, validosDoInvalido = 0;
    for (int i = 0; i < roster_size(); i++) {
        const MasterProfile *m = roster_get(i);
        for (int c = 0; c < 2; c++)
            for (int k = 0; k < lutas; k++) {
                uint32_t sem = 5000u + (uint32_t)k;
                double hz = k % 2 ? 60 : 144;
                RoboLuta ref = luta(robos[c], m, i, sem, hz, false, REF_EXATO, NULL);
                RoboLuta pipe = luta(robos[c], m, i, sem, hz, false, PIPE_EXATO, &invalidosDoExato);
                RoboLuta hoje = luta(robos[c], m, i, sem, hz, false, REF_MEIO, NULL);
                RoboLuta inv = luta(robos[c], m, i, sem, hz, false, PIPE_INVALIDO, &validosDoInvalido);
                comparadas++;
                if (!iguais(ref, pipe)) { if (difExato++ < 3) printf("DIFERE (exato): %s, robô %d, semente %u\n", m->name, c, sem); }
                if (!iguais(hoje, inv)) { if (difInvalido++ < 3) printf("DIFERE (inválido): %s, robô %d, semente %u\n", m->name, c, sem); }
            }
    }
    CHECK(difExato == 0, "carimbos exatos pelo pipeline inteiro: %ld de %ld lutas diferem do aperto em ms exato", difExato, comparadas);
    CHECK(invalidosDoExato == 0, "carimbos exatos: %ld foram tratados como inválidos", invalidosDoExato);
    CHECK(difInvalido == 0, "carimbos inválidos: %ld de %ld lutas diferem do aperto no meio do quadro (o de hoje)", difInvalido, comparadas);
    CHECK(validosDoInvalido == 0, "carimbos inválidos: %ld foram aceitos", validosDoInvalido);
    printf("entrada: %ld lutas (quadros irregulares, câmera lenta, hitstop, relógio com offset): carimbo exato = ms exato; carimbo inválido = meio do quadro\n", comparadas);
}

/* O casual a 30, 60, 144 e 240 Hz com carimbo exato é a coluna "ms exato" de make robos, luta a luta. */
static void teste_tabela(int lutas) {
    static const double HZ[] = {30, 60, 144, 240};
    long dif = 0;
    long vit[13][4], vitMs[13];
    memset(vit, 0, sizeof vit);
    memset(vitMs, 0, sizeof vitMs);
    for (int i = 0; i < roster_size(); i++) {
        const MasterProfile *m = roster_get(i);
        for (int k = 0; k < lutas; k++) {
            uint32_t sem = 1000u + (uint32_t)k;
            RoboLuta ms = robo_lutar_hz(&ROBO_HUMANO_CASUAL, m, i, sem, 60, false);
            vitMs[i] += ms.vitoria;
            for (int h = 0; h < 4; h++) {
                RoboLuta l = luta(&ROBO_HUMANO_CASUAL, m, i, sem, HZ[h], true, PIPE_EXATO, NULL);
                vit[i][h] += l.vitoria;
                if (!(l.vitoria == ms.vitoria && l.perfeitos == ms.perfeitos && l.bons == ms.bons && l.erros == ms.erros && fabs(l.duracao - ms.duracao) <= 1.0 / 30 + 1e-9)) {
                    if (m->burn > 0) continue;      /* a queimadura do enjin é somada por quadro (make robos-taxas) */
                    if (dif++ < 3) printf("DIFERE: %s, %.0f Hz, semente %u\n", m->name, HZ[h], sem);
                }
            }
        }
    }
    CHECK(dif == 0, "casual com carimbo exato: %ld lutas diferem da coluna ms exato", dif);
    printf("\nCasual, vitórias (%%) com o carimbo exato do clique, %d lutas por mestre:\n\n| Mestre | 30 Hz | 60 Hz | 144 Hz | 240 Hz | ms exato |\n|---|---|---|---|---|---|\n", lutas);
    for (int i = 0; i < roster_size(); i++) {
        printf("| %s |", roster_get(i)->name);
        for (int h = 0; h < 4; h++) printf(" %.0f |", 100.0 * (double)vit[i][h] / lutas);
        printf(" %.0f |\n", 100.0 * (double)vitMs[i] / lutas);
    }
    printf("\n");
}

/* ---- o clique no hitstop ---- */
/* O jogo de verdade (update_duel do main.c), quadro a quadro: o impacto congela o duelo por tempo real (hitstop_passo); o quadro em que o
 * congelamento acaba corre só o que sobra dele; o clique chega com o carimbo do seu instante real (entrada_no_quadro), no congelamento, no
 * quadro em que ele acaba ou depois. O que se espera: o clique é julgado no instante do núcleo que o relógio real tinha quando ele veio.
 * O núcleo para no congelamento, então o instante do núcleo é o mesmo do fim do congelamento para tudo o que veio nele. */
typedef struct { Duel d; float hs; double agora; } Jogo;

/* Um quadro real de `quadro` segundos. `clique`: o clique deste quadro, com o carimbo dele. Devolve se o quadro foi o em que o congelamento acabou. */
static bool quadro_do_jogo(Jogo *j, double quadro, bool clique, double carimbo, DuelEvent *ev, int *ne, long *invalidos) {
    j->agora += quadro;
    double passo = quadro, corrido = quadro;
    bool fimDoCongelamento = false;
    if (j->hs > 0) {
        float sobra = hitstop_passo(&j->hs, (float)quadro);
        if (sobra <= 0) {   /* o quadro inteiro congelado: o clique vale no instante em que o núcleo parou */
            if (clique) duel_press(&j->d);
            *ne = duel_drain(&j->d, ev, MAX_EVENTS);
            return false;
        }
        passo = corrido = sobra;
        fimDoCongelamento = true;
    }
    double t = -1;
    if (clique) {
        bool v;
        t = entrada_no_quadro(carimbo, j->agora, quadro, corrido, passo, &v);
        if (!v && invalidos) (*invalidos)++;
    }
    duel_step_at(&j->d, passo, t);
    *ne = duel_drain(&j->d, ev, MAX_EVENTS);
    for (int i = 0; i < *ne; i++)
        if (ev[i].kind == EV_IMPACT) j->hs = j->d.lastHitstop;   /* on_impact */
    return fimDoCongelamento;
}

static void teste_clique_no_hitstop(int cenas) {
    static const int MESTRES[] = {4, 6, 11, 12};      /* garfiel, hayate, jinshi, oboro: sequências de vários golpes */
    static const double HZ[] = {30, 60, 144, 240};
    long total = 0, difJulg = 0, difErro = 0, noQuadroDoFim = 0, noCongelado = 0, depois = 0, invalidos = 0, semSegundoGolpe = 0;
    uint64_t s = 424242;
    for (int mi = 0; mi < 4; mi++) {
        const MasterProfile *m = roster_get(MESTRES[mi]);
        for (int k = 0; k < cenas; k++) {
            double hz = HZ[k % 4];
            Settings st;
            settings_default(&st);
            settings_for_level(&st, MESTRES[mi]);
            Jogo base;
            memset(&base, 0, sizeof base);
            duel_init(&base.d, &st, m, 7000u + (uint32_t)k);
            base.agora = 5000.0 + 1000.0 * sorteio(&s);
            DuelEvent ev[MAX_EVENTS];
            int ne = 0;
            /* o primeiro golpe de uma sequência: apertado u antes do contato (perfeito, bom ou erro: o congelamento muda) */
            double u = 0.12 * sorteio(&s) - 0.02;
            bool achou = false;
            while (base.d.phase != PH_FINISHED && base.d.clock < 40 && !achou) {
                double quadro = (1.0 / hz) * (0.6 + 0.8 * sorteio(&s));
                bool clique = false;
                double carimbo = 0;
                if (base.hs <= 0 && base.d.phase == PH_WINDUP && !base.d.attempted) {
                    double alvo = base.d.strikeAt - u - base.d.clock;
                    if (alvo >= 0 && alvo < quadro) { clique = true; carimbo = base.agora + alvo; }
                }
                quadro_do_jogo(&base, quadro, clique, carimbo, ev, &ne, NULL);
                for (int i = 0; i < ne; i++)
                    if (ev[i].kind == EV_IMPACT && base.d.comboRemaining > 0 && base.hs > 0) achou = true;
            }
            if (!achou) continue;
            const double clock0 = base.d.clock, hs0 = base.hs, inicio = base.agora, fim = inicio + hs0;
            /* o segundo golpe: um clique de cada instante do congelamento e de um pouco depois dele */
            for (double delta = 0; delta <= hs0 + 0.06; delta += 0.0013) {
                const double tc = inicio + delta;
                const double relogio = tc <= fim ? clock0 : clock0 + (tc - fim);   /* o núcleo parado no congelamento */
                Jogo g = base;
                Duel w = base.d;
                bool clicou = false, terminou = false;
                double clockGolpe = 0;
                DuelEvent im = {0};
                int guarda = 0;
                while (!terminou && g.d.phase != PH_FINISHED && guarda++ < 100000) {
                    double quadro = (1.0 / hz) * (0.6 + 0.8 * sorteio(&s));
                    bool clique = !clicou && tc >= g.agora && tc < g.agora + quadro;
                    bool congeladoAntes = g.hs > 0;
                    if (clique) {
                        clicou = true;
                        if (tc <= fim) noCongelado++; else depois++;
                    }
                    bool fimDoCongel = quadro_do_jogo(&g, quadro, clique, tc, ev, &ne, &invalidos);
                    if (clique && fimDoCongel && congeladoAntes) noQuadroDoFim++;
                    for (int i = 0; i < ne; i++)
                        if (ev[i].kind == EV_IMPACT && g.d.attacks >= 0) { im = ev[i]; terminou = true; clockGolpe = g.d.clock; }
                }
                if (!terminou || !clicou) { semSegundoGolpe++; continue; }
                /* o mesmo duelo sem hitstop: o clique no instante do núcleo que o relógio real tinha (em ms exato) */
                double dd = relogio - clock0;
                if (dd <= 1e-12) duel_press(&w);
                else duel_step_at(&w, dd, dd);
                DuelEvent iw = {0};
                bool achouW = false;
                for (int guardaW = 0; !achouW && w.phase != PH_FINISHED && guardaW < 100000; guardaW++) {
                    duel_step_at(&w, 1.0 / 480, -1);
                    int nw = duel_drain(&w, ev, MAX_EVENTS);
                    for (int i = 0; i < nw; i++)
                        if (ev[i].kind == EV_IMPACT) { iw = ev[i]; achouW = true; }
                }
                (void)clockGolpe;
                total++;
                if (!achouW || im.judgement != iw.judgement) { if (difJulg++ < 3) printf("DIFERE (julgamento): %s, %.0f Hz, delta %.1f ms\n", m->name, hz, delta * 1000); }
                else if (fabsf(im.b - iw.b) > 1e-4f) { if (difErro++ < 3) printf("DIFERE (erro em ms): %s, %.0f Hz, delta %.1f ms: %.3f contra %.3f\n", m->name, hz, delta * 1000, im.b * 1000, iw.b * 1000); }
            }
        }
    }
    CHECK(total > 2000, "clique no hitstop: só %ld cliques conferidos", total);
    CHECK(noCongelado > 200 && noQuadroDoFim > 100 && depois > 200, "clique no hitstop: poucos casos (congelado %ld, no quadro em que acaba %ld, depois %ld)", noCongelado, noQuadroDoFim, depois);
    CHECK(invalidos == 0, "clique no hitstop: %ld carimbos exatos foram tratados como inválidos", invalidos);
    CHECK(difJulg == 0, "clique no hitstop: %ld de %ld cliques julgados diferente do mesmo clique em ms exato no núcleo", difJulg, total);
    CHECK(difErro == 0, "clique no hitstop: %ld de %ld cliques com o erro em ms diferente", difErro, total);
    printf("entrada: %ld cliques no hitstop (%ld durante o congelamento, %ld no quadro em que ele acaba, %ld depois), julgados como em ms exato\n", total,
           noCongelado, noQuadroDoFim, depois);
}

int main(int argc, char **argv) {
    int lutas = argc > 1 ? atoi(argv[1]) : 60;
    if (lutas < 1) lutas = 60;
    teste_conversao();
    teste_invalidos();
    teste_fuzz();
    teste_lutas(lutas);
    teste_tabela(lutas);
    teste_clique_no_hitstop(lutas > 12 ? lutas / 3 : 4);
    printf("entrada: %d verificações, %d falhas\n", checks, failures);
    return failures ? 1 : 0;
}
