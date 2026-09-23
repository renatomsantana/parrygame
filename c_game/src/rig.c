/*
 * rig.c - samurai provisório por articulações, com poses interpoladas,
 * movimento secundário (faixa e barra do casaco) e rastro da lâmina.
 */
#include "rig.h"

#include <math.h>
#include <string.h>

#define DEG (3.14159265f / 180.0f)

/*                     lean crouch handX handY sword  stepF stepB bodyX */
const Pose POSE_IDLE    = {6, 2, 7, 3, -38, 0, 0, 0};
const Pose POSE_WINDUP  = {-6, 3, 0, -11, -150, 2, -2, -2};
const Pose POSE_FEINT   = {8, 3, 6, -8, -80, 3, 0, 1};
const Pose POSE_CONTACT = {16, 5, 10, -3, -20, 7, 2, 5};
const Pose POSE_FOLLOW  = {20, 6, 10, 5, 30, 7, 2, 6};
const Pose POSE_PARRY   = {4, 4, 8, -4, -75, 1, -1, 0};
const Pose POSE_DEFLECT = {2, 3, 7, -8, -110, 0, -2, -1};
const Pose POSE_HURT    = {-18, 3, 3, 3, 35, -2, -4, -4};
const Pose POSE_STAGGER = {18, 9, 6, 6, 60, 3, -3, 0};
const Pose POSE_FALLEN  = {45, 14, 8, 10, 85, 6, -4, 2};
const Pose POSE_DASH    = {28, 8, 12, 2, 5, 10, -6, 6};
const Pose POSE_SHEATHE = {2, 1, 2, 6, 120, 0, 0, 0};
const Pose POSE_DISARMED = {-22, 3, -2, -12, -120, -2, -5, -4};
const Pose POSE_KNEEL   = {22, 12, 7, 9, 80, 5, -5, 1};
const Pose POSE_POINT   = {8, 3, 11, -2, 2, 6, 0, 3};
/* Preparações e contatos de cada tipo de golpe, e as defesas de Ren para cada um. */
const Pose POSE_WINDUP_LOW    = {-2, 6, 5, 9, 165, 2, -4, -2};
const Pose POSE_WINDUP_THRUST = {-8, 4, 1, 1, -5, 3, -3, -3};
const Pose POSE_CONTACT_LOW   = {16, 7, 11, 5, 12, 8, 2, 5};
const Pose POSE_CONTACT_THRUST = {20, 5, 13, -2, -5, 9, 2, 7};
const Pose POSE_REARM_HIGH    = {4, 4, 2, -9, -120, 4, 0, 2};
const Pose POSE_REARM_LOW     = {6, 6, 6, 8, 160, 4, -1, 2};
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

bool rig_busy(const Rig *r) { return r->t < 1 || r->hasNext; }

/* ------------------------------------------------------------------ */
/* Geometria                                                           */
/* ------------------------------------------------------------------ */

typedef struct {
    Vector2 footB, footF, hip, kneeB, kneeF, chest, head, shoulder, hands, elbowB, elbowF;
    Vector2 hiltEnd, guard, bladeStart, tip;
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

static Pose breathing(const Rig *r) {
    Pose p = r->cur;
    float b = sinf(r->time * 2.4f) * r->breath;
    p.crouch += b * 0.6f;
    p.handY += sinf(r->time * 2.4f + 0.6f) * 0.5f * r->breath;
    p.sword += b * 1.5f;
    return p;
}

/* Esqueleto em coordenadas locais (olhando para a direita, origem no chão). */
static Skeleton build(const Rig *r) {
    Pose p = breathing(r);
    float s = r->look.size;
    Skeleton k;
    float a = p.lean * DEG;
    k.footB = v2(-8 * s + p.stepB, 0);
    k.footF = v2(9 * s + p.stepF, 0);
    k.hip = v2(p.bodyX, -21 * s + p.crouch);
    k.kneeB = ik(k.hip, k.footB, 11 * s, 11 * s, -1);
    k.kneeF = ik(k.hip, k.footF, 11 * s, 11 * s, -1);
    k.chest = add(k.hip, v2(14 * s * sinf(a), -14 * s * cosf(a)));
    k.head = add(k.hip, v2(21 * s * sinf(a * 1.1f), -21 * s * cosf(a * 1.1f)));
    k.shoulder = add(k.chest, v2(-1 * s, 1));
    k.hands = add(k.chest, v2(p.handX * s, p.handY * s));
    /* Cotovelo dobra sempre para o mesmo lado; as poses nunca levam as mãos para trás do ombro. */
    k.elbowB = ik(add(k.shoulder, v2(-2 * s, 0)), k.hands, 7.5f * s, 7.5f * s, 1);
    k.elbowF = ik(add(k.shoulder, v2(1 * s, 0)), k.hands, 7.5f * s, 7.5f * s, 1);
    Vector2 dir = v2(cosf(p.sword * DEG), sinf(p.sword * DEG));
    k.hiltEnd = add(k.hands, scl(dir, -4 * s));
    k.guard = add(k.hands, scl(dir, 1.5f * s));
    k.bladeStart = add(k.hands, scl(dir, 2 * s));
    k.tip = add(k.hands, scl(dir, (2 + r->look.bladeLen) * s));
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

/* ------------------------------------------------------------------ */
/* Desenho                                                             */
/* ------------------------------------------------------------------ */

typedef struct {
    const Rig *r;
    bool flat, reflect;
    Color flatColor, light;
    float alpha;
} Paint;

static Color col(const Paint *pt, Color c) {
    Color o = pt->flat ? pt->flatColor
                       : (Color){(unsigned char)(c.r * pt->light.r / 255), (unsigned char)(c.g * pt->light.g / 255),
                                 (unsigned char)(c.b * pt->light.b / 255), c.a};
    if (!pt->flat && pt->r->flash > 0) {
        float f = fminf(1, pt->r->flash);
        Color w = pt->r->flashColor;
        o.r = (unsigned char)(o.r + (w.r - o.r) * f);
        o.g = (unsigned char)(o.g + (w.g - o.g) * f);
        o.b = (unsigned char)(o.b + (w.b - o.b) * f);
    }
    o.a = (unsigned char)(o.a * pt->alpha);
    return o;
}

static Vector2 P(const Paint *pt, Vector2 l) {
    const Rig *r = pt->r;
    float jit = r->shiver > 0 ? sinf(r->time * 70) * r->shiver : 0;
    float ox = r->x + r->offsetX + jit, oy = r->y - (pt->reflect ? -r->hopY : r->hopY);
    return v2(roundf(ox + (r->faceLeft ? -l.x : l.x)), roundf(oy + (pt->reflect ? -l.y : l.y)));
}

static void limb(const Paint *pt, Vector2 a, Vector2 b, float w, Color c) {
    Vector2 A = P(pt, a), B = P(pt, b);
    Color k = col(pt, c);
    DrawLineEx(A, B, w, k);
    DrawCircleV(A, w / 2, k);
    DrawCircleV(B, w / 2, k);
}

static void tri(Vector2 a, Vector2 b, Vector2 c, Color k) {
    float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (cross < 0) DrawTriangle(a, b, c, k); else DrawTriangle(a, c, b, k);
}

static void quad(const Paint *pt, Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color k) {
    Vector2 A = P(pt, a), B = P(pt, b), C = P(pt, c), D = P(pt, d);
    Color kk = col(pt, k);
    tri(A, B, C, kk);
    tri(A, C, D, kk);
}

static void circle(const Paint *pt, Vector2 c, float rad, Color k) { DrawCircleV(P(pt, c), rad, col(pt, k)); }

static Color darker(Color c, float k) {
    return (Color){(unsigned char)(c.r * k), (unsigned char)(c.g * k), (unsigned char)(c.b * k), c.a};
}

static Color lighter(Color c, float k) {
    return (Color){(unsigned char)fminf(255, c.r * k + 12), (unsigned char)fminf(255, c.g * k + 12), (unsigned char)fminf(255, c.b * k + 12), c.a};
}

/* Cabelo de musashi: volume até o ombro atrás, mecha atrás da orelha e franja cheia na testa. */
static void draw_shoulder_hair(const Paint *pt, Vector2 h, float s, Color hair) {
    limb(pt, add(h, v2(-2.6f * s, -1.0f * s)), add(h, v2(-3.2f * s, 4.2f * s)), 4.2f * s, hair);
    limb(pt, add(h, v2(-1.2f * s, -0.4f * s)), add(h, v2(-1.5f * s, 3.4f * s)), 1.4f * s, hair);
    circle(pt, add(h, v2(-1.2f * s, -1.9f * s)), 3.6f * s, hair);
    /* Franja: cinco mechas, as do meio mais longas, sem cobrir o olho. */
    static const float tipY[5] = {-1.9f, -1.3f, -1.1f, -1.4f, -2.0f};
    for (int i = 0; i < 5; i++) {
        float x = -0.8f + i * 1.0f;
        Vector2 a = P(pt, add(h, v2((x - 0.7f) * s, -3.9f * s))), b = P(pt, add(h, v2((x + 0.7f) * s, -3.9f * s)));
        Vector2 c = P(pt, add(h, v2((x + 0.3f) * s, tipY[i] * s)));
        tri(a, b, c, col(pt, hair));
    }
    limb(pt, add(h, v2(-2.8f * s, -3.5f * s)), add(h, v2(0.2f * s, -4.4f * s)), 1, lighter(hair, 2.2f)); /* brilho */
}

static void draw_head(const Paint *pt, const Skeleton *k) {
    const Look *L = &pt->r->look;
    float s = L->size;
    Vector2 h = k->head;
    switch (L->hat) {
        case HAT_HOOD:
            circle(pt, h, 4.6f * s, L->hair);
            limb(pt, add(h, v2(0.5f * s, -0.5f)), add(h, v2(3.5f * s, -0.5f)), 1, L->skin);
            break;
        default:
            circle(pt, h, 4.2f * s, L->skin);
            circle(pt, add(h, v2(0.6f * s, 1.6f * s)), 3.2f * s, darker(L->skin, 0.9f));   /* sombra do queixo */
            circle(pt, add(h, v2(0.9f * s, 0.6f * s)), 3.2f * s, L->skin);
            DrawPixelV(P(pt, add(h, v2(2.2f * s, -1.3f * s))), col(pt, darker(L->hair, 0.9f))); /* sobrancelha */
            DrawPixelV(P(pt, add(h, v2(2.9f * s, -1.3f * s))), col(pt, darker(L->hair, 0.9f)));
            if (L->hairTail) {
                draw_shoulder_hair(pt, h, s, L->hair);
                DrawPixelV(P(pt, add(h, v2(2.4f * s, -0.1f * s))), col(pt, (Color){20, 16, 20, 255}));
                DrawPixelV(P(pt, add(h, v2(3.2f * s, 1.6f * s))), col(pt, darker(L->skin, 0.8f)));
                break;
            }
            /* Cabelo: metade de trás e topo. */
            circle(pt, add(h, v2(-1.2f * s, -1.4f * s)), 3.6f * s, L->hair);
            if (L->hat == HAT_NONE) circle(pt, add(h, v2(-2.5f * s, -4.5f * s)), 1.6f * s, L->hair); /* coque */
            if (L->hat == HAT_LONG_HAIR && !L->hairTail) limb(pt, add(h, v2(-3 * s, 0)), add(h, v2(-5 * s, 10 * s)), 3 * s, L->hair);
            /* Olho. */
            DrawPixelV(P(pt, add(h, v2(2.4f * s, -0.3f * s))), col(pt, (Color){20, 16, 20, 255}));
            break;
    }
    if (L->hat == HAT_KASA) {
        Vector2 a = add(h, v2(-9 * s, -1.5f * s)), b = add(h, v2(9 * s, -1.5f * s)), c = add(h, v2(0, -7 * s));
        Vector2 A = P(pt, a), B = P(pt, b), C = P(pt, c);
        tri(A, B, C, col(pt, (Color){196, 160, 90, 255}));
        DrawLineV(A, B, col(pt, (Color){120, 90, 50, 255}));
    }
    if (L->hat == HAT_KABUTO) {
        circle(pt, add(h, v2(-0.5f * s, -1.5f * s)), 4.6f * s, darker(L->coat, 0.6f));
        limb(pt, add(h, v2(0, -5 * s)), add(h, v2(4 * s, -9 * s)), 1, (Color){210, 180, 90, 255});
        limb(pt, add(h, v2(0, -5 * s)), add(h, v2(-4 * s, -9 * s)), 1, (Color){210, 180, 90, 255});
    }
    if (L->headband) {
        limb(pt, add(h, v2(-4 * s, -1.5f * s)), add(h, v2(4 * s, -1.5f * s)), 1.5f * s, L->band);
    }
}

/* Pé: tabi escuro em cunha (calcanhar alto, dedos baixos), sola e a tira da sandália. */
static void draw_foot(const Paint *pt, Vector2 f, float s, float shade) {
    Color tabi = darker((Color){74, 62, 54, 255}, shade), sole = darker((Color){34, 24, 18, 255}, shade);
    quad(pt, add(f, v2(-1.4f * s, -2.2f * s)), add(f, v2(1.0f * s, -2.4f * s)), add(f, v2(4.0f * s, -0.6f * s)), add(f, v2(-1.4f * s, -0.4f * s)), tabi);
    limb(pt, add(f, v2(-1.6f * s, 0)), add(f, v2(4.2f * s, 0)), 1, sole);
    limb(pt, add(f, v2(0.6f * s, -1.9f * s)), add(f, v2(1.6f * s, -0.6f * s)), 1, darker((Color){170, 150, 120, 255}, shade));
}

static void draw_body(const Paint *pt) {
    const Rig *r = pt->r;
    const Look *L = &r->look;
    float s = L->size;
    Skeleton k = build(r);
    Color back = darker(L->pants, 0.7f), backSleeve = darker(L->sleeve, 0.7f);

    /* Faixa solta atrás de tudo. */
    if (L->headband && !pt->reflect) {
        Color bc = col(pt, L->band);
        for (int i = 0; i < 3; i++) DrawLineEx(r->band[i], r->band[i + 1], 1.5f * s, bc);
    }
    /* Cabelo até o ombro: a ponta solta balança um pouco atrás do pescoço. */
    if (L->hairTail && !pt->reflect) {
        Color hc = col(pt, L->hair);
        for (int i = 0; i < 2; i++) {
            float w = (3.4f - i * 1.0f) * s;
            DrawLineEx(r->band[i], r->band[i + 1], w, hc);
            DrawCircleV(r->band[i + 1], w / 2, hc);
        }
    }
    /* Capa e cachecol ficam atrás de tudo. */
    if ((L->extras & (EX_CAPE | EX_SCARF)) && !pt->flat) {
        bool cape = L->extras & EX_CAPE;
        Color cc = col(pt, L->extra);
        for (int i = 0; i < 4; i++) {
            float w = cape ? (7.5f - i * 0.8f) * s : (2.6f - i * 0.4f) * s;
            DrawLineEx(r->cape[i], r->cape[i + 1], w, cc);
            DrawCircleV(r->cape[i + 1], w / 2, cc);
        }
    }
    float pw = L->pantsWidth;
    /* Perna e braço de trás. */
    limb(pt, k.hip, k.kneeB, 4 * s * pw, back);
    limb(pt, k.kneeB, k.footB, 3.5f * s * (pw > 1 ? pw * 1.1f : 1), back);
    draw_foot(pt, k.footB, s, 0.7f);
    limb(pt, add(k.shoulder, v2(-2 * s, 0)), k.elbowB, 3 * s, backSleeve);
    limb(pt, k.elbowB, k.hands, 2.5f * s, backSleeve);
    /* Perna da frente. */
    limb(pt, k.hip, k.kneeF, 4 * s * pw, L->pants);
    limb(pt, add(k.hip, v2(1.4f * s, 0.5f * s)), add(k.kneeF, v2(1 * s, -0.5f * s)), 1, lighter(L->pants, 1.25f)); /* luz na coxa */
    limb(pt, k.kneeF, k.footF, 3.5f * s * (pw > 1 ? pw * 1.1f : 1), L->pants);
    if (pw <= 1) /* faixas amarradas na canela */
        for (int i = 1; i <= 2; i++) {
            Vector2 c = v2(k.kneeF.x + (k.footF.x - k.kneeF.x) * (0.35f + i * 0.2f), k.kneeF.y + (k.footF.y - k.kneeF.y) * (0.35f + i * 0.2f));
            limb(pt, add(c, v2(-1.6f * s, 0)), add(c, v2(1.6f * s, -0.4f * s)), 1, lighter(L->pants, 1.5f));
        }
    draw_foot(pt, k.footF, s, 1);
    /* Tronco e casaco. */
    quad(pt, k.coat[0], k.coat[1], k.coat[2], k.coat[3], L->coat);
    limb(pt, k.chest, k.hip, 7 * s, L->coat);
    /* Volume: metade de baixo do casaco na sombra, lado de trás do tronco mais escuro, frente com luz. */
    Vector2 waistB = add(k.hip, v2(-6.5f * s, 2 * s)), waistF = add(k.hip, v2(5.5f * s, 2 * s));
    quad(pt, waistB, waistF, k.coat[2], k.coat[3], darker(L->coat, 0.84f));
    limb(pt, add(k.chest, v2(-3 * s, 0.5f * s)), add(k.hip, v2(-3.2f * s, 0)), 1.6f * s, darker(L->coat, 0.88f));
    /* Dobra no meio do casaco, borda da frente iluminada e gola da camada de baixo. */
    if (L->trim.a > 0) {
        limb(pt, k.coat[1], k.coat[2], 1, L->trim);
        limb(pt, k.coat[2], k.coat[3], 1, L->trim);
    }
    /* Cruzado do quimono: a gola clara da camada de baixo e a borda da lapela descendo até a faixa. */
    limb(pt, add(k.chest, v2(-1.4f * s, -1.6f * s)), add(k.chest, v2(2.4f * s, 2.4f * s)), 1.2f * s, (Color){226, 214, 190, 255});
    limb(pt, add(k.chest, v2(-1.8f * s, -1.2f * s)), add(k.hip, v2(2.6f * s, -0.5f * s)), 1, darker(L->coat, 0.6f));
    if (L->extras & EX_APRON)
        quad(pt, add(k.chest, v2(-1 * s, 2 * s)), add(k.chest, v2(4.5f * s, 2 * s)), add(k.hip, v2(6 * s, 11 * s)), add(k.hip, v2(-1 * s, 11 * s)), L->extra);
    limb(pt, add(k.hip, v2(-5 * s, 0)), add(k.hip, v2(5 * s, 0)), 2.6f * s, darker(L->coat, 0.4f)); /* faixa da cintura (obi) */
    limb(pt, add(k.hip, v2(-5 * s, -0.8f * s)), add(k.hip, v2(5 * s, -0.8f * s)), 1, darker(L->coat, 0.62f));
    if (L->extras & EX_TIE) limb(pt, add(k.chest, v2(2.5f * s, -2 * s)), add(k.chest, v2(3 * s, 5 * s)), 1.2f * s, L->extra);
    if (L->extras & EX_BEADS)
        for (int i = 0; i < 4; i++) circle(pt, add(k.chest, v2(3.2f * s - i * 0.3f * s, -1.5f * s + i * 2.2f * s)), 0.9f * s, L->extra);
    circle(pt, add(k.head, v2(0.4f * s, 3.8f * s)), 1.8f * s, darker(L->skin, 0.78f)); /* pescoço na sombra */
    draw_head(pt, &k);
    if (L->extras & EX_VISOR) limb(pt, add(k.head, v2(0.5f * s, -0.4f * s)), add(k.head, v2(4.2f * s, -0.4f * s)), 1.3f * s, L->extra);
    if (L->extras & EX_PAULDRONS) {
        quad(pt, add(k.shoulder, v2(-4.5f * s, -2 * s)), add(k.shoulder, v2(3.5f * s, -2 * s)), add(k.shoulder, v2(4 * s, 2.5f * s)),
             add(k.shoulder, v2(-5 * s, 2.5f * s)), L->extra);
        limb(pt, add(k.shoulder, v2(-5 * s, 2.5f * s)), add(k.shoulder, v2(4 * s, 2.5f * s)), 1, darker(L->extra, 0.6f));
    }
    /* Espada. */
    if (!r->noSword && !r->hideBlade) {
        limb(pt, k.hiltEnd, k.guard, 2 * s, (Color){50, 30, 30, 255});
        DrawCircleV(P(pt, k.guard), 1.6f * s, col(pt, (Color){170, 140, 70, 255}));
        DrawLineEx(P(pt, k.bladeStart), P(pt, k.tip), 1.5f * s, col(pt, L->blade));
    }
    /* Braço da frente por cima da empunhadura. */
    limb(pt, add(k.shoulder, v2(1 * s, 0)), k.elbowF, 3 * s, L->sleeve);
    limb(pt, k.elbowF, k.hands, 2.5f * s, L->sleeve);
    Vector2 cuff = v2(k.elbowF.x + (k.hands.x - k.elbowF.x) * 0.72f, k.elbowF.y + (k.hands.y - k.elbowF.y) * 0.72f);
    limb(pt, cuff, k.hands, 2.7f * s, darker(L->sleeve, 0.72f));
    circle(pt, k.hands, 1.5f * s, L->skin);
}

static void draw_trail(const Rig *r, Color c) {
    if (r->tipCount < 2) return;
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i + 1 < r->tipCount; i++) {
        float a = 1 - (float)i / r->tipCount;
        Color k = c;
        k.a = (unsigned char)(170 * a);
        tri(r->hilt[i], r->tip[i], r->tip[i + 1], k);
        tri(r->hilt[i], r->tip[i + 1], r->hilt[i + 1], k);
    }
    EndBlendMode();
}

void rig_draw(const Rig *r, Color light) {
    Paint pt = {r, false, false, WHITE, light, 1};
    draw_trail(r, (Color){255, 245, 220, 255});
    draw_body(&pt);
}

void rig_draw_flat(const Rig *r, Color c) {
    Paint pt = {r, true, false, c, WHITE, c.a / 255.0f};
    pt.flatColor.a = 255;
    draw_body(&pt);
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

static Vector2 to_world_f(const Rig *r, Vector2 l) {
    float jit = r->shiver > 0 ? sinf(r->time * 70) * r->shiver : 0;
    float ox = r->x + r->offsetX + jit, oy = r->y - r->hopY;
    return v2(ox + (r->faceLeft ? -l.x : l.x), oy + l.y);
}

void rig_bones(const Rig *r, RigBones *o) {
    Skeleton k = build(r);
    float s = r->look.size;
    o->footB = to_world_f(r, k.footB);
    o->footF = to_world_f(r, k.footF);
    o->hip = to_world_f(r, k.hip);
    o->kneeB = to_world_f(r, k.kneeB);
    o->kneeF = to_world_f(r, k.kneeF);
    o->chest = to_world_f(r, k.chest);
    o->head = to_world_f(r, k.head);
    o->shoulderB = to_world_f(r, add(k.shoulder, v2(-2 * s, 0)));
    o->shoulderF = to_world_f(r, add(k.shoulder, v2(1 * s, 0)));
    o->hands = to_world_f(r, k.hands);
    o->elbowB = to_world_f(r, k.elbowB);
    o->elbowF = to_world_f(r, k.elbowF);
    o->hemF = to_world_f(r, k.coat[2]);
    o->hemB = to_world_f(r, k.coat[3]);
    o->s = s;
    o->lean = r->faceLeft ? -r->cur.lean : r->cur.lean;
}
