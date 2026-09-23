/*
 * core.h - regras puras do APARA: duelo de parry de um botão, só postura.
 * Não depende da raylib: compila sozinho e roda nos testes.
 */
#ifndef APARA_CORE_H
#define APARA_CORE_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_WINDUPS 6
#define MAX_STANCES 12         /* oboro usa as onze posturas dos aprendizes */
#define MAX_SEALS 3
#define MAX_FALSE_CUES 3
#define MAX_LINES 3
#define MAX_EVENTS 32
#define MASTER_COUNT 11   /* aprendizes antes de oboro */

/* ------------------------------------------------------------------ */
/* Parâmetros comuns. Não existe vida: só postura, de Ren e do mestre. */
/* ------------------------------------------------------------------ */
typedef struct {
    float renPosture;          /* postura inicial de Ren */
    float badPostureDamage;    /* ruim: Ren perde */
    float badBossRecover;      /* ruim: o mestre recupera, se tiver essa técnica */
    float goodBossDamage;      /* bom: o mestre perde */
    float goodRenCost;         /* bom: Ren perde um pouco (o impacto ainda pesa) */
    float perfectBossDamage;   /* perfeito: o mestre perde */
    float perfectRenRecover;   /* perfeito: Ren recupera */
    float inputCooldown;       /* intervalo mínimo entre gestos dentro do mesmo golpe */
    float attackLead;          /* a lâmina parte este tempo antes do contato */
    float cueLead;             /* o sinal toca este tempo antes do contato */
    float recovery;            /* pausa depois de cada golpe */
    float sealRecovery;        /* pausa depois de quebrar um selo do BIG BOSS */
    float sealRenRecover;      /* selo quebrado: Ren recupera o fôlego */
    float firstWindupDelay;    /* pausa antes do primeiro golpe */
    float pressureSpeed;       /* multiplicador de preparação com metade da postura */
    float feintDelayMin, feintDelayMax;
    float comboGap;               /* pausa depois de cada golpe de uma sequência */
    float minChainGap;            /* menor intervalo entre contatos de uma sequência */
    float goodHitstop, perfectHitstop, badHitstop, breakHitstop;
    /* Crescimento de Ren a cada mestre vencido. */
    float postureGrowth, perfectGrowth, goodGrowth;
} Settings;

void settings_default(Settings *s);
/* Ren mais forte: postura e dano crescem com os mestres já vencidos. */
void settings_for_level(Settings *s, int defeated);

/* ------------------------------------------------------------------ */
/* Mestres                                                             */
/* ------------------------------------------------------------------ */
typedef struct {
    const char *name;             /* "" para mestres de uma postura só */
    float perfectWindow, goodWindow;
    float windups[MAX_WINDUPS];
    int windupCount;
    float feintChance;
    int falseCues;
    float feintDelayMin, feintDelayMax; /* 0 = usa o padrão das Settings */
    bool mimicParry;              /* a partida falsa mostra o gesto de parry */
} Stance;

/* Um selo é uma barra de postura inteira. Mestres comuns têm um; o BIG BOSS, três. */
typedef struct {
    const char *name;
    float speedMultiplier;
    int stanceSwitchEvery;        /* sequências até trocar de postura; 0 = não troca */
} SealRule;

/*
 * Moveset: cada mestre tem um repertório fixo de sequências. Uma sequência é
 * uma preparação seguida de 1 a MAX_CHAIN golpes, com intervalos sempre iguais
 * entre um contato e o próximo. É isso que o jogador estuda e decora.
 */
#define MAX_MOVES 14
#define MAX_CHAIN 5

typedef enum { LOOK_HIGH, LOOK_LOW, LOOK_THRUST } MoveLook; /* preparação que denuncia a sequência */

typedef struct {
    const char *name;
    int strikes;
    float gaps[MAX_CHAIN - 1];    /* segundos entre um contato e o próximo */
    float weight;                 /* chance relativa de ser escolhida */
    int stance;                   /* -1 = qualquer postura */
    int minSeal;                  /* só a partir deste selo */
    MoveLook look;
} Move;

typedef enum {
    ARENA_DOJO, ARENA_SERRA, ARENA_CELEIRO, ARENA_COBERTURA, ARENA_PORTO, ARENA_SALAO,
    ARENA_TREM, ARENA_CACHOEIRA, ARENA_BAMBUZAL, ARENA_FORJA, ARENA_JARDIM, ARENA_CIDADELA,
    ARENA_COUNT
} ArenaId;

typedef struct {
    const char *speaker;
    const char *text;
} Line;

typedef struct {
    int id;
    const char *name, *title, *venue, *special;
    const char *style;            /* postura de combate, mostrada no lugar de dicas */
    ArenaId arena;
    float posture;                /* postura de cada selo */
    int hitsToFall;               /* erros que Ren aguenta contra este mestre (0 = padrão das Settings) */
    float specialChance;          /* golpe especial: dano dobrado (só o BIG BOSS) */
    bool healsOnHit;              /* acertar ren devolve postura ao mestre (do 5º mestre em diante) */
    Stance stances[MAX_STANCES];
    int stanceCount;
    SealRule seals[MAX_SEALS];
    int sealCount;                /* 1 para mestres comuns */
    float rhythmJitter;           /* ± segundos na preparação (Neon Jax) */
    int accelSteps;               /* Taiko: golpes por ciclo de aceleração */
    float accelFactor;            /* Taiko: cada golpe do ciclo encurta por este fator */
    float cueVisual, cueAudio;    /* força do sinal (1 = normal, 0 = escondido) */
    float blackoutChance;         /* Yoru: chance de apagar as luzes na preparação */
    Move moves[MAX_MOVES];
    int moveCount;
    uint32_t tint;                /* 0xRRGGBBAA aplicado ao sprite do mestre */
    bool useHeroSheet;            /* Sombra usa a prancha de Ren espelhada */
    bool isBigBoss;
    Line intro[MAX_LINES];
    int introCount;
    Line outro[MAX_LINES];
    int outroCount;
    Line sensei[MAX_LINES];       /* conselhos de hanzo para quem travou neste mestre */
    int senseiCount;
} MasterProfile;

#define ROSTER_SIZE 12
const MasterProfile *roster_get(int index);   /* 0..12 */
int roster_size(void);

#define LORE_PAGES 10
const char *lore_page(int index);

/* ------------------------------------------------------------------ */
/* Gerador com semente (mulberry32, idêntico ao do plugin JS)           */
/* ------------------------------------------------------------------ */
typedef struct { uint32_t state; } Rng;
void rng_seed(Rng *r, uint32_t seed);
double rng_next(Rng *r);

/* ------------------------------------------------------------------ */
/* Duelo                                                               */
/* ------------------------------------------------------------------ */
typedef enum { PH_READY, PH_WINDUP, PH_RECOVERY, PH_FINISHED } DuelPhase;
typedef enum { J_NONE, J_RUIM, J_BOM, J_PERFEITO } Judgement;

typedef enum {
    EV_WINDUP,        /* a: duração, flag: finta */
    EV_FEINT_LAUNCH,  /* partida falsa */
    EV_LAUNCH,        /* partida real */
    EV_CUE,           /* flag: falso */
    EV_PRESS,         /* gesto aceito */
    EV_IMPACT,        /* judgement, a: antecedência (-1 = sem defesa), flag: quebrou postura */
    EV_STANCE,        /* i: nova postura */
    EV_SEAL,          /* i: novo selo (o BIG BOSS entrou em outra fase) */
    EV_COMBO,         /* i: golpes na sequência (só quando mais de um) */
    EV_FINISHED,      /* flag: vitória */
    EV_SPECIAL        /* o próximo golpe é especial: dano dobrado */
} EventKind;

typedef struct {
    EventKind kind;
    Judgement judgement;
    float a;
    int i;
    bool flag;
} DuelEvent;

typedef enum { SCH_FAKE_LAUNCH, SCH_FAKE_CUE, SCH_LAUNCH, SCH_CUE } ScheduleKind;
typedef struct { double time; ScheduleKind kind; } ScheduleItem;

typedef struct {
    Settings s;
    const MasterProfile *m;
    SealRule commonSeal;          /* regra de quem não declara selos */
    Rng rng;

    double clock;
    DuelPhase phase;
    double phaseEnd;
    double strikeAt;
    double fakeStrikeAts[MAX_FALSE_CUES];
    int fakeCount;
    float windupDuration;
    bool isFeint, blackout, special;
    double lastPress;
    bool attempted, attackLaunched, feintLaunched, cuePlayed, fakeCuePlayed;

    float renPosture;
    float bossPosture;
    int seal;                     /* selo atual (0..sealCount-1) */
    int stanceIndex;
    int comboRemaining, comboStrike;
    int move, sequences;          /* sequência atual e quantas já saíram */
    int attacks, feints, perfects, goods, bads;

    ScheduleItem schedule[2 * MAX_FALSE_CUES + 2];
    int scheduleCount, scheduleIndex;

    DuelEvent events[MAX_EVENTS];
    int eventCount;
} Duel;

void duel_init(Duel *d, const Settings *s, const MasterProfile *m, uint32_t seed);
void duel_reset(Duel *d);
void duel_tick(Duel *d, double delta);
bool duel_press(Duel *d);
double duel_time_to_impact(const Duel *d);        /* -1 fora da preparação */
double duel_time_to_next_instant(const Duel *d);  /* falso ou real: o que a tela mostra */
const Stance *duel_stance(const Duel *d);
const SealRule *duel_seal_rule(const Duel *d);
bool duel_under_pressure(const Duel *d);          /* mestre com metade da postura ou menos */
bool duel_in_combo(const Duel *d);
const Move *duel_move(const Duel *d);             /* sequência em curso (NULL = golpe simples) */
float duel_ren_damage(const Duel *d);            /* dano de um erro contra este mestre */
/* Copia e esvazia a fila de eventos. Devolve quantos. */
int duel_drain(Duel *d, DuelEvent *out, int max);

/* ------------------------------------------------------------------ */
/* Trilha                                                              */
/* ------------------------------------------------------------------ */
typedef struct {
    int index;                    /* mestre atual */
    uint32_t clearedMask;
    bool completed;
    bool loreSeen;
} Campaign;

void campaign_reset(Campaign *c);
void campaign_mark_cleared(Campaign *c, int index);
bool campaign_is_cleared(const Campaign *c, int index);
int campaign_defeated(const Campaign *c);   /* mestres vencidos, sem contar o BIG BOSS */
bool campaign_big_boss_open(const Campaign *c);
bool campaign_advance(Campaign *c);   /* false quando a trilha acabou */

#endif
