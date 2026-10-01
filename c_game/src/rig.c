/*
 * rig.c - poses interpoladas e pontos de referência para golpes e efeitos.
 */
#include "rig.h"

#include <math.h>
#include <string.h>

#define DEG (3.14159265f / 180.0f)

/*                     lean crouch handX handY sword  stepF stepB bodyX */
const Pose POSE_IDLE    = {6, 2, 7, 3, -38, 0, 0, 0};
const Pose POSE_WINDUP  = {-6, 3, 0, -11, -150, 2, -2, -2};
const Pose POSE_CONTACT = {16, 5, 10, -3, -20, 7, 2, 5};
const Pose POSE_FOLLOW  = {20, 6, 10, 5, 30, 7, 2, 6};
const Pose POSE_PARRY   = {4, 4, 8, -4, -75, 1, -1, 0};
const Pose POSE_DEFLECT = {2, 3, 7, -8, -110, 0, -2, -1};
const Pose POSE_HURT    = {-18, 3, 3, 3, 35, -2, -4, -4};
const Pose POSE_STAGGER = {18, 9, 6, 6, 60, 3, -3, 0};
const Pose POSE_FALLEN  = {45, 14, 8, 10, 85, 6, -4, 2};
const Pose POSE_DISARMED = {-22, 3, -2, -12, -120, -2, -5, -4};
const Pose POSE_KNEEL   = {22, 12, 7, 9, 80, 5, -5, 1};
const Pose POSE_POINT   = {8, 3, 11, -2, 2, 6, 0, 3};
/* Preparações e contatos de cada tipo de golpe, e as defesas de Ren para cada um. */
const Pose POSE_WINDUP_LOW    = {-6, 8, 4, 10, 62, 0, -6, -3};
const Pose POSE_WINDUP_THRUST = {-10, 5, 0, -3, -18, 0, -6, -4};
const Pose POSE_CONTACT_LOW   = {18, 7, 13, 1, -6, 10, 1, 6};
const Pose POSE_CONTACT_THRUST = {22, 6, 14, -1, -3, 11, 2, 8};
const Pose POSE_REARM_HIGH    = {4, 4, 2, -9, -120, 4, 0, 2};
const Pose POSE_REARM_LOW     = {4, 8, 5, 9, 58, 5, -2, 1};
const Pose POSE_PARRY_LOW     = {6, 6, 9, 4, 40, 1, -1, 0};
const Pose POSE_PARRY_THRUST  = {4, 4, 9, -2, -35, 1, -1, 0};

/* ------------------------------------------------------------------ */

static float ease(Ease e, float t) {
    t = t < 0 ? 0 : (t > 1 ? 1 : t);
    switch (e) {
        case EASE_IN: return t * t * t;
        case EASE_OUT: return 1 - (1 - t) * (1 - t) * (1 - t);
        case EASE_INOUT: return t < 0.5f ? 4 * t * t * t : 1 - powf(-2 * t + 2, 3) / 2;
        default: return t;
    }
}

static Pose lerp_pose(Pose a, Pose b, float k) {
    Pose p;
    p.lean = a.lean + (b.lean - a.lean) * k;
    p.crouch = a.crouch + (b.crouch - a.crouch) * k;
    p.handX = a.handX + (b.handX - a.handX) * k;
    p.handY = a.handY + (b.handY - a.handY) * k;
    p.sword = a.sword + (b.sword - a.sword) * k;
    p.stepF = a.stepF + (b.stepF - a.stepF) * k;
    p.stepB = a.stepB + (b.stepB - a.stepB) * k;
    p.bodyX = a.bodyX + (b.bodyX - a.bodyX) * k;
    return p;
}

void rig_init(Rig *r, const Look *look, float x, float y, bool faceLeft) {
    memset(r, 0, sizeof *r);
    r->look = *look;
    r->x = r->lastX = x;
    r->y = y;
    r->faceLeft = faceLeft;
    r->from = r->to = r->cur = POSE_IDLE;
    r->dur = 0;
    r->t = 1;
    r->breath = 1;
    r->footF = POSE_IDLE.stepF;
    r->footB = POSE_IDLE.stepB;
    for (int i = 0; i < 4; i++) r->band[i] = (Vector2){x, y - 40};
    for (int i = 0; i < 5; i++) r->cape[i] = (Vector2){x, y - 30};
    if (r->look.pantsWidth <= 0) r->look.pantsWidth = 1;
    if (r->look.bladeWidth <= 0) r->look.bladeWidth = 1;
}

void rig_pose(Rig *r, Pose p, float dur, Ease e) {
    r->from = r->cur;
    r->to = p;
    r->t = 0;
    r->dur = dur > 0.0001f ? dur : 0.0001f;
    r->ease = e;
    r->hasNext = false;
}

void rig_then(Rig *r, Pose p, float dur, Ease e) {
    if (r->t >= 1 && !r->hasNext) { rig_pose(r, p, dur, e); return; }
    r->next = p;
    r->nextDur = dur;
    r->nextEase = e;
    r->hasNext = true;
}

/* ------------------------------------------------------------------ */
/* Geometria                                                           */
/* ------------------------------------------------------------------ */

typedef struct {
    Vector2 footB, footF, hip, kneeB, kneeF, chest, head, shoulder, hands, elbowB, elbowF;
    Vector2 hiltEnd, guard, bladeStart, tip;
    Vector2 hands2, offButt, offTip;  /* mão de trás e a lâmina dela */
    Vector2 coat[4];
} Skeleton;

static Vector2 v2(float x, float y) { return (Vector2){x, y}; }
static Vector2 add(Vector2 a, Vector2 b) { return v2(a.x + b.x, a.y + b.y); }
static Vector2 scl(Vector2 a, float k) { return v2(a.x * k, a.y * k); }

static Vector2 ik(Vector2 a, Vector2 b, float l1, float l2, float bend) {
    float dx = b.x - a.x, dy = b.y - a.y;
    float d = sqrtf(dx * dx + dy * dy);
    float lo = fabsf(l1 - l2) + 0.01f, hi = l1 + l2 - 0.01f;
    if (d < 0.001f) return v2(a.x, a.y + l1);
    float dc = d < lo ? lo : (d > hi ? hi : d);
    float k = (l1 * l1 - l2 * l2 + dc * dc) / (2 * dc);
    float h = sqrtf(fmaxf(0, l1 * l1 - k * k));
    Vector2 p = v2(a.x + dx / d * k, a.y + dy / d * k);
    return v2(p.x + bend * h * (-dy / d), p.y + bend * h * (dx / d));
}

/* A postura aparece no corpo: calmo até 40%, respiração curta e guarda mais baixa até 75%,
 * ofegante e com a lâmina tremendo depois disso. */
static Pose breathing(const Rig *r) {
    Pose p = r->cur;
    float f = r->fatigue;
    float tired = f > 0.4f ? fminf(1, (f - 0.4f) / 0.35f) : 0;
    float spent = f > 0.75f ? fminf(1, (f - 0.75f) / 0.25f) : 0;
    float rate = 2.4f + tired * 2.2f + spent * 2.4f, amp = 1 + tired * 0.8f + spent * 1.2f;
    float b = sinf(r->time * rate) * r->breath * amp;
    p.crouch += b * 0.6f + tired * 1.2f * r->breath;
    p.handY += (sinf(r->time * rate + 0.6f) * 0.5f * amp + tired * 1.5f + spent * 1.5f) * r->breath;
    p.lean += spent * 4 * r->breath;
    p.sword += b * 1.5f + sinf(r->time * 43) * spent * 2.5f * r->breath;
    return p;
}

/* Esqueleto em coordenadas locais (olhando para a direita, origem no chão). */
static Skeleton build(const Rig *r) {
    Pose p = breathing(r);
    float s = r->look.size;
    Skeleton k;
    float a = p.lean * DEG;
    k.footB = v2(-8 * s + r->footB, -r->liftB);
    k.footF = v2(9 * s + r->footF, -r->liftF);
    k.hip = v2(p.bodyX, -21 * s + p.crouch);
    k.kneeB = ik(k.hip, k.footB, 11 * s, 11 * s, -1);
    k.kneeF = ik(k.hip, k.footF, 11 * s, 11 * s, -1);
    k.chest = add(k.hip, v2(14 * s * sinf(a), -14 * s * cosf(a)));
    k.head = add(k.hip, v2(21 * s * sinf(a * 1.1f), -21 * s * cosf(a * 1.1f)));
    k.shoulder = add(k.chest, v2(-1 * s, 1));
    k.hands = add(k.chest, v2(p.handX * s, p.handY * s));
    /* Com arma na outra mão, a mão de trás fica mais baixa e recolhida. */
    k.hands2 = r->look.offhand != OFF_NONE ? add(k.chest, v2((p.handX * 0.55f - 2) * s, (p.handY * 0.5f + 5) * s)) : k.hands;
    /* Cotovelo dobra sempre para o mesmo lado; as poses nunca levam as mãos para trás do ombro. */
    k.elbowB = ik(add(k.shoulder, v2(-2 * s, 0)), k.hands2, 7.5f * s, 7.5f * s, 1);
    k.elbowF = ik(add(k.shoulder, v2(1 * s, 0)), k.hands, 7.5f * s, 7.5f * s, 1);
    Vector2 dir = v2(cosf(p.sword * DEG), sinf(p.sword * DEG));
    k.hiltEnd = add(k.hands, scl(dir, -4 * s));
    k.guard = add(k.hands, scl(dir, 1.5f * s));
    k.bladeStart = add(k.hands, scl(dir, 2 * s));
    k.tip = add(k.hands, scl(dir, (2 + r->look.bladeLen) * s));
    if (r->look.weapon != WEAPON_KATANA) k.hiltEnd = add(k.hands, scl(dir, -12 * s)); /* haste longa para trás */
    float offLen = r->look.offhand == OFF_SWORD ? r->look.bladeLen * 0.9f : 9;
    Vector2 odir = v2(cosf((p.sword + 38) * DEG), sinf((p.sword + 38) * DEG));
    k.offButt = add(k.hands2, scl(odir, -3 * s));
    k.offTip = add(k.hands2, scl(odir, (1 + offLen) * s));
    float hem = r->hem;
    k.coat[0] = add(k.chest, v2(-4 * s, -1));
    k.coat[1] = add(k.chest, v2(3.5f * s, -1));
    float len = (7 + r->look.robe * 6.5f) * s, wide = r->look.flare * 6 * s;
    k.coat[2] = add(k.hip, v2(6 * s + wide + hem * 0.3f, len));
    k.coat[3] = add(k.hip, v2(-7 * s - wide + hem, len + s - fabsf(hem) * 0.3f));
    return k;
}

/* ------------------------------------------------------------------ */
/* Atualização                                                         */
/* ------------------------------------------------------------------ */

static Vector2 to_world(const Rig *r, Vector2 l) {
    float ox = r->x + r->offsetX, oy = r->y - r->hopY;
    return v2(roundf(ox + (r->faceLeft ? -l.x : l.x)), roundf(oy + l.y));
}

void rig_update(Rig *r, float dt) {
    r->time += dt;
    if (r->t < 1) {
        r->t += dt / r->dur;
        if (r->t >= 1) {
            r->t = 1;
            r->cur = r->to;
            if (r->hasNext) {
                r->hasNext = false;
                rig_pose(r, r->next, r->nextDur, r->nextEase);
            }
        } else {
            r->cur = lerp_pose(r->from, r->to, ease(r->ease, r->t));
        }
    }
    r->flash = fmaxf(0, r->flash - dt * 5);

    /* Passos: o pé persegue a posição da pose e sobe enquanto está no ar. */
    if (dt > 0) {
        float df = r->cur.stepF - r->footF, db = r->cur.stepB - r->footB;
        float kf = fminf(1, dt * 16), kb = fminf(1, dt * 12);
        r->footF += df * kf;
        r->footB += db * kb;
        r->liftF = fminf(3.5f, fabsf(df) * 0.9f);
        r->liftB = fminf(2.5f, fabsf(db) * 0.7f);
    }

    Skeleton k = build(r);
    /* Barra do casaco: mola puxada pela velocidade do quadril. */
    float hipX = r->x + r->offsetX + (r->faceLeft ? -k.hip.x : k.hip.x);
    float vel = dt > 0 ? (hipX - r->lastX) / dt : 0;
    r->lastX = hipX;
    if (dt > 0) {
        float target = fmaxf(-5, fminf(5, -vel * (r->faceLeft ? -1 : 1) * 0.04f));
        r->hemVel += ((target - r->hem) * 160 - r->hemVel * 14) * dt;
        r->hem += r->hemVel * dt;
    }
    /* Faixa da testa: corrente de quatro pontos com gravidade e vento. */
    if ((r->look.headband || r->look.hairTail) && dt > 0) {
        Vector2 anchor = to_world(r, add(k.head, v2(-3.5f * r->look.size, r->look.hairTail ? 0.5f : -1)));
        r->band[0] = anchor;
        float seg = (r->look.hairTail ? 2.2f : 3) * r->look.size;
        for (int i = 1; i < 4; i++) {
            r->bandVel[i].y += 40 * dt;
            r->bandVel[i].x += ((r->faceLeft ? 1 : -1) * 18 - vel * 0.4f) * dt;
            r->bandVel[i] = scl(r->bandVel[i], expf(-dt * 3));
            r->band[i] = add(r->band[i], scl(r->bandVel[i], dt));
            Vector2 d = v2(r->band[i].x - r->band[i - 1].x, r->band[i].y - r->band[i - 1].y);
            float len = sqrtf(d.x * d.x + d.y * d.y);
            if (len > 0.001f) r->band[i] = add(r->band[i - 1], scl(d, seg / len));
        }
    }
    /* Capa e cachecol: corrente de cinco pontos presa às costas ou ao pescoço. */
    if ((r->look.extras & (EX_CAPE | EX_SCARF)) && dt > 0) {
        bool cape = r->look.extras & EX_CAPE;
        Vector2 at = cape ? add(k.chest, v2(-4 * r->look.size, -2 * r->look.size)) : add(k.chest, v2(-1 * r->look.size, -3 * r->look.size));
        r->cape[0] = to_world(r, at);
        float seg = (cape ? 6.0f : 3.5f) * r->look.size;
        float gust = 0.8f + 0.2f * sinf(r->time * 1.7f);
        for (int i = 1; i < 5; i++) {
            r->capeVel[i].y += (cape ? 40 : 30) * dt;
            r->capeVel[i].x += ((r->faceLeft ? 1 : -1) * (cape ? 34 : 22) * gust - vel * 0.5f) * dt;
            r->capeVel[i] = scl(r->capeVel[i], expf(-dt * 3));
            r->cape[i] = add(r->cape[i], scl(r->capeVel[i], dt));
            Vector2 d = v2(r->cape[i].x - r->cape[i - 1].x, r->cape[i].y - r->cape[i - 1].y);
            float l = sqrtf(d.x * d.x + d.y * d.y);
            if (l > 0.001f) r->cape[i] = add(r->cape[i - 1], scl(d, seg / l));
        }
    }
    /* Rastro da lâmina. */
    if (r->trail) {
        memmove(&r->tip[1], &r->tip[0], sizeof(Vector2) * 7);
        memmove(&r->hilt[1], &r->hilt[0], sizeof(Vector2) * 7);
        r->tip[0] = to_world(r, k.tip);
        r->hilt[0] = to_world(r, k.bladeStart);
        if (r->tipCount < 8) r->tipCount++;
    } else if (r->tipCount > 0) {
        r->tipCount--;
    }
}

Vector2 rig_sword_mid(const Rig *r) {
    Skeleton k = build(r);
    Vector2 mid = v2(k.bladeStart.x * 0.35f + k.tip.x * 0.65f, k.bladeStart.y * 0.35f + k.tip.y * 0.65f);
    return to_world(r, mid);
}

void rig_sword_line(const Rig *r, Vector2 *hilt, Vector2 *tip) {
    Skeleton k = build(r);
    *hilt = to_world(r, k.hiltEnd);
    *tip = to_world(r, k.tip);
}

bool rig_offhand_line(const Rig *r, Vector2 *hilt, Vector2 *tip) {
    if (r->look.offhand != OFF_DAGGER && r->look.offhand != OFF_SWORD) return false;
    if (r->noSword) return false;
    Skeleton k = build(r);
    *hilt = to_world(r, k.offButt);
    *tip = to_world(r, k.offTip);
    return true;
}
