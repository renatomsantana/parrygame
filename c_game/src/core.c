/*
 * core.c - relógio, tentativa, julgamento, vida, postura, selos e trilha.
 * Kojiro tem vida; o mestre, postura. Quem chega a zero cai.
 */
#include "core.h"

#include <math.h>
#include <string.h>

/* ------------------------------------------------------------------ */

void rng_seed(Rng *r, uint32_t seed) { r->state = seed ? seed : 1; }

double rng_next(Rng *r) {
    uint32_t t = (r->state += 0x6D2B79F5u);
    t = (t ^ (t >> 15)) * (t | 1u);
    t ^= t + (t ^ (t >> 7)) * (t | 61u);
    return (double)(t ^ (t >> 14)) / 4294967296.0;
}

/* ------------------------------------------------------------------ */

static void emit(Duel *d, EventKind kind, Judgement j, float a, int i, bool flag) {
    if (d->eventCount >= MAX_EVENTS) return;
    DuelEvent *e = &d->events[d->eventCount++];
    e->kind = kind;
    e->judgement = j;
    e->a = a;
    e->i = i;
    e->flag = flag;
}

int duel_drain(Duel *d, DuelEvent *out, int max) {
    int n = d->eventCount < max ? d->eventCount : max;
    memcpy(out, d->events, sizeof(DuelEvent) * (size_t)n);
    d->eventCount = 0;
    return n;
}

void duel_init(Duel *d, const Settings *s, const MasterProfile *m, uint32_t seed) {
    memset(d, 0, sizeof *d);
    d->s = *s;
    d->m = m;
    rng_seed(&d->rng, seed);
    d->commonSeal.name = "";
    d->commonSeal.speedMultiplier = 1;
    d->commonSeal.stanceSwitchEvery = 0;
    duel_reset(d);
}

void duel_reset(Duel *d) {
    d->clock = 0;
    d->phase = PH_READY;
    d->phaseEnd = d->s.firstWindupDelay;
    d->strikeAt = 0;
    d->windupDuration = 1;
    d->blackout = false;
    d->special = false;
    d->lastPress = -100;
    d->pressBlockedUntil = -100;
    d->attempted = d->attackLaunched = d->cuePlayed = false;
    d->renPosture = d->s.renPosture;
    d->bossPosture = d->m->posture;
    d->seal = 0;
    d->stanceIndex = 0;
    d->comboRemaining = d->comboStrike = 0;
    d->move = 0;
    d->sequences = 0;
    d->attacks = d->perfects = d->goods = d->bads = 0;
    d->scheduleCount = d->scheduleIndex = 0;
    d->eventCount = 0;
    d->lastStrikeAt = -1;
    d->lastJudgement = J_NONE;
    d->lastLead = -1;
    d->lastAttempted = false;
    d->lastGestureAt = -100;
    d->lastHitstop = 0;
}

const Stance *duel_stance(const Duel *d) {
    int n = d->m->stanceCount;
    int i = d->stanceIndex < n ? d->stanceIndex : n - 1;
    return &d->m->stances[i < 0 ? 0 : i];
}

/* Mestres comuns não declaram selos: o golpe composto vem do próprio perfil. */
const SealRule *duel_seal_rule(const Duel *d) {
    if (d->m->sealCount > 0) return &d->m->seals[d->seal < d->m->sealCount ? d->seal : d->m->sealCount - 1];
    return &d->commonSeal;
}

static int seal_total(const Duel *d) { return d->m->sealCount > 0 ? d->m->sealCount : 1; }

bool duel_under_pressure(const Duel *d) { return d->bossPosture <= d->m->posture / 2; }
float duel_ren_damage(const Duel *d) {
    float base = d->s.renPosture / (d->m->hitsToFall > 1 ? d->m->hitsToFall : 1);
    if (d->m->damage > 0) base *= d->m->damage;
    return d->special ? base * 2 : base;
}

bool duel_strike_dual(const Duel *d) {
    const Move *mv = duel_move(d);
    return mv && d->comboStrike < 32 && (mv->dual >> d->comboStrike) & 1u;
}

float duel_strike_lead(const Duel *d) {
    const Move *mv = duel_move(d);
    if (mv && mv->look == LOOK_FAR && d->comboStrike == 0) return d->s.attackLead * AJ_LANCA_PARTE_X;
    return d->s.attackLead;
}

/* O aviso do primeiro golpe vem sempre o mesmo tempo antes do contato (o da postura);
 * na lança, o tempo a mais da ponta viajando também, para o aviso vir antes da partida.
 * Na sequência, o aviso é o contato anterior: o brilho sai quando a preparação começa. */
float duel_aviso(const Duel *d) {
    if (d->comboStrike > 0) return d->windupDuration;
    return duel_stance(d)->aviso + (duel_strike_lead(d) - d->s.attackLead);
}

double duel_cue_time(const Duel *d) {
    double t = d->strikeAt - duel_aviso(d), start = d->strikeAt - d->windupDuration;
    return t < start ? start : t;
}

bool duel_in_combo(const Duel *d) { return d->comboRemaining > 0 || d->comboStrike > 0; }

double duel_time_to_impact(const Duel *d) {
    if (d->phase != PH_WINDUP) return -1;
    double t = d->strikeAt - d->clock;
    return t > 0 ? t : 0;
}

DuelTimeline duel_timeline(const Duel *d) {
    DuelTimeline t;
    memset(&t, 0, sizeof t);
    t.now = d->clock;
    if (d->phase != PH_WINDUP) return t;
    const Stance *st = duel_stance(d);
    t.active = true;
    t.strike = d->strikeAt;
    t.start = d->strikeAt - d->windupDuration;
    t.launch = d->strikeAt - duel_strike_lead(d);
    t.cue = duel_cue_time(d);
    t.perfectFrom = d->strikeAt - st->perfectWindow;
    t.goodFrom = d->strikeAt - st->goodWindow;
    return t;
}

static void push_schedule(Duel *d, double time, ScheduleKind kind) {
    ScheduleItem *it = &d->schedule[d->scheduleCount++];
    it->time = time;
    it->kind = kind;
}

static void build_schedule(Duel *d) {
    d->scheduleCount = 0;
    d->scheduleIndex = 0;
    push_schedule(d, d->strikeAt - duel_strike_lead(d), SCH_LAUNCH);
    push_schedule(d, duel_cue_time(d), SCH_CUE);
    /* o som do aviso adiantado (ou atrasado) para chegar junto com o brilho */
    double som = duel_cue_time(d) - d->s.audioLead, start = d->strikeAt - d->windupDuration;
    push_schedule(d, som < start ? start : (som > d->strikeAt ? d->strikeAt : som), SCH_CUE_SOUND);
    /* Ordenação estável por tempo (inserção: a lista é curta). */
    for (int i = 1; i < d->scheduleCount; i++) {
        ScheduleItem x = d->schedule[i];
        int j = i - 1;
        while (j >= 0 && d->schedule[j].time > x.time) { d->schedule[j + 1] = d->schedule[j]; j--; }
        d->schedule[j + 1] = x;
    }
}

/* Sorteia a próxima sequência entre as permitidas na postura e no selo atuais. */
static int pick_move(Duel *d) {
    const MasterProfile *m = d->m;
    float total = 0;
    for (int i = 0; i < m->moveCount; i++) {
        const Move *mv = &m->moves[i];
        if ((mv->stance < 0 || mv->stance == d->stanceIndex) && mv->minSeal <= d->seal) total += mv->weight;
    }
    if (total <= 0) return -1;
    double r = rng_next(&d->rng) * total;
    int last = -1;
    for (int i = 0; i < m->moveCount; i++) {
        const Move *mv = &m->moves[i];
        if (!((mv->stance < 0 || mv->stance == d->stanceIndex) && mv->minSeal <= d->seal)) continue;
        last = i;
        if (r < mv->weight) return i;
        r -= mv->weight;
    }
    return last;
}

const Move *duel_move(const Duel *d) {
    if (d->m->moveCount <= 0 || d->move < 0) return NULL;
    return &d->m->moves[d->move];
}

static void begin_attack(Duel *d) {
    const Settings *s = &d->s;
    const MasterProfile *m = d->m;
    d->phase = PH_WINDUP;
    d->attempted = d->attackLaunched = d->cuePlayed = false;
    /* Cada golpe novo zera a espera entre gestos: um gesto feito no intervalo
     * não pode roubar a defesa do golpe que está chegando. */
    d->lastPress = -100;
    d->pressBlockedUntil = -100;
    d->earlyUsed = false;

    bool continuing = d->comboRemaining > 0;
    if (continuing) { d->comboRemaining--; d->comboStrike++; }
    else d->comboStrike = 0;

    const SealRule *rule = duel_seal_rule(d);
    if (!continuing && rule->stanceSwitchEvery > 0 && m->stanceCount > 1 &&
        d->sequences > 0 && d->sequences % rule->stanceSwitchEvery == 0) {
        d->stanceIndex = (d->stanceIndex + 1) % m->stanceCount;
        emit(d, EV_STANCE, J_NONE, 0, d->stanceIndex, false);
    }
    const Stance *st = duel_stance(d);

    if (!continuing) {
        d->move = pick_move(d);
        const Move *mv = duel_move(d);
        int strikes = mv ? (mv->strikes < 1 ? 1 : (mv->strikes > MAX_CHAIN ? MAX_CHAIN : mv->strikes)) : 1;
        d->comboRemaining = strikes - 1;
        if (strikes > 1) emit(d, EV_COMBO, J_NONE, 0, strikes, false);
    }
    const Move *mv = duel_move(d);

    double duration;
    if (continuing) {
        /* O próximo contato chega exatamente `gap` segundos (de tempo real) depois do
         * anterior: o hitstop do impacto anterior congelou o duelo e sai daqui. */
        double gap = mv ? mv->gaps[d->comboStrike - 1] : s->minChainGap;
        if (gap < s->minChainGap) gap = s->minChainGap;
        duration = d->lastStrikeAt + gap - d->lastHitstop - d->clock;
        if (duration < duel_strike_lead(d) + AJ_PREPARO_MIN_CADEIA) duration = duel_strike_lead(d) + AJ_PREPARO_MIN_CADEIA;
    } else {
        /* A preparação da sequência: do aviso ao contato é sempre o mesmo tempo (o aviso
         * da postura; na lança, mais o tempo da ponta viajando). A pressa, a aceleração,
         * o selo e o traço aleatório mudam só a espera antes do aviso; o ritmo dentro de
         * uma sequência nunca muda. Tudo para que dê para decorar. */
        double base = mv && mv->windup > 0 ? mv->windup : st->aviso + 0.5;
        double antes = base - st->aviso;
        if (m->accelSteps > 1) antes *= pow(m->accelFactor, d->sequences % m->accelSteps);
        antes *= rule->speedMultiplier;
        if (m->sealCount <= 1 && duel_under_pressure(d)) antes *= s->pressureSpeed;
        if (m->rhythmJitter > 0) antes += (rng_next(&d->rng) * 2 - 1) * m->rhythmJitter;
        if (antes < AJ_PREPARO_ANTES_DO_AVISO) antes = AJ_PREPARO_ANTES_DO_AVISO;
        duration = duel_aviso(d) + antes;
    }
    if (!continuing && duration < duel_strike_lead(d) + 0.1) duration = duel_strike_lead(d) + 0.1;
    d->windupDuration = (float)duration;

    if (!continuing) {
        d->blackout = false;
        if (m->blackoutChance > 0) d->blackout = rng_next(&d->rng) < m->blackoutChance;
        /* O especial vale para a sequência inteira. */
        d->special = m->specialChance > 0 && rng_next(&d->rng) < m->specialChance;
        if (d->special) emit(d, EV_SPECIAL, J_NONE, 0, 0, false);
        d->sequences++;
    }

    d->strikeAt = d->clock + d->windupDuration;
    d->attacks++;
    build_schedule(d);
    emit(d, EV_WINDUP, J_NONE, d->windupDuration, 0, false);
}

static void fire(Duel *d, ScheduleKind kind) {
    switch (kind) {
        case SCH_LAUNCH: d->attackLaunched = true; emit(d, EV_LAUNCH, J_NONE, 0, 0, false); break;
        case SCH_CUE: d->cuePlayed = true; emit(d, EV_CUE, J_NONE, 0, d->comboStrike, false); break;
        case SCH_CUE_SOUND: emit(d, EV_CUE, J_NONE, 0, d->comboStrike, true); break;
    }
}

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

float duel_hitstop_for(const Settings *s, Judgement j, bool broke, bool secondBlade) {
    float h = j == J_PERFEITO ? (broke ? s->breakHitstop : s->perfectHitstop) : j == J_BOM ? s->goodHitstop : s->badHitstop;
    if (secondBlade && h < s->badHitstop) h = s->badHitstop;
    return h;
}

static void resolve(Duel *d) {
    const Settings *s = &d->s;
    /* Consumir o golpe antes dos eventos impede julgamento duplicado. */
    d->phase = PH_RECOVERY;
    d->phaseEnd = d->clock + (d->comboRemaining > 0 ? s->comboGap : s->recovery);
    const Stance *st = duel_stance(d);
    /* o aperto conta `latency` mais cedo: o que o jogador viu e ouviu chegou atrasado */
    double lead = d->attempted ? d->strikeAt - (d->lastPress - s->latency) : -1;
    bool dual = duel_strike_dual(d), second = false;
    Judgement j;
    if (d->attempted && lead >= 0 && lead <= st->perfectWindow + 1e-6 && !d->earlyUsed) {
        j = J_PERFEITO;
        d->perfects++;
        /* o parry perfeito apaga as brasas */
        if (d->burnLeft > 0) {
            d->burnLeft = 0;
            emit(d, EV_BURN, J_NONE, 0, 0, false);
        }
        d->bossPosture -= s->perfectBossDamage;
        d->renPosture = clampf(d->renPosture + s->perfectRenRecover, 0, s->renPosture);
    } else if (d->attempted && lead >= -s->lateGrace - 1e-6 && lead <= st->goodWindow + 1e-6) {
        j = J_BOM;
        d->goods++;
        d->bossPosture -= s->goodBossDamage;
        d->renPosture = clampf(d->renPosture - s->goodRenCost, 0, s->renPosture);
        /* duas lâminas: o bom apara uma, a outra entra */
        if (dual) {
            d->renPosture = clampf(d->renPosture - duel_ren_damage(d), 0, s->renPosture);
            second = true;
        }
    } else {
        j = J_RUIM;
        d->bads++;
        d->renPosture = clampf(d->renPosture - duel_ren_damage(d) * (dual ? 2 : 1), 0, s->renPosture);
        second = dual;
        if (d->m->burn > 0) {
            /* a lâmina de fogo deixa kojiro em brasas (um erro novo reacende) */
            d->burnLeft = AJ_BRASAS_TEMPO;
            d->burnRate = duel_ren_damage(d) * d->m->burn / AJ_BRASAS_TEMPO;
            emit(d, EV_BURN, J_NONE, 0, 0, true);
        }
        if (d->m->healsOnHit) d->bossPosture = clampf(d->bossPosture + s->badBossRecover, 0, d->m->posture);
    }

    d->lastStrikeAt = d->strikeAt;
    d->lastJudgement = j;
    d->lastLead = lead;
    d->lastAttempted = d->attempted;

    bool broke = d->bossPosture <= 0;
    d->lastHitstop = duel_hitstop_for(s, j, broke, second);
    /* na sequência, a pausa entre golpes conta do contato (o julgamento pode vir depois,
     * na tolerância tardia) e já passa congelada no hitstop */
    if (d->comboRemaining > 0 && !broke) {
        double pausa = s->comboGap - d->lastHitstop;
        double fim = d->strikeAt + (pausa > 0 ? pausa : 0);
        d->phaseEnd = fim > d->clock ? fim : d->clock;
    }
    if (broke) {
        d->bossPosture = 0;
        d->comboRemaining = 0; /* a quebra interrompe o composto */
    }
    /* i: bit 0 = golpe de duas lâminas, bit 1 = a segunda lâmina acertou kojiro */
    emit(d, EV_IMPACT, j, (float)lead, (dual ? 1 : 0) | (second ? 2 : 0), broke);

    if (broke) {
        if (d->seal + 1 < seal_total(d)) {
            d->seal++;
            d->bossPosture = d->m->posture;
            d->renPosture = clampf(d->renPosture + s->sealRenRecover, 0, s->renPosture);
            d->phaseEnd = d->clock + s->sealRecovery;
            emit(d, EV_SEAL, J_NONE, 0, d->seal, false);
            /* uma postura por selo: o selo novo traz a dele */
            if (d->m->stanceCount > 1 && d->seal < d->m->stanceCount) {
                d->stanceIndex = d->seal;
                emit(d, EV_STANCE, J_NONE, 0, d->stanceIndex, false);
            }
        } else {
            d->phase = PH_FINISHED;
            emit(d, EV_FINISHED, J_NONE, 0, 0, true);
        }
        return;
    }
    if (d->renPosture <= 0.001f) {
        d->renPosture = 0;
        d->phase = PH_FINISHED;
        d->comboRemaining = 0;
        emit(d, EV_FINISHED, J_NONE, 0, 0, false);
    }
}

void duel_tick(Duel *d, double delta) {
    if (d->phase == PH_FINISHED) return;
    d->clock += delta > 0 ? delta : 0;
    if (d->burnLeft > 0 && delta > 0) {
        float t = (float)delta < d->burnLeft ? (float)delta : d->burnLeft;
        d->burnLeft -= t;
        d->renPosture = clampf(d->renPosture - d->burnRate * t, 0, d->s.renPosture);
        if (d->renPosture <= 0.001f) {
            /* caiu queimando */
            d->renPosture = 0;
            d->burnLeft = 0;
            d->phase = PH_FINISHED;
            d->comboRemaining = 0;
            emit(d, EV_FINISHED, J_NONE, 0, 0, false);
            return;
        }
        if (d->burnLeft <= 0) emit(d, EV_BURN, J_NONE, 0, 0, false);
    }
    if (d->phase == PH_READY || d->phase == PH_RECOVERY) {
        if (d->clock >= d->phaseEnd) begin_attack(d);
        return;
    }
    /* Os eventos disparam na ordem do tempo, mesmo num passo grande. */
    while (d->scheduleIndex < d->scheduleCount && d->clock >= d->schedule[d->scheduleIndex].time) {
        fire(d, d->schedule[d->scheduleIndex].kind);
        d->scheduleIndex++;
    }
    /* com defesa, julga no contato; sem, espera a tolerância tardia (e o atraso calibrado) */
    if (d->clock >= d->strikeAt && (d->attempted || d->clock >= d->strikeAt + d->s.lateGrace + d->s.latency)) resolve(d);
}

void duel_step(Duel *d, double dt, bool press) {
    if (!press) {
        duel_tick(d, dt);
        return;
    }
    duel_tick(d, dt * 0.5);
    duel_press(d);
    duel_tick(d, dt * 0.5);
}

bool duel_press(Duel *d) {
    if (d->phase == PH_FINISHED) return false;
    if (d->clock < d->pressBlockedUntil - 1e-9) return false;
    if (d->phase == PH_WINDUP && d->attempted) return false;
    PressKind kind;
    if (d->phase == PH_WINDUP) {
        /* o jogador vê o aviso `latency` depois: é esse o aviso que conta para ele */
        double aviso = duel_cue_time(d) + d->s.latency;
        if (d->clock < aviso - 1e-9) {
            /* antes do aviso: não trava o golpe; a recarga acaba no aviso, no máximo (a janela
             * boa vem sempre depois dele). O custo: a defesa deste golpe não sai perfeita. */
            double fim = d->clock + AJ_RECARGA_CEDO;
            d->pressBlockedUntil = fim < aviso ? fim : aviso;
            d->earlyUsed = true;
            kind = PRESS_CEDO;
        } else {
            d->lastPress = d->clock;
            d->attempted = true;
            kind = PRESS_TENTATIVA;
            /* dentro da tolerância tardia: o contato já passou, julga agora */
            if (d->clock >= d->strikeAt) {
                emit(d, EV_PRESS, J_NONE, 0, kind, false);
                resolve(d);
                return true;
            }
        }
    } else {
        d->lastGestureAt = d->clock;
        d->pressBlockedUntil = d->clock + d->s.inputCooldown;
        bool tarde = d->lastJudgement == J_RUIM && !d->lastAttempted && d->clock - d->lastStrikeAt <= AJ_TARDE_JANELA;
        kind = tarde ? PRESS_TARDE : PRESS_GESTO;
    }
    emit(d, EV_PRESS, J_NONE, 0, kind, false);
    return true;
}

/* ------------------------------------------------------------------ */

float calibration_result(const float *offsets, int n) {
    float ok[64];
    int k = 0;
    for (int i = 0; i < n && k < 64; i++)
        if (offsets[i] > -AJ_CALIBRA_ACEITA && offsets[i] < AJ_CALIBRA_ACEITA) ok[k++] = offsets[i];
    if (k == 0 || k * 2 < n) return -1;
    for (int i = 1; i < k; i++) {
        float x = ok[i];
        int j = i - 1;
        while (j >= 0 && ok[j] > x) { ok[j + 1] = ok[j]; j--; }
        ok[j + 1] = x;
    }
    float med = k % 2 ? ok[k / 2] : (ok[k / 2 - 1] + ok[k / 2]) / 2;
    return clampf(med, 0, AJ_LATENCIA_MAX);
}

void campaign_reset(Campaign *c) {
    c->index = 0;
    c->clearedMask = 0;
    c->completed = false;
    c->loreSeen = false;
}

void campaign_mark_cleared(Campaign *c, int index) {
    if (index >= 0 && index < ROSTER_SIZE) c->clearedMask |= (1u << index);
}

bool campaign_is_cleared(const Campaign *c, int index) {
    return index >= 0 && index < ROSTER_SIZE && (c->clearedMask & (1u << index)) != 0;
}

int campaign_defeated(const Campaign *c) {
    int n = 0;
    for (int i = 0; i < MASTER_COUNT; i++) if (campaign_is_cleared(c, i)) n++;
    return n;
}

bool campaign_big_boss_open(const Campaign *c) { return campaign_defeated(c) == MASTER_COUNT; }

bool campaign_advance(Campaign *c) {
    if (c->index + 1 < ROSTER_SIZE) { c->index++; return true; }
    c->completed = true;
    return false;
}
