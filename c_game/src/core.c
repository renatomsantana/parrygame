/*
 * core.c - relógio, tentativa, julgamento, vida, postura, selos e trilha.
 * Kojiro tem vida; o mestre, postura. Quem chega a zero cai.
 */
#include "core.h"

#include <limits.h>
#include <math.h>
#include <string.h>

/* ------------------------------------------------------------------ */

/* Mulberry32: 32 bits de estado, a mesma sequência em qualquer máquina (as lutas dos testes e dos
 * robôs dependem disso). As constantes são as do algoritmo. */
void rng_seed(Rng *r, uint32_t seed) { r->state = seed ? seed : 1; }

double rng_next(Rng *r) {
    uint32_t t = (r->state += 0x6D2B79F5u);                /* num-ok: algoritmo */
    t = (t ^ (t >> 15)) * (t | 1u);                        /* num-ok: algoritmo */
    t ^= t + (t ^ (t >> 7)) * (t | 61u);                   /* num-ok: algoritmo */
    return (double)(t ^ (t >> 14)) / 4294967296.0;         /* num-ok: algoritmo (2^32) */
}

/* ------------------------------------------------------------------ */

static DuelEvent *emit(Duel *d, EventKind kind, Judgement j, float a, int i, bool flag) {
    if (d->eventCount >= MAX_EVENTS) return NULL;
    DuelEvent *e = &d->events[d->eventCount++];
    e->kind = kind;
    e->judgement = j;
    e->a = a;
    e->b = 0;
    e->i = i;
    e->flag = flag;
    return e;
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
    rng_seed(&d->bladeRng, seed ^ 0xA5F00D5Eu);
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
    d->strikeLead = 0;
    d->blackout = false;
    d->special = false;
    d->lastPress = AJ_NUNCA;
    d->pressBlockedUntil = AJ_NUNCA;
    d->attempted = false;
    d->renPosture = d->s.renPosture;
    d->shock = 0;
    d->seal = 0;
    d->bossPosture = duel_posture_max(d);
    memset(d->stanceSequences, 0, sizeof d->stanceSequences);
    d->advantage = false;
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
    d->lastGestureAt = AJ_NUNCA;
    d->lastHitstop = 0;
}

void duel_start_seal(Duel *d, int seal) {
    duel_reset(d);
    int n = d->m->sealCount > 0 ? d->m->sealCount : 1;
    d->seal = seal < 0 ? 0 : seal >= n ? n - 1 : seal;
    d->bossPosture = duel_posture_max(d);
    /* uma postura por selo, como quando o selo quebra */
    if (d->m->stanceCount > 1 && d->seal < d->m->stanceCount) d->stanceIndex = d->seal;
}

void duel_refill(Duel *d, bool vida, bool postura) {
    if (d->phase == PH_FINISHED) return;
    if (vida) {
        d->renPosture = d->s.renPosture;
        if (d->burnLeft > 0) {
            d->burnLeft = 0;
            emit(d, EV_BURN, J_NONE, 0, 0, false);
        }
    }
    if (postura) d->bossPosture = duel_posture_max(d);
    bool vantagem = duel_advantage(d);
    if (vantagem != d->advantage) {
        d->advantage = vantagem;
        emit(d, EV_ADVANTAGE, J_NONE, 0, 0, vantagem);
    }
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

float duel_posture_max(const Duel *d) {
    const SealRule *r = duel_seal_rule(d);
    return r->posture > 0 ? r->posture : d->m->posture;
}

bool duel_under_pressure(const Duel *d) { return d->bossPosture <= duel_posture_max(d) / 2; }

bool duel_advantage(const Duel *d) {
    return d->phase != PH_FINISHED && d->bossPosture > 0 && d->bossPosture <= d->s.perfectBossDamage + AJ_EPS_PERFEITO;
}
float duel_ren_damage(const Duel *d) {
    float base = d->s.renPosture / (d->m->hitsToFall > 1 ? d->m->hitsToFall : 1);
    if (d->m->damage > 0) base *= d->m->damage;
    const SealRule *r = duel_seal_rule(d);
    if (r->damageMultiplier > 0) base *= r->damageMultiplier;
    return d->special ? base * 2 : base;
}

bool duel_strike_dual(const Duel *d) {
    const Move *mv = duel_move(d);
    return mv && d->comboStrike < (int)(CHAR_BIT * sizeof mv->dual) && (mv->dual >> d->comboStrike) & 1u;
}

MoveLook move_contact_look(const Move *mv, int strike) {
    MoveLook look = mv ? mv->look : LOOK_HIGH;
    if (strike <= 0) return look;
    if (mv && mv->thrustOnly) return LOOK_THRUST;
    if (look == LOOK_HEAVY || look == LOOK_JUMP || look == LOOK_WARP) return strike % 2 ? LOOK_LOW : LOOK_HIGH;
    if (look == LOOK_THRUST || look == LOOK_DASH || look == LOOK_FAR) return strike % 2 ? LOOK_HIGH : LOOK_THRUST;
    if (strike % 2 == 0) return look;
    return look == LOOK_HIGH ? LOOK_LOW : LOOK_HIGH;
}

bool duel_event_after_contact(const DuelEvent *e) {
    if (e->kind == EV_PRESS && e->i == PRESS_TARDE) return true;
    return e->a < 0;
}

float duel_strike_lead_base(const Duel *d) {
    const Move *mv = duel_move(d);
    if (mv && mv->look == LOOK_FAR && d->comboStrike == 0) return d->s.attackLead * AJ_LANCA_PARTE_X;
    return d->s.attackLead;
}

float duel_strike_lead(const Duel *d) { return d->strikeLead > 0 ? d->strikeLead : duel_strike_lead_base(d); }

/* A lâmina variável (ajuste.h): sorteada a cada golpe, do mestre bladeFrom em diante.
 * A lança e a investida, no primeiro golpe, partem no tempo fixo: a partida delas toca
 * quadros (a ponta viajando, a corrida entrando no golpe). */
static float blade_lead(Duel *d) {
    float base = duel_strike_lead_base(d);
    const Move *mv = duel_move(d);
    if (d->s.bladeFrom <= 0 || d->m->id < d->s.bladeFrom) return base;
    if (d->comboStrike == 0 && mv && (mv->look == LOOK_FAR || mv->look == LOOK_DASH)) return base;
    float teto = d->m->bladeMax > 0 && d->m->bladeMax < d->s.bladeMax ? d->m->bladeMax : d->s.bladeMax;
    float hi = d->comboStrike > 0 && d->s.bladeChainMax < teto ? d->s.bladeChainMax : teto;
    float lo = d->s.bladeMin < hi ? d->s.bladeMin : hi;
    return lo + (hi - lo) * (float)rng_next(&d->bladeRng);
}

/* O aviso do primeiro golpe vem sempre o mesmo tempo antes do contato (o da postura);
 * na lança, o tempo a mais da ponta viajando também, para o aviso vir antes da partida.
 * Na sequência, o aviso é o contato anterior: o brilho sai quando a preparação começa. */
float duel_aviso(const Duel *d) {
    if (d->comboStrike > 0) return d->windupDuration;
    return duel_stance(d)->aviso + (duel_strike_lead_base(d) - d->s.attackLead);
}

float duel_launch_progress(const Duel *d) {
    if (d->phase != PH_WINDUP) return 0;
    const double partida = d->strikeAt - duel_strike_lead(d);
    if (d->clock < partida || d->clock >= d->strikeAt) return 0;
    return (float)((d->clock - partida) / (d->strikeAt - partida));
}

double duel_cue_time(const Duel *d) {
    double t = d->strikeAt - duel_aviso(d), start = d->strikeAt - d->windupDuration;
    return t < start ? start : t;
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
    t.perfectFrom = d->strikeAt - duel_perfect_window(d);
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

static bool move_allowed(const Duel *d, const Move *mv) {
    return (mv->stance < 0 || mv->stance == d->stanceIndex) && mv->minSeal <= d->seal;
}

/* Sorteia a próxima sequência entre as permitidas na postura e no selo atuais. Numa
 * postura em ordem, as primeiras seguem a ordem do roster. */
static int pick_move(Duel *d) {
    const MasterProfile *m = d->m;
    int n = d->stanceSequences[d->stanceIndex];
    if (n < duel_stance(d)->ordered) {
        int k = 0;
        for (int i = 0; i < m->moveCount; i++) {
            if (!move_allowed(d, &m->moves[i])) continue;
            if (k++ == n) return i;
        }
    }
    float total = 0;
    for (int i = 0; i < m->moveCount; i++) {
        const Move *mv = &m->moves[i];
        if (move_allowed(d, mv)) total += mv->weight;
    }
    if (total <= 0) return -1;
    double r = rng_next(&d->rng) * total;
    int last = -1;
    for (int i = 0; i < m->moveCount; i++) {
        const Move *mv = &m->moves[i];
        if (!move_allowed(d, mv)) continue;
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

float duel_perfect_window(const Duel *d) {
    return duel_stance(d)->perfectWindow * (1 - AJ_CHOQUE_JANELA * d->shock);
}

static void begin_attack(Duel *d) {
    const Settings *s = &d->s;
    const MasterProfile *m = d->m;
    d->phase = PH_WINDUP;
    d->attempted = false;
    /* Cada golpe novo zera a espera entre gestos: um gesto feito no intervalo
     * não pode roubar a defesa do golpe que está chegando. */
    d->lastPress = AJ_NUNCA;
    d->pressBlockedUntil = AJ_NUNCA;
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
        if (d->stanceIndex >= 0 && d->stanceIndex < MAX_STANCES) d->stanceSequences[d->stanceIndex]++;
        const Move *mv = duel_move(d);
        int strikes = mv ? (mv->strikes < 1 ? 1 : (mv->strikes > MAX_CHAIN ? MAX_CHAIN : mv->strikes)) : 1;
        d->comboRemaining = strikes - 1;
        if (strikes > 1) emit(d, EV_COMBO, J_NONE, 0, strikes, false);
    }
    const Move *mv = duel_move(d);
    d->strikeLead = blade_lead(d);
    /* A preparação começa em `phaseEnd`, o instante certo, e não no quadro em que o relógio o
     * passou: assim o contato, o aviso e o rearme não dependem da taxa de quadros. */
    const double t0 = d->phaseEnd;

    double duration;
    if (continuing) {
        /* O próximo contato chega exatamente `gap` segundos (de tempo real) depois do
         * anterior: o hitstop do impacto anterior congelou o duelo e sai daqui. */
        double gap = mv ? mv->gaps[d->comboStrike - 1] : s->minChainGap;
        if (gap < s->minChainGap) gap = s->minChainGap;
        duration = d->lastStrikeAt + gap - d->lastHitstop - t0;
        /* Com o atraso calibrado alto, um golpe sem defesa só é julgado na tolerância tardia mais o
         * atraso, e esta preparação começa depois: a lâmina parte no máximo o tempo que falta, e
         * nunca menos de AJ_LAMINA_MIN, para o contato chegar no tempo da sequência. */
        double lamina = duel_strike_lead(d), cabe = duration - AJ_PREPARO_MIN_CADEIA;
        if (lamina > cabe) lamina = cabe > AJ_LAMINA_MIN ? cabe : AJ_LAMINA_MIN;
        d->strikeLead = (float)lamina;
        if (duration < lamina + AJ_PREPARO_MIN_CADEIA) duration = lamina + AJ_PREPARO_MIN_CADEIA;
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
        antes *= m->waitScale > 0 ? m->waitScale : s->waitScale;
        if (m->rhythmJitter > 0) antes += (rng_next(&d->rng) * 2 - 1) * m->rhythmJitter;
        if (antes < AJ_PREPARO_ANTES_DO_AVISO) antes = AJ_PREPARO_ANTES_DO_AVISO;
        duration = duel_aviso(d) + antes;
    }
    if (!continuing && duration < duel_strike_lead(d) + AJ_PREPARO_MIN_PRIMEIRO) duration = duel_strike_lead(d) + AJ_PREPARO_MIN_PRIMEIRO;
    d->windupDuration = (float)duration;

    if (!continuing) {
        d->blackout = false;
        if (m->blackoutChance > 0) d->blackout = rng_next(&d->rng) < m->blackoutChance;
        /* O especial vale para a sequência inteira. */
        d->special = m->specialChance > 0 && !rule->noSpecial && rng_next(&d->rng) < m->specialChance;
        if (d->special) emit(d, EV_SPECIAL, J_NONE, 0, 0, false);
        d->sequences++;
    }

    d->strikeAt = t0 + d->windupDuration;
    d->attacks++;
    build_schedule(d);
    emit(d, EV_WINDUP, J_NONE, d->windupDuration, 0, false);
}

static void fire(Duel *d, ScheduleKind kind) {
    switch (kind) {
        case SCH_LAUNCH: emit(d, EV_LAUNCH, J_NONE, 0, 0, false); break;
        case SCH_CUE: emit(d, EV_CUE, J_NONE, 0, d->comboStrike, false); break;
        case SCH_CUE_SOUND: emit(d, EV_CUE, J_NONE, 0, d->comboStrike, true); break;
    }
}

/* Dispara, na ordem do tempo, os itens da agenda que já venceram: a lâmina que parte, o brilho e o
 * som do aviso. Vale para um passo grande (uma pausa, uma queda de quadro) e para o aperto que chega
 * logo depois de uma preparação que começou atrasada: o aviso e a lâmina nunca ficam sem sair antes
 * do julgamento do golpe. */
static void fire_due(Duel *d) {
    while (d->scheduleIndex < d->scheduleCount && d->clock >= d->schedule[d->scheduleIndex].time) {
        fire(d, d->schedule[d->scheduleIndex].kind);
        d->scheduleIndex++;
    }
}

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

float duel_hitstop_for(const Settings *s, Judgement j, bool broke, bool secondBlade) {
    float h = j == J_PERFEITO ? (broke ? s->breakHitstop : s->perfectHitstop) : j == J_BOM ? s->goodHitstop : s->badHitstop;
    if (secondBlade && h < s->badHitstop) h = s->badHitstop;
    return h;
}

/* O julgamento de um golpe, em tempo exato (o quadro do jogo não entra nesta conta):
 *   antecedência = contato - (instante do aperto - latência de vídeo): o que o jogador viu chegou
 *   atrasado, então o aperto conta a latência mais cedo;
 *   perfeito: antecedência de 0 até a janela perfeita do mestre (perfectWindow) e nenhum aperto antes
 *   do aviso neste golpe (earlyUsed);
 *   bom: até a janela boa (goodWindow) antes do contato, ou até a tolerância tardia (lateGrace) depois
 *   dele: nunca perfeito;
 *   ruim: o resto, inclusive não apertar (o golpe só é julgado quando a tolerância acaba).
 * Kojiro só tem uma tentativa por golpe, depois do aviso; antes dele o aperto não trava nada. */
static void resolve(Duel *d) {
    const Settings *s = &d->s;
    /* Consumir o golpe antes dos eventos impede julgamento duplicado. */
    d->phase = PH_RECOVERY;
    /* O instante do impacto: o contato; se o aperto veio na tolerância tardia, quando ele veio;
     * sem defesa, quando a tolerância acaba. Não é o quadro em que o núcleo julgou: as pausas
     * contam daqui, e por isso não dependem da taxa de quadros. */
    const double impacto = d->attempted ? (d->lastPress > d->strikeAt ? d->lastPress : d->strikeAt)
                                        : d->strikeAt + s->lateGrace + s->latency;
    d->phaseEnd = impacto + (d->comboRemaining > 0 ? s->comboGap : s->recovery);
    const Stance *st = duel_stance(d);
    /* o aperto conta `latency` mais cedo: o que o jogador viu e ouviu chegou atrasado */
    double lead = d->attempted ? d->strikeAt - (d->lastPress - s->latency) : -1;
    bool dual = duel_strike_dual(d), second = false;
    Judgement j;
    const double perfeita = duel_perfect_window(d);   /* com o choque do golpe anterior, menor; o choque vale para este golpe só */
    const bool chocado = d->shock > 0;
    if (d->attempted && lead >= 0 && lead <= perfeita + AJ_EPS_JANELA && !d->earlyUsed) {
        j = J_PERFEITO;
        d->perfects++;
        /* o parry perfeito apaga as brasas */
        if (d->burnLeft > 0) {
            d->burnLeft = 0;
            emit(d, EV_BURN, J_NONE, 0, 0, false);
        }
        d->bossPosture -= s->perfectBossDamage;
        d->renPosture = clampf(d->renPosture + s->perfectHeal * s->renPosture, 0, s->renPosture);
    } else if (d->attempted && lead >= -s->lateGrace - AJ_EPS_JANELA && lead <= st->goodWindow + AJ_EPS_JANELA) {
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
        if (d->m->healsOnHit) d->bossPosture = clampf(d->bossPosture + s->badBossRecover, 0, duel_posture_max(d));
    }

    /* o choque: gasto neste golpe, e renovado se este for o golpe que choca e acertou kojiro */
    const Move *mv = duel_move(d);
    d->shock = mv && mv->shock ? (j == J_RUIM ? 1.0f : (j == J_BOM && second) ? 0.5f : 0.0f) : 0.0f;

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
        d->phaseEnd = fim > impacto ? fim : impacto;
    }
    if (broke) {
        d->bossPosture = 0;
        d->comboRemaining = 0; /* a quebra interrompe o composto */
    }
    /* i: bit 0 = golpe de duas lâminas, bit 1 = a segunda lâmina acertou kojiro, bit 2 = este golpe deixou kojiro em choque, bit 3 = kojiro foi julgado em choque */
    DuelEvent *ev = emit(d, EV_IMPACT, j, (float)lead, (dual ? 1 : 0) | (second ? 2 : 0) | (d->shock > 0 ? 4 : 0) | (chocado ? 8 : 0), broke);
    if (ev && d->attempted) ev->b = lead > perfeita ? (float)(lead - perfeita) : lead < 0 ? (float)lead : 0;

    /* vantagem: falta só um perfeito (a postura cabe num perfeito) */
    bool vantagem = !broke && duel_advantage(d);
    if (vantagem != d->advantage) {
        d->advantage = vantagem;
        emit(d, EV_ADVANTAGE, J_NONE, 0, 0, vantagem);
    }
    if (broke) {
        d->advantage = false;
        if (d->seal + 1 < seal_total(d)) {
            d->seal++;
            d->bossPosture = duel_posture_max(d);
            d->renPosture = clampf(d->renPosture + s->sealHeal * s->renPosture, 0, s->renPosture);
            d->phaseEnd = impacto + s->sealRecovery;
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
    if (d->renPosture <= AJ_EPS_VIDA) {
        d->renPosture = 0;
        d->phase = PH_FINISHED;
        d->comboRemaining = 0;
        emit(d, EV_FINISHED, J_NONE, 0, 0, false);
    }
}

float hitstop_passo(float *restante, float dt) {
    if (*restante <= 0) return dt;
    if (dt <= *restante) { *restante -= dt; return 0; }
    float sobra = dt - *restante;
    *restante = 0;
    return sobra;
}

void duel_tick(Duel *d, double delta) {
    if (d->phase == PH_FINISHED) return;
    d->clock += delta > 0 ? delta : 0;
    if (d->burnLeft > 0 && delta > 0) {
        float t = (float)delta < d->burnLeft ? (float)delta : d->burnLeft;
        d->burnLeft -= t;
        d->renPosture = clampf(d->renPosture - d->burnRate * t, 0, d->s.renPosture);
        if (d->renPosture <= AJ_EPS_VIDA) {
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
        if (d->clock < d->phaseEnd) return;
        /* a preparação começa em phaseEnd; num passo grande o que já venceu nela sai neste mesmo passo
         * (no máximo um golpe por chamada: o seguinte começa na chamada seguinte) */
        begin_attack(d);
    }
    /* Os eventos disparam na ordem do tempo, mesmo num passo grande. */
    fire_due(d);
    /* com defesa, julga no contato; sem, espera a tolerância tardia (e o atraso calibrado) */
    if (d->clock >= d->strikeAt && (d->attempted || d->clock >= d->strikeAt + d->s.lateGrace + d->s.latency)) resolve(d);
}

void duel_step_at(Duel *d, double dt, double pressAt) {
    if (pressAt < 0) {
        duel_tick(d, dt);
        return;
    }
    if (pressAt > dt) pressAt = dt;
    duel_tick(d, pressAt);
    duel_press(d);
    duel_tick(d, dt - pressAt);
}

void duel_step(Duel *d, double dt, bool press) { duel_step_at(d, dt, press ? dt * 0.5 : -1); }

/* Um aperto. Três casos, pela fase e pelo instante:
 *   fora da preparação (gesto): vale só como gesto e trava os apertos por inputCooldown (a trava do
 *   aperto); um gesto logo depois de um golpe que entrou sem defesa vira "tarde";
 *   na preparação, antes do aviso (cedo): não gasta a tentativa, mas dá uma recarga que termina no
 *   aviso e tira o perfeito deste golpe;
 *   na preparação, depois do aviso: a tentativa do golpe (uma só); depois do contato, ainda dentro da
 *   tolerância tardia, julga na hora.
 * A trava se rearma a cada preparação nova (begin_attack): um gesto no intervalo nunca rouba a defesa
 * do golpe que está chegando. */
bool duel_press(Duel *d) {
    if (d->phase == PH_FINISHED) return false;
    /* O núcleo começa uma preparação por tick. Se ela já era devida quando o aperto chegou (o
     * quadro julgou o golpe anterior e o aperto veio logo depois), começa antes do aperto: o
     * aperto cai na preparação, como cairia com quadros menores. */
    if ((d->phase == PH_READY || d->phase == PH_RECOVERY) && d->clock >= d->phaseEnd) begin_attack(d);
    if (d->phase == PH_WINDUP) fire_due(d);   /* o aviso e a lâmina saem antes do aperto, na ordem */
    if (d->clock < d->pressBlockedUntil - AJ_EPS_TEMPO) return false;
    if (d->phase == PH_WINDUP && d->attempted) return false;
    PressKind kind;
    double quando = 0, antesDoAviso = 0;   /* o que o evento conta (EV_PRESS: a e b) */
    if (d->phase == PH_WINDUP) {
        /* o jogador vê o aviso `latency` depois: é esse o aviso que conta para ele */
        double aviso = duel_cue_time(d) + d->s.latency;
        quando = d->strikeAt - (d->clock - d->s.latency);
        if (d->clock < aviso - AJ_EPS_TEMPO) {
            /* antes do aviso: não trava o golpe; a recarga acaba no aviso, no máximo (a janela
             * boa vem sempre depois dele). O custo: a defesa deste golpe não sai perfeita. */
            double fim = d->clock + AJ_RECARGA_CEDO;
            d->pressBlockedUntil = fim < aviso ? fim : aviso;
            d->earlyUsed = true;
            kind = PRESS_CEDO;
            antesDoAviso = aviso - d->clock;
        } else {
            d->lastPress = d->clock;
            d->attempted = true;
            kind = PRESS_TENTATIVA;
            /* dentro da tolerância tardia: o contato já passou, julga agora */
            if (d->clock >= d->strikeAt) {
                emit(d, EV_PRESS, J_NONE, (float)quando, kind, false);
                resolve(d);
                return true;
            }
        }
    } else {
        d->lastGestureAt = d->clock;
        d->pressBlockedUntil = d->clock + d->s.inputCooldown;
        bool tarde = d->lastJudgement == J_RUIM && !d->lastAttempted && d->clock - d->lastStrikeAt <= AJ_TARDE_JANELA;
        kind = tarde ? PRESS_TARDE : PRESS_GESTO;
        if (tarde) quando = (d->clock - d->s.latency) - d->lastStrikeAt;
    }
    DuelEvent *e = emit(d, EV_PRESS, J_NONE, (float)quando, kind, false);
    if (e) e->b = (float)antesDoAviso;
    return true;
}

/* ------------------------------------------------------------------ */

float calibration_result(const float *offsets, int n) {
    float ok[AJ_CALIBRA_MAX_APERTOS];
    int k = 0;
    for (int i = 0; i < n && k < AJ_CALIBRA_MAX_APERTOS; i++)
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

void campaign_win(Campaign *c, int index) {
    campaign_mark_cleared(c, index);
    if (c->index == index) campaign_advance(c);
}
