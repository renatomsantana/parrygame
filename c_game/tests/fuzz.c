/*
 * fuzz.c - fuzz do núcleo do duelo, sem janela nem raylib.
 *
 * Cada cenário é um mestre, um nível, uma latência de 0 a 120 ms, uma sequência de passos de tempo e
 * apertos aleatórios (rajadas, dois no mesmo instante, cedo, tarde, nenhum) e, de vez em quando,
 * duel_refill, duel_reset e duel_start_seal. Depois de CADA chamada ao núcleo confere:
 *   - valores finitos; vida e postura dentro do intervalo; o relógio só anda para a frente;
 *   - os eventos na ordem: EV_WINDUP, depois EV_LAUNCH e o brilho e o som do aviso (uma vez cada),
 *     depois EV_IMPACT; nada depois de EV_FINISHED; contatos em ordem;
 *   - uma tentativa só por golpe (nada de tentativa dupla);
 *   - o dano de cada impacto (perfeito, bom, erro, golpe duplo) e a postura que o mestre perde;
 *   - vida e postura só mudam num impacto;
 *   - o duelo não trava (30 s de duelo sem golpe nenhum).
 *
 *   fuzz [cenários [semente [modo]]]        make fuzz N=3000   (make teste roda 600 por modo)
 * Os modos dizem como o tempo chega ao núcleo; sem modo, roda todos os que o jogo pode produzir.
 */
#include "../src/robo.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *nome;
    double dtMax;          /* passo de tempo máximo (0 = sem limite: pausa e retomada de até 2 s) */
    bool meio;             /* o aperto entra exatamente no meio do quadro, como no jogo (duel_step) */
    bool padrao;           /* roda quando não se escolhe o modo */
    const char *descricao;
} Modo;

static const Modo MODOS[] = {
    {"50ms",   0.05, false, true,  "quedas de quadro de 33 e 50 ms; aperto em ms exato dentro do quadro"},
    {"100ms",  0.10, false, true,  "quedas de até 100 ms; aperto em ms exato dentro do quadro"},
    {"meio",   0.20, true,  true,  "como o jogo: passos de até 0,2 s (acima disso ele pausa) e aperto no meio do quadro"},
    {"grande", 0,    false, false, "pausa e retomada: passos de até 2 s, e o aperto logo depois do passo (só se pede: fuzz N S grande)"},
};

static uint64_t rs = 88172645463325252ull;
static uint64_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static double ur(void) { return (double)(rnd() >> 11) / 9007199254740992.0; }
static int ri(int n) { return (int)(rnd() % (uint64_t)n); }

static long violacoes = 0, chamadas = 0, cenarios = 0, impactosTotal = 0;
static long porTipo[64];
static const Modo *modo;

#define VIOL(tipo, ...) do { \
    violacoes++; porTipo[tipo]++; \
    if (porTipo[tipo] <= 3) { printf("VIOLAÇÃO[%d] modo %s, cenário %ld: ", tipo, modo->nome, cenarios); printf(__VA_ARGS__); printf("\n"); } \
} while (0)

typedef struct {
    bool aberto;                 /* uma preparação em curso (EV_WINDUP sem EV_IMPACT) */
    bool lancou, avisou;         /* EV_LAUNCH e o brilho do aviso já saíram neste golpe */
    int sons;                    /* o som do aviso: exatamente um por golpe */
    int tentativas;              /* EV_PRESS TENTATIVA neste golpe */
    bool terminou;               /* EV_FINISHED já saiu */
    long impactos, preparacoes;
    double ultimoContato;
} Ordem;

static Settings S;
static Duel d;
static Ordem o;
static const MasterProfile *M;
static bool drenar;              /* em 15% dos cenários ninguém esvazia a fila: ela tem que aguentar */

static void confere_estado(const char *onde) {
    if (!isfinite(d.clock) || !isfinite(d.strikeAt) || !isfinite(d.phaseEnd) || !isfinite(d.windupDuration) || !isfinite(d.renPosture) || !isfinite(d.bossPosture))
        VIOL(1, "valor não finito em %s (relógio %f strikeAt %f phaseEnd %f vida %f postura %f)", onde, d.clock, d.strikeAt, d.phaseEnd, d.renPosture, d.bossPosture);
    if (d.renPosture < -1e-4f || d.renPosture > S.renPosture + 1e-3f) VIOL(2, "vida fora do intervalo em %s: %f", onde, d.renPosture);
    float pm = duel_posture_max(&d);
    if (d.bossPosture < -1e-4f || d.bossPosture > pm + 1e-3f) VIOL(3, "postura fora do intervalo em %s: %f (máx %f)", onde, d.bossPosture, pm);
    if (d.phase > PH_FINISHED) VIOL(4, "fase inválida %d", d.phase);
    if (d.eventCount < 0 || d.eventCount > MAX_EVENTS) VIOL(5, "eventCount %d", d.eventCount);
    if (d.scheduleCount < 0 || d.scheduleCount > 3 || d.scheduleIndex < 0 || d.scheduleIndex > d.scheduleCount) VIOL(6, "agenda %d/%d", d.scheduleIndex, d.scheduleCount);
    if (d.seal < 0 || d.seal >= (M->sealCount > 0 ? M->sealCount : 1)) VIOL(7, "selo %d", d.seal);
    if (d.stanceIndex < 0 || d.stanceIndex >= M->stanceCount) VIOL(8, "postura de luta %d", d.stanceIndex);
    if (d.phase == PH_WINDUP) {
        if (d.move < 0 || d.move >= M->moveCount) VIOL(9, "golpe %d fora do repertório", d.move);
        if (d.comboStrike < 0 || d.comboStrike >= MAX_CHAIN) VIOL(10, "golpe da sequência %d", d.comboStrike);
        if (d.windupDuration <= 0) VIOL(11, "preparação %f", d.windupDuration);
    }
    if (d.comboRemaining < 0 || d.comboRemaining >= MAX_CHAIN) VIOL(13, "comboRemaining %d", d.comboRemaining);
    if (drenar && d.perfects + d.goods + d.bads != (int)o.impactos) VIOL(14, "contagem de julgamentos %d != impactos %ld", d.perfects + d.goods + d.bads, o.impactos);
}

/* Os eventos de uma chamada, na ordem em que saíram. */
static void confere_eventos(const DuelEvent *ev, int n, const char *rotulo, int *impactos, int *preparacoes) {
    for (int i = 0; i < n; i++) {
        const DuelEvent *e = &ev[i];
        if (o.terminou && e->kind != EV_PRESS) VIOL(21, "evento %d depois de EV_FINISHED (%s)", e->kind, rotulo);
        switch (e->kind) {
            case EV_WINDUP:
                if (o.aberto) VIOL(22, "EV_WINDUP com o golpe anterior sem impacto (%s)", rotulo);
                o.aberto = true;
                o.lancou = o.avisou = false;
                o.sons = 0;
                o.tentativas = 0;
                o.preparacoes++;
                (*preparacoes)++;
                break;
            case EV_LAUNCH:
                if (!o.aberto) VIOL(23, "EV_LAUNCH fora de um golpe (%s)", rotulo);
                if (o.lancou) VIOL(24, "EV_LAUNCH duas vezes no mesmo golpe (%s)", rotulo);
                o.lancou = true;
                break;
            case EV_CUE:
                if (!o.aberto) VIOL(25, "EV_CUE fora de um golpe (%s)", rotulo);
                if (!e->flag && o.avisou) VIOL(26, "brilho do aviso duas vezes (%s)", rotulo);
                if (e->flag) o.sons++; else o.avisou = true;
                break;
            case EV_PRESS:
                if (e->i == PRESS_TENTATIVA) {
                    if (!o.aberto) VIOL(27, "tentativa fora de um golpe (%s)", rotulo);
                    if (++o.tentativas > 1) VIOL(28, "TENTATIVA DUPLA no mesmo golpe (%s)", rotulo);
                }
                break;
            case EV_IMPACT:
                if (!o.aberto) VIOL(29, "EV_IMPACT sem golpe aberto (%s)", rotulo);
                else {
                    if (!o.lancou) VIOL(30, "impacto sem a lâmina ter partido (EV_LAUNCH) (%s)", rotulo);
                    if (!o.avisou) VIOL(31, "impacto sem o brilho do aviso (EV_CUE) (%s)", rotulo);
                    if (o.sons != 1) VIOL(41, "som do aviso %d vezes neste golpe (%s)", o.sons, rotulo);
                }
                o.aberto = false;
                o.impactos++;
                (*impactos)++;
                impactosTotal++;
                if (o.ultimoContato > 0 && d.lastStrikeAt < o.ultimoContato - 1e-9) VIOL(32, "contatos fora de ordem: %f depois de %f (%s)", d.lastStrikeAt, o.ultimoContato, rotulo);
                o.ultimoContato = d.lastStrikeAt;
                break;
            case EV_FINISHED:
                if (o.terminou) VIOL(33, "EV_FINISHED duas vezes (%s)", rotulo);
                o.terminou = true;
                break;
            default: break;
        }
    }
}

/* O dano de um impacto que a chamada trouxe sozinho (sem preparação nova, sem brasas, sem selo novo). */
static void confere_dano(const DuelEvent *ev, int n, const char *rotulo, float vidaAntes, float postAntes, float danoAntes, bool dualAntes) {
    for (int i = 0; i < n; i++) {
        if (ev[i].kind != EV_IMPACT) continue;
        float dv = vidaAntes - d.renPosture, esperado;
        bool dual = ev[i].i & 1;
        if (ev[i].judgement == J_PERFEITO) {
            esperado = -fminf(S.perfectHeal * S.renPosture, S.renPosture - vidaAntes);
        } else if (ev[i].judgement == J_BOM) {
            esperado = S.goodRenCost + (dual ? danoAntes : 0);
            if (esperado > vidaAntes) esperado = vidaAntes;
        } else {
            esperado = danoAntes * (dual ? 2 : 1);
            if (esperado > vidaAntes) esperado = vidaAntes;
        }
        if (dualAntes != dual) VIOL(34, "golpe duplo diverge: %d x %d (%s)", dualAntes, dual, rotulo);
        if (fabsf(dv - esperado) > 0.02f) VIOL(35, "DANO fora do esperado: vida %.2f -> %.2f (esperado %.2f), julgamento %d, duplo %d (%s)", vidaAntes, d.renPosture, vidaAntes - esperado, ev[i].judgement, dual, rotulo);
        float pb = postAntes - d.bossPosture;
        if (ev[i].judgement == J_PERFEITO && !ev[i].flag && fabsf(pb - S.perfectBossDamage) > 0.02f) VIOL(36, "postura: perfeito tirou %.2f (esperado %.2f)", pb, S.perfectBossDamage);
        if (ev[i].judgement == J_BOM && !ev[i].flag && fabsf(pb - S.goodBossDamage) > 0.02f) VIOL(37, "postura: bom tirou %.2f (esperado %.2f)", pb, S.goodBossDamage);
    }
}

/* Uma chamada ao núcleo (passo de tempo ou aperto), com tudo conferido em volta. */
typedef enum { C_TICK, C_PRESS } Chamada;
static void chama(Chamada c, double dt, const char *rotulo) {
    chamadas++;
    float vidaAntes = d.renPosture, postAntes = d.bossPosture, danoAntes = duel_ren_damage(&d), brasasAntes = d.burnLeft;
    double relogioAntes = d.clock;
    int faseAntes = d.phase, seloAntes = d.seal;
    bool dualAntes = d.phase == PH_WINDUP && duel_strike_dual(&d);
    if (c == C_TICK) duel_tick(&d, dt); else duel_press(&d);
    if (d.clock + 1e-12 < relogioAntes) VIOL(20, "o relógio andou para trás: %f -> %f (%s)", relogioAntes, d.clock, rotulo);
    if (drenar) {
        DuelEvent ev[MAX_EVENTS];
        int n = duel_drain(&d, ev, MAX_EVENTS), impactos = 0, preparacoes = 0;
        confere_eventos(ev, n, rotulo, &impactos, &preparacoes);
        if (impactos == 1 && preparacoes == 0 && M->burn <= 0 && brasasAntes <= 0 && seloAntes == d.seal) {
            confere_dano(ev, n, rotulo, vidaAntes, postAntes, danoAntes, dualAntes);
        } else if (impactos == 0 && faseAntes == (int)d.phase && d.phase != PH_FINISHED) {
            if (d.renPosture < vidaAntes - 1e-3f && brasasAntes <= 0) VIOL(38, "vida caiu sem impacto: %.2f -> %.2f (%s)", vidaAntes, d.renPosture, rotulo);
            if (d.bossPosture < postAntes - 1e-3f) VIOL(39, "postura caiu sem impacto: %.2f -> %.2f (%s)", postAntes, d.bossPosture, rotulo);
        }
    }
    confere_estado(rotulo);
}

static double proximo_dt(void) {
    double r = ur();
    if (r < 0.40) return 1.0 / 60;
    if (r < 0.50) return 1.0 / 144;
    if (r < 0.58) return 1.0 / 240;
    if (r < 0.68) return 1.0 / 30;
    if (r < 0.72) return 0.050;
    if (r < 0.90) return 0.0005 + ur() * 0.0495;
    if (r < 0.97) return 0.0;                    /* quadro de dt 0 (o relógio para no hitstop) */
    double grande = 0.2 + ur() * 1.8;            /* pausa e retomada, queda grande */
    return grande;
}

static void cenario(uint64_t semente) {
    cenarios++;
    rs = semente * 0x9E3779B97F4A7C15ull + 1;
    M = roster_get(ri(roster_size()));
    settings_default(&S);
    settings_for_level(&S, ri(13));
    static const double LATENCIAS[] = {0, 0.03, 0.06, 0.09, 0.12};
    S.latency = ur() < 0.5 ? (float)LATENCIAS[ri(5)] : (float)(ur() * AJ_LATENCIA_MAX);
    S.audioLead = (float)((ur() - 0.5) * AJ_LATENCIA_MAX);
    if (ur() < 0.15) S.bladeFrom = 0;
    duel_init(&d, &S, M, (uint32_t)rnd());
    memset(&o, 0, sizeof o);
    drenar = ur() < 0.85;
    /* vida e postura variadas para chegar a quebras e a quedas */
    if (ur() < 0.4) d.renPosture = fminf(S.renPosture, (float)(ur() * S.renPosture) + 1);
    if (ur() < 0.4) d.bossPosture = fminf(duel_posture_max(&d), (float)(ur() * duel_posture_max(&d)) + 1);
    if (M->sealCount > 1 && ur() < 0.3) { duel_start_seal(&d, ri(3)); memset(&o, 0, sizeof o); }
    const double perfil = ur();          /* metralhadora, esparso, mira o contato, misto */
    const long passos = 6000 + ri(6000);
    double ultimoProgresso = 0;
    long ultimoImpacto = 0, ultimaPreparacao = 0;
    for (long k = 0; k < passos && d.phase != PH_FINISHED; k++) {
        double dt = proximo_dt();
        if (modo->dtMax > 0 && dt > modo->dtMax) dt = modo->dtMax;
        /* apertos: quantos, e em que instante do quadro */
        int quantos;
        double q = ur();
        if (perfil < 0.25) quantos = q < 0.6 ? 1 + ri(3) : 0;
        else if (perfil < 0.5) quantos = q < 0.05 ? 1 : 0;
        else if (perfil < 0.7) quantos = (d.phase == PH_WINDUP && !d.attempted && d.strikeAt - d.clock < dt + 0.03 && q < 0.7) ? 1 : 0;
        else quantos = q < 0.15 ? 1 : (q < 0.17 ? 5 : 0);
        double desde[6];
        for (int a = 0; a < quantos; a++) desde[a] = modo->meio ? dt * 0.5 : ur() * dt;
        for (int a = 0; a < quantos; a++)
            for (int b = a + 1; b < quantos; b++)
                if (desde[b] < desde[a]) { double t = desde[a]; desde[a] = desde[b]; desde[b] = t; }
        if (!modo->meio && perfil >= 0.5 && perfil < 0.7 && quantos == 1) {     /* mira o contato em ms, com ruído */
            double alvo = d.strikeAt - (0.005 + ur() * 0.06) + S.latency;
            desde[0] = alvo > d.clock ? (alvo - d.clock < dt ? alvo - d.clock : dt) : 0;
        }
        double t = 0;
        for (int a = 0; a < quantos; a++) {
            if (desde[a] > t) { chama(C_TICK, desde[a] - t, "tick"); t = desde[a]; }
            chama(C_PRESS, 0, "aperto");
            if (ur() < 0.08) chama(C_PRESS, 0, "aperto duplo no mesmo instante");
            if (d.phase == PH_FINISHED) break;
        }
        if (d.phase != PH_FINISHED) chama(C_TICK, dt - t > 0 ? dt - t : 0, "tick");
        /* operações raras */
        double r = ur();
        if (r < 0.0008) {
            duel_refill(&d, ur() < 0.5, ur() < 0.5);
            if (drenar) { DuelEvent ev[MAX_EVENTS]; duel_drain(&d, ev, MAX_EVENTS); }
            confere_estado("refill");
        } else if (r < 0.0012) {
            duel_reset(&d);
            memset(&o, 0, sizeof o);
            confere_estado("reset");
        } else if (r < 0.0016 && M->sealCount > 1) {
            duel_start_seal(&d, ri(4));
            memset(&o, 0, sizeof o);
            confere_estado("start_seal");
        }
        /* não travar: o duelo tem que continuar produzindo golpes */
        if (d.clock - ultimoProgresso > 30) {
            if (drenar && o.impactos == ultimoImpacto && o.preparacoes == ultimaPreparacao && d.phase != PH_FINISHED)
                VIOL(50, "TRAVOU: %.0f s de duelo sem golpe (fase %d, mestre %s, selo %d)", d.clock - ultimoProgresso, d.phase, M->name, d.seal);
            ultimoProgresso = d.clock;
            ultimoImpacto = o.impactos;
            ultimaPreparacao = o.preparacoes;
        }
        if (d.clock > 4000) break;
    }
}

static int roda(const Modo *m, long n, uint64_t semente) {
    modo = m;
    violacoes = chamadas = cenarios = impactosTotal = 0;
    memset(porTipo, 0, sizeof porTipo);
    for (long i = 0; i < n; i++) cenario(semente + (uint64_t)i);
    printf("fuzz %-6s: %ld cenários, %ld chamadas ao núcleo, %ld impactos, %ld violações (%s)\n", m->nome, cenarios, chamadas, impactosTotal, violacoes, m->descricao);
    for (int t = 0; t < 64; t++) if (porTipo[t]) printf("  tipo %d: %ld\n", t, porTipo[t]);
    return violacoes ? 1 : 0;
}

int main(int argc, char **argv) {
    long n = argc > 1 ? atol(argv[1]) : 600;
    uint64_t semente = argc > 2 ? (uint64_t)atoll(argv[2]) : 1;
    const char *pedido = argc > 3 ? argv[3] : NULL;
    int falhas = 0, rodou = 0;
    for (unsigned i = 0; i < sizeof MODOS / sizeof MODOS[0]; i++) {
        if (pedido ? strcmp(pedido, MODOS[i].nome) != 0 : !MODOS[i].padrao) continue;
        falhas += roda(&MODOS[i], n, semente);
        rodou++;
    }
    if (!rodou) { fprintf(stderr, "fuzz: modo desconhecido \"%s\"\n", pedido); return 2; }
    return falhas ? 1 : 0;
}
