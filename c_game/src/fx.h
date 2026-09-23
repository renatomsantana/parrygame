/*
 * fx.h - partículas, anéis, arcos de corte, lampejo de tela, tremor e textos.
 */
#ifndef APARA_FX_H
#define APARA_FX_H

#include <stdbool.h>

#include "raylib.h"

#define MAX_PARTICLES 900
#define MAX_RINGS 16
#define MAX_POPUPS 8
#define MAX_ARCS 8

typedef enum { P_SPARK, P_EMBER, P_DUST, P_SHARD, P_PETAL, P_GEM } ParticleKind;

typedef struct {
    ParticleKind kind;
    Vector2 pos, vel;
    float life, maxLife, size, drag, gravity, spin, angle;
    Color color;
    bool alive;
} Particle;

typedef struct { Vector2 pos; float radius, speed, life, maxLife, width; Color color; } Ring;
typedef struct { char text[32]; Vector2 pos; float life, maxLife, scale; Color color; } Popup;
typedef struct { Vector2 center; float radius, start, sweep, life, maxLife, width; Color color; } Arc;

typedef struct {
    Particle p[MAX_PARTICLES];
    Ring rings[MAX_RINGS];
    Popup popups[MAX_POPUPS];
    Arc arcs[MAX_ARCS];
    float flash;               /* 0..1 */
    Color flashColor;
    float shake, shakeTime;    /* amplitude em px, duração */
    float zoom, zoomVel;       /* soco de câmera: 1 = normal */
    float vignettePulse;
    bool shakeEnabled;
} Fx;

void fx_init(Fx *fx);
void fx_update(Fx *fx, float dt);
void fx_draw_world(const Fx *fx);           /* partículas, anéis e arcos */
void fx_draw_popups(const Fx *fx, Font font, float unit); /* unit: px da tela por px do mundo */
void fx_draw_flash(const Fx *fx, int w, int h);
Vector2 fx_shake_offset(const Fx *fx);

void fx_burst(Fx *fx, ParticleKind kind, Vector2 at, int count, float speed, float spread, float dir, Color a, Color b);
void fx_ring(Fx *fx, Vector2 at, float speed, float life, float width, Color c);
void fx_arc(Fx *fx, Vector2 center, float radius, float start, float sweep, float life, float width, Color c);
void fx_popup(Fx *fx, const char *text, Vector2 at, float scale, Color c);
void fx_flash(Fx *fx, Color c, float strength);
void fx_kick(Fx *fx, float amp, float time);
void fx_punch(Fx *fx, float amount);
void fx_clear(Fx *fx);

float frand(float lo, float hi);

#endif
