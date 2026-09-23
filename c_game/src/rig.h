/*
 * rig.h - personagens provisórios montados por articulações.
 * Cada pose é um punhado de números; o movimento é a interpolação entre elas,
 * então nunca pula quadro e chega no instante exato pedido.
 * Tudo em pixels de 320 x 180, olhando para a direita (faceLeft espelha).
 */
#ifndef APARA_RIG_H
#define APARA_RIG_H

#include <stdbool.h>

#include "raylib.h"

typedef struct {
    float lean;          /* graus; + inclina para a frente */
    float crouch;        /* px que o quadril desce */
    float handX, handY;  /* mãos em relação ao peito */
    float sword;         /* graus: 0 = para a frente, -90 = para cima, 90 = para baixo */
    float stepF, stepB;  /* deslocamento dos pés da frente e de trás */
    float bodyX;         /* quadril para a frente/trás */
} Pose;

typedef enum { EASE_LINEAR, EASE_IN, EASE_OUT, EASE_INOUT } Ease;

typedef enum { HAT_NONE, HAT_KASA, HAT_KABUTO, HAT_LONG_HAIR, HAT_HOOD } HatKind;

/* Peças extras da roupa, combináveis. */
enum {
    EX_PAULDRONS = 1 << 0,   /* ombreiras */
    EX_CAPE = 1 << 1,        /* capa que balança */
    EX_SCARF = 1 << 2,       /* cachecol com a ponta solta */
    EX_TIE = 1 << 3,         /* gravata */
    EX_BEADS = 1 << 4,       /* contas no peito (ou botões) */
    EX_VISOR = 1 << 5,       /* faixa nos olhos: óculos, visor */
    EX_APRON = 1 << 6,       /* avental */
};

typedef struct {
    Color coat, sleeve, pants, skin, hair, blade, band;
    HatKind hat;
    float size;          /* 1 = ~46 px de altura */
    float bladeLen;
    bool headband;       /* faixa na testa com a ponta solta */
    bool hairTail;       /* cabelo longo que balança atrás (ren) */
    /* Roupa. */
    float robe;          /* comprimento do casaco: 0 = quadril, 1 = joelho, 2 = tornozelo */
    float flare;         /* abertura da barra (vestido, hakama) */
    float pantsWidth;    /* 1 = justa; 1,6 = hakama */
    Color trim;          /* acabamento na borda do casaco (alfa 0 = sem) */
    Color extra;         /* cor das peças extras */
    unsigned extras;     /* EX_* */
    /* Katana. */
    Color handle;        /* cor do cabo (alfa 0 = escuro padrão) */
    float bladeWidth;    /* 1 = normal */
} Look;

typedef struct {
    Look look;
    float x, y;          /* pé de apoio (chão) */
    float offsetX;       /* recuo e avanço, separado da pose */
    bool faceLeft;
    Pose from, to, cur;
    float t, dur;
    Ease ease;
    Pose next;
    float nextDur;
    Ease nextEase;
    bool hasNext;
    float time;          /* relógio próprio (respiração) */
    float breath;        /* 0..1: quanto respira */
    float shiver;        /* tremor da tensão (px) */
    float hopY;
    /* Movimento secundário: ponta da faixa e barra do casaco. */
    Vector2 band[4], bandVel[4];
    Vector2 cape[5], capeVel[5];
    float hem, hemVel;
    float lastX;
    /* Rastro da lâmina (smear). */
    Vector2 tip[8], hilt[8];
    int tipCount;
    bool trail;
    bool noSword;        /* desarmado: a espada voou */
    bool hideBlade;      /* a espada é desenhada por fora (katana 3D) */
    float flash;
    Color flashColor;
} Rig;

extern const Pose POSE_IDLE, POSE_WINDUP, POSE_CONTACT, POSE_FOLLOW, POSE_FEINT, POSE_PARRY,
    POSE_DEFLECT, POSE_HURT, POSE_STAGGER, POSE_FALLEN, POSE_DASH, POSE_SHEATHE,
    POSE_DISARMED, POSE_KNEEL, POSE_POINT, POSE_WINDUP_LOW, POSE_WINDUP_THRUST, POSE_CONTACT_LOW,
    POSE_CONTACT_THRUST, POSE_REARM_HIGH, POSE_REARM_LOW, POSE_PARRY_LOW, POSE_PARRY_THRUST;

/* Esqueleto no mundo, sem arredondar: usado para desenhar o corpo em 3D. */
typedef struct {
    Vector2 footB, footF, hip, kneeB, kneeF, chest, head, shoulderB, shoulderF, hands, elbowB, elbowF;
    Vector2 hemF, hemB;  /* barra do casaco, frente e trás */
    float s;             /* escala do corpo */
    float lean;          /* inclinação do tronco em graus (já espelhada) */
} RigBones;

void rig_init(Rig *r, const Look *look, float x, float y, bool faceLeft);
void rig_bones(const Rig *r, RigBones *out);
void rig_pose(Rig *r, Pose p, float dur, Ease e);        /* sai da pose atual */
void rig_then(Rig *r, Pose p, float dur, Ease e);        /* encadeia depois da atual */
bool rig_busy(const Rig *r);
void rig_update(Rig *r, float dt);
void rig_draw(const Rig *r, Color light);
void rig_draw_flat(const Rig *r, Color c);               /* silhueta chapada */
Vector2 rig_sword_mid(const Rig *r);                     /* meio da lâmina, para faíscas */
void rig_sword_line(const Rig *r, Vector2 *hilt, Vector2 *tip); /* empunhadura e ponta, no mundo */

#endif
