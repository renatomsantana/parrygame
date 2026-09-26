/*
 * core.h - regras puras do APARA: duelo de parry de um botão, só postura.
 * Não depende da raylib: compila sozinho e roda nos testes.
 */
#ifndef APARA_CORE_H
#define APARA_CORE_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_WINDUPS 6
#define MAX_STANCES 4
#define MAX_SEALS 3
#define MAX_FALSE_CUES 3
#define MAX_LINES 8
#define MAX_EVENTS 32
#define MASTER_COUNT 12   /* aprendizes antes de oboro */

/* ------------------------------------------------------------------ */
/* Parâmetros comuns. Kojiro ("Ren" no código) tem vida; o mestre, postura.  */
/* ------------------------------------------------------------------ */
typedef struct {
    float renPosture;          /* vida inicial de kojiro (o nome do campo ficou) */
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
#define MAX_MOVES 24
#define MAX_CHAIN 8

/* Preparação que denuncia a sequência. LOOK_HEAVY é o golpe forte: o salto com a
 * pancada de cima (STRONG_ATTACK), de preparação longa e bem visível. LOOK_DASH
 * recua e vem correndo até o alcance; LOOK_JUMP salta e desce cortando, com o
 * contato no instante em que os pés tocam o chão. LOOK_FAR é a estocada de longe
 * (a lança): o mestre fica afastado e a ponta viaja mais, então entre a lâmina
 * partir e chegar passa mais tempo que nos outros golpes (FAR_LEAD). LOOK_WARP é o
 * sumiço do corvo: ele vira penas no meio da preparação e reaparece na frente de
 * kojiro para terminar o golpe; o reaparecer é o aviso. */
typedef enum { LOOK_HIGH, LOOK_LOW, LOOK_THRUST, LOOK_HEAVY, LOOK_DASH, LOOK_JUMP, LOOK_FAR, LOOK_WARP } MoveLook;
#define FAR_LEAD 1.75f            /* a estocada de longe parte 1,75 x mais cedo */
#define BURN_TIME 3.0f            /* segundos em brasas depois de um golpe do enjin */

typedef struct {
    const char *name;
    int strikes;
    float gaps[MAX_CHAIN - 1];    /* segundos entre um contato e o próximo */
    float weight;                 /* chance relativa de ser escolhida */
    int stance;                   /* -1 = qualquer postura */
    int minSeal;                  /* só a partir deste selo */
    MoveLook look;
    /* Golpes de duas lâminas (bit k = o golpe k da sequência). Um parry só apara
     * as duas se for perfeito; no bom, a segunda passa; no erro, entram as duas. */
    unsigned dual;
} Move;

typedef enum {
    ARENA_DOJO, ARENA_SERRA, ARENA_CELEIRO, ARENA_TELHADOS, ARENA_PORTO, ARENA_SALAO,
    ARENA_PONTE, ARENA_CACHOEIRA, ARENA_BAMBUZAL, ARENA_FORJA, ARENA_JARDIM, ARENA_CIDADELA,
    ARENA_TEMPLO,
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
    float damage;                 /* multiplica o dano em kojiro (0 = 1) */
    float burn;                   /* enjin: um erro deixa kojiro em brasas; em BURN_TIME s ele
                                     perde mais esta fração do golpe (0 = não queima) */
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
    Line visit[MAX_LINES];        /* depois da vitória, na cabana de hanzo: o que ele diz deste e do próximo */
    int visitCount;
} MasterProfile;

#define ROSTER_SIZE 13
const MasterProfile *roster_get(int index);   /* 0..12; o último é oboro */
int roster_size(void);

#define LORE_PAGES 10
const char *lore_page(int index);

/* A luta final e os finais: falas e o que acontece em cena quando cada uma começa.
 * Uma entrada sem fala é só a ação, e passa sozinha quando ela termina. */
typedef enum {
    CUE_NONE,
    CUE_MASK_ON,      /* oboro põe a máscara de oni */
    CUE_MASK_OFF,     /* de joelhos, desarmado, ele tira a máscara */
    CUE_RAISE,        /* kojiro chega perto e ergue a espada */
    CUE_KILL,         /* kojiro mata oboro */
    CUE_HANZO_CLAP,   /* hanzo chega do dojo aplaudindo */
    CUE_LOWER,        /* kojiro abaixa a espada e vai embora */
    CUE_HANZO_IN,     /* hanzo aparece do escuro, e kojiro vira */
    CUE_HANZO_MASK,   /* hanzo pega a máscara do chão e põe no rosto */
    CUE_HANZO_KILL,   /* hanzo pega a katana dele do chão e mata oboro */
    CUE_CHASE,        /* kojiro corre atrás dele, e hanzo some */
} Cue;

typedef struct {
    const char *speaker, *text;   /* sem fala: NULL */
    Cue cue;
} Beat;

typedef enum {
    SCENE_SEAL_1,     /* primeiro selo quebrado: oboro fala de hanzo */
    SCENE_SEAL_2,     /* segundo selo: a máscara */
    SCENE_KNEEL,      /* postura quebrada: de joelhos, sem a máscara */
    SCENE_SIM,        /* matou oboro */
    SCENE_NAO,        /* não matou */
    SCENE_COUNT
} SceneId;

const Beat *story_scene(SceneId id, int *count);

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
    EV_SPECIAL,       /* o próximo golpe é especial: dano dobrado */
    EV_BURN           /* flag: kojiro pegou fogo (true) ou as brasas apagaram (false) */
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

    float renPosture;             /* vida de kojiro */
    float burnLeft, burnRate;     /* brasas: segundos que faltam e vida perdida por segundo */
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
float duel_ren_damage(const Duel *d);
bool duel_strike_dual(const Duel *d);     /* o golpe que vem é de duas lâminas */
float duel_strike_lead(const Duel *d);    /* segundos entre a lâmina partir e o contato */            /* dano de um erro contra este mestre */
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
