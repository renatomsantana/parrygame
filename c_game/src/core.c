/*
 * core.c - relógio, tentativa, fintas, julgamento, postura, selos e trilha.
 * Sem vida: Ren e o mestre só têm postura. Quem chega a zero cai.
 */
#include "core.h"

#include <math.h>
#include <string.h>

void settings_default(Settings *s) {
    s->renPosture = 250;
    s->badPostureDamage = 25;
    s->badBossRecover = 20;
    s->goodBossDamage = 6;
    s->goodRenCost = 4;
    s->perfectBossDamage = 20;
    s->perfectRenRecover = 20;
    s->inputCooldown = 0.300f;
    s->attackLead = 0.220f;
    s->cueLead = 0.180f;
    s->recovery = 0.800f;
    s->sealRecovery = 2.200f;
    s->sealRenRecover = 75;
    s->firstWindupDelay = 0.650f;
    s->pressureSpeed = 0.90f;
    s->feintDelayMin = 0.200f;
    s->feintDelayMax = 0.400f;
    s->comboGap = 0.080f;
    s->minChainGap = 0.400f;
    s->goodHitstop = 0.045f;
    s->perfectHitstop = 0.090f;
    s->badHitstop = 0.045f;
    s->breakHitstop = 0.160f;
    s->postureGrowth = 25;
    s->perfectGrowth = 2;
    s->goodGrowth = 0.5f;
}

void settings_for_level(Settings *s, int defeated) {
    if (defeated < 0) defeated = 0;
    s->renPosture += s->postureGrowth * defeated;
    s->perfectBossDamage += s->perfectGrowth * defeated;
    s->goodBossDamage += s->goodGrowth * defeated;
}

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
    d->fakeCount = 0;
    d->windupDuration = 1;
    d->isFeint = false;
    d->blackout = false;
    d->special = false;
    d->lastPress = -100;
    d->attempted = d->attackLaunched = d->feintLaunched = false;
    d->cuePlayed = d->fakeCuePlayed = false;
    d->renPosture = d->s.renPosture;
    d->bossPosture = d->m->posture;
    d->seal = 0;
    d->stanceIndex = 0;
    d->comboRemaining = d->comboStrike = 0;
    d->move = 0;
    d->sequences = 0;
    d->attacks = d->feints = d->perfects = d->goods = d->bads = 0;
    d->scheduleCount = d->scheduleIndex = 0;
    d->eventCount = 0;
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
    float base = d->m->hitsToFall > 0 ? d->s.renPosture / d->m->hitsToFall : d->s.badPostureDamage;
    if (d->m->damage > 0) base *= d->m->damage;
    return d->special ? base * 2 : base;
}

bool duel_strike_dual(const Duel *d) {
    const Move *mv = duel_move(d);
    return mv && d->comboStrike < 32 && (mv->dual >> d->comboStrike) & 1u;
}

float duel_strike_lead(const Duel *d) {
    const Move *mv = duel_move(d);
    if (mv && mv->look == LOOK_FAR && d->comboStrike == 0) return d->s.attackLead * FAR_LEAD;
    return d->s.attackLead;
}

bool duel_in_combo(const Duel *d) { return d->comboRemaining > 0 || d->comboStrike > 0; }

double duel_time_to_impact(const Duel *d) {
    if (d->phase != PH_WINDUP) return -1;
    double t = d->strikeAt - d->clock;
    return t > 0 ? t : 0;
}

double duel_time_to_next_instant(const Duel *d) {
    if (d->phase != PH_WINDUP) return -1;
    double next = d->strikeAt;
    for (int i = 0; i < d->fakeCount; i++) {
        double t = d->fakeStrikeAts[i];
        if (t >= d->clock && t < next) next = t;
    }
    double t = next - d->clock;
    return t > 0 ? t : 0;
}

static void push_schedule(Duel *d, double time, ScheduleKind kind) {
    ScheduleItem *it = &d->schedule[d->scheduleCount++];
    it->time = time;
    it->kind = kind;
}

static void build_schedule(Duel *d) {
    d->scheduleCount = 0;
    d->scheduleIndex = 0;
    for (int i = 0; i < d->fakeCount; i++) {
        push_schedule(d, d->fakeStrikeAts[i] - d->s.attackLead, SCH_FAKE_LAUNCH);
        push_schedule(d, d->fakeStrikeAts[i] - d->s.cueLead, SCH_FAKE_CUE);
    }
    push_schedule(d, d->strikeAt - duel_strike_lead(d), SCH_LAUNCH);
    push_schedule(d, d->strikeAt - d->s.cueLead, SCH_CUE);
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
    d->attempted = d->attackLaunched = d->feintLaunched = false;
    d->cuePlayed = d->fakeCuePlayed = false;
    /* Cada golpe novo zera a espera entre gestos: um gesto feito no intervalo
     * não pode roubar a defesa do golpe que está chegando. */
    d->lastPress = -100;

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
        /* O próximo contato chega exatamente `gap` segundos depois do anterior. */
        double gap = mv ? mv->gaps[d->comboStrike - 1] : s->minChainGap;
        if (gap < s->minChainGap) gap = s->minChainGap;
        duration = gap - s->comboGap;
    } else {
        duration = st->windups[d->sequences % st->windupCount];
        if (m->accelSteps > 1) duration *= pow(m->accelFactor, d->sequences % m->accelSteps);
        if (m->rhythmJitter > 0) duration += (rng_next(&d->rng) * 2 - 1) * m->rhythmJitter;
    }
    /* A pressa acelera só a preparação; o ritmo dentro de uma sequência nunca muda,
     * para que dê para decorar. */
    if (!continuing) {
        duration *= rule->speedMultiplier;
        if (m->sealCount <= 1 && duel_under_pressure(d)) duration *= s->pressureSpeed;
        /* a estocada de longe ganha o tempo extra da ponta viajando: a preparação não encolhe */
        duration += duel_strike_lead(d) - s->attackLead;
    }
    if (duration < duel_strike_lead(d) + 0.1) duration = duel_strike_lead(d) + 0.1;
    d->windupDuration = (float)duration;

    double delay = 0;
    int cues = 0;
    d->isFeint = false;
    if (!continuing) {
        d->blackout = false;
        double roll = rng_next(&d->rng);
        d->isFeint = st->feintChance > 0 && st->falseCues > 0 && roll < st->feintChance;
        if (d->isFeint) {
            double lo = st->feintDelayMax > 0 ? st->feintDelayMin : s->feintDelayMin;
            double hi = st->feintDelayMax > 0 ? st->feintDelayMax : s->feintDelayMax;
            delay = lo + rng_next(&d->rng) * (hi - lo);
            cues = st->falseCues > MAX_FALSE_CUES ? MAX_FALSE_CUES : st->falseCues;
            d->feints++;
        }
        if (m->blackoutChance > 0) d->blackout = rng_next(&d->rng) < m->blackoutChance;
        /* O especial vale para a sequência inteira. */
        d->special = m->specialChance > 0 && rng_next(&d->rng) < m->specialChance;
        if (d->special) emit(d, EV_SPECIAL, J_NONE, 0, 0, false);
        d->sequences++;
    }

    double first = d->clock + d->windupDuration;
    d->strikeAt = first + delay;
    d->fakeCount = cues;
    for (int i = 0; i < cues; i++) d->fakeStrikeAts[i] = first + delay * i / cues;
    d->attacks++;
    build_schedule(d);
    emit(d, EV_WINDUP, J_NONE, d->windupDuration, 0, d->isFeint);
}

static void fire(Duel *d, ScheduleKind kind) {
    switch (kind) {
        case SCH_FAKE_LAUNCH: d->feintLaunched = true; emit(d, EV_FEINT_LAUNCH, J_NONE, 0, 0, false); break;
        case SCH_FAKE_CUE: d->fakeCuePlayed = true; emit(d, EV_CUE, J_NONE, 0, 0, true); break;
        case SCH_LAUNCH: d->attackLaunched = true; emit(d, EV_LAUNCH, J_NONE, 0, 0, false); break;
        case SCH_CUE: d->cuePlayed = true; emit(d, EV_CUE, J_NONE, 0, 0, false); break;
    }
}

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static void resolve(Duel *d) {
    const Settings *s = &d->s;
    /* Consumir o golpe antes dos eventos impede julgamento duplicado. */
    d->phase = PH_RECOVERY;
    d->phaseEnd = d->clock + (d->comboRemaining > 0 ? s->comboGap : s->recovery);
    const Stance *st = duel_stance(d);
    double lead = d->attempted ? d->strikeAt - d->lastPress : -1;
    bool dual = duel_strike_dual(d), second = false;
    Judgement j;
    if (d->attempted && lead >= 0 && lead <= st->perfectWindow + 1e-6) {
        j = J_PERFEITO;
        d->perfects++;
        /* o parry perfeito apaga as brasas */
        if (d->burnLeft > 0) {
            d->burnLeft = 0;
            emit(d, EV_BURN, J_NONE, 0, 0, false);
        }
        d->bossPosture -= s->perfectBossDamage;
        d->renPosture = clampf(d->renPosture + s->perfectRenRecover, 0, s->renPosture);
    } else if (d->attempted && lead >= 0 && lead <= st->goodWindow + 1e-6) {
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
            d->burnLeft = BURN_TIME;
            d->burnRate = duel_ren_damage(d) * d->m->burn / BURN_TIME;
            emit(d, EV_BURN, J_NONE, 0, 0, true);
        }
        if (d->m->healsOnHit) d->bossPosture = clampf(d->bossPosture + s->badBossRecover, 0, d->m->posture);
    }

    bool broke = d->bossPosture <= 0;
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
    if (d->clock >= d->strikeAt) resolve(d);
}

bool duel_press(Duel *d) {
    if (d->phase == PH_FINISHED) return false;
    if (d->clock - d->lastPress < d->s.inputCooldown) return false;
    if (d->phase == PH_WINDUP && d->attempted) return false;
    d->lastPress = d->clock;
    if (d->phase == PH_WINDUP) d->attempted = true;
    emit(d, EV_PRESS, J_NONE, 0, 0, false);
    return true;
}

/* ------------------------------------------------------------------ */

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
