/*
 * core_test.c - verificações do núcleo, sem janela nem raylib.
 * Rodar: make test
 */
#include "../src/core.h"
#include "../src/robo.h"
#include "../src/vozes.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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
    CHECK(s.renPosture == AJ_VIDA_INICIAL, "vida de kojiro vem do ajuste.h");
    CHECK(s.perfectBossDamage == AJ_PERFEITO_POSTURA && s.perfectGrowth == AJ_PERFEITO_POSTURA_NIVEL &&
          s.perfectHeal == AJ_PERFEITO_CURA, "perfeito vem do ajuste.h");
    CHECK(s.goodBossDamage == AJ_BOM_POSTURA && s.goodGrowth == AJ_BOM_POSTURA_NIVEL && s.goodRenCost == AJ_BOM_CUSTO,
          "bom vem do ajuste.h");
    CHECK(s.badBossRecover == AJ_ERRO_MESTRE_RECUPERA && s.sealHeal == AJ_SELO_CURA,
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
            CHECK(st->perfectWindow > 0 && st->perfectWindow < st->goodWindow, "perfeito dentro do bom (%s)", m->name);
        }
        if (!m->isBigBoss) {
            CHECK(m->stances[0].perfectWindow <= lastPerfect + 1e-6, "janela perfeita não cresce (%s)", m->name);
            CHECK(m->stances[0].goodWindow <= lastGood + 1e-6, "janela boa não cresce (%s)", m->name);
            lastPerfect = m->stances[0].perfectWindow;
            lastGood = m->stances[0].goodWindow;
        }
    }
    /* Quantos erros kojiro aguenta é de cada mestre (a curva de dificuldade, conferida
     * pelos robôs em test_curva); arashi bate mais pesado que o próprio erro dele. */
    Duel da;
    duel_init(&da, s, roster_get(9), 1);
    CHECK(roster_get(9)->damage > 1 && duel_ren_damage(&da) > s->renPosture / roster_get(9)->hitsToFall, "arashi bate mais pesado");
    static const int HITS[13] = {8, 8, 7, 7, 6, 7, 6, 6, 6, 10, 5, 5, 5};
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
    CHECK(t.impacts[J_RUIM] == roster_get(0)->hitsToFall, "contra o primeiro mestre, kojiro aguenta %d erros (%d)", roster_get(0)->hitsToFall,
          t.impacts[J_RUIM]);
    /* Cada lâmina que entra tira o mesmo; o golpe duplo errado conta duas, e arashi pesa mais. */
    for (int i = 0; i < MASTER_COUNT; i++) {
        const MasterProfile *m = roster_get(i);
        Tally k = play(m, 7, -1, 600);
        int need = (int)ceilf(m->hitsToFall / (m->damage > 0 ? m->damage : 1) - 1e-4f);
        if (m->burn > 0) CHECK(k.blades <= need && k.blades >= need / 2, "%s: as brasas ajudam a derrubar Ren (%d de %d lâminas)", m->name, k.blades, need);
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

/* Suiren: as ondas aceleram. Só a espera antes do aviso encurta, a cada sequência do
 * ciclo; do aviso ao contato é sempre o mesmo tempo. */
static void test_accelerando(void) {
    const MasterProfile *taiko = roster_get(8);
    Settings s;
    settings_default(&s);
    s.pressureSpeed = 1; /* isola o efeito do ciclo */
    Duel d;
    duel_init(&d, &s, taiko, 11);
    double antes[6], esperado[6];
    int got = 0;
    bool avisoFixo = true;
    while (got < 6 && d.clock < 60 && d.phase != PH_FINISHED) {
        d.bossPosture = 1e6f; /* o duelo não acaba antes de seis sequências */
        duel_tick(&d, DT);
        if (d.phase == PH_WINDUP && d.comboStrike == 0 && d.strikeAt - d.clock > d.windupDuration - DT * 1.5 && got < 6 &&
            (got == 0 || fabs(d.strikeAt - d.windupDuration - antes[5]) > 1e-9)) {
            const Move *mv = duel_move(&d);
            double aviso = duel_aviso(&d);
            antes[got] = d.windupDuration - aviso;
            esperado[got] = (mv->windup - taiko->stances[0].aviso) * pow(taiko->accelFactor, got % taiko->accelSteps) * s.waitScale;
            if (esperado[got] < AJ_PREPARO_ANTES_DO_AVISO) esperado[got] = AJ_PREPARO_ANTES_DO_AVISO;
            if (fabs(aviso - (taiko->stances[0].aviso + duel_strike_lead_base(&d) - s.attackLead)) > 1e-6) avisoFixo = false;
            got++;
            antes[5] = d.strikeAt - d.windupDuration;
        }
        if (d.phase == PH_WINDUP && !d.attempted && d.strikeAt - d.clock <= 0.02) duel_press(&d);
        duel_drain(&d, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
    }
    CHECK(got == 6, "seis sequências da suiren");
    bool ok = true;
    for (int i = 0; i < got && i < 5; i++) if (fabs(antes[i] - esperado[i]) > 1e-4) ok = false;
    CHECK(ok, "a espera antes do aviso encurta %.2f x a cada sequência do ciclo", taiko->accelFactor);
    CHECK(avisoFixo, "do aviso ao contato, sempre o mesmo tempo");
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
    const Move *mv = duel_move(&d);
    float aviso = gorou->stances[0].aviso;
    CHECK(fabsf(d.windupDuration - (aviso + (mv->windup - aviso) * s.pressureSpeed * s.waitScale)) < 1e-4,
          "com metade da postura, a espera antes do aviso fica 10%% mais curta");
}

static void test_big_boss(void) {
    const MasterProfile *oboro = roster_get(12);
    Tally t = play(oboro, 99, 0.02, 600);
    CHECK(t.finished == 1 && t.victory, "perfeitos vencem o Oboro");
    CHECK(t.seals == 2, "dois selos quebrados antes do último (%d)", t.seals);
    int need = 0;
    for (int k = 0; k < oboro->sealCount; k++) need += (int)ceilf(oboro->seals[k].posture / 20.0f);
    CHECK(t.impacts[J_PERFEITO] == need, "%d perfeitos nos três selos (%d)", need, t.impacts[J_PERFEITO]);
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
    CHECK(fabsf(d.renPosture - fminf(s.renPosture, 40 + (s.perfectHeal + s.sealHeal) * s.renPosture)) < 1e-4 &&
              fabsf(d.renPosture - s.renPosture) < 1e-4,
          "selo quebrado enche a vida (%.1f)", d.renPosture);
    CHECK(fabsf(d.bossPosture - oboro->seals[1].posture) < 1e-4 && duel_posture_max(&d) == oboro->seals[1].posture,
          "novo selo com a postura cheia dele");
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
                    else if (probe(&d, d.strikeAt + s.lateGrace * 0.5) != J_BOM) { ok = false; CHECK(false, "%s/%s: dentro da tolerância tardia não dá bom", m->name, mv); }
                    else if (probe(&d, d.strikeAt + s.lateGrace + 0.005) != J_RUIM) { ok = false; CHECK(false, "%s/%s: depois da tolerância tardia não dá erro", m->name, mv); }
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
        double contatoAnterior = -1, congelado = 0;
        for (int n = 0; n < 60 && d.phase != PH_FINISHED; n++) {
            while (d.phase != PH_WINDUP && d.phase != PH_FINISHED) duel_tick(&d, DT);
            if (d.phase == PH_FINISHED) break;
            double inicio = d.clock, contato = d.strikeAt, aviso = -1, partida = -1;
            /* o aviso não anda com a lâmina variável: só a lança avisa mais cedo */
            double esperado = d.m->stances[d.stanceIndex].aviso + (duel_strike_lead_base(&d) - s.attackLead);
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
                if (contatoAnterior < 0 || contato - contatoAnterior + congelado < s.minChainGap - 1e-3 || aviso > partida) ok = false;
                cadeias++;
            }
            contatoAnterior = contato;
            congelado = d.lastHitstop;
        }
        CHECK(ok && primeiros > 0, "%s: o aviso sai no tempo fixo, antes da lâmina partir (%d primeiros, %d na sequência)",
              roster_get(i)->name, primeiros, cadeias);
    }
}

/* Cada sequência tem a sua preparação, e ela é sempre a mesma: sem pressa e sem traço
 * aleatório, o mesmo golpe prepara sempre no mesmo tempo. O hayate (±120 ms) e o jinshi
 * (±80 ms) variam só a espera antes do aviso: do aviso ao contato nunca muda. */
static void test_preparacao_por_golpe(void) {
    Settings s;
    settings_default(&s);
    for (int i = 0; i < roster_size(); i++) {
        const MasterProfile *m = roster_get(i);
        for (int k = 0; k < m->moveCount; k++) {
            const Move *mv = &m->moves[k];
            const Stance *st = &m->stances[mv->stance < 0 ? 0 : mv->stance];
            CHECK(mv->windup >= st->aviso + AJ_PREPARO_ANTES_DO_AVISO - 1e-6, "%s: %s prepara %.0f ms, mais que o aviso", m->name,
                  mv->name, mv->windup * 1000);
        }
        double vista[MAX_MOVES], avisoVisto[MAX_MOVES];
        for (int k = 0; k < MAX_MOVES; k++) vista[k] = avisoVisto[k] = -1;
        bool fixo = true, avisoFixo = true, dentro = true;
        for (uint32_t seed = 1; seed <= 12; seed++) {
            Duel d;
            duel_init(&d, &s, m, seed);
            int ultimo = -1;
            while (d.phase != PH_FINISHED && d.clock < 400) {
                d.bossPosture = 1e6f;   /* sem pressa, e a luta não acaba */
                d.renPosture = s.renPosture;
                duel_tick(&d, DT);
                if (d.phase == PH_WINDUP && d.attacks != ultimo) {
                    ultimo = d.attacks;
                    if (d.comboStrike != 0 || d.move < 0 || m->accelSteps > 1) continue;
                    double aviso = d.strikeAt - duel_cue_time(&d);
                    const Move *mv = duel_move(&d);
                    const Stance *st = duel_stance(&d);
                    double base = st->aviso + (mv->windup - st->aviso) * duel_seal_rule(&d)->speedMultiplier * (m->waitScale > 0 ? m->waitScale : s.waitScale) +
                                  (duel_strike_lead_base(&d) - s.attackLead);
                    if (m->rhythmJitter > 0) {
                        if (fabs(d.windupDuration - base) > m->rhythmJitter + 1e-4) dentro = false;
                    } else if (vista[d.move] >= 0 && fabs(vista[d.move] - d.windupDuration) > 1e-6) fixo = false;
                    if (avisoVisto[d.move] >= 0 && fabs(avisoVisto[d.move] - aviso) > 1e-6) avisoFixo = false;
                    vista[d.move] = d.windupDuration;
                    avisoVisto[d.move] = aviso;
                }
                if (d.phase == PH_WINDUP && !d.attempted && d.strikeAt - d.clock <= 0.02) duel_press(&d);
                duel_drain(&d, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
            }
        }
        if (m->rhythmJitter > 0) CHECK(dentro, "%s: o traço aleatório fica dentro de ±%.0f ms", m->name, m->rhythmJitter * 1000);
        else CHECK(fixo, "%s: cada sequência prepara sempre no mesmo tempo", m->name);
        CHECK(avisoFixo, "%s: do aviso ao contato, sempre o mesmo tempo", m->name);
    }
}

/* O hitstop congela o duelo (o jogo para de avançar o relógio). Dentro de uma sequência
 * ele sai da preparação seguinte: em tempo real, o próximo contato chega exatamente o
 * intervalo da sequência depois do anterior, seja o impacto perfeito, bom ou erro. */
static void test_hitstop_ritmo(void) {
    Settings s;
    settings_default(&s);
    CHECK(duel_hitstop_for(&s, J_PERFEITO, false, false) == s.perfectHitstop && duel_hitstop_for(&s, J_PERFEITO, true, false) == s.breakHitstop &&
          duel_hitstop_for(&s, J_BOM, false, false) == s.goodHitstop && duel_hitstop_for(&s, J_RUIM, false, false) == s.badHitstop &&
          duel_hitstop_for(&s, J_BOM, false, true) >= s.badHitstop, "hitstop de cada impacto");
    static const double LEADS[4] = {0.02, -2, -0.02, -9};   /* perfeito, bom (meio da janela boa), 20 ms tarde, sem aperto */
    int medidos = 0;
    double pior = 0;
    for (int i = 0; i < roster_size(); i++) {
        for (int c = 0; c < 4; c++) {
            Duel d;
            duel_init(&d, &s, roster_get(i), 21 + c);
            double real = 0, congela = 0, contatoReal = -1;
            int passos = 0;
            while (d.phase != PH_FINISHED && d.clock < 120 && passos++ < 2000000) {
                d.renPosture = s.renPosture;   /* ninguém cai: só o ritmo importa */
                if (d.bossPosture < 60) d.bossPosture = roster_get(i)->posture;
                if (congela > 0) { congela -= 1.0 / 1000; real += 1.0 / 1000; continue; }
                if (d.phase == PH_WINDUP && !d.attempted) {
                    const Stance *st = duel_stance(&d);
                    double lead = LEADS[c] == -2 ? (st->perfectWindow + st->goodWindow) / 2 : LEADS[c];
                    if (lead > -5 && d.strikeAt - d.clock <= lead) duel_press(&d);
                }
                duel_tick(&d, 1.0 / 1000);
                real += 1.0 / 1000;
                DuelEvent ev[MAX_EVENTS];
                int k = duel_drain(&d, ev, MAX_EVENTS);
                for (int e = 0; e < k; e++) {
                    if (ev[e].kind != EV_IMPACT) continue;
                    /* o instante real do contato: agora menos o que o relógio passou do contato */
                    double agora = real - (d.clock - d.lastStrikeAt);
                    if (d.comboStrike > 0 && contatoReal >= 0) {
                        const Move *mv = duel_move(&d);
                        double erro = fabs((agora - contatoReal) - mv->gaps[d.comboStrike - 1]);
                        if (erro > pior) pior = erro;
                        medidos++;
                    }
                    contatoReal = agora;
                    congela = d.lastHitstop;
                }
            }
        }
    }
    CHECK(medidos > 300 && pior < 0.0015, "em tempo real, o ritmo da sequência não muda com o hitstop (%d golpes, pior %.1f ms)", medidos, pior * 1000);
}

/* Aperto cedo. Antes do aviso, apertar não trava o golpe: dá uma recarga de no máximo
 * 0,5 s que termina no aviso, e a defesa daquele golpe não sai perfeita. A recarga nunca
 * cobre a janela boa: em todo golpe de todo mestre, apertando cedo em qualquer ponto
 * antes do aviso, a janela boa inteira continua aceitando a defesa. Depois do aviso,
 * vale uma tentativa só. E o jogo sabe dizer "cedo" e "tarde". */
static void test_aperto_cedo(void) {
    int golpes = 0, sondas = 0;
    bool recargaOk = true, janelaOk = true, cedoOk = true, travaOk = true;
    for (int i = 0; i < roster_size() && recargaOk && janelaOk; i++) {
        const MasterProfile *m = roster_get(i);
        for (uint32_t seed = 1; seed <= 25; seed++) {
            Settings s;
            settings_default(&s);
            settings_for_level(&s, i);
            Duel d;
            duel_init(&d, &s, m, seed);
            RoboMente r;
            robo_iniciar(&r, &ROBO_DO_DEMO, seed);
            int ultimo = -1;
            while (d.phase != PH_FINISHED && d.clock < 900) {
                if (d.phase == PH_WINDUP && d.attacks != ultimo) {
                    ultimo = d.attacks;
                    golpes++;
                    DuelTimeline t = duel_timeline(&d);
                    /* cedo, em qualquer ponto antes do aviso (a cada 5 ms) */
                    for (double q = t.start; q < t.cue - 1e-6; q += 0.005) {
                        Duel c = d;
                        if (q > c.clock) duel_tick(&c, q - c.clock);
                        if (c.phase != PH_WINDUP) break;
                        duel_drain(&c, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
                        const double quando = c.clock;   /* a preparação começa no instante certo, que pode ser antes do quadro */
                        bool aceito = duel_press(&c);
                        DuelEvent ev[MAX_EVENTS];
                        int k = duel_drain(&c, ev, MAX_EVENTS);
                        if (!aceito || c.attempted || k < 1 || ev[k - 1].kind != EV_PRESS || ev[k - 1].i != PRESS_CEDO) cedoOk = false;
                        if (c.pressBlockedUntil > t.cue + 1e-9 || c.pressBlockedUntil > quando + AJ_RECARGA_CEDO + 1e-9 ||
                            c.pressBlockedUntil >= t.goodFrom) recargaOk = false;
                        sondas++;
                    }
                    /* a janela boa inteira continua valendo: apertou cedo no começo e logo
                     * antes do aviso, e depois no começo, no meio e no fim da janela boa */
                    double cedos[2] = {t.start + 1e-4, t.cue - 0.002};
                    double depois[3] = {t.goodFrom + 0.001, (t.goodFrom + t.strike) / 2, t.strike - 0.001};
                    for (int a = 0; a < 2 && cedos[a] < t.cue && cedos[a] >= t.start; a++)
                        for (int b = 0; b < 3; b++) {
                            Duel c = d;
                            if (cedos[a] > c.clock) duel_tick(&c, cedos[a] - c.clock);
                            duel_press(&c);
                            Judgement j = probe(&c, depois[b]);
                            if (j != J_BOM) janelaOk = false;
                        }
                    /* depois do aviso, uma tentativa só: cedo demais trava e dá erro */
                    if (t.goodFrom - t.cue > 0.02) {
                        Duel c = d;
                        duel_tick(&c, t.cue + 0.005 - c.clock);
                        duel_press(&c);
                        if (!c.attempted || duel_press(&c) || probe(&c, t.goodFrom + 0.01) != J_RUIM) travaOk = false;
                    }
                }
                bool p = robo_quer_apertar(&r, &d, ROBO_QUADRO);
                duel_step(&d, ROBO_QUADRO, p);
                duel_drain(&d, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
            }
        }
    }
    CHECK(recargaOk, "a recarga do aperto cedo termina no aviso, no máximo, e nunca cobre a janela boa");
    CHECK(janelaOk, "depois de apertar cedo, a janela boa inteira continua aceitando a defesa (sem perfeito)");
    CHECK(cedoOk, "antes do aviso, apertar não trava o golpe e é marcado como cedo");
    CHECK(travaOk, "depois do aviso, vale uma tentativa só");
    CHECK(golpes > 2000 && sondas > 50000, "aperto cedo conferido em %d golpes, %d pontos", golpes, sondas);
    /* tarde: um aperto logo depois de um golpe que entrou sem defesa */
    Settings s;
    settings_default(&s);
    Duel d;
    duel_init(&d, &s, roster_get(0), 2);
    while (d.phase != PH_WINDUP) duel_tick(&d, DT);
    while (d.phase == PH_WINDUP) duel_tick(&d, DT);
    duel_tick(&d, 0.1);
    duel_drain(&d, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
    duel_press(&d);
    DuelEvent ev[MAX_EVENTS];
    int k = duel_drain(&d, ev, MAX_EVENTS);
    CHECK(k >= 1 && ev[k - 1].kind == EV_PRESS && ev[k - 1].i == PRESS_TARDE, "apertar 0,1 s depois de levar o golpe é tarde");
    /* O que a sonda antiga (0,42 s depois do golpe, com a pausa de 0,55 s) garantia: um aperto dentro da pausa
     * entre sequências, depois da janela de "tarde", é só um gesto: não vira "cedo" nem gasta a defesa do golpe
     * seguinte. Com a pausa de 0,40 s a trava do "tarde" (0,1 + 0,3 s) já acaba junto com ela, e não sobra um
     * instante depois dela: a sonda mede pela pausa, num golpe sem o "tarde" antes, e confere o que importa, que a
     * defesa perfeita do golpe seguinte continua saindo perfeita. */
    CHECK(s.recovery - 0.05 > AJ_TARDE_JANELA + 0.02, "há pausa depois da janela de tarde (a pausa é de %.2f s)", s.recovery);
    Duel g;
    duel_init(&g, &s, roster_get(0), 2);
    while (g.phase != PH_WINDUP) duel_tick(&g, DT);
    while (g.phase == PH_WINDUP) duel_tick(&g, DT);
    duel_drain(&g, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
    const double alvo = g.phaseEnd - 0.05;                        /* 50 ms antes de a pausa acabar */
    duel_tick(&g, alvo - g.clock);
    CHECK(g.phase == PH_RECOVERY, "%.2f s antes do fim da pausa ainda é a pausa", g.phaseEnd - g.clock);
    duel_press(&g);
    k = duel_drain(&g, ev, MAX_EVENTS);
    CHECK(k >= 1 && ev[k - 1].kind == EV_PRESS && ev[k - 1].i == PRESS_GESTO, "no fim da pausa, depois do \"tarde\", o aperto é só um gesto");
    while (g.phase != PH_WINDUP) duel_tick(&g, DT);
    CHECK(!g.earlyUsed && !g.attempted, "o gesto da pausa não gastou a defesa do golpe seguinte");
    CHECK(g.pressBlockedUntil < g.clock, "a trava do gesto se rearmou na preparação nova");
    while (g.phase == PH_WINDUP && g.strikeAt - g.clock > 0.02) duel_tick(&g, DT);
    duel_press(&g);
    while (g.phase == PH_WINDUP) duel_tick(&g, DT);
    k = duel_drain(&g, ev, MAX_EVENTS);
    Judgement julgado = J_NONE;
    for (int i = 0; i < k; i++) if (ev[i].kind == EV_IMPACT) julgado = ev[i].judgement;
    CHECK(julgado == J_PERFEITO, "depois do gesto na pausa, a defesa perfeita do golpe seguinte sai perfeita");
}

/* Tolerância tardia: um aperto até 30 ms depois do contato ainda defende, como bom
 * (nunca perfeito); sem aperto, o golpe entra quando ela acaba. */
static void test_tolerancia_tardia(void) {
    Settings s;
    settings_default(&s);
    CHECK(fabsf(s.lateGrace - 0.030f) < 1e-6 && s.lateGrace == AJ_TOLERANCIA_TARDIA, "tolerância tardia de 30 ms, do ajuste.h");
    for (int i = 0; i < roster_size(); i++) {
        Duel d;
        duel_init(&d, &s, roster_get(i), 4);
        while (d.phase != PH_WINDUP) duel_tick(&d, DT);
        double contato = d.strikeAt;
        Duel a = d;
        duel_tick(&a, contato + 0.020 - a.clock);
        CHECK(a.phase == PH_WINDUP, "%s: 20 ms depois do contato, o golpe ainda espera", roster_get(i)->name);
        duel_press(&a);
        CHECK(a.lastJudgement == J_BOM && fabs(a.lastLead + 0.020) < 1e-6 && a.lastAttempted,
              "%s: apertar 20 ms depois do contato é bom, julgado na hora (%.1f ms)", roster_get(i)->name, a.lastLead * 1000);
        Duel b = d;
        double quando = -1;
        while (b.phase == PH_WINDUP) {
            duel_tick(&b, 0.0005);
            if (b.phase != PH_WINDUP) quando = b.clock;
        }
        CHECK(b.lastJudgement == J_RUIM && fabs(quando - (contato + s.lateGrace)) < 0.0011,
              "%s: sem aperto, o golpe entra quando a tolerância acaba (%.1f ms depois)", roster_get(i)->name, (quando - contato) * 1000);
    }
}

/* Calibração de latência. A tela de teste mede quanto o jogador aperta depois da
 * batida (mediana, sem os apertos perdidos). O atraso de vídeo entra no julgamento (o
 * aperto conta esse tanto mais cedo, e o golpe espera por ele); o de áudio adianta o som
 * do aviso para chegar junto com o brilho. */
static void test_calibracao(void) {
    float a[8] = {0.050f, 0.060f, 0.040f, 0.055f, 0.045f, 0.9f, 0.052f, 0.048f};
    CHECK(fabsf(calibration_result(a, 8) - 0.050f) < 0.0021f, "mediana dos apertos, sem o perdido (%.1f ms)", calibration_result(a, 8) * 1000);
    float b[4] = {-0.030f, -0.020f, -0.025f, -0.010f};
    CHECK(calibration_result(b, 4) == 0, "adiantado vira zero (%.1f)", calibration_result(b, 4));
    float c[4] = {0.25f, 0.28f, 0.26f, 0.27f};
    CHECK(fabsf(calibration_result(c, 4) - AJ_LATENCIA_MAX) < 1e-6, "no máximo AJ_LATENCIA_MAX");
    float d0[6] = {0.9f, -0.8f, 0.7f, 0.05f, 0.6f, 0.5f};
    CHECK(calibration_result(d0, 6) < 0, "apertos demais fora da batida: refazer");

    Settings s;
    settings_default(&s);
    s.latency = 0.050f;
    s.audioLead = 0.030f;
    for (int i = 0; i < roster_size(); i++) {
        Duel d;
        duel_init(&d, &s, roster_get(i), 6);
        while (d.phase != PH_WINDUP) duel_tick(&d, DT);
        const Stance *st = duel_stance(&d);
        double contato = d.strikeAt;
        /* o aperto 30 ms depois do contato conta como 20 ms antes: perfeito */
        CHECK(probe(&d, contato + 0.030) == J_PERFEITO, "%s: com 50 ms de atraso, 30 ms depois do contato é perfeito", roster_get(i)->name);
        CHECK(probe(&d, contato + 0.050 + s.lateGrace * 0.5) == J_BOM, "%s: e a tolerância tardia soma ao atraso", roster_get(i)->name);
        CHECK(probe(&d, contato + 0.050 - st->perfectWindow - 0.005) == J_BOM, "%s: a janela anda junto", roster_get(i)->name);
        /* sem aperto, espera o atraso e a tolerância */
        Duel b = d;
        double quando = -1;
        while (b.phase == PH_WINDUP) { duel_tick(&b, 0.0005); if (b.phase != PH_WINDUP) quando = b.clock; }
        CHECK(fabs(quando - (contato + s.latency + s.lateGrace)) < 0.0011, "%s: sem aperto, o golpe espera o atraso (%.1f ms)",
              roster_get(i)->name, (quando - contato) * 1000);
        /* o som do aviso vem 30 ms antes do brilho; o brilho, no tempo de sempre */
        Duel c = d;
        double brilho = -1, som = -1;
        while (c.phase == PH_WINDUP) {
            duel_tick(&c, 0.0005);
            DuelEvent ev[MAX_EVENTS];
            int k = duel_drain(&c, ev, MAX_EVENTS);
            for (int e = 0; e < k; e++)
                if (ev[e].kind == EV_CUE) { if (ev[e].flag) som = c.clock; else brilho = c.clock; }
        }
        CHECK(brilho > 0 && fabs(brilho - duel_cue_time(&d)) < 0.0011 && fabs((brilho - som) - s.audioLead) < 0.0011,
              "%s: o som do aviso adiantado %.0f ms", roster_get(i)->name, s.audioLead * 1000);
        /* antes do aviso que o jogador vê (o do jogo mais o atraso), o aperto ainda é cedo */
        Duel e = d;
        double visto = duel_cue_time(&d) + s.latency;
        if (visto - 0.01 > e.clock && visto < contato - st->goodWindow) {
            duel_tick(&e, visto - 0.01 - e.clock);
            duel_press(&e);
            CHECK(!e.attempted && e.pressBlockedUntil <= visto + 1e-9, "%s: o aperto cedo usa o aviso visto", roster_get(i)->name);
        }
    }
    /* com 60 ms de atraso, apertando no ritmo, a sequência continua no mesmo ritmo */
    s.latency = 0.060f;
    s.audioLead = 0;
    int medidos = 0, perfeitos = 0, total = 0;
    double pior = 0;
    for (int i = 0; i < roster_size(); i++) {
        Duel d;
        duel_init(&d, &s, roster_get(i), 31);
        double real = 0, congela = 0, contatoReal = -1;
        while (d.phase != PH_FINISHED && d.clock < 90) {
            d.renPosture = s.renPosture;
            if (d.bossPosture < 60) d.bossPosture = roster_get(i)->posture;
            if (congela > 0) { congela -= 1.0 / 1000; real += 1.0 / 1000; continue; }
            /* os eventos de cada passo são lidos antes do passo seguinte (o golpe que vem pode
             * começar no mesmo quadro em que o anterior é julgado) */
            DuelEvent ev[MAX_EVENTS];
            int k = 0;
            if (d.phase == PH_WINDUP && !d.attempted && d.clock >= d.strikeAt - 0.02 + s.latency) {
                duel_press(&d);
                k = duel_drain(&d, ev, MAX_EVENTS);
            }
            if (k == 0) {
                duel_tick(&d, 1.0 / 1000);
                real += 1.0 / 1000;
                k = duel_drain(&d, ev, MAX_EVENTS);
            }
            for (int e = 0; e < k; e++) {
                if (ev[e].kind != EV_IMPACT) continue;
                total++;
                perfeitos += ev[e].judgement == J_PERFEITO;
                double agora = real - (d.clock - d.lastStrikeAt);
                if (d.comboStrike > 0 && contatoReal >= 0) {
                    double erro = fabs((agora - contatoReal) - duel_move(&d)->gaps[d.comboStrike - 1]);
                    if (erro > pior) pior = erro;
                    medidos++;
                }
                contatoReal = agora;
                congela = d.lastHitstop;
            }
        }
    }
    CHECK(perfeitos == total && total > 300, "com 60 ms de atraso calibrado, apertar 60 ms depois é sempre perfeito (%d de %d)", perfeitos, total);
    CHECK(medidos > 100 && pior < 0.0015, "com 60 ms de atraso, o ritmo das sequências não muda (%d golpes, pior %.1f ms)", medidos, pior * 1000);
}

/* A cura do perfeito é uma fração da vida (e a do selo do oboro também): vale o mesmo
 * em qualquer ponto da trilha, em proporção. */
static void test_cura_em_porcentagem(void) {
    for (int i = 0; i < roster_size(); i++) {
        Settings s;
        settings_default(&s);
        settings_for_level(&s, i);
        Duel d;
        duel_init(&d, &s, roster_get(i), 8);
        d.renPosture = s.renPosture * 0.5f;
        while (d.phase != PH_WINDUP) duel_tick(&d, DT);
        duel_tick(&d, d.strikeAt - 0.01 - d.clock);
        duel_press(&d);
        while (d.phase == PH_WINDUP) duel_tick(&d, DT);
        CHECK(d.lastJudgement == J_PERFEITO && fabsf(d.renPosture - s.renPosture * (0.5f + AJ_PERFEITO_CURA)) < 1e-3,
              "%s: o perfeito cura %.0f%% da vida (%.1f de %.0f)", roster_get(i)->name, AJ_PERFEITO_CURA * 100, d.renPosture, s.renPosture);
    }
    CHECK(AJ_PERFEITO_CURA > 0 && AJ_PERFEITO_CURA < 0.1f && fabsf(AJ_SELO_CURA - 1.0f) < 1e-6f, "curas em fração da vida (o selo devolve a vida inteira)");
}

/* A curva de dificuldade, pelos robôs (make robos mostra a tabela). O humano casual que
 * decora o ritmo: nos quatro primeiros, 95% ou mais; depois a vitória só cai (com 4 pontos
 * de folga para o sorteio de 300 lutas), chega a uns 55% no jinshi (faixa aprovada de 52 a 58) e a uns 40% no oboro. A ordem
 * exata, com folga de 0,5 ponto, é o make curva-ordem (tests/robos.c --ordem). E apertar sem
 * olhar, em qualquer ritmo de 0,05 a 0,8 s, perde de todos. */
static void test_curva(void) {
    double anterior = 101, vit[ROSTER_SIZE];
    bool crescente = true;
    for (int i = 0; i < roster_size(); i++) {
        int w = 0;
        for (uint32_t k = 0; k < 300; k++) w += robo_lutar(&ROBO_HUMANO_CASUAL, roster_get(i), i, 20000 + k).vitoria;
        vit[i] = 100.0 * w / 300;
        if (vit[i] > anterior + 4) crescente = false;
        if (vit[i] < anterior) anterior = vit[i];
    }
    for (int i = 0; i < 4; i++) CHECK(vit[i] >= 95, "o humano casual vence %s em 95%% ou mais (%.0f%%)", roster_get(i)->name, vit[i]);
    CHECK(crescente, "a dificuldade só cresce pela trilha");
    /* a ordem fina (yoru nunca mais fácil que o arashi, jinshi nunca mais fácil que o yoru) precisa de muitas lutas: make curva-ordem */
    CHECK(vit[9] <= vit[8] + 4 && vit[10] <= vit[9] + 4 && vit[11] <= vit[9] + 4, "yoru e jinshi não são mais fáceis que o arashi (%.0f, %.0f, %.0f)",
          vit[9], vit[10], vit[11]);
    CHECK(vit[11] >= 52 && vit[11] <= 75, "uns 55%% no jinshi, a faixa aprovada é de 52 a 58 e o sorteio de 300 lutas varia 3 pontos (%.0f%%)", vit[11]);
    CHECK(vit[12] >= 30 && vit[12] <= 55 && vit[12] <= vit[11], "uns 40%% no oboro (%.0f%%)", vit[12]);
    static const float PERIODOS[12] = {0.05f, 0.10f, 0.15f, 0.20f, 0.25f, 0.30f, 0.35f, 0.40f, 0.45f, 0.50f, 0.60f, 0.80f};
    int spam = 0;
    for (int i = 0; i < roster_size(); i++)
        for (int p = 0; p < 12; p++) {
            Robo r = ROBO_APERTA_SEM_PARAR;
            r.periodo = PERIODOS[p];
            for (uint32_t k = 0; k < 20; k++) spam += robo_lutar(&r, roster_get(i), i, 30000 + k).vitoria;
        }
    CHECK(spam == 0, "apertar sem olhar, em qualquer ritmo de 0,05 a 0,8 s, perde de todos (%d vitórias em 3120)", spam);
}

/* Os robôs do "cedo" e do "tarde" (nos vídeos: APARA_ROBO=cedo|tarde): um aperto sempre no mesmo deslocamento do contato. Antes do
 * aviso, o golpe entra sem defesa e o jogo diz "cedo"; depois de o golpe entrar, diz "tarde"; nenhum dos dois defende (no
 * segundo golpe de uma sequência o aviso é o contato anterior, e o mesmo aperto vira uma tentativa que erra a janela). */
static void test_robos_deslocados(void) {
    for (int quem = 0; quem < 2; quem++) {
        Robo r = robo_deslocado(quem == 0 ? -0.60f : 0.10f);
        const MasterProfile *m = roster_get(0);
        Settings s;
        settings_default(&s);
        Duel d;
        duel_init(&d, &s, m, 7);
        RoboMente me;
        robo_iniciar(&me, &r, 7);
        DuelEvent ev[MAX_EVENTS];
        int cedo = 0, tarde = 0, tentativas = 0, defendidos = 0, entradas = 0;
        while (d.phase != PH_FINISHED && d.clock < 60) {
            d.renPosture = s.renPosture;                       /* a luta não acaba: interessa o comportamento */
            duel_step_at(&d, 1.0 / 60, robo_aperto_em(&me, &d, 1.0 / 60));
            int n = duel_drain(&d, ev, MAX_EVENTS);
            for (int i = 0; i < n; i++) {
                if (ev[i].kind == EV_PRESS) { cedo += ev[i].i == PRESS_CEDO; tarde += ev[i].i == PRESS_TARDE; tentativas += ev[i].i == PRESS_TENTATIVA; }
                if (ev[i].kind == EV_IMPACT) { entradas++; defendidos += ev[i].judgement != J_RUIM; }
            }
        }
        CHECK(entradas > 10, "%s: o golpe chega (%d golpes)", quem == 0 ? "cedo" : "tarde", entradas);
        /* nos golpes de uma sequência a preparação já começou depois do aviso (o contato anterior): o aperto vira uma tentativa fora da janela */
        CHECK(defendidos == 0 && tentativas <= entradas / 2, "%s: nunca defende (%d defesas; %d tentativas fora da janela em %d golpes)", quem == 0 ? "cedo" : "tarde", defendidos,
              tentativas, entradas);
        if (quem == 0) CHECK(cedo >= entradas / 2 && tarde == 0, "cedo: o jogo diz \"cedo\" (%d em %d golpes) e nunca \"tarde\" (%d)", cedo, entradas, tarde);
        else CHECK(tarde >= entradas / 2 && cedo == 0, "tarde: o jogo diz \"tarde\" (%d em %d golpes) e nunca \"cedo\" (%d)", tarde, entradas, cedo);
    }
}

/* Oboro. Fase 1: abre com a lição completa, sete golpes. Fase 2: os doze padrões dos
 * aprendizes, cada um igual ao do aprendiz (intervalos, aparência, duplo e preparação),
 * na ordem da trilha na primeira volta e depois sorteados. Fase 3: os mesmos doze com a
 * espera antes do aviso x0,85, dano x1,25, aviso nunca abaixo de 320 ms e sem especial. */
static const struct { int aprendiz; const char *golpe; } ECO[12] = {
    {0, "desabamento"}, {1, "mordida"}, {2, "fenda dupla"}, {3, "geada"}, {4, "fúria do tigre"}, {5, "revoada"},
    {6, "foices gêmeas"}, {7, "incêndio"}, {8, "maré longa"}, {9, "tormenta"}, {10, "meia-noite"}, {11, "lua cheia"},
};

static const Move *move_named(const MasterProfile *m, const char *name, int stance) {
    for (int k = 0; k < m->moveCount; k++)
        if (!strcmp(m->moves[k].name, name) && (stance < -1 || m->moves[k].stance == stance)) return &m->moves[k];
    return NULL;
}

static void test_oboro_fases(void) {
    const MasterProfile *o = roster_get(12);
    const Move *licao = &o->moves[0];
    CHECK(!strcmp(licao->name, "lição completa") && licao->strikes == 7 && licao->stance == 0, "fase 1: a lição completa, sete golpes");
    /* os ecos, fase 2 e fase 3, iguais ao padrão de cada aprendiz */
    for (int e = 0; e < 12; e++) {
        const MasterProfile *a = roster_get(ECO[e].aprendiz);
        const Move *src = move_named(a, ECO[e].golpe, -2);
        char nome[64];
        snprintf(nome, sizeof nome, "eco %s", a->style + 8);
        for (int f = 1; f <= 2; f++) {
            const Move *eco = move_named(o, nome, f);
            bool igual = src && eco && eco->strikes == src->strikes && eco->look == src->look && eco->dual == src->dual &&
                         fabsf(eco->windup - src->windup) < 1e-6f;
            for (int g = 0; igual && g + 1 < src->strikes; g++) igual = fabsf(eco->gaps[g] - src->gaps[g]) < 1e-6f;
            CHECK(igual, "fase %d: %s é o %s de %s", f + 1, nome, ECO[e].golpe, a->name);
        }
    }
    CHECK(o->stances[0].ordered == 1 && o->stances[1].ordered == 12 && o->stances[2].ordered == 0, "ordem: 1 na fase 1, 12 na fase 2");
    CHECK(fabsf(o->seals[2].speedMultiplier - 0.85f) < 1e-6f && fabsf(o->seals[2].damageMultiplier - 1.25f) < 1e-6f,
          "fase 3: x0,85 na preparação, x1,25 no dano");
    CHECK(o->seals[0].noSpecial && o->seals[1].noSpecial && o->seals[2].noSpecial, "nenhuma fase tem o especial x2");
    CHECK(fabsf(o->seals[1].damageMultiplier - 0.5f) < 1e-6f, "fase 2: dano x0,5");
    /* erros até cair em cada fase, com a vida cheia a cada selo: 5, 10 e 4 */
    {
        static const int ERROS[3] = {5, 10, 4};
        Settings s0;
        settings_default(&s0);
        for (int f = 0; f < 3; f++) {
            Duel d;
            duel_init(&d, &s0, o, 1);
            duel_start_seal(&d, f);
            int n = (int)ceilf(s0.renPosture / duel_ren_damage(&d) - 1e-4f);
            CHECK(n == ERROS[f], "fase %d: %d erros até cair (%d)", f + 1, ERROS[f], n);
        }
    }
    static const float PW[3] = {0.062f, 0.051f, 0.042f};
    for (int k = 0; k < 3; k++) CHECK(fabsf(o->stances[k].perfectWindow - PW[k]) < 1e-6f, "fase %d: perfeita de %.0f ms", k + 1, PW[k] * 1000);
    for (int k = 0; k < o->stanceCount; k++) CHECK(o->stances[k].aviso >= AJ_AVISO_MENOR - 1e-6f, "fase %d: aviso de 320 ms ou mais", k + 1);

    /* no duelo, com o robô do demo */
    Settings s;
    settings_default(&s);
    settings_for_level(&s, MASTER_COUNT);
    bool abre = true, ordem = true, fase3 = true, semEspecial = true, dano = true;
    int vistosFase2 = 0, especiaisAntes = 0;
    for (uint32_t seed = 1; seed <= 40; seed++) {
        Duel d;
        duel_init(&d, &s, o, seed);
        RoboMente r;
        robo_iniciar(&r, &ROBO_DO_DEMO, seed);
        int ultimo = -1, seqFase[3] = {0, 0, 0};
        while (d.phase != PH_FINISHED && d.clock < 900) {
            if (d.phase == PH_WINDUP && d.attacks != ultimo) {
                ultimo = d.attacks;
                const Move *mv = duel_move(&d);
                if (d.comboStrike == 0) {
                    int n = seqFase[d.seal]++;
                    if (d.seal == 0 && n == 0 && mv != licao) abre = false;
                    if (d.seal == 1 && n < 12) {
                        char nome[64];
                        snprintf(nome, sizeof nome, "eco %s", roster_get(ECO[n].aprendiz)->style + 8);
                        if (strcmp(mv->name, nome) || mv->stance != 1) ordem = false;
                        vistosFase2++;
                    }
                    if (d.seal == 2) {
                        double antes = (mv->windup - o->stances[2].aviso) * 0.85 * s.waitScale;
                        double esperado = duel_aviso(&d) + (antes < AJ_PREPARO_ANTES_DO_AVISO ? AJ_PREPARO_ANTES_DO_AVISO : antes);
                        if (mv->stance != 2 || strncmp(mv->name, "eco ", 4) || fabs(d.windupDuration - esperado) > 1e-4) fase3 = false;
                        float base = s.renPosture / o->hitsToFall;
                        if (fabsf(duel_ren_damage(&d) - base * 1.25f) > 1e-3f) dano = false;
                    } else if (d.seal == 1 && fabsf(duel_ren_damage(&d) - s.renPosture / o->hitsToFall * 0.5f) > 1e-3f) {
                        dano = false;
                    }
                    if (d.special) semEspecial = false;
                    especiaisAntes++;
                }
            }
            bool p = robo_quer_apertar(&r, &d, ROBO_QUADRO);
            duel_step(&d, ROBO_QUADRO, p);
            duel_drain(&d, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
        }
    }
    CHECK(abre, "a fase 1 sempre abre com a lição completa");
    CHECK(ordem && vistosFase2 == 40 * 12, "na fase 2, os doze saem na ordem da trilha (%d de %d)", vistosFase2, 40 * 12);
    CHECK(fase3, "na fase 3, os doze ecos com a espera antes do aviso x0,85");
    CHECK(semEspecial && especiaisAntes > 0, "o especial x2 nunca sai (%d sequências)", especiaisAntes);
    CHECK(dano, "o dano de um erro: x0,5 na fase 2, x1,25 na fase 3");
}

/* Vantagem: quando falta só um perfeito para quebrar a postura, o duelo avisa (e o
 * perfeito seguinte quebra: é a execução, sem botão). Se o mestre se recupera acima
 * disso, a vantagem acaba. */
static void test_vantagem(void) {
    Settings s;
    settings_default(&s);
    for (int i = 0; i < roster_size(); i++) {
        Duel d;
        duel_init(&d, &s, roster_get(i), 12);
        while (d.phase != PH_WINDUP) duel_tick(&d, DT);
        d.bossPosture = s.perfectBossDamage + s.goodBossDamage * 0.5f;   /* um bom leva à vantagem */
        CHECK(!duel_advantage(&d), "%s: ainda sem vantagem", roster_get(i)->name);
        const Stance *st = duel_stance(&d);
        duel_tick(&d, d.strikeAt - (st->perfectWindow + st->goodWindow) / 2 - d.clock);
        duel_press(&d);
        DuelEvent ev[MAX_EVENTS];
        bool avisou = false;
        while (d.phase == PH_WINDUP) duel_tick(&d, DT);
        int k = duel_drain(&d, ev, MAX_EVENTS);
        for (int e = 0; e < k; e++) if (ev[e].kind == EV_ADVANTAGE && ev[e].flag) avisou = true;
        CHECK(d.lastJudgement == J_BOM && avisou && d.advantage && duel_advantage(&d), "%s: um bom deixa a vantagem, e o duelo avisa", roster_get(i)->name);
        /* o próximo perfeito quebra (a menos que seja o último golpe de uma sequência sem fim) */
        while (d.phase != PH_WINDUP && d.phase != PH_FINISHED) duel_tick(&d, DT);
        int sealAntes = d.seal;
        duel_tick(&d, d.strikeAt - 0.01 - d.clock);
        duel_press(&d);
        while (d.phase == PH_WINDUP) duel_tick(&d, DT);
        bool quebrou = d.phase == PH_FINISHED || d.seal > sealAntes;
        k = duel_drain(&d, ev, MAX_EVENTS);
        bool acabou = false;
        for (int e = 0; e < k; e++) if (ev[e].kind == EV_ADVANTAGE && !ev[e].flag) acabou = true;
        CHECK(quebrou && acabou && !d.advantage, "%s: o perfeito seguinte quebra a postura e a vantagem acaba", roster_get(i)->name);
    }
    /* quem se cura sai da vantagem */
    Duel d;
    duel_init(&d, &s, roster_get(4), 3);   /* garfiel se recupera quando acerta */
    while (d.phase != PH_WINDUP) duel_tick(&d, DT);
    d.bossPosture = s.perfectBossDamage * 0.5f;
    d.advantage = true;
    while (d.phase == PH_WINDUP) duel_tick(&d, DT);   /* sem defesa: ele acerta e se recupera */
    CHECK(!d.advantage && d.bossPosture > s.perfectBossDamage * 0.5f, "o mestre que se recupera sai da vantagem (%.1f)", d.bossPosture);
}

/* Taxa de quadros. O núcleo não pode depender dela: a mesma sequência de apertos, nos
 * mesmos ms, dá o mesmo julgamento (e o mesmo erro, com 1 ms de folga) a 30, 60, 120, 144
 * e 240 Hz, e o contato, o aviso e o rearme dentro das sequências caem nos mesmos
 * instantes. O tempo avança em duel_tick(delta); o julgamento é em resolve(), com o
 * instante do aperto e do contato, nunca com o quadro. Só o clique do jogo, que só
 * sabe em que quadro veio, entra no meio do quadro (duel_step). */
typedef struct { int move, strike, julg; double lead, contato, preparacao, cue; } GolpeFps;

/* aperta em cada golpe numa antecedência (dentro e fora das janelas, e sem aperto), guardada
 * em instantes absolutos para ser reproduzida em outras taxas */
static double antecedencia_fps(int k, const Stance *st) {
    double pw = st->perfectWindow, gw = st->goodWindow;
    const double L[] = {pw * 0.5, pw - 0.002, pw + 0.002, (pw + gw) * 0.5, gw - 0.002, gw + 0.005, 0.0, -0.010, -0.040, 0.020, 1e9};
    return L[k % 11];
}

static int roda_fps_selo(const MasterProfile *m, int nivel, int seal, uint32_t seed, double latencia, double hz, double *aperta, int *np, bool gera,
                         GolpeFps *g, int max) {
    Settings s;
    settings_default(&s);
    settings_for_level(&s, nivel);
    s.latency = (float)latencia;
    Duel d;
    duel_init(&d, &s, m, seed);
    if (seal > 0) duel_start_seal(&d, seal);
    const double dt = 1.0 / hz;
    int n = 0, ultimo = -1, ip = 0;
    if (gera) *np = 0;
    while (d.phase != PH_FINISHED && d.clock < 300 && n < max) {
        d.bossPosture = 1e6f;   /* a luta não acaba */
        d.renPosture = s.renPosture;
        if (d.phase == PH_WINDUP && d.attacks != ultimo) {
            ultimo = d.attacks;
            g[n] = (GolpeFps){d.move, d.comboStrike, -2, 0, d.strikeAt, d.windupDuration, d.strikeAt - duel_cue_time(&d)};
            if (gera) {
                double L = antecedencia_fps(n, duel_stance(&d));
                if (L < 1e8) aperta[(*np)++] = d.strikeAt - L;
            }
            n++;
        }
        double off = -1;
        if (ip < *np && aperta[ip] < d.clock + dt) {
            off = aperta[ip] > d.clock ? aperta[ip] - d.clock : 0;
            ip++;
        }
        duel_step_at(&d, dt, off);
        DuelEvent ev[MAX_EVENTS];
        int k = duel_drain(&d, ev, MAX_EVENTS);
        for (int e = 0; e < k; e++)
            if (ev[e].kind == EV_IMPACT && n > 0) {
                g[n - 1].julg = ev[e].judgement;
                g[n - 1].lead = ev[e].a;
            }
    }
    return n;
}

static int roda_fps(const MasterProfile *m, int nivel, uint32_t seed, double latencia, double hz, double *aperta, int *np, bool gera,
                    GolpeFps *g, int max) {
    return roda_fps_selo(m, nivel, 0, seed, latencia, hz, aperta, np, gera, g, max);
}

static void test_taxa_de_quadros(void) {
    static const double HZ[] = {30, 60, 120, 144, 240};
    static const double LAT[] = {0, 0.060};
    double dContato = 0, dPrep = 0, dCue = 0, dErro = 0;
    long comparados = 0, julgDif = 0, movDif = 0, cadeias = 0;
    for (int i = 0; i < roster_size(); i++)
        for (int li = 0; li < 2; li++)
            for (uint32_t seed = 1; seed <= 4; seed++) {
                double aperta[128];
                int np = 0;
                GolpeFps ref[80], run[80];
                int nr = roda_fps(roster_get(i), i, seed, LAT[li], 1000, aperta, &np, true, ref, 60);
                for (int h = 0; h < 5; h++) {
                    int n = roda_fps(roster_get(i), i, seed, LAT[li], HZ[h], aperta, &np, false, run, 60);
                    if (n < nr - 1) julgDif++;   /* mesmo número de golpes (o último pode estar em curso) */
                    for (int k = 0; k < (n < nr ? n : nr) - 1; k++) {
                        comparados++;
                        if (run[k].move != ref[k].move || run[k].strike != ref[k].strike) { movDif++; continue; }
                        if (run[k].strike > 0) cadeias++;
                        if (run[k].julg != ref[k].julg) julgDif++;
                        else if (run[k].julg > 0 && ref[k].lead > -0.5 && fabs(run[k].lead - ref[k].lead) > dErro) dErro = fabs(run[k].lead - ref[k].lead);
                        if (fabs(run[k].contato - ref[k].contato) > dContato) dContato = fabs(run[k].contato - ref[k].contato);
                        if (fabs(run[k].preparacao - ref[k].preparacao) > dPrep) dPrep = fabs(run[k].preparacao - ref[k].preparacao);
                        if (fabs(run[k].cue - ref[k].cue) > dCue) dCue = fabs(run[k].cue - ref[k].cue);
                    }
                }
            }
    printf("taxa de quadros: %ld golpes conferidos (%ld em sequência); dif máx: contato %.3f ms, preparação %.3f ms, aviso %.3f ms, erro %.3f ms\n",
           comparados, cadeias, dContato * 1000, dPrep * 1000, dCue * 1000, dErro * 1000);
    CHECK(comparados > 20000 && cadeias > 8000, "conferidos %ld golpes a 30, 60, 120, 144 e 240 Hz (%ld em sequência)", comparados, cadeias);
    CHECK(movDif == 0, "os mesmos golpes na mesma ordem em qualquer taxa");
    CHECK(julgDif == 0, "o mesmo julgamento (perfeito, bom ou erro) em qualquer taxa (%ld diferentes)", julgDif);
    CHECK(dErro < 0.001, "o erro do aperto muda menos de 1 ms entre as taxas (%.4f ms)", dErro * 1000);
    CHECK(dContato < 0.001, "o contato cai no mesmo instante em qualquer taxa (%.4f ms)", dContato * 1000);
    CHECK(dPrep < 0.001 && dCue < 0.001, "a preparação e o aviso, também nas sequências, não dependem da taxa (%.4f, %.4f ms)", dPrep * 1000, dCue * 1000);
    /* os robôs, que decidem em ms, também: a mesma luta, o mesmo resultado, a 60 e a 144 Hz */
    int igual = 0, total = 0;
    for (int i = 0; i < roster_size(); i++)
        for (uint32_t k = 0; k < 12; k++) {
            RoboLuta a = robo_lutar_hz(&ROBO_HUMANO_CASUAL, roster_get(i), i, 40000 + k, 60, false);
            RoboLuta b = robo_lutar_hz(&ROBO_HUMANO_CASUAL, roster_get(i), i, 40000 + k, 144, false);
            total++;
            igual += a.vitoria == b.vitoria && a.perfeitos == b.perfeitos && a.bons == b.bons && a.erros == b.erros;
        }
    CHECK(igual == total, "o humano casual, decidindo em ms, luta igual a 60 e a 144 Hz (%d de %d lutas idênticas)", igual, total);
}

/* A Shizuku tem quatro leituras de florete. A finta é só um passo sem contato;
 * o eco de gelo copia a dupla e também mantém o segundo contato na linha reta. */
static void test_florete(void) {
    const MasterProfile *m = roster_get(3), *o = roster_get(12);
    static const char *NOMES[] = {"floco", "geada", "deslize", "finta de gelo"};
    CHECK(m->moveCount == 4, "shizuku tem exatamente quatro padrões de florete");
    int fintas = 0;
    for (int k = 0; k < m->moveCount; k++) {
        const Move *mv = &m->moves[k];
        CHECK(!strcmp(mv->name, NOMES[k]), "florete %d: %s", k + 1, NOMES[k]);
        CHECK(mv->look == LOOK_THRUST || mv->look == LOOK_DASH, "%s não corta alto ou baixo", mv->name);
        CHECK(mv->thrustOnly && mv->dual == 0, "%s mantém uma lâmina na linha central", mv->name);
        CHECK(mv->strikes == (k == 1 ? 2 : 1), "%s tem só os contatos anunciados", mv->name);
        for (int hit = 0; hit < mv->strikes; hit++)
            CHECK(move_contact_look(mv, hit) == (k == 2 ? LOOK_DASH : LOOK_THRUST), "%s contato %d é estocada", mv->name, hit + 1);
        fintas += mv->feint;
        if (mv->feint) CHECK(k == 3 && mv->strikes == 1, "a finta não cria contato extra");
    }
    CHECK(fintas == 1, "só a quarta sequência tem finta visual");
    CHECK(fabsf(m->moves[1].gaps[0] - 0.40f) < 1e-6f, "a dupla deixa 400 ms para o segundo parry");
    for (int seal = 1; seal <= 2; seal++) {
        const Move *echo = move_named(o, "eco do gelo", seal);
        const Move *src = &m->moves[1];
        CHECK(echo && echo->stance == seal && echo->strikes == src->strikes &&
              echo->look == src->look && echo->thrustOnly && !echo->feint && echo->dual == src->dual &&
              fabsf(echo->windup - src->windup) < 1e-6f && fabsf(echo->gaps[0] - src->gaps[0]) < 1e-6f,
              "eco de gelo do selo %d copia a dupla do florete", seal + 1);
        if (echo) CHECK(move_contact_look(echo, 1) == LOOK_THRUST, "eco de gelo do selo %d não vira corte alto", seal + 1);
    }
    CHECK(o->moveCount == 31, "oboro preserva 31 padrões com os doze ecos em cada selo");

    /* O campo feint só chega ao desenho: removê-lo de uma cópia do roster não
     * pode mudar nenhum resultado ou duração do núcleo. */
    MasterProfile semFinta = *m;
    semFinta.moves[3].feint = false;
    int iguais = 0;
    for (uint32_t seed = 1; seed <= 60; seed++) {
        RoboLuta a = robo_lutar_hz(&ROBO_HUMANO_CASUAL, m, 3, seed, 60, false);
        RoboLuta b = robo_lutar_hz(&ROBO_HUMANO_CASUAL, &semFinta, 3, seed, 60, false);
        iguais += a.vitoria == b.vitoria && a.perfeitos == b.perfeitos && a.bons == b.bons && a.erros == b.erros &&
                  fabs(a.duracao - b.duracao) < 1e-9;
    }
    CHECK(iguais == 60, "finta é só apresentação: 60 lutas idênticas");

    /* Cada padrão novo e os dois ecos: perfeita e boa ainda existem no atraso
     * máximo; o mesmo roteiro produz os mesmos contatos a 60 e 144 Hz. */
    int cobertos = 0, fpsIguais = 0;
    for (int grupo = 0; grupo < 3; grupo++) {
        const MasterProfile *base = grupo == 0 ? m : o;
        int seal = grupo == 0 ? 0 : grupo;
        int moves = grupo == 0 ? 4 : 1;
        for (int k = 0; k < moves; k++) {
            MasterProfile one = *base;
            one.moves[0] = grupo == 0 ? m->moves[k] : *move_named(o, "eco do gelo", seal);
            one.moves[0].stance = -1;
            one.moveCount = 1;
            Settings s;
            settings_default(&s);
            settings_for_level(&s, grupo == 0 ? 3 : 12);
            s.latency = AJ_LATENCIA_MAX;
            Duel d;
            duel_init(&d, &s, &one, 33);
            if (grupo > 0) duel_start_seal(&d, seal);
            RoboMente r;
            robo_iniciar(&r, &ROBO_DO_DEMO, 33);
            int seen[MAX_CHAIN] = {0}, last = -1;
            for (int steps = 0; steps < 20000; steps++) {
                d.bossPosture = 1e6f;
                d.renPosture = s.renPosture;
                if (d.phase == PH_WINDUP && d.attacks != last) {
                    last = d.attacks;
                    int hit = d.comboStrike;
                    if (hit < one.moves[0].strikes && !seen[hit]) {
                        const Stance *st = duel_stance(&d);
                        double pw = st->perfectWindow, gw = st->goodWindow;
                        bool ok = probe(&d, d.strikeAt - pw * 0.5 + s.latency) == J_PERFEITO &&
                                  probe(&d, d.strikeAt - (pw + gw) * 0.5 + s.latency) == J_BOM;
                        CHECK(ok, "%s/selo %d contato %d: perfeita e boa viáveis com 120 ms de atraso", one.moves[0].name, seal + 1, hit + 1);
                        seen[hit] = 1;
                        cobertos++;
                    }
                }
                duel_step_at(&d, 1.0 / 60, robo_aperto_em(&r, &d, 1.0 / 60));
                duel_drain(&d, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
                bool complete = true;
                for (int hit = 0; hit < one.moves[0].strikes; hit++) complete &= seen[hit] != 0;
                if (complete) break;
            }
            for (int hit = 0; hit < one.moves[0].strikes; hit++) CHECK(seen[hit], "%s/selo %d contato %d foi observado", one.moves[0].name, seal + 1, hit + 1);
            double aperta[128];
            int np = 0;
            GolpeFps a[60], b[60];
            int na = roda_fps_selo(&one, grupo == 0 ? 3 : 12, seal, 33, AJ_LATENCIA_MAX, 60, aperta, &np, true, a, 60);
            int nb = roda_fps_selo(&one, grupo == 0 ? 3 : 12, seal, 33, AJ_LATENCIA_MAX, 144, aperta, &np, false, b, 60);
            CHECK(na >= 20 && nb >= 20, "%s/selo %d: pelo menos 20 contatos em cada taxa", one.moves[0].name, seal + 1);
            bool igual = na == nb;
            for (int hit = 0; igual && hit < na - 1; hit++)
                igual = a[hit].move == b[hit].move && a[hit].strike == b[hit].strike && a[hit].julg == b[hit].julg &&
                        fabs(a[hit].contato - b[hit].contato) < 0.001;
            fpsIguais += igual;
        }
    }
    CHECK(cobertos == 9, "nove contatos dos quatro padrões e dois ecos conferidos no atraso máximo (%d)", cobertos);
    CHECK(fpsIguais == 6, "quatro padrões e dois ecos iguais a 60 e 144 Hz com atraso máximo (%d)", fpsIguais);
}

/* Cada golpe de cada mestre comum, sozinho num repertório de um golpe só (assim o sorteio não esconde nenhum): com o atraso máximo de 120 ms, a perfeita e a boa
 * existem em todos os contatos da sequência, e o mesmo roteiro dá os mesmos contatos a 60 e a 144 Hz. É o que o test_florete faz com o florete, para o repertório
 * inteiro: um golpe novo entra no roster já coberto. (O oboro tem os seus ecos no test_florete e no test_oboro; a shizuku fica de fora porque é dele.) */
static void test_cada_golpe(void) {
    int golpes = 0, contatos = 0, iguais = 0;
    for (int i = 0; i < roster_size(); i++) {
        const MasterProfile *m = roster_get(i);
        if (m->isBigBoss) continue;
        for (int k = 0; k < m->moveCount; k++) {
            MasterProfile one = *m;
            one.moves[0] = m->moves[k];
            one.moves[0].stance = -1;
            one.moveCount = 1;
            Settings s;
            settings_default(&s);
            settings_for_level(&s, i);
            s.latency = AJ_LATENCIA_MAX;
            Duel d;
            duel_init(&d, &s, &one, 33);
            RoboMente r;
            robo_iniciar(&r, &ROBO_DO_DEMO, 33);
            int seen[MAX_CHAIN] = {0}, last = -1;
            for (int steps = 0; steps < 20000; steps++) {
                d.bossPosture = 1e6f;
                d.renPosture = s.renPosture;
                if (d.phase == PH_WINDUP && d.attacks != last) {
                    last = d.attacks;
                    int hit = d.comboStrike;
                    if (hit < one.moves[0].strikes && !seen[hit]) {
                        const Stance *st = duel_stance(&d);
                        double pw = st->perfectWindow, gw = st->goodWindow;
                        bool ok = probe(&d, d.strikeAt - pw * 0.5 + s.latency) == J_PERFEITO &&
                                  probe(&d, d.strikeAt - (pw + gw) * 0.5 + s.latency) == J_BOM;
                        CHECK(ok, "%s/%s contato %d: perfeita e boa viáveis com %.0f ms de atraso", m->name, one.moves[0].name, hit + 1, s.latency * 1000);
                        seen[hit] = 1;
                        contatos++;
                    }
                }
                duel_step_at(&d, 1.0 / 60, robo_aperto_em(&r, &d, 1.0 / 60));
                duel_drain(&d, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
                bool complete = true;
                for (int hit = 0; hit < one.moves[0].strikes; hit++) complete &= seen[hit] != 0;
                if (complete) break;
            }
            for (int hit = 0; hit < one.moves[0].strikes; hit++) CHECK(seen[hit], "%s/%s contato %d foi observado", m->name, one.moves[0].name, hit + 1);
            double aperta[128];
            int np = 0;
            GolpeFps a[60], b[60];
            int na = roda_fps_selo(&one, i, 0, 33, AJ_LATENCIA_MAX, 60, aperta, &np, true, a, 60);
            int nb = roda_fps_selo(&one, i, 0, 33, AJ_LATENCIA_MAX, 144, aperta, &np, false, b, 60);
            bool igual = na >= 20 && nb >= 20 && na == nb;
            for (int hit = 0; igual && hit < na - 1; hit++)
                igual = a[hit].move == b[hit].move && a[hit].strike == b[hit].strike && a[hit].julg == b[hit].julg && fabs(a[hit].contato - b[hit].contato) < 0.001;
            CHECK(igual, "%s/%s: pelo menos 20 contatos e os mesmos a 60 e a 144 Hz com atraso máximo (%d e %d)", m->name, one.moves[0].name, na, nb);
            iguais += igual;
            golpes++;
        }
    }
    CHECK(iguais == golpes, "todo golpe dos mestres comuns igual a 60 e 144 Hz (%d de %d)", iguais, golpes);
    printf("cada golpe: %d golpes dos mestres comuns, %d contatos conferidos no atraso máximo, iguais a 60 e 144 Hz\n", golpes, contatos);
}

/* O traço aleatório do hayate (±120 ms) e do jinshi (±80 ms) só mexe na espera antes do
 * aviso: ela nunca fica abaixo do piso de 100 ms (AJ_PREPARO_ANTES_DO_AVISO), o aviso fica
 * sempre no mesmo lugar (o tempo fixo antes do contato) e nunca sai antes de a preparação
 * começar. Também com um traço absurdo, que empurra a espera para baixo o tempo todo. */
static void test_traco_aleatorio(void) {
    Settings s;
    settings_default(&s);
    for (int volta = 0; volta < 3; volta++) {
        /* 0: hayate, 1: jinshi, 2: hayate com traço de ±5 s (só para provar o piso) */
        int id = volta == 1 ? 11 : 6;
        MasterProfile m = *roster_get(id);
        if (volta == 2) m.rhythmJitter = 5.0f;
        CHECK(volta == 2 || (volta == 0 && fabsf(m.rhythmJitter - 0.12f) < 1e-6f) || (volta == 1 && fabsf(m.rhythmJitter - 0.08f) < 1e-6f),
              "%s: traço de ±%.0f ms", m.name, m.rhythmJitter * 1000);
        double minEspera = 9, maxEspera = 0;
        long sequencias = 0, noPiso = 0;
        bool piso = true, avisoFixo = true, avisoDepois = true;
        for (uint32_t seed = 1; seed <= 400; seed++) {
            Duel d;
            duel_init(&d, &s, &m, seed);
            int ultimo = -1;
            for (int passos = 0; passos < 40 && d.phase != PH_FINISHED;) {
                d.bossPosture = 1e6f;
                d.renPosture = s.renPosture;
                if (d.phase == PH_WINDUP && d.attacks != ultimo) {
                    ultimo = d.attacks;
                    passos++;
                    if (d.comboStrike == 0) {
                        const Stance *st = duel_stance(&d);
                        double espera = d.windupDuration - duel_aviso(&d), aviso = d.strikeAt - duel_cue_time(&d);
                        sequencias++;
                        if (espera < minEspera) minEspera = espera;
                        if (espera > maxEspera) maxEspera = espera;
                        if (espera < AJ_PREPARO_ANTES_DO_AVISO - 1e-6) piso = false;
                        if (fabs(espera - AJ_PREPARO_ANTES_DO_AVISO) < 1e-6) noPiso++;
                        if (fabs(aviso - st->aviso) > 1e-6) avisoFixo = false;
                        if (duel_cue_time(&d) < d.strikeAt - d.windupDuration - 1e-9) avisoDepois = false;
                    }
                }
                bool p = d.phase == PH_WINDUP && !d.attempted && d.strikeAt - d.clock <= 0.03;
                duel_step(&d, 1.0 / 60, p);
                duel_drain(&d, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
            }
        }
        CHECK(sequencias > 4000, "%s: %ld sequências sorteadas", m.name, sequencias);
        CHECK(piso && minEspera >= AJ_PREPARO_ANTES_DO_AVISO - 1e-6, "%s: a espera antes do aviso nunca fica abaixo de %.0f ms (mínimo %.0f ms, máximo %.0f ms)",
              m.name, AJ_PREPARO_ANTES_DO_AVISO * 1000, minEspera * 1000, maxEspera * 1000);
        CHECK(avisoFixo, "%s: o aviso fica sempre %.0f ms antes do contato", m.name, duel_stance(&(Duel){.m = &m})->aviso * 1000);
        CHECK(avisoDepois, "%s: o aviso nunca sai antes de a preparação começar", m.name);
        if (volta != 1) CHECK(noPiso > 0, "%s: o piso é exercitado (%ld sequências)", m.name, noPiso);
    }
}

/* Calibração alta (atraso de até AJ_LATENCIA_MAX = 120 ms). Um golpe sem defesa só é julgado
 * na tolerância tardia mais o atraso, e a preparação seguinte da sequência começa depois; a
 * lâmina parte, no máximo, o tempo que falta até o contato (nunca menos de AJ_LAMINA_MIN),
 * então o ritmo da sequência não atrasa: o contato seguinte chega exatamente `intervalo`
 * depois do anterior, em tempo real, em todos os mestres e com todo o atraso. */
static void test_calibracao_alta(void) {
    static const double ATRASO[] = {0, 0.030, 0.060, 0.090, 0.120};
    double pior = 0, menorLamina = 9;
    bool cabe = true;
    long medidos = 0;
    for (int i = 0; i < roster_size(); i++)
        for (int a = 0; a < 5; a++)
            for (int modo = 0; modo < 3; modo++)   /* 0: sem defesa (o pior), 1: perfeito calibrado, 2: bom */
                for (uint32_t seed = 1; seed <= 6; seed++) {
                    const MasterProfile *m = roster_get(i);
                    Settings s;
                    settings_default(&s);
                    settings_for_level(&s, i);
                    s.latency = (float)ATRASO[a];
                    Duel d;
                    duel_init(&d, &s, m, seed);
                    int ultimo = -1;
                    double contatoAnt = -1, congelado = 0;
                    for (int passos = 0; passos < 60 && d.phase != PH_FINISHED;) {
                        d.bossPosture = 1e6f;
                        d.renPosture = s.renPosture;
                        if (d.phase == PH_WINDUP && d.attacks != ultimo) {
                            ultimo = d.attacks;
                            passos++;
                            double lead = duel_strike_lead(&d);
                            if (d.comboStrike > 0) {
                                const Move *mv = duel_move(&d);
                                double gap = mv->gaps[d.comboStrike - 1];
                                if (gap < s.minChainGap) gap = s.minChainGap;
                                double erro = fabs((d.strikeAt - contatoAnt + congelado) - gap);
                                if (erro > pior) pior = erro;
                                medidos++;
                                if (lead > d.windupDuration + 1e-9 || lead < AJ_LAMINA_MIN - 1e-9) cabe = false;
                                if (lead < menorLamina) menorLamina = lead;
                            }
                            contatoAnt = d.strikeAt;
                        }
                        double off = -1;
                        if (d.phase == PH_WINDUP && !d.attempted && modo > 0) {
                            const Stance *st = duel_stance(&d);
                            double alvo = d.strikeAt - (modo == 1 ? 0.5 * st->perfectWindow : 0.5 * (st->perfectWindow + st->goodWindow)) + ATRASO[a];
                            if (alvo < d.clock + 1.0 / 60) off = alvo > d.clock ? alvo - d.clock : 0;
                        }
                        duel_step_at(&d, 1.0 / 60, off);
                        DuelEvent ev[MAX_EVENTS];
                        int k = duel_drain(&d, ev, MAX_EVENTS);
                        for (int e = 0; e < k; e++) if (ev[e].kind == EV_IMPACT) congelado = d.lastHitstop;
                    }
                }
    printf("calibração alta: %ld intervalos de sequência; pior atraso do ritmo %.3f ms; menor partida da lâmina %.0f ms\n", medidos, pior * 1000, menorLamina * 1000);
    CHECK(medidos > 5000, "%ld intervalos de sequência conferidos, com atraso de 0 a 120 ms", medidos);
    CHECK(pior < 0.001, "o ritmo da sequência não atrasa com o atraso calibrado (pior %.3f ms)", pior * 1000);
    CHECK(cabe, "a lâmina parte no máximo o tempo que falta, e nunca menos de %.0f ms nas sequências", AJ_LAMINA_MIN * 1000);
}

/* Rastro fantasma (só desenho). A função do núcleo que guia o rastro dá 0 antes da partida
 * da lâmina e do contato em diante, e cresce de 0 a 1 entre os dois: o rastro nunca aparece
 * no instante do julgamento, e ler o duelo não o muda (o teste compara o duelo antes e
 * depois). As constantes do rastro moram no ajuste.h. */
static void test_rastro_fantasma(void) {
    CHECK(AJ_RASTRO_FANTASMA == 0 || AJ_RASTRO_FANTASMA == 1, "o rastro fantasma liga (1) e desliga (0) no ajuste.h");
    CHECK(AJ_RASTRO_FANTASMAS >= 1 && AJ_RASTRO_FANTASMAS <= 6 && AJ_RASTRO_ESPACO > 0 && AJ_RASTRO_ALFA > 0 && AJ_RASTRO_ALFA <= 1,
          "constantes do rastro dentro do que o desenho aceita");
    Settings s;
    settings_default(&s);
    long golpes = 0, passos = 0;
    bool zeroAntes = true, zeroNoContato = true, cresce = true, dentro = true, intacto = true, ultimoPerto1 = true;
    for (int i = 0; i < roster_size(); i++)
        for (uint32_t seed = 1; seed <= 6; seed++) {
            Duel d;
            duel_init(&d, &s, roster_get(i), seed);
            int ultimo = -1;
            for (int n = 0; n < 30 && d.phase != PH_FINISHED;) {
                d.bossPosture = 1e6f;
                d.renPosture = s.renPosture;
                if (d.phase == PH_WINDUP && d.attacks != ultimo) {
                    ultimo = d.attacks;
                    n++;
                    golpes++;
                    /* varre o golpe inteiro numa cópia, de 2 em 2 ms, sem apertar */
                    Duel c = d;
                    float anterior = 0, ultimoValor = 0;
                    while (c.phase == PH_WINDUP && c.clock < c.strikeAt + 0.001) {
                        Duel antes = c;
                        float p = duel_launch_progress(&c);
                        if (memcmp(&antes, &c, sizeof c) != 0) intacto = false;
                        passos++;
                        if (c.clock < c.strikeAt - duel_strike_lead(&c) && p != 0) zeroAntes = false;
                        if (c.clock >= c.strikeAt && p != 0) zeroNoContato = false;
                        if (p < 0 || p > 1) dentro = false;
                        if (p > 0 && p + 1e-6f < anterior) cresce = false;
                        if (p > 0) { anterior = p; ultimoValor = p; }
                        duel_tick(&c, 0.002);
                    }
                    if (ultimoValor < 0.9f) ultimoPerto1 = false;
                }
                bool p = d.phase == PH_WINDUP && !d.attempted && d.strikeAt - d.clock <= 0.03;
                duel_step(&d, 1.0 / 60, p);
                duel_drain(&d, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
            }
        }
    CHECK(zeroAntes, "antes da partida da lâmina não há rastro");
    CHECK(zeroNoContato, "do contato em diante, no instante do julgamento, não há rastro (nunca se vê o contato antes do impacto)");
    CHECK(dentro && cresce && ultimoPerto1, "entre a partida e o contato o rastro cresce de 0 a 1");
    CHECK(intacto, "ler o rastro não muda o duelo (%ld leituras em %ld golpes)", passos, golpes);
}

/* Ritmo: a espera antes do aviso (o mestre segurando a preparação) e a pausa entre
 * sequências são tempo morto, e encurtam. Nada que se julga muda: golpe a golpe, o
 * mesmo movimento, o mesmo aviso e o mesmo intervalo dentro da sequência. Com a espera
 * x1 e a pausa antiga (0,8 s) volta o ritmo de antes. */
typedef struct { int move, strike; double aviso, gap, espera, contato; } GolpeRitmo;

static int coleta_ritmo(const Settings *s, const MasterProfile *m, uint32_t seed, GolpeRitmo *g, int max, double *duracao) {
    Duel d;
    duel_init(&d, s, m, seed);
    RoboMente r;
    robo_iniciar(&r, &ROBO_DO_DEMO, seed);
    int n = 0, ultimo = -1;
    double contatoAnt = -1, hitAnt = 0;
    while (n < max && d.clock < 600) {
        d.bossPosture = 1e6f;   /* sem pressa, e a luta não acaba */
        d.renPosture = s->renPosture;
        if (d.phase == PH_WINDUP && d.attacks != ultimo) {
            ultimo = d.attacks;
            g[n].move = d.move;
            g[n].strike = d.comboStrike;
            g[n].aviso = duel_aviso(&d);
            g[n].espera = d.windupDuration - duel_aviso(&d);
            g[n].gap = d.comboStrike > 0 ? d.strikeAt - contatoAnt + hitAnt : 0;
            g[n].contato = d.strikeAt;
            contatoAnt = d.strikeAt;
            n++;
        }
        bool p = robo_quer_apertar(&r, &d, ROBO_QUADRO);
        duel_step(&d, ROBO_QUADRO, p);
        DuelEvent ev[MAX_EVENTS];
        int k = duel_drain(&d, ev, MAX_EVENTS);
        for (int e = 0; e < k; e++) if (ev[e].kind == EV_IMPACT) hitAnt = d.lastHitstop;
    }
    *duracao = d.clock;
    return n;
}

static void test_ritmo(void) {
    Settings novo, antigo;
    settings_default(&novo);
    settings_default(&antigo);
    antigo.waitScale = 1;
    antigo.recovery = 0.8f;
    CHECK(novo.waitScale == AJ_ESPERA_X && novo.waitScale > 0 && novo.waitScale < 1, "a espera antes do aviso encurta (x%.2f)", novo.waitScale);
    CHECK(novo.recovery == AJ_PAUSA_SEQUENCIA && novo.recovery < 0.8f, "a pausa entre sequências encurta (%.2f s)", novo.recovery);
    CHECK(novo.recovery >= 0.40f, "a pausa não corta a recuperação do mestre (0,32 s de hurt, com folga)");
    /* só o hayate e o jinshi (ritmo irregular) encurtam a espera menos; nos outros vale a global */
    for (int i = 0; i < roster_size(); i++) {
        bool irregular = !strcmp(roster_get(i)->name, "hayate") || !strcmp(roster_get(i)->name, "jinshi");
        CHECK(irregular ? roster_get(i)->waitScale == AJ_ESPERA_X_IRREGULAR : roster_get(i)->waitScale == 0, "%s: espera x%.2f do roster", roster_get(i)->name,
              roster_get(i)->waitScale);
    }
    CHECK(AJ_ESPERA_X_IRREGULAR > AJ_ESPERA_X && AJ_ESPERA_X_IRREGULAR < 1, "a espera dos irregulares encurta, mas menos que a dos outros");
    /* a cena de fala do selo do oboro começa AJ_QUEBRA_ATE_A_CENA depois da quebra (tempo de jogo, na câmera lenta): a pausa
     * do núcleo depois do selo tem de durar mais que isso, ou o mestre começaria o golpe seguinte antes da cena */
    /* quem aperta sem parar ainda vê o resultado (ajuste.h, AJ_FADE_RESULTADO) */
    CHECK(AJ_DERROTA_OPCOES >= AJ_DERROTA_TITULO + AJ_FADE_RESULTADO + 0.06f, "derrota: o título está inteiro %.2f s antes de as opções aceitarem clique",
          AJ_DERROTA_OPCOES - AJ_DERROTA_TITULO - AJ_FADE_RESULTADO);
    CHECK(AJ_VITORIA_TRAVA >= AJ_FADE_RESULTADO + 0.3f, "vitória: o pergaminho está inteiro por %.2f s antes do primeiro clique que vale", AJ_VITORIA_TRAVA - AJ_FADE_RESULTADO);
    CHECK(AJ_PAUSA_SELO >= AJ_QUEBRA_ATE_A_CENA + 0.5f, "a pausa do selo (%.1f s) cobre a quebra até a cena (%.1f s) com folga", AJ_PAUSA_SELO, AJ_QUEBRA_ATE_A_CENA);
    bool mesmoGolpe = true, mesmoAviso = true, mesmaCadeia = true, espera = true, maisRapido = true;
    int golpes = 0;
    for (int i = 0; i < roster_size(); i++) {
        const MasterProfile *m = roster_get(i);
        MasterProfile ma = *m;
        ma.waitScale = 0;                 /* o "antigo" é a espera x1,0 em todos, sem a exceção do roster */
        double dn = 0, da = 0;
        for (uint32_t seed = 1; seed <= 12; seed++) {
            GolpeRitmo n[40], a[40];
            int kn = coleta_ritmo(&novo, m, seed, n, 40, &dn), ka = coleta_ritmo(&antigo, &ma, seed, a, 40, &da);
            int k = kn < ka ? kn : ka;
            for (int j = 0; j < k; j++) {
                golpes++;
                if (n[j].move != a[j].move || n[j].strike != a[j].strike) mesmoGolpe = false;
                /* o 1º golpe avisa no tempo fixo; nos outros o aviso é o contato anterior (o intervalo, abaixo) */
                if (n[j].strike == 0 && fabs(n[j].aviso - a[j].aviso) > 1e-9) mesmoAviso = false;
                if (n[j].strike > 0 && fabs(n[j].gap - a[j].gap) > 1e-6) mesmaCadeia = false;
                /* a espera é a de antes x a do mestre, com o piso de sempre (nos de traço aleatório, conferida abaixo,
                 * numa cópia sem o traço) */
                if (n[j].strike == 0 && m->rhythmJitter == 0) {
                    double esp = a[j].espera * (m->waitScale > 0 ? m->waitScale : novo.waitScale);
                    if (esp < AJ_PREPARO_ANTES_DO_AVISO) esp = AJ_PREPARO_ANTES_DO_AVISO;
                    if (fabs(n[j].espera - esp) > 1e-6) espera = false;
                }
            }
        }
        if (m->rhythmJitter > 0) {
            MasterProfile sem = *m, semAntigo = ma;
            sem.rhythmJitter = semAntigo.rhythmJitter = 0;
            int conferidas = 0;
            bool ok = true;
            for (uint32_t seed = 1; seed <= 12; seed++) {
                GolpeRitmo n[40], a[40];
                double x, y;
                int kn = coleta_ritmo(&novo, &sem, seed, n, 40, &x), ka = coleta_ritmo(&antigo, &semAntigo, seed, a, 40, &y);
                for (int j = 0; j < (kn < ka ? kn : ka); j++) {
                    if (n[j].strike != 0) continue;
                    double esp = a[j].espera * m->waitScale;
                    if (esp < AJ_PREPARO_ANTES_DO_AVISO) esp = AJ_PREPARO_ANTES_DO_AVISO;
                    if (fabs(n[j].espera - esp) > 1e-6) ok = false;
                    conferidas++;
                }
            }
            CHECK(ok && conferidas > 50, "%s: sem o traço aleatório, a espera é a de antes x%.2f (%d sequências conferidas)", m->name, m->waitScale, conferidas);
        }
        /* mais golpes no mesmo tempo (o mesmo número de golpes, em menos tempo) */
        GolpeRitmo n[40], a[40];
        double tempoN, tempoA;
        int kn = coleta_ritmo(&novo, m, 3, n, 40, &tempoN), ka = coleta_ritmo(&antigo, &ma, 3, a, 40, &tempoA);
        int k = kn < ka ? kn : ka;
        if (k > 8 && n[k - 1].contato > a[k - 1].contato * 0.90) maisRapido = false;
    }
    CHECK(mesmoGolpe, "com o ritmo novo ou o antigo, os mesmos golpes na mesma ordem");
    CHECK(mesmoAviso, "o aviso não muda (do aviso ao contato é o mesmo tempo)");
    CHECK(mesmaCadeia, "o intervalo dentro da sequência não muda");
    CHECK(espera, "a espera antes do aviso é a de antes x%.2f, com o piso de %.0f ms", novo.waitScale, AJ_PREPARO_ANTES_DO_AVISO * 1000);
    CHECK(maisRapido, "o mesmo número de golpes chega em pelo menos 10%% menos tempo, em todos os mestres");
    CHECK(golpes > 2000, "ritmo conferido em %d golpes", golpes);
}

/* Lâmina variável: do garfiel em diante, a lâmina parte entre 140 e 320 ms antes do
 * contato (na sequência, até 240), sorteada a cada golpe; os quatro primeiros, a lança
 * e a investida (no primeiro golpe) partem no tempo fixo. Com ela desligada, os
 * contatos, os avisos e as preparações são exatamente os mesmos: só a partida muda. */
static void test_lamina_variavel(void) {
    CHECK(AJ_LAMINA_VARIAVEL == 1 && fabsf(AJ_LAMINA_MIN - 0.140f) < 1e-6f && fabsf(AJ_LAMINA_MAX - 0.320f) < 1e-6f &&
              fabsf(AJ_LAMINA_MAX_CADEIA - 0.240f) < 1e-6f && AJ_LAMINA_VARIA_DESDE == 5,
          "lâmina variável ligada: 140 a 320 ms, 240 na sequência, do garfiel em diante");
    Settings on, off;
    settings_default(&on);
    settings_default(&off);
    off.bladeFrom = 0;
    CHECK(on.bladeFrom == 5 && off.bladeFrom == 0, "a chave desliga (0 = fixa)");
    double menor = 1, maior = 0, maiorCadeia = 0;
    bool fixos = true, faixa = true, iguais = true, desligada = true, partida = true;
    int golpes = 0;
    for (int i = 0; i < roster_size(); i++) {
        const MasterProfile *m = roster_get(i);
        for (uint32_t seed = 1; seed <= 15; seed++) {
            Duel a, b;
            duel_init(&a, &on, m, seed);
            duel_init(&b, &off, m, seed);
            RoboMente ra, rb;
            robo_iniciar(&ra, &ROBO_DO_DEMO, seed);
            robo_iniciar(&rb, &ROBO_DO_DEMO, seed);
            int ultimo = -1;
            while (a.phase != PH_FINISHED && b.phase != PH_FINISHED && a.clock < 900) {
                if (a.phase == PH_WINDUP && a.attacks != ultimo) {
                    ultimo = a.attacks;
                    golpes++;
                    double lead = duel_strike_lead(&a), base = duel_strike_lead_base(&a);
                    const Move *mv = duel_move(&a);
                    bool especial = a.comboStrike == 0 && mv && (mv->look == LOOK_FAR || mv->look == LOOK_DASH);
                    if (m->id < 5 || especial) {
                        if (fabs(lead - base) > 1e-6) fixos = false;
                    } else if (a.comboStrike == 0) {
                        if (lead < 0.140 - 1e-6 || lead > 0.320 + 1e-6) faixa = false;
                        if (lead < menor) menor = lead;
                        if (lead > maior) maior = lead;
                    } else {
                        if (lead < 0.140 - 1e-6 || lead > 0.240 + 1e-6) faixa = false;
                        if (lead > maiorCadeia) maiorCadeia = lead;
                    }
                    /* desligada: o mesmo golpe, a mesma preparação, o mesmo aviso, o mesmo contato */
                    if (b.phase != PH_WINDUP || b.attacks != a.attacks || fabs(b.strikeAt - a.strikeAt) > 1e-9 ||
                        fabs(b.windupDuration - a.windupDuration) > 1e-6 || fabs(duel_cue_time(&b) - duel_cue_time(&a)) > 1e-9 ||
                        b.move != a.move)
                        iguais = false;
                    if (fabs(duel_strike_lead(&b) - duel_strike_lead_base(&b)) > 1e-6) desligada = false;
                    DuelTimeline t = duel_timeline(&a);
                    if (fabs(t.launch - (a.strikeAt - lead)) > 1e-9) partida = false;
                }
                bool pa = robo_quer_apertar(&ra, &a, ROBO_QUADRO), pb = robo_quer_apertar(&rb, &b, ROBO_QUADRO);
                duel_step(&a, ROBO_QUADRO, pa);
                duel_step(&b, ROBO_QUADRO, pb);
                duel_drain(&a, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
                duel_drain(&b, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
            }
            if (a.phase != b.phase || a.attacks != b.attacks) iguais = false;
        }
    }
    CHECK(fixos, "os quatro primeiros, a lança e a investida partem no tempo fixo");
    CHECK(faixa, "a partida fica entre 140 e 320 ms (240 na sequência)");
    CHECK(menor < 0.160 && maior > 0.300 && maiorCadeia > 0.220, "o sorteio cobre a faixa (%.0f a %.0f ms; %.0f na sequência)", menor * 1000,
          maior * 1000, maiorCadeia * 1000);
    CHECK(iguais, "com a lâmina ligada ou não, os mesmos golpes, preparações, avisos e contatos (%d golpes)", golpes);
    CHECK(desligada, "desligada, a lâmina parte sempre no tempo fixo");
    CHECK(partida, "a lâmina parte no tempo sorteado (linha do tempo)");
}

/* Aperta em `quando` (tempo do duelo) numa cópia e devolve o EV_PRESS e o EV_IMPACT. */
static bool aperta_e_ve(const Duel *d, double quando, DuelEvent *aperto, DuelEvent *impacto) {
    Duel c = *d;
    DuelEvent ev[MAX_EVENTS];
    duel_drain(&c, ev, MAX_EVENTS);
    if (quando > c.clock) duel_tick(&c, quando - c.clock);
    duel_drain(&c, ev, MAX_EVENTS);
    aperto->kind = impacto->kind = EV_WINDUP;
    if (quando >= 0 && !duel_press(&c)) return false;
    for (int n = 0; n < 100000; n++) {
        int k = duel_drain(&c, ev, MAX_EVENTS);
        for (int e = 0; e < k; e++) {
            if (ev[e].kind == EV_PRESS) *aperto = ev[e];
            if (ev[e].kind == EV_IMPACT) { *impacto = ev[e]; return true; }
        }
        if (c.phase == PH_FINISHED) break;
        duel_tick(&c, 1.0 / 1000);
    }
    return false;
}

/* Build de teste: começar num selo do oboro, reabastecer vida e postura, e o que o F3
 * mostra de cada aperto (resultado e erro em ms) vem certo do core. */
static void test_teste_a_mao(void) {
    const MasterProfile *o = roster_get(12);
    Settings s;
    settings_default(&s);
    settings_for_level(&s, 12);
    for (int f = 0; f < 3; f++) {
        Duel d;
        duel_init(&d, &s, o, 7);
        duel_start_seal(&d, f);
        CHECK(d.seal == f && d.stanceIndex == f && fabsf(d.bossPosture - o->seals[f].posture) < 1e-3f &&
                  fabsf(d.renPosture - s.renPosture) < 1e-3f && d.phase == PH_READY,
              "começa direto na fase %d do oboro: postura %.0f, postura de luta %d, vida cheia", f + 1, o->seals[f].posture, f);
        while (d.phase != PH_WINDUP) duel_tick(&d, DT);
        const Move *mv = duel_move(&d);
        char eco[64];
        snprintf(eco, sizeof eco, "eco %s", roster_get(0)->style + 8);
        if (f == 0) CHECK(mv && !strcmp(mv->name, "lição completa"), "fase 1 começa pela lição completa");
        if (f == 1) CHECK(mv && !strcmp(mv->name, eco), "fase 2 começa pelo padrão do primeiro aprendiz (%s)", mv ? mv->name : "-");
        if (f == 2) CHECK(fabsf(duel_ren_damage(&d) - s.renPosture / o->hitsToFall * 1.25f) < 1e-3f, "fase 3: o erro dói x1,25");
    }
    Duel d;
    duel_init(&d, &s, o, 7);
    duel_start_seal(&d, 9);
    CHECK(d.seal == 2, "selo além do último fica no último");
    duel_init(&d, &s, roster_get(3), 7);
    duel_start_seal(&d, 2);
    CHECK(d.seal == 0 && fabsf(d.bossPosture - roster_get(3)->posture) < 1e-3f, "mestre comum: um selo só");

    /* vida e postura cheias de novo; as brasas do enjin apagam */
    Settings s7;
    settings_default(&s7);
    settings_for_level(&s7, 7);
    duel_init(&d, &s7, roster_get(7), 3);
    while (d.bads == 0 && d.phase != PH_FINISHED) duel_tick(&d, DT);
    CHECK(d.renPosture < s7.renPosture && d.burnLeft > 0, "enjin: o erro tira vida e deixa em brasas");
    duel_drain(&d, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
    duel_refill(&d, true, false);
    CHECK(fabsf(d.renPosture - s7.renPosture) < 1e-3f && d.burnLeft <= 0, "vida cheia de novo, sem brasas");
    duel_drain(&d, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
    d.bossPosture = s7.perfectBossDamage * 0.5f;
    d.advantage = true;
    duel_refill(&d, false, true);
    DuelEvent ev[MAX_EVENTS];
    int k = duel_drain(&d, ev, MAX_EVENTS);
    CHECK(fabsf(d.bossPosture - duel_posture_max(&d)) < 1e-3f && !d.advantage && k == 1 && ev[0].kind == EV_ADVANTAGE && !ev[0].flag,
          "postura cheia de novo e a vantagem some");

    /* o resultado de cada aperto e o erro em ms */
    bool cedo = true, perfeito = true, bomCedo = true, bomTarde = true, erroCedo = true, semAperto = true, tarde = true;
    bool lado = true;   /* o F3 escreve "antes" ou "depois" do contato: duel_event_after_contact dá o lado de cada evento */
    int tardes = 0;
    for (int i = 0; i < roster_size(); i++) {
        Settings si;
        settings_default(&si);
        settings_for_level(&si, i);
        /* duas latências x três sementes: o primeiro golpe sorteado muda com o repertório, e o número de casos do "tarde" não pode depender disso */
        for (int lat = 0; lat < 6; lat++) {
            si.latency = lat % 2 ? 0.050f : 0;
            duel_init(&d, &si, roster_get(i), 11 + (uint32_t)(lat / 2));
            while (d.phase != PH_WINDUP) duel_tick(&d, DT);
            DuelTimeline t = duel_timeline(&d);
            const Stance *st = duel_stance(&d);
            double L = si.latency, pw = st->perfectWindow, gw = st->goodWindow;
            DuelEvent ap, im;
            /* cedo: antes do aviso; a = antes do contato, b = antes do aviso */
            if (t.cue - t.start > 0.05) {
                double q = t.start + 0.02;
                Duel c = d;
                duel_tick(&c, q - c.clock);
                duel_drain(&c, ev, MAX_EVENTS);
                duel_press(&c);
                k = duel_drain(&c, ev, MAX_EVENTS);
                if (!(k >= 1 && ev[k - 1].i == PRESS_CEDO && fabs(ev[k - 1].a - (t.strike - (c.clock - L))) < 1e-4 &&
                      fabs(ev[k - 1].b - (t.cue + L - c.clock)) < 1e-4))
                    cedo = false;
                if (k >= 1 && duel_event_after_contact(&ev[k - 1])) lado = false;   /* cedo: antes do contato */
            }
            if (!aperta_e_ve(&d, t.strike - 0.010 + L, &ap, &im) || ap.i != PRESS_TENTATIVA || fabs(ap.a - 0.010) > 1e-4 ||
                im.judgement != J_PERFEITO || im.b != 0)
                perfeito = false;
            if (duel_event_after_contact(&ap) || duel_event_after_contact(&im)) lado = false;   /* 10 ms antes do contato */
            double lb = (pw + gw) / 2;
            if (!aperta_e_ve(&d, t.strike - lb + L, &ap, &im) || im.judgement != J_BOM || fabs(im.b - (lb - pw)) > 1e-4) bomCedo = false;
            if (!aperta_e_ve(&d, t.strike + 0.015 + L, &ap, &im) || im.judgement != J_BOM || fabs(im.b + 0.015) > 1e-4 ||
                fabs(im.a + 0.015) > 1e-4)
                bomTarde = false;
            if (!duel_event_after_contact(&im)) lado = false;   /* 15 ms depois do contato */
            if (t.strike - gw - 0.010 > t.cue + 0.001 &&
                (!aperta_e_ve(&d, t.strike - gw - 0.010 + L, &ap, &im) || im.judgement != J_RUIM || fabs(im.b - (gw + 0.010 - pw)) > 1e-4))
                erroCedo = false;
            if (!aperta_e_ve(&d, -1, &ap, &im) || im.judgement != J_RUIM || im.a != -1 || im.b != 0) semAperto = false;
            /* tarde: 50 ms depois de levar o golpe que fecha a sequência; a = depois do contato */
            Duel c = d;
            while (c.phase != PH_FINISHED && !(c.phase == PH_WINDUP && c.comboRemaining == 0)) duel_tick(&c, 0.001);
            while (c.phase == PH_WINDUP) duel_tick(&c, 0.001);
            duel_tick(&c, 0.05);
            if (c.phase == PH_RECOVERY) {
                duel_drain(&c, ev, MAX_EVENTS);
                duel_press(&c);
                k = duel_drain(&c, ev, MAX_EVENTS);
                if (!(k >= 1 && ev[k - 1].i == PRESS_TARDE && fabs(ev[k - 1].a - (c.clock - L - c.lastStrikeAt)) < 1e-4 &&
                      ev[k - 1].a > 0.05 + si.lateGrace - 0.002))
                    tarde = false;
                if (!duel_event_after_contact(&ev[k - 1])) lado = false;   /* tarde: a > 0 e é DEPOIS (o F3 dizia "antes") */
                tardes++;
            }
        }
    }
    CHECK(cedo, "cedo: quanto antes do contato e quanto antes do aviso");
    CHECK(perfeito, "perfeito: 10 ms antes do contato, erro 0 (também com 50 ms de atraso calibrado)");
    CHECK(bomCedo, "bom cedo: o erro é quanto passou da janela perfeita");
    CHECK(bomTarde, "bom tarde: 15 ms depois do contato, erro de 15 ms tarde");
    CHECK(erroCedo, "erro cedo: o erro é a distância até a janela perfeita");
    CHECK(semAperto, "erro sem aperto: a = -1, sem erro em ms");
    CHECK(tarde && tardes > 20, "tarde: quanto depois do contato (%d casos)", tardes);
    CHECK(lado, "o F3 diz se o aperto foi antes ou depois do contato: cedo e perfeito antes, tarde e bom tarde depois");
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
    CHECK(d.lastJudgement == J_RUIM && !d.lastAttempted && fabs((d.lastGestureAt - d.lastStrikeAt) - 0.040) < 1e-6,
          "aperto 40 ms depois do contato fica registrado como tarde");
}

/* Os robôs, com o mesmo passo e os mesmos tempos do demo: o perfeito vence todos sem
 * errar, quem nunca defende perde de todos, e martelar o botão também perde. (O
 * "metrônomo cego", que aperta devagar sem olhar, é cobrado na curva: ver test_curva.) */
static void test_robos(void) {
    Robo spam[3] = {ROBO_APERTA_SEM_PARAR, ROBO_APERTA_SEM_PARAR, ROBO_APERTA_SEM_PARAR};
    spam[0].periodo = 0.05f;
    spam[1].periodo = 0.10f;
    spam[2].periodo = 0.20f;
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
        CHECK(spamPerdeu == 15, "martelar o botão perde de %s (%d/15)", m->name, spamPerdeu);
    }
    /* o robô do demo aperta dentro de qualquer janela perfeita do jogo */
    for (int i = 0; i < roster_size(); i++)
        for (int k = 0; k < roster_get(i)->stanceCount; k++)
            CHECK(AJ_ROBO_ANTECEDENCIA < roster_get(i)->stances[k].perfectWindow, "o robô do demo cabe na janela perfeita de %s", roster_get(i)->name);
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
    CHECK(b.renPosture == a.renPosture, "a vida de kojiro não cresce com os mestres vencidos");
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

/* O especial x2: o oboro não usa mais (nenhuma fase), mas a regra continua no core;
 * confere com uma cópia dele que libera o especial no primeiro selo. */
static void test_special(void) {
    MasterProfile oboro = *roster_get(12);
    oboro.seals[0].noSpecial = false;
    Settings s;
    settings_default(&s);
    int specials = 0, doubled = 0, real = 0;
    for (uint32_t seed = 1; seed < 80; seed++) {
        Duel d;
        duel_init(&d, &s, &oboro, seed);
        while (d.phase != PH_WINDUP) duel_tick(&d, DT);
        Duel r;
        duel_init(&r, &s, roster_get(12), seed);
        while (r.phase != PH_WINDUP) duel_tick(&r, DT);
        real += r.special;
        if (!d.special) continue;
        specials++;
        float before = d.renPosture;
        while (d.phase == PH_WINDUP) duel_tick(&d, DT);
        if (fabsf((before - d.renPosture) - 2 * s.renPosture / oboro.hitsToFall) < 1e-3) doubled++;
    }
    CHECK(specials > 5, "com o especial liberado, ele sai (%d)", specials);
    CHECK(doubled == specials, "o especial tira o dobro (%d de %d)", doubled, specials);
    CHECK(real == 0, "o oboro de verdade não solta o especial (%d)", real);
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
        /* a shizuku tem as quatro leituras do florete; os mestres comuns, de 7 a 12 (os três primeiros, sete: acima); o oboro, os doze ecos em cada selo */
        CHECK(i == 3 ? m->moveCount == 4 : m->moveCount >= 7 && (m->isBigBoss || m->moveCount <= 12),
              "%s tem o tamanho aprovado do repertório (%d)", m->name, m->moveCount);
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
        if (!m->isBigBoss && i != 1 && i != 3) CHECK(heavy, "%s tem um golpe forte", m->name);
        /* e todos vêm correndo e saltando em alguma sequência */
        bool dash = false, jump = false;
        for (int k = 0; k < m->moveCount; k++) {
            dash |= m->moves[k].look == LOOK_DASH;
            jump |= m->moves[k].look == LOOK_JUMP;
        }
        CHECK(dash, "%s tem uma investida correndo", m->name);
        if (i != 1 && i != 3) CHECK(jump, "%s tem um golpe saltando", m->name);
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
            double congelado = d.lastHitstop;   /* o jogo congela o duelo esse tanto */
            while (d.phase != PH_WINDUP) duel_tick(&d, DT);
            double real = d.strikeAt - first + congelado;
            CHECK(fabs(real - mv->gaps[0]) < 0.001, "segundo golpe chega %.2f s (de tempo real) depois (%.3f)", mv->gaps[0], real);
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

/* A vitória vale desde o golpe final: campaign_win marca e avança uma vez só, mesmo chamada de novo. */
static void test_campaign_win(void) {
    Campaign c;
    campaign_reset(&c);
    campaign_win(&c, 0);
    CHECK(campaign_is_cleared(&c, 0) && c.index == 1 && !c.completed, "a vitória marca o mestre e avança a trilha");
    campaign_win(&c, 0);
    CHECK(c.index == 1 && campaign_defeated(&c) == 1, "a mesma vitória de novo não avança outra vez");
    campaign_win(&c, 5);
    CHECK(campaign_is_cleared(&c, 5) && c.index == 1, "vencer outro mestre (teste) só marca, sem mover a trilha");
    for (int i = 1; i < 12; i++) campaign_win(&c, i);
    CHECK(c.index == 12 && campaign_big_boss_open(&c) && !c.completed, "os doze vencidos abrem o oboro, sem completar");
    campaign_win(&c, 12);
    CHECK(c.index == 12 && c.completed && campaign_is_cleared(&c, 12), "vencer o oboro completa a trilha");
    campaign_win(&c, 12);
    CHECK(c.index == 12 && c.completed, "e de novo não muda nada");
}

/* O rodízio de vozes do audio.c: cada disparo pega a próxima voz; se ela ainda está tocando, a cauda é cortada. */
typedef struct { double livreAte[VOZES_MAX]; int n, proxima; long cortes, disparos; } Rodizio;

static void dispara(Rodizio *r, double t, double cauda) {
    int v = r->proxima;
    r->proxima = (r->proxima + 1) % r->n;
    if (r->livreAte[v] > t + 1e-9) r->cortes++;
    r->livreAte[v] = t + cauda;
    r->disparos++;
}

/* O som do parry perfeito (cauda de 1,6 s de reverb) toca uma vez por golpe perfeito e duas no golpe
 * duplo. Com as vozes que o audio.c tem, nenhum disparo corta a cauda do anterior em milhares de lutas
 * dos robôs (o tempo é o real: o do núcleo mais o hitstop que congelou o duelo); com as quatro de antes,
 * cortava. */
static void test_vozes(void) {
    CHECK(VOZES_PERFEITO_N == vozes_precisas(SOM_PERFEITO_CAUDA, AJ_CADEIA_MIN, SOM_PERFEITO_DISPAROS_POR_GOLPE) && VOZES_PERFEITO_N <= VOZES_MAX,
          "as vozes do perfeito (%d) são as que a cauda pede e cabem no máximo (%d)", VOZES_PERFEITO_N, VOZES_MAX);
    CHECK(vozes_precisas(0.5f, AJ_CADEIA_MIN, 1) == 2 && vozes_precisas(0.6f, AJ_CADEIA_MIN, 2) == 4 && vozes_precisas(0.22f, AJ_CADEIA_MIN, 3) == 3,
          "as vozes do bom, do erro e do assobio (4) bastam para as caudas curtas deles");
    long cortes8 = 0, cortes4 = 0, disparos = 0, duplos = 0;
    Robo robos[2] = {ROBO_DO_DEMO, ROBO_HUMANO_CASUAL};
    for (int mi = 0; mi < roster_size(); mi++) {
        for (uint32_t seed = 1; seed <= 30; seed++) {
            for (int c = 0; c < 2; c++) {
                Settings s;
                settings_default(&s);
                settings_for_level(&s, mi);
                Duel d;
                duel_init(&d, &s, roster_get(mi), seed);
                RoboMente mente;
                robo_iniciar(&mente, &robos[c], seed);
                Rodizio r8 = {{0}, VOZES_PERFEITO_N, 0, 0, 0}, r4 = {{0}, VOZES_PADRAO, 0, 0, 0};
                double congelado = 0;
                DuelEvent ev[MAX_EVENTS];
                while (d.phase != PH_FINISHED && d.clock < 600) {
                    duel_step_at(&d, 1.0 / 60, robo_aperto_em(&mente, &d, 1.0 / 60));
                    int n = duel_drain(&d, ev, MAX_EVENTS);
                    for (int i = 0; i < n; i++) {
                        if (ev[i].kind != EV_IMPACT) continue;
                        double t = d.lastStrikeAt + congelado;
                        if (ev[i].judgement == J_PERFEITO) {
                            dispara(&r8, t, SOM_PERFEITO_CAUDA); dispara(&r4, t, SOM_PERFEITO_CAUDA);
                            if (ev[i].i & 1) { dispara(&r8, t, SOM_PERFEITO_CAUDA / 1.25); dispara(&r4, t, SOM_PERFEITO_CAUDA / 1.25); duplos++; }
                        }
                        congelado += d.lastHitstop;
                    }
                }
                cortes8 += r8.cortes;
                cortes4 += r4.cortes;
                disparos += r8.disparos;
            }
        }
    }
    CHECK(disparos > 5000 && duplos > 100, "o teste do rodízio cobriu o caso (%ld disparos, %ld golpes duplos perfeitos)", disparos, duplos);
    CHECK(cortes8 == 0, "com %d vozes nenhuma cauda do parry perfeito é cortada (%ld cortes em %ld disparos)", VOZES_PERFEITO_N, cortes8, disparos);
    CHECK(cortes4 > 0, "e o teste enxerga o problema: com as %d vozes de antes cortava (%ld cortes)", VOZES_PADRAO, cortes4);
    printf("vozes do parry perfeito: %ld disparos (%ld de golpe duplo); %d vozes: %ld cortes; as %d de antes: %ld cortes\n", disparos, duplos, VOZES_PERFEITO_N, cortes8, VOZES_PADRAO, cortes4);
}

/* O hitstop no jogo: o duelo congela por lastHitstop segundos de tempo real e o quadro em que o
 * congelamento acaba corre só o que sobra (hitstop_passo). O núcleo desconta o hitstop da preparação
 * seguinte de uma sequência, então o tempo real do contato de cada golpe tem que ser exatamente o
 * tempo do núcleo mais os congelamentos anteriores, em qualquer taxa de quadros. Antes o quadro em que o
 * congelamento acabava era jogado fora e cada golpe alongava a luta em até um quadro (16,7 ms a 60 Hz,
 * 33 ms a 30 Hz), e o erro se acumulava. O laço é o do jogo (update_duel). */
typedef struct { double c0, realDesde; } TrechoQueEsCorre;

static void test_hitstop_exato(void) {
    static const double HZ[] = {30, 60, 120, 144, 240};
    Robo robos[2] = {ROBO_DO_DEMO, ROBO_HUMANO_CASUAL};
    long contatos = 0, perdidosAntes = 0;
    double pior = 0, piorAntigo = 0;
    static TrechoQueEsCorre trechos[70000];
    for (int mi = 0; mi < roster_size(); mi++) {
        for (int h = 0; h < 5; h++) {
            for (uint32_t seed = 1; seed <= 4; seed++) {
                for (int c = 0; c < 2; c++) {
                    Settings s;
                    settings_default(&s);
                    settings_for_level(&s, mi);
                    Duel d;
                    duel_init(&d, &s, roster_get(mi), seed);
                    RoboMente mente;
                    robo_iniciar(&mente, &robos[c], seed);
                    const double dtReal = 1.0 / HZ[h];
                    float congelado = 0;                      /* o G.hitstop do jogo */
                    double real = 0, congeladoTotal = 0, nominalTotal = 0, somaAnteriores = 0;
                    double contatoCore[400], esperado[400];
                    int nc = 0, nt = 0;
                    DuelEvent ev[MAX_EVENTS];
                    while (d.phase != PH_FINISHED && real < 300 && nt < 70000 && nc < 400) {
                        double inicio = real, c0 = d.clock;
                        real += dtReal;
                        float sobra = hitstop_passo(&congelado, (float)dtReal);
                        double parado = dtReal - sobra;
                        congeladoTotal += parado;
                        if (sobra > 0) {
                            trechos[nt++] = (TrechoQueEsCorre){c0, inicio + parado};
                            duel_step_at(&d, sobra, robo_aperto_em(&mente, &d, sobra));
                        }
                        int n = duel_drain(&d, ev, MAX_EVENTS);
                        for (int i = 0; i < n; i++) {
                            if (ev[i].kind != EV_IMPACT) continue;
                            contatoCore[nc] = d.lastStrikeAt;
                            esperado[nc] = d.lastStrikeAt + somaAnteriores;   /* o núcleo mais os congelamentos anteriores */
                            somaAnteriores += d.lastHitstop;
                            nominalTotal += d.lastHitstop;
                            congelado = d.lastHitstop;
                            double antigo = ceil(d.lastHitstop / dtReal - 1e-9) * dtReal - d.lastHitstop;
                            if (antigo > piorAntigo) piorAntigo = antigo;
                            if (antigo > 1e-6) perdidosAntes++;
                            nc++;
                        }
                    }
                    /* o tempo real de cada contato, pelos trechos em que o duelo correu */
                    for (int k = 0; k < nc; k++) {
                        int f = 0;
                        for (int lo = 0, hi = nt - 1; lo <= hi;) {
                            int mid = (lo + hi) / 2;
                            if (trechos[mid].c0 <= contatoCore[k] + 1e-12) { f = mid; lo = mid + 1; } else hi = mid - 1;
                        }
                        double erro = fabs(trechos[f].realDesde + (contatoCore[k] - trechos[f].c0) - esperado[k]);
                        if (erro > pior) pior = erro;
                        contatos++;
                    }
                    CHECK(fabs(congeladoTotal + congelado - nominalTotal) < 1e-3, "%s, %.0f Hz: o congelamento total (%.4f s + %.4f s pendentes) é o que o núcleo descontou (%.4f s)",
                          roster_get(mi)->name, HZ[h], congeladoTotal, congelado, nominalTotal);
                }
            }
        }
    }
    CHECK(contatos > 5000, "o teste do hitstop cobriu as lutas (%ld contatos)", contatos);
    CHECK(pior < 1e-4, "o tempo real de cada contato é o do núcleo mais os congelamentos, em qualquer taxa (pior erro %.6f s)", pior);
    CHECK(piorAntigo > 0.015 && perdidosAntes > 1000, "e o teste enxerga o problema de antes: até %.1f ms perdidos por congelamento (%ld deles)", piorAntigo * 1000, perdidosAntes);
    printf("hitstop: %ld contatos a 30, 60, 120, 144 e 240 Hz, pior erro de tempo real %.4f ms; o jogo de antes perdia até %.1f ms por congelamento (%ld congelamentos)\n",
           contatos, pior * 1000, piorAntigo * 1000, perdidosAntes);
}

/* Um passo grande (uma pausa, uma queda de quadro) não pode pular o aviso: WINDUP, LAUNCH, o brilho e o
 * som do aviso saem, nessa ordem de tempo, antes do PRESS e do IMPACT. Antes, o núcleo começava a
 * preparação numa chamada e só disparava a agenda na seguinte, e um aperto entre as duas julgava o golpe
 * sem lâmina nem aviso (o jogo nunca chegava lá; quem chamava o núcleo com passos grandes, sim). */
typedef struct { int windup, launch, glint, sound, press, impact, n; } OrdemDoGolpe;

static OrdemDoGolpe ordem_do_golpe(Duel *d, int base) {
    OrdemDoGolpe o = {-1, -1, -1, -1, -1, -1, 0};
    DuelEvent ev[MAX_EVENTS];
    int n = duel_drain(d, ev, MAX_EVENTS);
    for (int i = 0; i < n; i++) {
        int k = base + i;
        switch (ev[i].kind) {
            case EV_WINDUP: if (o.windup < 0) o.windup = k; break;
            case EV_LAUNCH: if (o.launch < 0) o.launch = k; break;
            case EV_CUE: if (ev[i].flag) { if (o.sound < 0) o.sound = k; } else if (o.glint < 0) o.glint = k; break;
            case EV_PRESS: if (o.press < 0) o.press = k; break;
            case EV_IMPACT: if (o.impact < 0) o.impact = k; break;
            default: break;
        }
    }
    o.n = n;
    return o;
}

static void test_passo_grande(void) {
    int passos = 0, comAperto = 0;
    for (int mi = 0; mi < roster_size(); mi++) {
        for (uint32_t seed = 1; seed <= 12; seed++) {
            for (int como = 0; como < 4; como++) {
                const MasterProfile *m = roster_get(mi);
                Settings s;
                settings_default(&s);
                settings_for_level(&s, mi);
                s.latency = (como & 1) ? 0.06f : 0;
                Duel d;
                duel_init(&d, &s, m, seed);
                DuelEvent ev[MAX_EVENTS];
                /* deixa o primeiro golpe entrar sem defesa e para na recuperação */
                duel_tick(&d, 1.0);
                while (d.phase != PH_RECOVERY && d.clock < 60) duel_tick(&d, 1.0 / 60);
                duel_drain(&d, ev, MAX_EVENTS);
                if (d.phase != PH_RECOVERY) continue;
                OrdemDoGolpe o;
                if (como < 2) {
                    /* um passo enorme: a preparação seguinte, o aviso e o julgamento acontecem dentro dele */
                    duel_tick(&d, 5.0);
                    o = ordem_do_golpe(&d, 0);
                    CHECK(o.windup >= 0 && o.launch > o.windup && o.glint > o.windup && o.sound > o.windup && (o.impact < 0 || (o.launch < o.impact && o.glint < o.impact && o.sound < o.impact)),
                          "%s, semente %u: passo de 5 s: WINDUP %d, LAUNCH %d, brilho %d, som %d, IMPACT %d", m->name, seed, o.windup, o.launch, o.glint, o.sound, o.impact);
                    passos++;
                } else {
                    /* o passo leva a preparação a uns 10 ms depois do contato, sem julgar, e o aperto vem logo depois */
                    Duel sonda = d;
                    duel_tick(&sonda, d.phaseEnd - d.clock + 1e-9);
                    if (sonda.phase != PH_WINDUP) continue;
                    double ate = sonda.strikeAt + 0.010 - d.clock;
                    duel_tick(&d, ate);
                    if (d.phase != PH_WINDUP) continue;         /* o passo já julgou (chain curta): nada a testar aqui */
                    OrdemDoGolpe a = ordem_do_golpe(&d, 0);
                    bool tentou = duel_press(&d);
                    OrdemDoGolpe b = ordem_do_golpe(&d, a.n);
                    CHECK(a.windup >= 0 && a.launch > a.windup && a.glint > a.windup && a.sound > a.windup,
                          "%s, semente %u: passo até 10 ms depois do contato: WINDUP %d, LAUNCH %d, brilho %d, som %d", m->name, seed, a.windup, a.launch, a.glint, a.sound);
                    CHECK(tentou && b.press >= 0 && b.impact > b.press, "%s, semente %u: o aperto de depois do passo é aceito e julga o golpe (PRESS %d, IMPACT %d)", m->name, seed, b.press, b.impact);
                    comAperto++;
                }
            }
        }
    }
    CHECK(passos > 100 && comAperto > 100, "o teste do passo grande cobriu os casos (%d passos, %d com aperto)", passos, comAperto);
}

/* Traços de cada mestre: o primeiro é de katana e lento, garfiel faz combos longos,
 * karasu e arashi usam as duas lâminas, suiren ataca de longe e jinshi é o mais variado. */
static void test_traits(void) {
    const MasterProfile *daichi = roster_get(0), *genbu = roster_get(1);
    float wd = 0, wg = 0;
    for (int k = 0; k < daichi->moveCount; k++) wd += daichi->moves[k].windup / daichi->moveCount;
    for (int k = 0; k < genbu->moveCount; k++) wg += genbu->moves[k].windup / genbu->moveCount;
    CHECK(wd > wg, "daichi prepara mais devagar que genbu (%.2f x %.2f s)", wd, wg);
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
    CHECK(looks >= 6 && roster_get(11)->moveCount == 12, "jinshi tem o repertório mais variado (%d preparações)", looks);
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
            d.renPosture = s.renPosture;     /* sem gesto, a vida acabaria antes do primeiro golpe duplo que o sorteio traz: o que se mede é cada golpe */
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
    for (int n = 0; n < 200 && !seen && d.phase != PH_FINISHED; n++) {
        d.renPosture = s.renPosture;   /* só mede o tempo da lança: kojiro não cai */
        while (d.phase != PH_WINDUP && d.phase != PH_FINISHED) duel_tick(&d, DT);
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
    test_preparacao_por_golpe();
    test_hitstop_ritmo();
    test_aperto_cedo();
    test_tolerancia_tardia();
    test_calibracao();
    test_cura_em_porcentagem();
    test_curva();
    test_oboro_fases();
    test_vantagem();
    test_lamina_variavel();
    test_taxa_de_quadros();
    test_florete();
    test_cada_golpe();
    test_rastro_fantasma();
    test_calibracao_alta();
    test_traco_aleatorio();
    test_ritmo();
    test_robos_deslocados();
    test_teste_a_mao();
    test_timeline();
    test_movesets();
    test_traits();
    test_dual();
    test_far_lead();
    test_burn();
    test_levels();
    test_special();
    test_campaign();
    test_campaign_win();
    test_passo_grande();
    test_vozes();
    test_hitstop_exato();
    printf("%d verificações, %d falhas\n", checks, failures);
    return failures ? 1 : 0;
}
