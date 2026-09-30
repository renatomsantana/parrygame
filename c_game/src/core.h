/*
 * core.h - regras puras do APARA: duelo de parry de um botão, só postura.
 * Não depende da raylib: compila sozinho e roda nos testes.
 */
#ifndef APARA_CORE_H
#define APARA_CORE_H

#include <stdbool.h>
#include <stdint.h>

#include "ajuste.h"   /* constantes globais de equilíbrio e de sensação */

#define MAX_STANCES 4
#define MAX_SEALS 3
#define MAX_LINES 8
#define MAX_EVENTS 32
#define MASTER_COUNT 12   /* aprendizes antes de oboro */

/* ------------------------------------------------------------------ */
/* Parâmetros comuns, com os valores de ajuste.h (ajuste.c). Kojiro     */
/* ("Ren" no código) tem vida; o mestre, postura.                      */
/* ------------------------------------------------------------------ */
typedef struct {
    float renPosture;          /* vida inicial de kojiro (o nome do campo ficou) */
    float badBossRecover;      /* ruim: o mestre recupera, se tiver essa técnica */
    float goodBossDamage;      /* bom: o mestre perde */
    float goodRenCost;         /* bom: Ren perde um pouco (o impacto ainda pesa) */
    float perfectBossDamage;   /* perfeito: o mestre perde */
    float perfectHeal;         /* perfeito: Ren recupera esta fração da vida */
    float inputCooldown;       /* intervalo mínimo entre gestos dentro do mesmo golpe */
    float lateGrace;           /* um aperto até isto depois do contato ainda é bom */
    float latency;             /* atraso de vídeo (calibração): o aperto conta isto mais cedo */
    float audioLead;           /* o som do aviso toca isto antes do brilho (atraso de áudio menos o de vídeo) */
    float attackLead;          /* a lâmina parte este tempo antes do contato */
    float bladeMin, bladeMax;  /* lâmina variável: a partida sorteada neste intervalo antes do contato */
    float bladeChainMax;       /*   dentro da sequência, no máximo isto */
    int bladeFrom;             /*   do mestre com este id em diante (0 = desligada: sempre attackLead) */
    float recovery;            /* pausa depois de cada golpe */
    float sealRecovery;        /* pausa depois de quebrar um selo do BIG BOSS */
    float sealHeal;            /* selo quebrado: Ren recupera esta fração da vida */
    float firstWindupDelay;    /* pausa antes do primeiro golpe */
    float pressureSpeed;       /* multiplicador de preparação com metade da postura */
    float waitScale;           /* multiplicador da espera antes do aviso (a preparação parada) */
    float comboGap;               /* pausa depois de cada golpe de uma sequência */
    float minChainGap;            /* menor intervalo entre contatos de uma sequência */
    float goodHitstop, perfectHitstop, badHitstop, breakHitstop;
    /* O parry de Ren fica mais forte a cada mestre vencido (a vida não cresce). */
    float perfectGrowth, goodGrowth;
} Settings;

void settings_default(Settings *s);
/* Ren mais forte: o dano do perfeito e do bom cresce com os mestres já vencidos. */
void settings_for_level(Settings *s, int defeated);

/* ------------------------------------------------------------------ */
/* Mestres                                                             */
/* ------------------------------------------------------------------ */
typedef struct {
    const char *name;             /* "" para mestres de uma postura só */
    float perfectWindow, goodWindow;
    float aviso;                  /* o aviso (som e brilho) vem este tempo antes do contato */
    int ordered;                  /* as primeiras N sequências desta postura saem na ordem em que
                                     estão no roster; depois, sorteadas (0 = sempre sorteadas) */
} Stance;

/* Um selo é uma barra de postura inteira. Mestres comuns têm um; o BIG BOSS, três. */
typedef struct {
    const char *name;
    float speedMultiplier;        /* a espera antes do aviso x isto */
    int stanceSwitchEvery;        /* sequências até trocar de postura; 0 = não troca */
    float posture;                /* postura deste selo (0 = a do mestre) */
    float damageMultiplier;       /* o dano de um erro x isto (0 = 1) */
    bool noSpecial;               /* neste selo, nada de golpe especial */
} SealRule;

/*
 * Moveset: cada mestre tem um repertório fixo de sequências. Uma sequência é
 * uma preparação seguida de 1 a MAX_CHAIN golpes, com intervalos sempre iguais
 * entre um contato e o próximo. É isso que o jogador estuda e decora.
 */
#define MAX_MOVES 32
#define MAX_CHAIN 8

/* Preparação que denuncia a sequência. LOOK_HEAVY é o golpe forte: o salto com a
 * pancada de cima (STRONG_ATTACK), de preparação longa e bem visível. LOOK_DASH
 * recua e vem correndo até o alcance; LOOK_JUMP salta e desce cortando, com o
 * contato no instante em que os pés tocam o chão. LOOK_FAR é a estocada de longe
 * (a lança): o mestre fica afastado e a ponta viaja mais, então entre a lâmina
 * partir e chegar passa mais tempo que nos outros golpes (AJ_LANCA_PARTE_X). LOOK_WARP é o
 * sumiço do corvo: ele vira penas no meio da preparação e reaparece na frente de
 * kojiro para terminar o golpe; o reaparecer é o aviso. */
typedef enum { LOOK_HIGH, LOOK_LOW, LOOK_THRUST, LOOK_HEAVY, LOOK_DASH, LOOK_JUMP, LOOK_FAR, LOOK_WARP } MoveLook;

typedef struct {
    const char *name;
    int strikes;
    float gaps[MAX_CHAIN - 1];    /* segundos entre um contato e o próximo */
    float weight;                 /* chance relativa de ser escolhida */
    int stance;                   /* -1 = qualquer postura */
    int minSeal;                  /* só a partir deste selo */
    MoveLook look;
    /* Preparação do primeiro golpe, do começo ao contato: cada sequência tem a sua.
     * Pressa, aceleração, selo e o traço aleatório mudam só a parte antes do aviso;
     * do aviso ao contato é sempre o mesmo tempo. */
    float windup;
    /* Golpes de duas lâminas (bit k = o golpe k da sequência). Um parry só apara
     * as duas se for perfeito; no bom, a segunda passa; no erro, entram as duas. */
    unsigned dual;
    bool thrustOnly;             /* apresentação: todos os contatos seguem retos, sem alternar para corte */
    bool feint;                  /* apresentação: ameaça sem contato antes da estocada real */
} Move;

/* Direção visual de cada contato. Não participa do julgamento do parry. */
MoveLook move_contact_look(const Move *move, int strike);

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
    int hitsToFall;               /* erros que kojiro aguenta contra este mestre (o dano de um erro é a vida / isto) */
    float specialChance;          /* golpe especial: dano dobrado (só o BIG BOSS) */
    float damage;                 /* multiplica o dano em kojiro (0 = 1) */
    float burn;                   /* enjin: um erro deixa kojiro em brasas; em AJ_BRASAS_TEMPO s ele
                                     perde mais esta fração do golpe (0 = não queima) */
    bool healsOnHit;              /* acertar ren devolve postura ao mestre (do 5º mestre em diante) */
    Stance stances[MAX_STANCES];
    int stanceCount;
    SealRule seals[MAX_SEALS];
    int sealCount;                /* 1 para mestres comuns */
    float rhythmJitter;           /* ± segundos na preparação (Neon Jax) */
    float waitScale;              /* a espera antes do aviso só deste mestre, x isto (0 = a global, Settings.waitScale) */
    int accelSteps;               /* Taiko: golpes por ciclo de aceleração */
    float accelFactor;            /* Taiko: cada golpe do ciclo encurta por este fator */
    float cueVisual, cueAudio;    /* força do sinal (1 = normal, 0 = escondido) */
    float blackoutChance;         /* Yoru: chance de apagar as luzes na preparação */
    Move moves[MAX_MOVES];
    int moveCount;
    uint32_t tint;                /* 0xRRGGBBAA aplicado ao sprite do mestre */
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
    EV_WINDUP,        /* a: duração */
    EV_LAUNCH,        /* a lâmina parte */
    EV_CUE,           /* o aviso; i: qual golpe da sequência (0 = o primeiro); flag: o som (false: o brilho) */
    EV_PRESS,         /* gesto aceito; i: PressKind; a: antecedência ao contato (CEDO, TENTATIVA) ou atraso
                         depois do contato que entrou (TARDE); b: CEDO, quanto faltava para o aviso */
    EV_IMPACT,        /* judgement, a: antecedência (negativa = depois do contato; -1 = sem defesa), flag: quebrou
                         postura; b: quanto o aperto ficou fora da janela perfeita (>0 cedo, <0 tarde, 0 dentro) */
    EV_STANCE,        /* i: nova postura */
    EV_SEAL,          /* i: novo selo (o BIG BOSS entrou em outra fase) */
    EV_COMBO,         /* i: golpes na sequência (só quando mais de um) */
    EV_FINISHED,      /* flag: vitória */
    EV_SPECIAL,       /* o próximo golpe é especial: dano dobrado */
    EV_BURN,          /* flag: kojiro pegou fogo (true) ou as brasas apagaram (false) */
    EV_ADVANTAGE      /* flag: falta só um perfeito para quebrar a postura (true) ou não falta mais (false) */
} EventKind;

/* O que foi cada aperto aceito. */
typedef enum {
    PRESS_GESTO,      /* fora da preparação */
    PRESS_TENTATIVA,  /* a defesa do golpe (depois do aviso: uma só) */
    PRESS_CEDO,       /* antes do aviso: não trava o golpe, dá uma recarga curta */
    PRESS_TARDE       /* logo depois de um golpe que entrou sem defesa */
} PressKind;

typedef struct {
    EventKind kind;
    Judgement judgement;
    float a, b;
    int i;
    bool flag;
} DuelEvent;

typedef enum { SCH_LAUNCH, SCH_CUE, SCH_CUE_SOUND } ScheduleKind;
typedef struct { double time; ScheduleKind kind; } ScheduleItem;

typedef struct {
    Settings s;
    const MasterProfile *m;
    SealRule commonSeal;          /* regra de quem não declara selos */
    Rng rng;
    Rng bladeRng;                 /* só a lâmina variável: os outros sorteios não mudam com ela */

    double clock;
    DuelPhase phase;
    double phaseEnd;
    double strikeAt;
    float windupDuration;
    float strikeLead;             /* a lâmina deste golpe parte isto antes do contato (0 = ainda nenhum) */
    bool blackout, special;
    double lastPress;
    double pressBlockedUntil;     /* recarga: nenhum aperto aceito antes disto */
    bool attempted;
    bool earlyUsed;               /* apertou antes do aviso neste golpe: a defesa não sai perfeita */

    float renPosture;             /* vida de kojiro */
    float burnLeft, burnRate;     /* brasas: segundos que faltam e vida perdida por segundo */
    float bossPosture;
    int seal;                     /* selo atual (0..sealCount-1) */
    int stanceIndex;
    int comboRemaining, comboStrike;
    int move, sequences;          /* sequência atual e quantas já saíram */
    int stanceSequences[MAX_STANCES]; /* sequências que já saíram em cada postura */
    bool advantage;               /* falta só um perfeito para quebrar a postura */
    int attacks, perfects, goods, bads;
    /* o último golpe julgado e o último gesto, para o overlay de debug e o "cedo/tarde" */
    double lastStrikeAt;          /* instante do contato (-1 = nenhum ainda) */
    Judgement lastJudgement;
    double lastLead;              /* segundos entre o aperto e o contato; negativo = depois dele */
    bool lastAttempted;           /* o último golpe teve defesa */
    double lastGestureAt;         /* último aperto fora da preparação (-100 = nenhum) */
    float lastHitstop;            /* quanto o último impacto congela o duelo (o jogo aplica) */

    ScheduleItem schedule[3];
    int scheduleCount, scheduleIndex;

    DuelEvent events[MAX_EVENTS];
    int eventCount;
} Duel;

void duel_init(Duel *d, const Settings *s, const MasterProfile *m, uint32_t seed);
void duel_reset(Duel *d);
/* Para testar à mão (F3): recomeça a luta direto no selo `seal` (0 = o primeiro), com
 * vida cheia e a postura e a postura de luta desse selo; e reabastece vida e postura. */
void duel_start_seal(Duel *d, int seal);
void duel_refill(Duel *d, bool vida, bool postura);
void duel_tick(Duel *d, double delta);
bool duel_press(Duel *d);
/* Um quadro do jogo: o aperto chegou em algum ponto do quadro e entra no meio dele (não há
 * como saber o instante, só o quadro). */
void duel_step(Duel *d, double dt, bool press);
/* Um quadro com o aperto num instante conhecido (pressAt segundos depois do começo do quadro,
 * -1 = nenhum): os testes e os robôs, que decidem em ms. */
void duel_step_at(Duel *d, double dt, double pressAt);
const Stance *duel_stance(const Duel *d);
const SealRule *duel_seal_rule(const Duel *d);
bool duel_under_pressure(const Duel *d);          /* mestre com metade da postura ou menos */
float duel_posture_max(const Duel *d);            /* postura cheia do selo atual */
bool duel_advantage(const Duel *d);               /* um perfeito agora quebra a postura */
const Move *duel_move(const Duel *d);             /* sequência em curso (NULL = golpe simples) */
float duel_ren_damage(const Duel *d);     /* dano de um erro contra este mestre */
bool duel_strike_dual(const Duel *d);     /* o golpe que vem é de duas lâminas */
float duel_strike_lead(const Duel *d);    /* segundos entre a lâmina partir e o contato */
float duel_strike_lead_base(const Duel *d); /* o mesmo, sem a lâmina variável (os quadros tocam neste tempo) */
float duel_aviso(const Duel *d);          /* segundos entre o aviso e o contato */
/* Só para o desenho (o rastro fantasma): 0 antes da partida da lâmina e do contato em
 * diante (no instante do julgamento não há rastro); entre os dois, quanto do caminho a
 * lâmina já andou, de 0 a 1. Lê o duelo e nada mais. */
float duel_launch_progress(const Duel *d);
/* Quanto um impacto congela o duelo: o perfeito mais que o bom, a quebra mais ainda;
 * se a segunda lâmina de um golpe duplo entrou, pelo menos o do erro. */
float duel_hitstop_for(const Settings *s, Judgement j, bool broke, bool secondBlade);
double duel_cue_time(const Duel *d);      /* instante do aviso do golpe em preparação */
/* Linha do tempo do golpe em preparação (tempos do relógio do duelo), para o overlay
 * de debug: perfeito se o aperto cai em [perfectFrom, strike], bom em [goodFrom, strike]. */
typedef struct {
    bool active;                  /* há golpe em preparação */
    double now, start, launch, cue, strike;
    double perfectFrom, goodFrom;
} DuelTimeline;
DuelTimeline duel_timeline(const Duel *d);
/* Calibração: com os apertos da tela de teste (segundos depois da batida; negativo =
 * antes), o atraso a usar: a mediana dos que ficaram perto da batida, entre 0 e
 * AJ_LATENCIA_MAX. -1 se sobraram menos da metade. */
float calibration_result(const float *offsets, int n);
/* Copia e esvazia a fila de eventos. Devolve quantos. */
int duel_drain(Duel *d, DuelEvent *out, int max);
/* O aperto de um EV_PRESS ou o resultado de um EV_IMPACT veio DEPOIS do contato? O sinal de `a` não é o mesmo em todos:
 * no EV_PRESS TARDE, a é o atraso (positivo = depois); nos demais, a é a antecedência (negativa = depois). */
bool duel_event_after_contact(const DuelEvent *e);
/* O hitstop congela o duelo por `*restante` segundos de tempo real. Gasta `dt` desse congelamento e
 * devolve quanto do quadro sobra para o duelo correr (o quadro inteiro se não há mais o que congelar).
 * O quadro em que o congelamento acaba corre a parte que sobra: o congelamento dura exatamente o que o
 * núcleo descontou da sequência (duel_hitstop_for), em qualquer taxa de quadros. */
float hitstop_passo(float *restante, float dt);

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
/* O mestre `index` caiu: marca como vencido e, se é o mestre atual, avança a trilha (no último,
 * marca a trilha como completa). Pode ser chamada mais de uma vez pela mesma vitória: a segunda
 * não avança de novo, porque a trilha já não está mais nesse mestre. */
void campaign_win(Campaign *c, int index);

#endif
