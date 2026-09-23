/*
 * core_test.c - verificações do núcleo, sem janela nem raylib.
 * Rodar: make test
 */
#include "../src/core.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int checks = 0, failures = 0;

#define CHECK(cond, ...) do { \
    checks++; \
    if (!(cond)) { failures++; printf("FALHA %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } \
} while (0)

#define DT (1.0 / 240.0)

typedef struct {
    int impacts[4];
    int seals, stances, combos, windups, finished, victory, feintLaunches, fakeCues, realCues;
    Judgement last;
} Tally;

static void count(Duel *d, Tally *t) {
    DuelEvent ev[MAX_EVENTS];
    int n = duel_drain(d, ev, MAX_EVENTS);
    for (int i = 0; i < n; i++) {
        switch (ev[i].kind) {
            case EV_IMPACT: t->impacts[ev[i].judgement]++; t->last = ev[i].judgement; break;
            case EV_SEAL: t->seals++; break;
            case EV_STANCE: t->stances++; break;
            case EV_COMBO: t->combos++; break;
            case EV_WINDUP: t->windups++; break;
            case EV_FINISHED: t->finished++; t->victory = ev[i].flag; break;
            case EV_FEINT_LAUNCH: t->feintLaunches++; break;
            case EV_CUE: if (ev[i].flag) t->fakeCues++; else t->realCues++; break;
            default: break;
        }
    }
}

/* Joga até o fim apertando `lead` segundos antes do contato real (lead < 0: nunca aperta). */
static Tally play(const MasterProfile *m, uint32_t seed, double lead, double limit) {
    Settings s;
    settings_default(&s);
    Duel d;
    duel_init(&d, &s, m, seed);
    Tally t;
    memset(&t, 0, sizeof t);
    while (d.phase != PH_FINISHED && d.clock < limit) {
        duel_tick(&d, DT);
        if (lead >= 0 && d.phase == PH_WINDUP && !d.attempted && d.strikeAt - d.clock <= lead) duel_press(&d);
        count(&d, &t);
    }
    return t;
}

static void test_settings(void) {
    Settings s;
    settings_default(&s);
    CHECK(s.renPosture == 250, "musashi começa com 250 de postura");
    CHECK(s.perfectBossDamage > s.goodBossDamage, "perfeito vale mais que bom");
    CHECK(s.goodRenCost < s.badPostureDamage, "bom custa menos que ruim");
}

static void test_roster(const Settings *s) {
    CHECK(roster_size() == 12, "onze aprendizes e oboro");
    CHECK(roster_get(-1) == NULL && roster_get(12) == NULL, "índices fora da trilha");
    float lastPerfect = 1, lastGood = 1;
    for (int i = 0; i < roster_size(); i++) {
        const MasterProfile *m = roster_get(i);
        CHECK(m->id == i + 1, "id em ordem (%s)", m->name);
        CHECK(m->name && m->name[0], "nome do mestre %d", i + 1);
        CHECK(m->style && (m->isBigBoss || strncmp(m->style, "postura d", 9) == 0), "cada aprendiz tem uma postura (%s)", m->name);
        CHECK(m->posture > 0, "postura positiva (%s)", m->name);
        CHECK(m->introCount >= 2 && m->outroCount >= 1, "falas antes e depois (%s)", m->name);
        for (int o = 0; o < i; o++) CHECK(roster_get(o)->arena != m->arena, "um cenário próprio por lutador (%s)", m->name);
        CHECK(m->isBigBoss == (i == 11), "só o último é oboro (%s)", m->name);
        CHECK(m->stanceCount >= 1, "ao menos uma guarda (%s)", m->name);
        for (int k = 0; k < m->stanceCount; k++) {
            const Stance *st = &m->stances[k];
            CHECK(st->windupCount > 0, "preparações (%s)", m->name);
            CHECK(st->perfectWindow > 0 && st->perfectWindow < st->goodWindow, "perfeito dentro do bom (%s)", m->name);
            if (st->feintChance > 0) {
                double lo = st->feintDelayMax > 0 ? st->feintDelayMin : s->feintDelayMin;
                CHECK(lo > st->goodWindow, "atraso mínimo da finta maior que a janela boa (%s)", m->name);
                CHECK(lo / st->falseCues >= s->cueLead - 1e-6, "instantes falsos afastados um sinal inteiro (%s)", m->name);
                CHECK(st->falseCues >= 1 && st->falseCues <= MAX_FALSE_CUES, "instantes falsos (%s)", m->name);
            }
        }
        if (!m->isBigBoss) {
            CHECK(m->stances[0].perfectWindow <= lastPerfect + 1e-6, "janela perfeita não cresce (%s)", m->name);
            CHECK(m->stances[0].goodWindow <= lastGood + 1e-6, "janela boa não cresce (%s)", m->name);
            lastPerfect = m->stances[0].perfectWindow;
            lastGood = m->stances[0].goodWindow;
        }
    }
    float lastDamage = 0;
    for (int i = 0; i < roster_size(); i++) {
        Duel d;
        duel_init(&d, s, roster_get(i), 1);
        float dmg = duel_ren_damage(&d);
        CHECK(dmg >= lastDamage, "o dano de um erro não diminui ao longo da trilha (%s)", roster_get(i)->name);
        lastDamage = dmg;
    }
    static const int HITS[12] = {50, 40, 35, 30, 25, 22, 20, 18, 15, 12, 10, 10};
    for (int i = 0; i < roster_size(); i++) {
        CHECK(roster_get(i)->hitsToFall == HITS[i], "Ren aguenta %d erros contra %s", HITS[i], roster_get(i)->name);
        CHECK(roster_get(i)->senseiCount >= 1, "hanzo tem conselho para %s", roster_get(i)->name);
        for (int k = 0; k < roster_get(i)->senseiCount; k++)
            CHECK(strcmp(roster_get(i)->sensei[k].speaker, "hanzo") == 0, "quem aconselha é hanzo (%s)", roster_get(i)->name);
    }
    CHECK(roster_get(11)->specialChance > 0, "oboro tem golpe especial");
    for (int i = 0; i < 11; i++) CHECK(roster_get(i)->specialChance == 0, "só o BIG BOSS tem especial (%s)", roster_get(i)->name);
    const MasterProfile *boss = roster_get(11);
    CHECK(boss->sealCount == 3, "o BIG BOSS tem três selos");
    CHECK(boss->stanceCount == 11, "oboro domina as onze posturas");
    for (int k = 0; k < 11; k++) CHECK(strcmp(boss->stances[k].name, roster_get(k)->style) == 0, "a %dª postura de oboro é a de %s", k + 1, roster_get(k)->name);
    for (int i = 0; i < LORE_PAGES; i++) CHECK(lore_page(i)[0] != 0, "página %d da lore", i);
}

static void test_rng(void) {
    Rng a, b;
    rng_seed(&a, 42);
    rng_seed(&b, 42);
    bool same = true, inRange = true;
    for (int i = 0; i < 1000; i++) {
        double x = rng_next(&a), y = rng_next(&b);
        if (x != y) same = false;
        if (x < 0 || x >= 1) inRange = false;
    }
    CHECK(same, "mesma semente, mesma sequência");
    CHECK(inRange, "valores em [0, 1)");
    /* Mesmo valor do plugin JS (mulberry32 com semente 1). */
    Rng c;
    rng_seed(&c, 1);
    CHECK(fabs(rng_next(&c) - 0.6270739405881613) < 1e-12, "mulberry32 idêntico ao JS");
}

static void test_perfect_victory(void) {
    const MasterProfile *gorou = roster_get(0);
    Tally t = play(gorou, 7, 0.02, 120);
    CHECK(t.finished == 1 && t.victory, "perfeitos vencem o Gorou");
    int need = (int)ceilf(gorou->posture / 20.0f);
    CHECK(t.impacts[J_PERFEITO] == need, "%d perfeitos quebram %d de postura (%d)", need, (int)gorou->posture, t.impacts[J_PERFEITO]);
    CHECK(t.impacts[J_RUIM] == 0, "nenhum erro");
}

static void test_no_defense(void) {
    Tally t = play(roster_get(0), 7, -1, 120);
    CHECK(t.finished == 1 && !t.victory, "sem defesa, Ren cai");
    CHECK(t.impacts[J_RUIM] == 50, "contra o primeiro mestre, musashi aguenta 50 erros (%d)", t.impacts[J_RUIM]);
    for (int i = 0; i < 11; i++) {
        Tally k = play(roster_get(i), 7, -1, 600);
        CHECK(k.impacts[J_RUIM] == roster_get(i)->hitsToFall, "%s derruba Ren em %d erros (%d)", roster_get(i)->name,
              roster_get(i)->hitsToFall, k.impacts[J_RUIM]);
    }
}

static void test_good_only(void) {
    const MasterProfile *gorou = roster_get(0);
    Settings s;
    settings_default(&s);
    Duel d;
    duel_init(&d, &s, gorou, 3);
    Tally t;
    memset(&t, 0, sizeof t);
    /* Apertar no meio entre a janela perfeita e a boa. */
    double lead = (gorou->stances[0].perfectWindow + gorou->stances[0].goodWindow) / 2;
    while (t.impacts[J_BOM] == 0 && d.clock < 10) {
        duel_tick(&d, DT);
        if (d.phase == PH_WINDUP && !d.attempted && d.strikeAt - d.clock <= lead) duel_press(&d);
        count(&d, &t);
    }
    CHECK(t.impacts[J_BOM] == 1, "bom no meio das janelas");
    CHECK(fabsf(d.bossPosture - (gorou->posture - s.goodBossDamage)) < 1e-4, "bom tira postura do mestre");
    CHECK(fabsf(d.renPosture - (s.renPosture - s.goodRenCost)) < 1e-4, "bom custa um pouco a Ren");

    Tally all = play(gorou, 3, lead, 600);
    CHECK(all.finished == 1 && all.victory, "só bons ainda vencem o Gorou");
}

/* Um perfeito e depois um erro: devolve a postura final do mestre. */
static float posture_after_perfect_then_miss(const MasterProfile *m, Duel *d) {
    Settings s;
    settings_default(&s);
    duel_init(d, &s, m, 5);
    Tally t;
    memset(&t, 0, sizeof t);
    int state = 0;
    while (t.impacts[J_RUIM] == 0 && d->clock < 20) {
        duel_tick(d, DT);
        if (state == 0 && d->phase == PH_WINDUP && !d->attempted && d->strikeAt - d->clock <= 0.02) duel_press(d);
        count(d, &t);
        if (t.impacts[J_PERFEITO] == 1) state = 1;
    }
    return d->bossPosture;
}

static void test_bad_recovers_boss(void) {
    Settings s;
    settings_default(&s);
    Duel d;
    for (int i = 0; i < ROSTER_SIZE; i++) CHECK(roster_get(i)->healsOnHit == (i >= 4), "%s %s postura ao acertar", roster_get(i)->name,
                                                i >= 4 ? "recupera" : "não recupera");
    const MasterProfile *tetsu = roster_get(0), *kaelen = roster_get(4);
    float a = posture_after_perfect_then_miss(tetsu, &d);
    CHECK(fabsf(a - (tetsu->posture - s.perfectBossDamage)) < 1e-4, "os quatro primeiros não recuperam postura ao acertar (%.1f)", a);
    CHECK(fabsf(d.renPosture - (s.renPosture - duel_ren_damage(&d))) < 1e-4, "perfeito não passa de 100; o erro tira o dano do mestre");
    float b = posture_after_perfect_then_miss(kaelen, &d);
    CHECK(fabsf(b - (kaelen->posture - s.perfectBossDamage + s.badBossRecover)) < 1e-4, "do quinto em diante, acertar devolve postura (%.1f)", b);
}

static void test_one_attempt_and_cooldown(void) {
    Settings s;
    settings_default(&s);
    CHECK(fabsf(s.inputCooldown - 0.30f) < 1e-6, "0,3 s entre gestos");
    Duel d;
    duel_init(&d, &s, roster_get(0), 9);
    while (d.phase != PH_WINDUP) duel_tick(&d, DT);
    CHECK(duel_press(&d), "primeiro gesto aceito");
    CHECK(!duel_press(&d), "segundo gesto no mesmo golpe recusado");
    Duel e;
    duel_init(&e, &s, roster_get(0), 9);
    CHECK(duel_press(&e), "gesto fora da preparação aceito");
    duel_tick(&e, 0.1);
    CHECK(!duel_press(&e), "intervalo mínimo entre gestos");
    duel_tick(&e, s.inputCooldown);
    CHECK(duel_press(&e), "depois do intervalo, aceito de novo");
    /* Um gesto no intervalo não rouba a defesa do golpe que chega logo depois. */
    Duel f;
    duel_init(&f, &s, roster_get(0), 9);
    while (f.phase != PH_READY || f.phaseEnd - f.clock > 0.1) duel_tick(&f, DT);
    duel_press(&f);
    while (f.phase != PH_WINDUP) duel_tick(&f, DT);
    CHECK(duel_press(&f), "o golpe novo aceita defesa mesmo logo depois de um gesto");
}

static MasterProfile with_feints(int index, float chance, int cues, float lo, float hi) {
    MasterProfile m = *roster_get(index);
    for (int i = 0; i < m.stanceCount; i++) {
        m.stances[i].feintChance = chance;
        m.stances[i].falseCues = cues;
        m.stances[i].feintDelayMin = lo;
        m.stances[i].feintDelayMax = hi;
    }
    return m;
}

static void test_no_feints_in_trail(void) {
    for (int i = 0; i < roster_size(); i++)
        for (int k = 0; k < roster_get(i)->stanceCount; k++) {
            CHECK(roster_get(i)->stances[k].feintChance == 0, "%s não finta", roster_get(i)->name);
            CHECK(!roster_get(i)->stances[k].mimicParry, "%s não imita o parry", roster_get(i)->name);
        }
}

static void test_feint_punishes(void) {
    MasterProfile vanceFeint = with_feints(3, 0.4f, 2, 0.40f, 0.55f);
    const MasterProfile *vance = &vanceFeint;
    Settings s;
    settings_default(&s);
    int tested = 0;
    for (uint32_t seed = 1; seed < 200 && tested < 5; seed++) {
        Duel d;
        duel_init(&d, &s, vance, seed);
        while (d.phase != PH_WINDUP) duel_tick(&d, DT);
        if (!d.isFeint) continue;
        tested++;
        CHECK(d.fakeCount == 2, "Vance usa dois instantes falsos");
        for (int i = 0; i + 1 < d.fakeCount; i++)
            CHECK(d.fakeStrikeAts[i + 1] - d.fakeStrikeAts[i] >= s.cueLead - 1e-6, "instantes falsos afastados");
        CHECK(d.strikeAt - d.fakeStrikeAts[d.fakeCount - 1] >= s.cueLead - 1e-6, "contato real afastado do último falso");
        double fake = d.fakeStrikeAts[0];
        while (d.clock < fake - 0.01) duel_tick(&d, DT);
        CHECK(duel_time_to_next_instant(&d) < duel_time_to_impact(&d), "a tela conta até o instante falso");
        duel_press(&d);
        Tally t;
        memset(&t, 0, sizeof t);
        while (t.impacts[J_RUIM] + t.impacts[J_BOM] + t.impacts[J_PERFEITO] == 0) { duel_tick(&d, DT); count(&d, &t); }
        CHECK(t.impacts[J_RUIM] == 1, "caiu na finta");
    }
    CHECK(tested == 5, "fintas sorteadas para o Vance");
}

static void test_big_step_order(void) {
    MasterProfile eleonorFeint = with_feints(6, 0.55f, 3, 0.54f, 0.72f);
    const MasterProfile *eleonor = &eleonorFeint;
    Settings s;
    settings_default(&s);
    for (uint32_t seed = 1; seed < 100; seed++) {
        Duel d;
        duel_init(&d, &s, eleonor, seed);
        while (d.phase != PH_WINDUP) duel_tick(&d, DT);
        if (!d.isFeint) continue;
        duel_drain(&d, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
        duel_tick(&d, 5.0);
        DuelEvent ev[MAX_EVENTS];
        int n = duel_drain(&d, ev, MAX_EVENTS);
        int fakeCues = 0, lastKindWasImpact = n > 0 && ev[n - 1].kind == EV_IMPACT;
        bool realAfterFakes = true;
        for (int i = 0; i < n; i++) {
            if (ev[i].kind == EV_CUE && ev[i].flag) fakeCues++;
            if (ev[i].kind == EV_CUE && !ev[i].flag && fakeCues != 3) realAfterFakes = false;
        }
        CHECK(fakeCues == 3, "Eleonor: três sinais falsos num passo grande");
        CHECK(realAfterFakes, "o sinal real vem depois dos falsos");
        CHECK(lastKindWasImpact, "o impacto fecha o golpe");
        return;
    }
    CHECK(false, "nenhuma finta da Eleonor sorteada");
}

static void test_accelerando(void) {
    const MasterProfile *taiko = roster_get(7);
    Settings s;
    settings_default(&s);
    s.pressureSpeed = 1; /* isola o efeito do ciclo */
    Duel d;
    duel_init(&d, &s, taiko, 11);
    float durations[6];
    int got = 0;
    while (got < 6 && d.clock < 60 && d.phase != PH_FINISHED) {
        d.bossPosture = 1e6f; /* o duelo não acaba antes de seis sequências */
        duel_tick(&d, DT);
        if (d.phase == PH_WINDUP && !d.attempted && d.strikeAt - d.clock <= 0.02) duel_press(&d);
        DuelEvent ev[MAX_EVENTS];
        int n = duel_drain(&d, ev, MAX_EVENTS);
        /* Só a primeira preparação de cada sequência acelera. */
        for (int i = 0; i < n; i++) if (ev[i].kind == EV_WINDUP && d.comboStrike == 0 && got < 6) durations[got++] = ev[i].a;
    }
    CHECK(got == 6, "seis sequências do Taiko");
    CHECK(durations[4] < durations[0] * 0.6f, "a quinta sequência do ciclo arma bem mais rápido (%.2f -> %.2f)", durations[0], durations[4]);
    CHECK(durations[5] > durations[4], "o ciclo recomeça");
}

static void test_combos(void) {
    Tally kira = play(roster_get(9), 21, 0.02, 300);
    CHECK(kira.combos > 0, "Kira abre golpes duplos");
    CHECK(kira.victory, "perfeitos vencem a Kira mesmo nos compostos");
    Tally magna = play(roster_get(6), 21, 0.02, 300);
    CHECK(magna.combos > 0, "Magna abre golpes triplos");
}

static void test_blackout_and_cues(void) {
    const MasterProfile *yoru = roster_get(2);
    Settings s;
    settings_default(&s);
    int dark = 0, lit = 0;
    for (uint32_t seed = 1; seed < 60; seed++) {
        Duel d;
        duel_init(&d, &s, yoru, seed);
        while (d.phase != PH_WINDUP) duel_tick(&d, DT);
        if (d.blackout) dark++; else lit++;
    }
    CHECK(dark > 10 && lit > 10, "Yoru apaga as luzes em parte dos golpes (%d/%d)", dark, lit);
    CHECK(roster_get(10)->cueAudio == 0 && roster_get(10)->cueVisual > 0, "jinshi: sinal mudo, só visual");
}

static void test_pressure(void) {
    const MasterProfile *gorou = roster_get(0);
    Settings s;
    settings_default(&s);
    Duel d;
    duel_init(&d, &s, gorou, 2);
    d.bossPosture = gorou->posture / 2;
    while (d.phase != PH_WINDUP) duel_tick(&d, DT);
    CHECK(fabsf(d.windupDuration - gorou->stances[0].windups[0] * s.pressureSpeed) < 1e-4, "com metade da postura, 10%% mais rápido");
}

static void test_big_boss(void) {
    const MasterProfile *oboro = roster_get(11);
    Tally t = play(oboro, 99, 0.02, 600);
    CHECK(t.finished == 1 && t.victory, "perfeitos vencem o Oboro");
    CHECK(t.seals == 2, "dois selos quebrados antes do último (%d)", t.seals);
    int perSeal = (int)ceilf(oboro->posture / 20.0f);
    CHECK(t.impacts[J_PERFEITO] == perSeal * 3, "%d perfeitos por selo (%d)", perSeal, t.impacts[J_PERFEITO]);
    CHECK(t.stances > 0, "Oboro troca de guarda");
    CHECK(t.combos > 0, "Oboro abre golpes compostos nos selos seguintes");

    Settings s;
    settings_default(&s);
    Duel d;
    duel_init(&d, &s, oboro, 99);
    for (int i = 0; i < oboro->moveCount; i++)
        if (oboro->moves[i].minSeal > 0) CHECK(oboro->moves[i].strikes >= 4, "as sequências longas ficam para os selos seguintes");
    /* No primeiro selo, nenhuma sequência de selo avançado sai. */
    {
        Duel f;
        duel_init(&f, &s, oboro, 5);
        bool early = true;
        for (int k = 0; k < 40 && f.phase != PH_FINISHED; k++) {
            while (f.phase != PH_WINDUP) duel_tick(&f, DT);
            if (duel_move(&f) && duel_move(&f)->minSeal > 0) early = false;
            while (f.phase == PH_WINDUP) { if (!f.attempted && f.strikeAt - f.clock <= 0.01) duel_press(&f); duel_tick(&f, DT); }
            if (f.seal > 0) break;
        }
        CHECK(early, "sequências de selos avançados não aparecem no primeiro selo");
    }
    /* Quebrar um selo devolve fôlego a Ren. */
    d.renPosture = 40;
    d.bossPosture = s.perfectBossDamage;
    while (d.phase != PH_WINDUP) duel_tick(&d, DT);
    while (d.phase == PH_WINDUP) {
        if (!d.attempted && d.strikeAt - d.clock <= 0.02) duel_press(&d);
        duel_tick(&d, DT);
    }
    CHECK(d.seal == 1, "primeiro selo quebrado");
    CHECK(fabsf(d.renPosture - (40 + s.perfectRenRecover + s.sealRenRecover)) < 1e-4, "selo quebrado devolve fôlego (%.1f)", d.renPosture);
    CHECK(fabsf(d.bossPosture - oboro->posture) < 1e-4, "novo selo com postura cheia");
    Tally lose = play(oboro, 99, -1, 600);
    CHECK(!lose.victory && lose.finished == 1, "sem defesa, Ren cai contra o Oboro");
}

/* Oboro mostra as posturas uma a uma: a guarda muda ao longo do duelo e cada eco sai na postura certa. */
static void test_mimic(void) {
    const MasterProfile *oboro = roster_get(11);
    Settings s;
    settings_default(&s);
    Duel d;
    duel_init(&d, &s, oboro, 5);
    int seen[MAX_STANCES] = {0};
    bool matching = true;
    for (int k = 0; k < 40 && d.phase != PH_FINISHED; k++) {
        while (d.phase != PH_WINDUP) duel_tick(&d, DT);
        const Move *mv = duel_move(&d);
        if (mv && d.comboStrike == 0 && mv->stance >= 0 && mv->stance != d.stanceIndex) matching = false;
        seen[d.stanceIndex] = 1;
        while (d.phase == PH_WINDUP) { if (!d.attempted && d.strikeAt - d.clock <= 0.01) duel_press(&d); duel_tick(&d, DT); }
    }
    int count = 0;
    for (int i = 0; i < MAX_STANCES; i++) count += seen[i];
    CHECK(count >= 6, "oboro passa por várias posturas (%d)", count);
    CHECK(matching, "cada eco sai na postura do aprendiz dele");
    Tally t = play(oboro, 5, 0.02, 900);
    CHECK(t.victory && t.feintLaunches == 0, "perfeitos vencem oboro, sem fintas");
}

static void test_determinism(void) {
    for (int i = 0; i < roster_size(); i++) {
        Tally a = play(roster_get(i), 1234, 0.1, 300);
        Tally b = play(roster_get(i), 1234, 0.1, 300);
        CHECK(memcmp(a.impacts, b.impacts, sizeof a.impacts) == 0 && a.windups == b.windups,
              "mesma semente, mesmo duelo (%s)", roster_get(i)->name);
    }
}

static void test_every_master_beatable(void) {
    for (int i = 0; i < roster_size(); i++) {
        for (uint32_t seed = 1; seed <= 20; seed++) {
            Tally t = play(roster_get(i), seed, 0.01, 900);
            if (!(t.finished && t.victory && t.impacts[J_RUIM] == 0)) {
                CHECK(false, "%s vencível com timing perfeito (semente %u)", roster_get(i)->name, seed);
                break;
            }
            checks++;
        }
    }
}

static void test_levels(void) {
    Settings a, b;
    settings_default(&a);
    settings_default(&b);
    settings_for_level(&b, 5);
    CHECK(b.renPosture > a.renPosture, "Ren ganha postura a cada mestre vencido");
    CHECK(b.perfectBossDamage > a.perfectBossDamage, "Ren bate mais forte a cada mestre vencido");
    CHECK(b.goodBossDamage > a.goodBossDamage, "o bom também cresce");
    /* Com Ren mais forte, o número de erros continua o do mestre. */
    Duel d;
    duel_init(&d, &b, roster_get(5), 3);
    Tally t;
    memset(&t, 0, sizeof t);
    while (d.phase != PH_FINISHED && d.clock < 600) { duel_tick(&d, DT); count(&d, &t); }
    CHECK(t.impacts[J_RUIM] == roster_get(5)->hitsToFall, "nível alto não muda quantos erros Ren aguenta (%d)", t.impacts[J_RUIM]);
    /* Menos perfeitos para quebrar o mestre quando Ren é mais forte. */
    Settings lo, hi;
    settings_default(&lo);
    settings_default(&hi);
    settings_for_level(&hi, 11);
    CHECK(ceilf(roster_get(11)->posture / hi.perfectBossDamage) < ceilf(roster_get(11)->posture / lo.perfectBossDamage),
          "Ren forte precisa de menos perfeitos");
}

static void test_special(void) {
    const MasterProfile *oboro = roster_get(11);
    Settings s;
    settings_default(&s);
    int specials = 0, doubled = 0;
    for (uint32_t seed = 1; seed < 80; seed++) {
        Duel d;
        duel_init(&d, &s, oboro, seed);
        while (d.phase != PH_WINDUP) duel_tick(&d, DT);
        if (!d.special) continue;
        specials++;
        float before = d.renPosture;
        while (d.phase == PH_WINDUP) duel_tick(&d, DT);
        if (fabsf((before - d.renPosture) - 2 * s.renPosture / oboro->hitsToFall) < 1e-3) doubled++;
    }
    CHECK(specials > 5, "oboro solta golpes especiais (%d)", specials);
    CHECK(doubled == specials, "o especial tira o dobro (%d de %d)", doubled, specials);
}

static void test_movesets(void) {
    Settings s;
    settings_default(&s);
    /* Os três primeiros são simples: dois golpes, nenhum com mais de dois contatos. */
    for (int i = 0; i < 3; i++) {
        CHECK(roster_get(i)->moveCount == 2, "%s tem só dois golpes", roster_get(i)->name);
        for (int k = 0; k < roster_get(i)->moveCount; k++) CHECK(roster_get(i)->moves[k].strikes <= 2, "%s: nada acima de dois contatos", roster_get(i)->name);
    }
    /* Cada golpe tem nome próprio: nenhum mestre repete o de outro. */
    for (int a = 0; a < roster_size(); a++)
        for (int i = 0; i < roster_get(a)->moveCount; i++)
            for (int b = a + 1; b < roster_size(); b++)
                for (int k = 0; k < roster_get(b)->moveCount; k++)
                    CHECK(strcmp(roster_get(a)->moves[i].name, roster_get(b)->moves[k].name) != 0, "golpe %s é só de %s", roster_get(a)->moves[i].name, roster_get(a)->name);
    for (int i = 0; i < roster_size(); i++) {
        const MasterProfile *m = roster_get(i);
        CHECK(m->moveCount >= 2, "%s tem um repertório de sequências", m->name);
        bool chain = false;
        for (int k = 0; k < m->moveCount; k++) {
            const Move *mv = &m->moves[k];
            CHECK(mv->strikes >= 1 && mv->strikes <= MAX_CHAIN, "%s: %s tem de 1 a %d golpes", m->name, mv->name, MAX_CHAIN);
            CHECK(mv->weight > 0, "%s: %s pode ser sorteada", m->name, mv->name);
            if (mv->strikes > 1) chain = true;
            for (int g = 0; g + 1 < mv->strikes; g++)
                CHECK(mv->gaps[g] >= s.minChainGap - 1e-6, "%s: %s dá tempo de aparar cada golpe", m->name, mv->name);
        }
        CHECK(chain, "%s tem sequências de mais de um golpe", m->name);
    }
    /* O intervalo entre contatos de uma sequência é exatamente o do moveset. */
    const MasterProfile *tetsu = roster_get(0);
    Duel d;
    duel_init(&d, &s, tetsu, 1);
    bool checked = false;
    for (int k = 0; k < 60 && !checked; k++) {
        while (d.phase != PH_WINDUP) duel_tick(&d, DT);
        const Move *mv = duel_move(&d);
        if (mv && mv->strikes >= 2 && d.comboStrike == 0 && !d.isFeint) {
            double first = d.strikeAt;
            while (d.phase == PH_WINDUP) { if (!d.attempted && d.strikeAt - d.clock <= 0.01) duel_press(&d); duel_tick(&d, DT); }
            while (d.phase != PH_WINDUP) duel_tick(&d, DT);
            CHECK(fabs((d.strikeAt - first) - mv->gaps[0]) < 0.01, "segundo golpe chega %.2f s depois (%.3f)", mv->gaps[0], d.strikeAt - first);
            checked = true;
        }
        while (d.phase == PH_WINDUP) { if (!d.attempted && d.strikeAt - d.clock <= 0.01) duel_press(&d); duel_tick(&d, DT); }
    }
    CHECK(checked, "uma sequência dupla do tetsu foi observada");
}

static void test_campaign(void) {
    Campaign c;
    campaign_reset(&c);
    CHECK(c.index == 0 && campaign_defeated(&c) == 0, "trilha começa vazia");
    for (int i = 0; i < 11; i++) {
        CHECK(!campaign_big_boss_open(&c), "oboro fechado antes dos onze aprendizes");
        campaign_mark_cleared(&c, c.index);
        CHECK(campaign_advance(&c), "avança ao próximo");
    }
    CHECK(campaign_defeated(&c) == 11, "onze aprendizes vencidos");
    CHECK(campaign_big_boss_open(&c), "oboro aberto");
    CHECK(c.index == 11, "o décimo segundo é oboro");
    campaign_mark_cleared(&c, 11);
    CHECK(campaign_defeated(&c) == 11, "oboro não conta entre os onze");
    CHECK(!campaign_advance(&c) && c.completed, "trilha completa");
    campaign_mark_cleared(&c, 3);
    campaign_mark_cleared(&c, 3);
    CHECK(campaign_defeated(&c) == 11, "cada aprendiz contado uma vez só");
}

int main(void) {
    Settings s;
    settings_default(&s);
    test_settings();
    test_roster(&s);
    test_rng();
    test_perfect_victory();
    test_no_defense();
    test_good_only();
    test_bad_recovers_boss();
    test_one_attempt_and_cooldown();
    test_no_feints_in_trail();
    test_feint_punishes();
    test_big_step_order();
    test_accelerando();
    test_combos();
    test_blackout_and_cues();
    test_pressure();
    test_big_boss();
    test_mimic();
    test_determinism();
    test_every_master_beatable();
    test_movesets();
    test_levels();
    test_special();
    test_campaign();
    printf("%d verificações, %d falhas\n", checks, failures);
    return failures ? 1 : 0;
}
