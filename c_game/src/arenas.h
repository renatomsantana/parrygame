/*
 * arenas.h - os treze cenários, desenhados em código e animados.
 * Tudo é pintado em 320 x 180 e ampliado por número inteiro, sem filtro.
 * A frente (chuva, névoa, brasas) vai por cima dos lutadores.
 */
#ifndef APARA_ARENAS_H
#define APARA_ARENAS_H

#include "core.h"
#include "raylib.h"

#define LOW_W 320
#define LOW_H 180
#define GROUND_LOW 150      /* linha do chão no fundo */

typedef struct {
    float t;          /* relógio do cenário (segundos) */
    float blackout;   /* 0..1: Yoru apagou as luzes */
    float beat;       /* 0..1: pulso que decai (batida, tambor) */
    float danger;     /* 0..1: tensão da preparação */
    float impact;     /* 0..1: impacto recente */
    int seal;         /* selo atual do BIG BOSS */
    float lightning;  /* 0..1: relâmpago */
} ArenaCtx;

void arena_draw_back(ArenaId id, const ArenaCtx *c);
void arena_draw_front(ArenaId id, const ArenaCtx *c);
Color arena_light(ArenaId id, const ArenaCtx *c);      /* luz ambiente sobre os lutadores */
float arena_reflection(ArenaId id);
Color arena_rim(ArenaId id);                          /* luz de contorno (neon) do cenário */                    /* 0 = chão fosco */
const char *arena_name(ArenaId id);

#endif
