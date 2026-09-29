/*
 * core_test.c - verificações do núcleo, sem janela nem raylib.
 * Rodar: make test
 */
#include "../src/core.h"
#include "../src/robo.h"

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
    int blades;                   /* lâminas que acertaram kojiro (o duplo errado conta duas) */
    int seals, stances, combos, windups, finished, victory, cues;
    Judgement last;
} Tally;

static void count(Duel *d, Tally *t) {
    DuelEvent ev[MAX_EVENTS];
    int n = duel_drain(d, ev, MAX_EVENTS);
    for (int i = 0; i < n; i++) {
        switch (ev[i].kind) {
            case EV_IMPACT:
                t->impacts[ev[i].judgement]++;
                t->last = ev[i].judgement;
                if (ev[i].judgement == J_RUIM) t->blades += (ev[i].i & 1) ? 2 : 1;
                else if (ev[i].i & 2) t->blades++;
                break;
            case EV_SEAL: t->seals++; break;
            case EV_STANCE: t->stances++; break;
            case EV_COMBO: t->combos++; break;
            case EV_WINDUP: t->windups++; break;
            case EV_FINISHED: t->finished++; t->victory = ev[i].flag; break;
            case EV_CUE: t->cues++; break;
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
    CHECK(s.renPosture == 250, "kojiro começa com 250 de vida");
    CHECK(s.perfectBossDamage > s.goodBossDamage, "perfeito vale mais que bom");
    for (int i = 0; i < roster_size(); i++) {
        Duel d;
        duel_init(&d, &s, roster_get(i), 1);
        CHECK(roster_get(i)->hitsToFall >= 1, "%s diz quantos erros kojiro aguenta", roster_get(i)->name);
        CHECK(s.goodRenCost < duel_ren_damage(&d), "bom custa menos que um erro (%s)", roster_get(i)->name);
    }
}

/* Cada campo das Settings vem da constante certa de ajuste.h (nada trocado de lugar). */
static void test_ajuste(void) {
    Settings s;
    settings_default(&s);
    CHECK(s.renPosture == AJ_VIDA_INICIAL && s.postureGrowth == AJ_VIDA_POR_MESTRE, "vida de kojiro vem do ajuste.h");
    CHECK(s.perfectBossDamage == AJ_PERFEITO_POSTURA && s.perfectGrowth == AJ_PERFEITO_POSTURA_NIVEL &&
          s.perfectRenRecover == AJ_PERFEITO_CURA, "perfeito vem do ajuste.h");
    CHECK(s.goodBossDamage == AJ_BOM_POSTURA && s.goodGrowth == AJ_BOM_POSTURA_NIVEL && s.goodRenCost == AJ_BOM_CUSTO,
          "bom vem do ajuste.h");
    CHECK(s.badBossRecover == AJ_ERRO_MESTRE_RECUPERA && s.sealRenRecover == AJ_SELO_CURA,
          "erro e selo vêm do ajuste.h");
    CHECK(s.attackLead == AJ_LAMINA_PARTE && s.inputCooldown == AJ_ENTRE_GESTOS &&
          s.recovery == AJ_PAUSA_SEQUENCIA && s.sealRecovery == AJ_PAUSA_SELO && s.firstWindupDelay == AJ_PAUSA_INICIO &&
          s.pressureSpeed == AJ_PRESSA && s.comboGap == AJ_PAUSA_NA_CADEIA && s.minChainGap == AJ_CADEIA_MIN,
          "tempos vêm do ajuste.h");
    CHECK(s.perfectHitstop == AJ_HITSTOP_PERFEITO && s.goodHitstop == AJ_HITSTOP_BOM && s.badHitstop == AJ_HITSTOP_ERRO &&
          s.breakHitstop == AJ_HITSTOP_QUEBRA, "hitstop vem do ajuste.h");
    CHECK(AJ_HITSTOP_PERFEITO > AJ_HITSTOP_BOM && AJ_HITSTOP_QUEBRA > AJ_HITSTOP_PERFEITO, "o perfeito segura mais que o bom; a quebra, mais ainda");
    CHECK(AJ_TREMOR_PERFEITO < AJ_TREMOR_ERRO && AJ_TREMOR_ERRO < AJ_TREMOR_QUEBRA, "tremor: perfeito leve, erro forte, quebra mais forte");
}

static void test_roster(const Settings *s) {
    CHECK(roster_size() == 13, "doze aprendizes e oboro");
    CHECK(roster_get(-1) == NULL && roster_get(13) == NULL, "índices fora da trilha");
    float lastPerfect = 1, lastGood = 1;
    for (int i = 0; i < roster_size(); i++) {
        const MasterProfile *m = roster_get(i);
        CHECK(m->id == i + 1, "id em ordem (%s)", m->name);
        CHECK(m->name && m->name[0], "nome do mestre %d", i + 1);
        CHECK(m->style && (m->isBigBoss || strncmp(m->style, "postura d", 9) == 0), "cada aprendiz tem uma postura (%s)", m->name);
        CHECK(m->posture > 0, "postura positiva (%s)", m->name);
        /* oboro não tem fala de saída: de joelhos, a cena da máscara (story_scene) */
        CHECK(m->introCount >= 2 && (m->outroCount >= 1 || m->isBigBoss), "falas antes e depois (%s)", m->name);
        for (int o = 0; o < i; o++) CHECK(roster_get(o)->arena != m->arena, "um cenário próprio por lutador (%s)", m->name);
        CHECK(m->isBigBoss == (i == 12), "só o último é oboro (%s)", m->name);
        CHECK(m->stanceCount >= 1, "ao menos uma guarda (%s)", m->name);
        for (int k = 0; k < m->stanceCount; k++) {
            const Stance *st = &m->stances[k];
            CHECK(st->windupCount > 0, "preparações (%s)", m->name);
            CHECK(st->perfectWindow > 0 && st->perfectWindow < st->goodWindow, "perfeito dentro do bom (%s)", m->name);
        }
        if (!m->isBigBoss) {
            CHECK(m->stances[0].perfectWindow <= lastPerfect + 1e-6, "janela perfeita não cresce (%s)", m->name);
            CHECK(m->stances[0].goodWindow <= lastGood + 1e-6, "janela boa não cresce (%s)", m->name);
            lastPerfect = m->stances[0].perfectWindow;
            lastGood = m->stances[0].goodWindow;
        }
    }
    /* Sem contar quem bate mais pesado (arashi), o dano de um erro só cresce. */
    float lastDamage = 0;
    for (int i = 0; i < roster_size(); i++) {
        float dmg = s->renPosture / roster_get(i)->hitsToFall;
        CHECK(dmg >= lastDamage, "o dano de um erro não diminui ao longo da trilha (%s)", roster_get(i)->name);
        lastDamage = dmg;
    }
    Duel da, dn;
    duel_init(&da, s, roster_get(9), 1);
    duel_init(&dn, s, roster_get(10), 1);
    CHECK(roster_get(9)->damage > 1 && duel_ren_damage(&da) > duel_ren_damage(&dn), "arashi bate mais pesado que o mestre seguinte");
    static const int HITS[13] = {50, 45, 40, 35, 30, 25, 22, 20, 18, 15, 12, 10, 10};
    for (int i = 0; i < roster_size(); i++) {
        CHECK(roster_get(i)->hitsToFall == HITS[i], "Ren aguenta %d erros contra %s", HITS[i], roster_get(i)->name);
        CHECK(roster_get(i)->senseiCount >= 1, "hanzo tem conselho para %s", roster_get(i)->name);
        for (int k = 0; k < roster_get(i)->senseiCount; k++)
            CHECK(strcmp(roster_get(i)->sensei[k].speaker, "hanzo") == 0, "quem aconselha é hanzo (%s)", roster_get(i)->name);
    }
    CHECK(roster_get(12)->specialChance > 0, "oboro tem golpe especial");
    for (int i = 0; i < 12; i++) CHECK(roster_get(i)->specialChance == 0, "só o BIG BOSS tem especial (%s)", roster_get(i)->name);
    const MasterProfile *boss = roster_get(12);
    CHECK(boss->sealCount == 3, "o BIG BOSS tem três selos");
    CHECK(boss->stanceCount == 3, "oboro tem uma postura por selo");
    CHECK(!strcmp(boss->stances[0].name, "postura de hanzo") && !strcmp(boss->stances[1].name, "devorador de posturas") &&
          !strcmp(boss->stances[2].name, "postura do oni"), "de hanzo, devorador de posturas, do oni");
    /* no devorador de posturas, um eco de cada aprendiz: "eco da terra" é a "postura da terra" */
    for (int k = 0; k < 12; k++) {
        bool found = false;
        for (int i = 0; i < boss->moveCount; i++)
            found |= !strncmp(boss->moves[i].name, "eco ", 4) && !strcmp(boss->moves[i].name + 4, roster_get(k)->style + 8) &&
                     boss->moves[i].stance == 1;
        CHECK(found, "oboro devora a postura de %s", roster_get(k)->name);
    }
    for (int i = 0; i < LORE_PAGES; i++) CHECK(lore_page(i)[0] != 0, "página %d da lore", i);
    CHECK(boss->senseiCount == 1 && strcmp(boss->sensei[0].text, "Confie em você mesmo. Use tudo que aprendeu.") == 0,
          "no oboro, hanzo não dá dica");
}

/* Depois de cada vitória, a cabana de hanzo: kojiro conta quem venceu, hanzo fala dele
 * e do próximo. */
static void test_visits(void) {
    for (int i = 0; i < MASTER_COUNT; i++) {
        const MasterProfile *m = roster_get(i), *next = roster_get(i + 1);
        CHECK(m->visitCount >= 3 && m->visitCount <= MAX_LINES, "visita a hanzo depois de %s", m->name);
        CHECK(strcmp(m->visit[0].speaker, "kojiro") == 0, "kojiro conta que venceu %s", m->name);
        bool hanzo = false, nextNamed = false;
        for (int k = 0; k < m->visitCount; k++) {
            const char *who = m->visit[k].speaker;
            CHECK(!strcmp(who, "kojiro") || !strcmp(who, "hanzo"), "na cabana só kojiro e hanzo (%s)", m->name);
            hanzo |= !strcmp(who, "hanzo");
            char low[256];
            size_t n = strlen(m->visit[k].text);
            for (size_t c = 0; c <= n && c < sizeof low; c++) {
                char ch = m->visit[k].text[c];
                low[c] = (char)(ch >= 'A' && ch <= 'Z' ? ch + 32 : ch);
            }
            low[sizeof low - 1] = 0;
            nextNamed |= !strcmp(who, "hanzo") && strstr(low, next->name) != NULL;
        }
        CHECK(hanzo, "hanzo fala na visita (%s)", m->name);
        CHECK(nextNamed, "hanzo fala do próximo, %s, depois de %s", next->name, m->name);
    }
    CHECK(roster_get(12)->visitCount == 0, "depois de oboro não há cabana");
}

static bool scene_has(SceneId id, Cue cue) {
    int n;
    const Beat *b = story_scene(id, &n);
    for (int i = 0; i < n; i++)
        if (b[i].cue == cue) return true;
    return false;
}

static void test_story(void) {
    for (int id = 0; id < SCENE_COUNT; id++) {
        int n = 0;
        const Beat *b = story_scene((SceneId)id, &n);
        CHECK(b && n > 0, "cena %d", id);
        for (int i = 0; i < n; i++)
            CHECK((b[i].text == NULL) == (b[i].speaker == NULL) && (b[i].text || b[i].cue != CUE_NONE),
                  "cada momento da cena %d tem fala ou ação", id);
    }
    CHECK(scene_has(SCENE_SEAL_2, CUE_MASK_ON), "a máscara só vem no segundo selo");
    CHECK(!scene_has(SCENE_SEAL_1, CUE_MASK_ON), "no primeiro selo, ainda sem máscara");
    CHECK(scene_has(SCENE_KNEEL, CUE_MASK_OFF), "de joelhos, ele tira a máscara");
    CHECK(scene_has(SCENE_SIM, CUE_KILL) && scene_has(SCENE_SIM, CUE_HANZO_CLAP), "sim: kojiro mata, hanzo aplaude");
    CHECK(scene_has(SCENE_NAO, CUE_HANZO_KILL) && scene_has(SCENE_NAO, CUE_CHASE), "não: hanzo mata e some");
    CHECK(!scene_has(SCENE_NAO, CUE_KILL), "no não, kojiro não mata");
    CHECK(scene_has(SCENE_SIM, CUE_HANZO_MASK) && scene_has(SCENE_NAO, CUE_HANZO_MASK), "nos dois finais, hanzo põe a máscara");
    int n;
    story_scene(SCENE_COUNT, &n);
    CHECK(n == 0, "cena fora da lista");
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
    CHECK(t.impacts[J_RUIM] == 50, "contra o primeiro mestre, kojiro aguenta 50 erros (%d)", t.impacts[J_RUIM]);
    /* Cada lâmina que entra tira o mesmo; o golpe duplo errado conta duas, e arashi pesa mais. */
    for (int i = 0; i < MASTER_COUNT; i++) {
        const MasterProfile *m = roster_get(i);
        Tally k = play(m, 7, -1, 600);
        int need = (int)ceilf(m->hitsToFall / (m->damage > 0 ? m->damage : 1) - 1e-4f);
        if (m->burn > 0) CHECK(k.blades < need && k.blades >= need / 2, "%s: as brasas derrubam Ren antes (%d de %d lâminas)", m->name, k.blades, need);
        else CHECK(k.blades >= need && k.blades <= need + 1, "%s derruba Ren com %d lâminas (%d)", m->name, need, k.blades);
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

static void test_accelerando(void) {
    const MasterProfile *taiko = roster_get(8); /* suiren: as ondas aceleram */
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
    Tally magna = play(roster_get(7), 21, 0.02, 300); /* enjin */
    CHECK(magna.combos > 0, "Magna abre golpes triplos");
}

static void test_blackout_and_cues(void) {
    const MasterProfile *yoru = roster_get(10);
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
    CHECK(roster_get(11)->cueAudio == 0 && roster_get(11)->cueVisual > 0, "jinshi: sinal mudo, só visual");
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
    const MasterProfile *oboro = roster_get(12);
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
    /* No primeiro selo, só a postura de hanzo. */
    {
        Duel f;
        duel_init(&f, &s, oboro, 5);
        bool early = true;
        for (int k = 0; k < 40 && f.phase != PH_FINISHED; k++) {
            while (f.phase != PH_WINDUP) duel_tick(&f, DT);
            if (f.seal > 0) break;
            if (duel_move(&f) && duel_move(&f)->stance != 0) early = false;
            while (f.phase == PH_WINDUP) { if (!f.attempted && f.strikeAt - f.clock <= 0.01) duel_press(&f); duel_tick(&f, DT); }
        }
        CHECK(early, "no primeiro selo, só a postura de hanzo");
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

/* Oboro muda de postura a cada selo, e cada sequência sai na postura do selo. */
static void test_mimic(void) {
    const MasterProfile *oboro = roster_get(12);
    Settings s;
    settings_default(&s);
    Duel d;
    duel_init(&d, &s, oboro, 5);
    int seen[MAX_STANCES] = {0};
    bool matching = true;
    for (int k = 0; k < 400 && d.phase != PH_FINISHED; k++) {
        while (d.phase != PH_WINDUP && d.phase != PH_FINISHED) duel_tick(&d, DT);
        if (d.phase == PH_FINISHED) break;
        const Move *mv = duel_move(&d);
        if (mv && d.comboStrike == 0 && (mv->stance != d.stanceIndex || d.stanceIndex != d.seal)) matching = false;
        seen[d.stanceIndex] = 1;
        while (d.phase == PH_WINDUP) { if (!d.attempted && d.strikeAt - d.clock <= 0.01) duel_press(&d); duel_tick(&d, DT); }
    }
    CHECK(seen[0] && seen[1] && seen[2], "oboro passa pelas três posturas");
    CHECK(matching, "cada sequência sai na postura do selo");
    Tally t = play(oboro, 5, 0.02, 900);
    CHECK(t.victory, "perfeitos vencem oboro");
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

/* Aperta numa cópia do duelo exatamente em `quando` (-1 = não aperta) e devolve o julgamento do golpe. */
static Judgement probe(const Duel *d, double quando) {
    Duel c = *d;
    DuelEvent ev[MAX_EVENTS];
    duel_drain(&c, ev, MAX_EVENTS);
    if (quando >= 0) {
        if (quando > c.clock) duel_tick(&c, quando - c.clock);
        if (c.phase == PH_WINDUP && c.clock >= quando - 1e-9) duel_press(&c);
    }
    for (int n = 0; n < 100000; n++) {
        int k = duel_drain(&c, ev, MAX_EVENTS);
        for (int e = 0; e < k; e++) if (ev[e].kind == EV_IMPACT) return ev[e].judgement;
        if (c.phase == PH_FINISHED) break;
        duel_tick(&c, 1.0 / 1000);
    }
    return J_NONE;
}

/* Todo golpe de todo mestre tem janela de parry viável: a janela boa inteira cabe na
 * preparação, a perfeita tem ao menos dois quadros de 60 Hz, apertar no meio da
 * perfeita dá perfeito, no meio do resto da boa dá bom, e fora dela (ou sem apertar)
 * dá erro. Cada sequência de cada mestre precisa aparecer ao menos uma vez. */
static void test_janelas_viaveis(void) {
    int golpes = 0;
    for (int i = 0; i < roster_size(); i++) {
        const MasterProfile *m = roster_get(i);
        bool visto[MAX_MOVES] = {false};
        bool ok = true;
        int seeds = m->isBigBoss ? 120 : 60;
        for (uint32_t seed = 1; seed <= (uint32_t)seeds && ok; seed++) {
            Settings s;
            settings_default(&s);
            settings_for_level(&s, i);
            Duel d;
            duel_init(&d, &s, m, seed);
            RoboMente r;
            robo_iniciar(&r, &ROBO_DO_DEMO, seed);
            int ultimo = -1;
            while (d.phase != PH_FINISHED && d.clock < 900 && ok) {
                if (d.phase == PH_WINDUP && d.attacks != ultimo) {
                    ultimo = d.attacks;
                    golpes++;
                    if (d.move >= 0) visto[d.move] = true;
                    const Stance *st = duel_stance(&d);
                    double pw = st->perfectWindow, gw = st->goodWindow, inicio = d.strikeAt - d.windupDuration;
                    const char *mv = duel_move(&d) ? duel_move(&d)->name : "?";
                    if (!(pw >= 2.0 / 60 - 1e-6 && gw > pw)) { ok = false; CHECK(false, "%s/%s: janela perfeita de %.0f ms", m->name, mv, pw * 1000); }
                    else if (!(inicio <= d.strikeAt - gw + 1e-9)) { ok = false; CHECK(false, "%s/%s: a janela boa não cabe na preparação (%.0f ms)", m->name, mv, d.windupDuration * 1000); }
                    else if (probe(&d, d.strikeAt - pw * 0.5) != J_PERFEITO) { ok = false; CHECK(false, "%s/%s golpe %d: o meio da janela perfeita não dá perfeito", m->name, mv, d.comboStrike + 1); }
                    else if (probe(&d, d.strikeAt - (pw + gw) * 0.5) != J_BOM) { ok = false; CHECK(false, "%s/%s golpe %d: o meio da janela boa não dá bom", m->name, mv, d.comboStrike + 1); }
                    else if (probe(&d, -1) != J_RUIM) { ok = false; CHECK(false, "%s/%s: sem apertar não dá erro", m->name, mv); }
                    else if (d.comboStrike == 0 && d.strikeAt - duel_cue_time(&d) < (m->cueAudio > 0 ? AJ_AVISO_MENOR : AJ_AVISO_SO_BRILHO) - 1e-6) {
                        ok = false; CHECK(false, "%s/%s: aviso só %.0f ms antes do contato", m->name, mv, (d.strikeAt - duel_cue_time(&d)) * 1000); }
                    else if (d.strikeAt - gw - 0.02 >= inicio && probe(&d, d.strikeAt - gw - 0.02) != J_RUIM) { ok = false; CHECK(false, "%s/%s: cedo demais não dá erro", m->name, mv); }
                }
                bool p = robo_quer_apertar(&r, &d, ROBO_QUADRO);
                duel_step(&d, ROBO_QUADRO, p);
                duel_drain(&d, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
            }
        }
        CHECK(ok, "todo golpe de %s tem janela viável", m->name);
        for (int k = 0; k < m->moveCount; k++) CHECK(visto[k], "a sequência %s de %s foi testada", m->moves[k].name, m->name);
    }
    CHECK(golpes > 5000, "golpes conferidos: %d", golpes);
    printf("janelas viáveis: %d golpes conferidos\n", golpes);
}

/* O aviso: som e brilho sempre o mesmo tempo antes do contato, 450 ms no daichi
 * descendo até 320 ms; o jinshi, sem som, avisa só com o brilho e nunca com menos de
 * 350 ms. Na sequência, o aviso de cada golpe é o contato anterior (400 ms ou mais). */
static void test_aviso(void) {
    float anterior = 1;
    for (int i = 0; i < MASTER_COUNT; i++) {
        const MasterProfile *m = roster_get(i);
        float a = m->stances[0].aviso;
        CHECK(a >= AJ_AVISO_MENOR - 1e-6 && a <= AJ_AVISO_PRIMEIRO + 1e-6, "%s avisa entre 320 e 450 ms (%.0f)", m->name, a * 1000);
        CHECK(a > m->stances[0].goodWindow + 0.1f, "%s: o aviso vem bem antes da janela boa", m->name);
        if (m->cueAudio > 0) {
            CHECK(a < anterior, "o aviso de %s é menor que o do anterior (%.0f)", m->name, a * 1000);
            anterior = a;
        } else {
            CHECK(m->cueVisual > 0 && a >= AJ_AVISO_SO_BRILHO - 1e-6, "%s avisa só com o brilho, %.0f ms antes", m->name, a * 1000);
        }
    }
    CHECK(fabsf(roster_get(0)->stances[0].aviso - AJ_AVISO_PRIMEIRO) < 1e-6, "daichi avisa 450 ms antes");
    CHECK(fabsf(roster_get(10)->stances[0].aviso - AJ_AVISO_MENOR) < 1e-6, "yoru, o último com som, avisa 320 ms antes");
    CHECK(roster_get(11)->cueAudio == 0, "jinshi não tem som de aviso");
    for (int k = 0; k < roster_get(12)->stanceCount; k++)
        CHECK(roster_get(12)->stances[k].aviso >= AJ_AVISO_MENOR - 1e-6, "oboro nunca avisa com menos de 320 ms (fase %d)", k + 1);
    /* no duelo: o aviso sai no instante certo, antes da lâmina partir */
    Settings s;
    settings_default(&s);
    for (int i = 0; i < roster_size(); i++) {
        Duel d;
        duel_init(&d, &s, roster_get(i), 5);
        int primeiros = 0, cadeias = 0;
        bool ok = true;
        double contatoAnterior = -1;
        for (int n = 0; n < 60 && d.phase != PH_FINISHED; n++) {
            while (d.phase != PH_WINDUP && d.phase != PH_FINISHED) duel_tick(&d, DT);
            if (d.phase == PH_FINISHED) break;
            double inicio = d.clock, contato = d.strikeAt, aviso = -1, partida = -1;
            double esperado = d.m->stances[d.stanceIndex].aviso + (duel_strike_lead(&d) - s.attackLead);
            while (d.phase == PH_WINDUP) {
                if (!d.attempted && d.strikeAt - d.clock <= 0.02) duel_press(&d);
                duel_tick(&d, 1.0 / 1000);
                DuelEvent ev[MAX_EVENTS];
                int k = duel_drain(&d, ev, MAX_EVENTS);
                for (int e = 0; e < k; e++) {
                    if (ev[e].kind == EV_CUE && aviso < 0) aviso = d.clock;
                    if (ev[e].kind == EV_LAUNCH && partida < 0) partida = d.clock;
                }
            }
            (void)inicio;
            if (d.lastStrikeAt != contato) break;   /* o golpe terminou de outro jeito */
            bool primeiro = d.comboStrike == 0;
            if (primeiro) {
                if (fabs((contato - aviso) - esperado) > 0.0015 || aviso > partida) ok = false;
                primeiros++;
            } else {
                if (contatoAnterior < 0 || contato - contatoAnterior < s.minChainGap - 1e-3 || aviso > partida) ok = false;
                cadeias++;
            }
            contatoAnterior = contato;
        }
        CHECK(ok && primeiros > 0, "%s: o aviso sai no tempo fixo, antes da lâmina partir (%d primeiros, %d na sequência)",
              roster_get(i)->name, primeiros, cadeias);
    }
}

/* O overlay de debug: a linha do tempo bate com o julgamento, e o último aperto fica
 * registrado com a antecedência certa (ou o atraso, se veio depois do contato). */
static void test_timeline(void) {
    Settings s;
    settings_default(&s);
    for (int i = 0; i < roster_size(); i++) {
        Duel d;
        duel_init(&d, &s, roster_get(i), 7);
        CHECK(!duel_timeline(&d).active, "%s: sem golpe, sem linha do tempo", roster_get(i)->name);
        while (d.phase != PH_WINDUP) duel_tick(&d, DT);
        DuelTimeline t = duel_timeline(&d);
        const Stance *st = duel_stance(&d);
        CHECK(t.active && t.strike == d.strikeAt && fabs(t.start - (d.strikeAt - d.windupDuration)) < 1e-9 &&
              fabs(t.launch - (d.strikeAt - duel_strike_lead(&d))) < 1e-9 && t.cue >= t.start && t.cue <= t.strike,
              "%s: linha do tempo do golpe", roster_get(i)->name);
        CHECK(fabs(t.perfectFrom - (t.strike - st->perfectWindow)) < 1e-9 && fabs(t.goodFrom - (t.strike - st->goodWindow)) < 1e-9,
              "%s: janelas na linha do tempo", roster_get(i)->name);
        CHECK(probe(&d, t.perfectFrom + 0.001) == J_PERFEITO && probe(&d, t.perfectFrom - 0.001) == J_BOM &&
              probe(&d, t.goodFrom + 0.001) == J_BOM && probe(&d, t.goodFrom - 0.001) == J_RUIM,
              "%s: as bordas da linha do tempo são as do julgamento", roster_get(i)->name);
        /* aperta 30 ms antes: o último golpe fica com essa antecedência */
        double at = d.strikeAt;
        duel_tick(&d, at - 0.030 - d.clock);
        duel_press(&d);
        while (d.phase == PH_WINDUP) duel_tick(&d, DT);
        CHECK(d.lastStrikeAt == at && fabs(d.lastLead - 0.030) < 1e-6 && d.lastJudgement == (0.030 <= st->perfectWindow ? J_PERFEITO : J_BOM),
              "%s: último aperto 30 ms antes (%.1f ms)", roster_get(i)->name, d.lastLead * 1000);
    }
    /* sem aperto, e um gesto 40 ms depois do contato: tarde */
    Duel d;
    duel_init(&d, &s, roster_get(0), 7);
    while (d.phase != PH_WINDUP) duel_tick(&d, DT);
    double at = d.strikeAt;
    duel_tick(&d, at + 0.040 - d.clock);
    duel_press(&d);
    CHECK(d.lastJudgement == J_RUIM && d.lastLead < 0 && fabs((d.lastGestureAt - d.lastStrikeAt) - 0.040) < 1e-6,
          "aperto 40 ms depois do contato fica registrado como tarde");
}

/* Os robôs, com o mesmo passo e os mesmos tempos do demo: o perfeito vence todos sem
 * errar, quem nunca defende perde de todos, e apertar sem parar também perde. */
static void test_robos(void) {
    Robo spam[3] = {ROBO_APERTA_SEM_PARAR, ROBO_APERTA_SEM_PARAR, ROBO_APERTA_SEM_PARAR};
    spam[0].periodo = 0.10f;
    spam[2].periodo = 0.30f;
    for (int i = 0; i < roster_size(); i++) {
        const MasterProfile *m = roster_get(i);
        int venceu = 0, limpo = 0, perdeu = 0, spamPerdeu = 0;
        for (uint32_t k = 1; k <= 20; k++) {
            RoboLuta l = robo_lutar(&ROBO_DO_DEMO, m, i, k);
            venceu += l.vitoria;
            limpo += l.erros == 0 && l.bons == 0;
        }
        for (uint32_t k = 1; k <= 5; k++) {
            perdeu += !robo_lutar(&ROBO_SEM_DEFESA, m, i, k).vitoria;
            for (int p = 0; p < 3; p++) spamPerdeu += !robo_lutar(&spam[p], m, i, k).vitoria;
        }
        CHECK(venceu == 20 && limpo == 20, "o robô do demo vence %s só com perfeitos (%d/20, %d limpas)", m->name, venceu, limpo);
        CHECK(perdeu == 5, "quem nunca defende perde de %s (%d/5)", m->name, perdeu);
        CHECK(spamPerdeu == 15, "apertar sem parar perde de %s (%d/15)", m->name, spamPerdeu);
    }
    /* o demo usa o mesmo robô: aperta no máximo AJ_ROBO_ANTECEDENCIA antes do contato */
    Settings s;
    settings_default(&s);
    Duel d;
    duel_init(&d, &s, roster_get(0), 3);
    RoboMente r;
    robo_iniciar(&r, &ROBO_DO_DEMO, 3);
    double lead = -1;
    while (lead < 0 && d.clock < 10) {
        bool p = robo_quer_apertar(&r, &d, ROBO_QUADRO);
        duel_step(&d, ROBO_QUADRO, p);
        if (p) lead = d.strikeAt - d.lastPress;
    }
    CHECK(lead > AJ_ROBO_ANTECEDENCIA - ROBO_QUADRO - 1e-6 && lead <= AJ_ROBO_ANTECEDENCIA + 1e-6,
          "o robô do demo aperta até %.0f ms antes (%.1f ms)", AJ_ROBO_ANTECEDENCIA * 1000, lead * 1000);
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
    duel_init(&d, &b, roster_get(3), 3);
    Tally t;
    memset(&t, 0, sizeof t);
    while (d.phase != PH_FINISHED && d.clock < 600) { duel_tick(&d, DT); count(&d, &t); }
    CHECK(t.impacts[J_RUIM] == roster_get(3)->hitsToFall, "nível alto não muda quantos erros Ren aguenta (%d)", t.impacts[J_RUIM]);
    /* Menos perfeitos para quebrar o mestre quando Ren é mais forte. */
    Settings lo, hi;
    settings_default(&lo);
    settings_default(&hi);
    settings_for_level(&hi, 11);
    CHECK(ceilf(roster_get(12)->posture / hi.perfectBossDamage) < ceilf(roster_get(12)->posture / lo.perfectBossDamage),
          "Ren forte precisa de menos perfeitos");
}

static void test_special(void) {
    const MasterProfile *oboro = roster_get(12);
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
    /* Os três primeiros são simples: sete sequências, nenhuma com mais de dois contatos. */
    for (int i = 0; i < 3; i++) {
        CHECK(roster_get(i)->moveCount == 7, "%s tem sete sequências", roster_get(i)->name);
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
        CHECK(m->moveCount >= 7 && (m->isBigBoss || m->moveCount <= 10), "%s tem de 7 a 10 sequências (%d)", m->name, m->moveCount);
        CHECK(m->moveCount <= MAX_MOVES, "%s cabe no repertório", m->name);
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
        /* do Daichi ao Jinshi, cada um tem o seu golpe forte (o salto com a pancada) */
        bool heavy = false;
        for (int k = 0; k < m->moveCount; k++) heavy |= m->moves[k].look == LOOK_HEAVY;
        if (!m->isBigBoss && i != 1) CHECK(heavy, "%s tem um golpe forte", m->name);
        /* e todos vêm correndo e saltando em alguma sequência */
        bool dash = false, jump = false;
        for (int k = 0; k < m->moveCount; k++) {
            dash |= m->moves[k].look == LOOK_DASH;
            jump |= m->moves[k].look == LOOK_JUMP;
        }
        CHECK(dash, "%s tem uma investida correndo", m->name);
        if (i != 1) CHECK(jump, "%s tem um golpe saltando", m->name);
    }
    /* O intervalo entre contatos de uma sequência é exatamente o do moveset. */
    const MasterProfile *tetsu = roster_get(0);
    Duel d;
    duel_init(&d, &s, tetsu, 1);
    bool checked = false;
    for (int k = 0; k < 60 && !checked; k++) {
        while (d.phase != PH_WINDUP) duel_tick(&d, DT);
        const Move *mv = duel_move(&d);
        if (mv && mv->strikes >= 2 && d.comboStrike == 0) {
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
    for (int i = 0; i < 12; i++) {
        CHECK(!campaign_big_boss_open(&c), "oboro fechado antes dos doze aprendizes");
        campaign_mark_cleared(&c, c.index);
        CHECK(campaign_advance(&c), "avança ao próximo");
    }
    CHECK(campaign_defeated(&c) == 12, "doze aprendizes vencidos");
    CHECK(campaign_big_boss_open(&c), "oboro aberto");
    CHECK(c.index == 12, "o décimo terceiro é oboro");
    campaign_mark_cleared(&c, 12);
    CHECK(campaign_defeated(&c) == 12, "oboro não conta entre os doze");
    CHECK(!campaign_advance(&c) && c.completed, "trilha completa");
    campaign_mark_cleared(&c, 3);
    campaign_mark_cleared(&c, 3);
    CHECK(campaign_defeated(&c) == 12, "cada aprendiz contado uma vez só");
}

/* Traços de cada mestre: o primeiro é de katana e lento, garfiel faz combos longos,
 * karasu e arashi usam as duas lâminas, suiren ataca de longe e jinshi é o mais variado. */
static void test_traits(void) {
    const MasterProfile *daichi = roster_get(0), *genbu = roster_get(1);
    CHECK(daichi->stances[0].windups[0] > genbu->stances[0].windups[0], "daichi prepara mais devagar que genbu");
    int longest = 0;
    for (int k = 0; k < roster_get(4)->moveCount; k++)
        if (roster_get(4)->moves[k].strikes > longest) longest = roster_get(4)->moves[k].strikes;
    CHECK(longest >= 7, "garfiel tem combo de sete golpes ou mais (%d)", longest);
    for (int i = 5; i <= 9; i += 4) {
        bool dual = false;
        for (int k = 0; k < roster_get(i)->moveCount; k++) dual |= roster_get(i)->moves[k].dual != 0;
        CHECK(dual, "%s ataca com as duas lâminas", roster_get(i)->name);
    }
    int arashiDual = 0, karasuDual = 0;
    for (int k = 0; k < roster_get(9)->moveCount; k++) arashiDual += roster_get(9)->moves[k].dual != 0;
    for (int k = 0; k < roster_get(5)->moveCount; k++) karasuDual += roster_get(5)->moves[k].dual != 0;
    CHECK(arashiDual > karasuDual, "arashi usa as duas lâminas mais que karasu (%d x %d)", arashiDual, karasuDual);
    bool far = false, warp = false;
    for (int k = 0; k < roster_get(8)->moveCount; k++) far |= roster_get(8)->moves[k].look == LOOK_FAR;
    for (int k = 0; k < roster_get(5)->moveCount; k++) warp |= roster_get(5)->moves[k].look == LOOK_WARP;
    CHECK(far, "suiren ataca de longe");
    CHECK(warp, "karasu some em penas e reaparece na frente de kojiro");
    CHECK(roster_get(10)->blackoutChance >= 0.6f, "yoru apaga as luzes quase sempre");
    int looks = 0;
    for (int l = LOOK_HIGH; l <= LOOK_FAR; l++) {
        bool has = false;
        for (int k = 0; k < roster_get(11)->moveCount; k++) has |= roster_get(11)->moves[k].look == (MoveLook)l;
        looks += has;
    }
    CHECK(looks >= 6 && roster_get(11)->moveCount == 10, "jinshi tem o repertório mais variado (%d preparações)", looks);
}

/* Golpe de duas lâminas: perfeito apara as duas, bom deixa passar a segunda, erro leva as duas. */
static void test_dual(void) {
    Settings s;
    settings_default(&s);
    const MasterProfile *arashi = roster_get(9);
    static const double LEADS[3] = {0.02, 0.12, -1};   /* perfeito, bom, sem gesto */
    for (int c = 0; c < 3; c++) {
        Duel d;
        duel_init(&d, &s, arashi, 11);
        bool seen = false;
        for (int n = 0; n < 400 && !seen && d.phase != PH_FINISHED; n++) {
            while (d.phase != PH_WINDUP && d.phase != PH_FINISHED) duel_tick(&d, DT);
            bool dual = duel_strike_dual(&d);
            float before = d.renPosture, hit = duel_ren_damage(&d);
            while (d.phase == PH_WINDUP) {
                if (LEADS[c] >= 0 && !d.attempted && d.strikeAt - d.clock <= LEADS[c]) duel_press(&d);
                duel_tick(&d, DT);
            }
            DuelEvent ev[MAX_EVENTS];
            int k = duel_drain(&d, ev, MAX_EVENTS), flags = -1;
            for (int e = 0; e < k; e++) if (ev[e].kind == EV_IMPACT) flags = ev[e].i;
            if (!dual || flags < 0) continue;
            float lost = before - d.renPosture;
            if (c == 0) CHECK(lost <= 0 && !(flags & 2), "perfeito apara as duas lâminas (perdeu %.1f)", lost);
            if (c == 1) CHECK(fabsf(lost - (hit + s.goodRenCost)) < 0.01f && (flags & 2), "bom: a segunda lâmina entra (%.1f)", lost);
            if (c == 2) CHECK(fabsf(lost - 2 * hit) < 0.01f || d.renPosture <= 0, "erro: as duas lâminas entram (%.1f)", lost);
            seen = true;
        }
        CHECK(seen, "um golpe duplo de arashi foi observado (%d)", c);
    }
}

/* Estocada de longe: a lâmina parte AJ_LANCA_PARTE_X vezes mais cedo que nos outros golpes. */
static void test_far_lead(void) {
    Settings s;
    settings_default(&s);
    Duel d;
    duel_init(&d, &s, roster_get(8), 5);
    bool seen = false;
    for (int n = 0; n < 200 && !seen; n++) {
        while (d.phase != PH_WINDUP) duel_tick(&d, DT);
        const Move *mv = duel_move(&d);
        bool far = mv && mv->look == LOOK_FAR && d.comboStrike == 0;
        double launchAt = -1;
        while (d.phase == PH_WINDUP) {
            duel_tick(&d, DT);
            DuelEvent ev[MAX_EVENTS];
            int k = duel_drain(&d, ev, MAX_EVENTS);
            for (int e = 0; e < k; e++) if (ev[e].kind == EV_LAUNCH) launchAt = d.clock;
        }
        if (!far || launchAt < 0) continue;
        double lead = d.strikeAt - launchAt;
        CHECK(fabs(lead - s.attackLead * AJ_LANCA_PARTE_X) < 0.02, "a estocada de longe parte %.2f s antes (%.3f)", s.attackLead * AJ_LANCA_PARTE_X, lead);
        seen = true;
    }
    CHECK(seen, "uma estocada de longe de suiren foi observada");
}

/* Enjin: o erro deixa kojiro em brasas, a vida continua caindo, e o perfeito apaga. */
static void test_burn(void) {
    Settings s;
    settings_default(&s);
    const MasterProfile *enjin = roster_get(7);
    CHECK(enjin->burn > 0, "enjin queima");
    for (int i = 0; i < roster_size(); i++)
        if (i != 7) CHECK(roster_get(i)->burn == 0, "só o enjin queima (%s)", roster_get(i)->name);
    Duel d;
    duel_init(&d, &s, enjin, 3);
    while (d.phase != PH_WINDUP) duel_tick(&d, DT);
    while (d.phase == PH_WINDUP) duel_tick(&d, DT);            /* sem gesto: erro */
    CHECK(d.burnLeft > AJ_BRASAS_TEMPO - 0.1f, "o erro acende as brasas (%.2f s)", d.burnLeft);
    float before = d.renPosture;
    double t0 = d.clock;
    while (d.phase == PH_RECOVERY && d.clock - t0 < 0.5) duel_tick(&d, DT);
    CHECK(d.renPosture < before - 1e-3f, "em brasas, a vida cai sem golpe (%.2f -> %.2f)", before, d.renPosture);
    CHECK(fabsf((before - d.renPosture) - d.burnRate * (float)(d.clock - t0)) < 0.05f, "a queimadura é contínua");
    float total = duel_ren_damage(&d) * enjin->burn;
    CHECK(fabsf(d.burnRate * AJ_BRASAS_TEMPO - total) < 0.01f, "a queimadura inteira vale %.0f%% de um golpe", enjin->burn * 100);
    /* perfeito apaga */
    bool out = false;
    for (int n = 0; n < 40 && !out && d.phase != PH_FINISHED; n++) {
        while (d.phase != PH_WINDUP && d.phase != PH_FINISHED) duel_tick(&d, DT);
        if (d.burnLeft <= 0) { d.burnLeft = 1; d.burnRate = 0; }  /* garante brasas para o teste */
        while (d.phase == PH_WINDUP) {
            if (!d.attempted && d.strikeAt - d.clock <= 0.02) duel_press(&d);
            duel_tick(&d, DT);
        }
        out = d.burnLeft == 0;
    }
    CHECK(out, "o parry perfeito apaga as brasas");
}

int main(void) {
    Settings s;
    settings_default(&s);
    test_settings();
    test_ajuste();
    test_roster(&s);
    test_visits();
    test_story();
    test_rng();
    test_perfect_victory();
    test_no_defense();
    test_good_only();
    test_bad_recovers_boss();
    test_one_attempt_and_cooldown();
    test_accelerando();
    test_combos();
    test_blackout_and_cues();
    test_pressure();
    test_big_boss();
    test_mimic();
    test_determinism();
    test_every_master_beatable();
    test_robos();
    test_janelas_viaveis();
    test_aviso();
    test_timeline();
    test_movesets();
    test_traits();
    test_dual();
    test_far_lead();
    test_burn();
    test_levels();
    test_special();
    test_campaign();
    printf("%d verificações, %d falhas\n", checks, failures);
    return failures ? 1 : 0;
}
