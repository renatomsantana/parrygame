/*
 * fx.c - efeitos de impacto, em pixels de 320 x 180.
 */
#include "fx.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

float frand(float lo, float hi) { return lo + (hi - lo) * (float)rand() / (float)RAND_MAX; }

static Color lerp_color(Color a, Color b, float t) {
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t), (unsigned char)(a.a + (b.a - a.a) * t)};
}

void fx_init(Fx *fx) {
    memset(fx, 0, sizeof *fx);
    fx->zoom = 1;
    fx->shakeEnabled = true;
}

void fx_clear(Fx *fx) {
    bool shake = fx->shakeEnabled;
    fx_init(fx);
    fx->shakeEnabled = shake;
}

void fx_update(Fx *fx, float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &fx->p[i];
        if (!p->alive) continue;
        p->life -= dt;
        if (p->life <= 0) { p->alive = false; continue; }
        float d = expf(-p->drag * dt);
        p->vel.x *= d;
        p->vel.y = p->vel.y * d + p->gravity * dt;
        p->pos.x += p->vel.x * dt;
        p->pos.y += p->vel.y * dt;
        p->angle += p->spin * dt;
    }
    for (int i = 0; i < MAX_RINGS; i++) {
        Ring *r = &fx->rings[i];
        if (r->life <= 0) continue;
        r->life -= dt;
        r->radius += r->speed * dt;
        r->speed *= expf(-4 * dt);
    }
    for (int i = 0; i < MAX_POPUPS; i++) if (fx->popups[i].life > 0) {
        fx->popups[i].life -= dt;
        fx->popups[i].pos.y -= 7 * dt;
    }
    for (int i = 0; i < MAX_ARCS; i++) if (fx->arcs[i].life > 0) fx->arcs[i].life -= dt;
    fx->flash = fmaxf(0, fx->flash - dt * 3.5f);
    if (fx->shakeTime > 0) fx->shakeTime -= dt; else fx->shake = 0;
    /* Mola crítica para o soco de câmera. */
    float k = 180, c = 22;
    float acc = -k * (fx->zoom - 1) - c * fx->zoomVel;
    fx->zoomVel += acc * dt;
    fx->zoom += fx->zoomVel * dt;
    fx->vignettePulse = fmaxf(0, fx->vignettePulse - dt * 2);
}

static Particle *spawn(Fx *fx) {
    for (int i = 0; i < MAX_PARTICLES; i++) if (!fx->p[i].alive) return &fx->p[i];
    return &fx->p[rand() % MAX_PARTICLES];
}

void fx_burst(Fx *fx, ParticleKind kind, Vector2 at, int count, float speed, float spread, float dir, Color a, Color b) {
    for (int i = 0; i < count; i++) {
        Particle *p = spawn(fx);
        float ang = dir + frand(-spread, spread);
        float sp = speed * frand(0.35f, 1.0f);
        p->kind = kind;
        p->pos = at;
        p->vel = (Vector2){cosf(ang) * sp, sinf(ang) * sp};
        p->maxLife = p->life = frand(0.25f, 0.6f);
        p->size = frand(1, 2.2f);
        p->drag = 3;
        p->gravity = 600;
        p->spin = frand(-10, 10);
        p->angle = frand(0, 6.28f);
        p->color = lerp_color(a, b, frand(0, 1));
        p->alive = true;
        switch (kind) {
            case P_SPARK: p->gravity = 230; p->drag = 2.5f; break;
            case P_EMBER: p->gravity = -15; p->drag = 1; p->maxLife = p->life = frand(0.8f, 1.6f); break;
            case P_DUST: p->gravity = -6; p->drag = 4; p->size = frand(1, 3); p->maxLife = p->life = frand(0.4f, 0.9f); break;
            case P_SHARD: p->gravity = 280; p->drag = 1; p->size = frand(1.5f, 3); p->maxLife = p->life = frand(0.6f, 1.1f); break;
            case P_PETAL: p->gravity = 10; p->drag = 1.5f; p->size = frand(1, 2); p->maxLife = p->life = frand(1.2f, 2.2f); break;
            case P_GEM: p->gravity = 0; p->drag = 2; p->size = frand(1, 2); p->maxLife = p->life = frand(0.6f, 1.2f); break;
        }
    }
}

void fx_ring(Fx *fx, Vector2 at, float speed, float life, float width, Color c) {
    for (int i = 0; i < MAX_RINGS; i++) {
        if (fx->rings[i].life > 0) continue;
        fx->rings[i] = (Ring){at, 2, speed, life, life, width, c};
        return;
    }
}

void fx_arc(Fx *fx, Vector2 center, float radius, float start, float sweep, float life, float width, Color c) {
    for (int i = 0; i < MAX_ARCS; i++) {
        if (fx->arcs[i].life > 0) continue;
        fx->arcs[i] = (Arc){center, radius, start, sweep, life, life, width, c};
        return;
    }
}

void fx_popup(Fx *fx, const char *text, Vector2 at, float scale, Color c) {
    int slot = 0;
    float oldest = 1e9f;
    for (int i = 0; i < MAX_POPUPS; i++) {
        if (fx->popups[i].life <= 0) { slot = i; break; }
        if (fx->popups[i].life < oldest) { oldest = fx->popups[i].life; slot = i; }
    }
    Popup *p = &fx->popups[slot];
    strncpy(p->text, text, sizeof p->text - 1);
    p->text[sizeof p->text - 1] = 0;
    p->pos = at;
    p->life = p->maxLife = 0.9f;
    p->scale = scale;
    p->color = c;
}

void fx_flash(Fx *fx, Color c, float strength) {
    fx->flashColor = c;
    fx->flash = fmaxf(fx->flash, strength);
}

void fx_kick(Fx *fx, float amp, float time) {
    if (amp > fx->shake || fx->shakeTime <= 0) fx->shake = amp;
    fx->shakeTime = fmaxf(fx->shakeTime, time);
}

void fx_punch(Fx *fx, float amount) { fx->zoomVel += amount; }

Vector2 fx_shake_offset(const Fx *fx) {
    if (!fx->shakeEnabled || fx->shakeTime <= 0) return (Vector2){0, 0};
    float t = (float)GetTime();
    float a = fx->shake * fminf(1, fx->shakeTime * 6);
    return (Vector2){roundf(sinf(t * 97) * a), roundf(cosf(t * 83) * a * 0.6f)};
}

void fx_draw_world(const Fx *fx) {
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < MAX_RINGS; i++) {
        const Ring *r = &fx->rings[i];
        if (r->life <= 0) continue;
        float t = r->life / r->maxLife;
        Color c = r->color;
        c.a = (unsigned char)(c.a * t);
        DrawRing(r->pos, r->radius, r->radius + fmaxf(1, r->width * t), 0, 360, 32, c);
    }
    for (int i = 0; i < MAX_ARCS; i++) {
        const Arc *a = &fx->arcs[i];
        if (a->life <= 0) continue;
        float t = a->life / a->maxLife;
        Color c = a->color;
        c.a = (unsigned char)(c.a * t);
        /* O arco "cresce" nos primeiros quadros e depois afina. */
        float grow = fminf(1, (1 - t) * 5 + 0.2f);
        DrawRing(a->center, a->radius - a->width * t, a->radius, a->start, a->start + a->sweep * grow, 40, c);
    }
    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle *p = &fx->p[i];
        if (!p->alive) continue;
        float t = p->life / p->maxLife;
        Color c = p->color;
        c.a = (unsigned char)(c.a * fminf(1, t * 1.5f));
        switch (p->kind) {
            case P_SPARK: {
                /* Risco na direção do movimento. */
                Vector2 tail = {p->pos.x - p->vel.x * 0.025f, p->pos.y - p->vel.y * 0.025f};
                DrawLineV((Vector2){roundf(tail.x), roundf(tail.y)}, (Vector2){roundf(p->pos.x), roundf(p->pos.y)}, c);
                break;
            }
            case P_SHARD:
                DrawPoly(p->pos, 3, p->size * (0.4f + t * 0.6f), p->angle * 57.3f, c);
                break;
            case P_PETAL:
                DrawEllipse((int)p->pos.x, (int)p->pos.y, p->size * fabsf(cosf(p->angle)) + 1, p->size * 0.6f, c);
                break;
            case P_GEM:
                DrawPoly(p->pos, 4, p->size * t + 1, 45, c);
                break;
            default:
                DrawRectangle((int)(p->pos.x - p->size / 2), (int)(p->pos.y - p->size / 2), (int)p->size, (int)p->size, c);
                break;
        }
    }
    EndBlendMode();
}

void fx_draw_popups(const Fx *fx, Font font, float unit) {
    for (int i = 0; i < MAX_POPUPS; i++) {
        const Popup *p = &fx->popups[i];
        if (p->life <= 0) continue;
        float t = p->life / p->maxLife;
        float pop = 1 + 0.25f * powf(fmaxf(0, t - 0.8f) * 5, 2);
        float size = 40 * p->scale * pop;
        Vector2 m = MeasureTextEx(font, p->text, size, 2);
        Vector2 at = {p->pos.x * unit - m.x / 2, p->pos.y * unit - m.y / 2};
        Color sh = {20, 16, 12, (unsigned char)(160 * fminf(1, t * 2))};
        Color c = p->color;
        c.a = (unsigned char)(255 * fminf(1, t * 2));
        DrawTextEx(font, p->text, (Vector2){at.x + 2, at.y + 3}, size, 2, sh);
        DrawTextEx(font, p->text, at, size, 2, c);
    }
}

void fx_draw_flash(const Fx *fx, int w, int h) {
    if (fx->flash <= 0) return;
    Color c = fx->flashColor;
    c.a = (unsigned char)(c.a * fminf(1, fx->flash));
    DrawRectangle(0, 0, w, h, c);
}
