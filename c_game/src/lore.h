/*
 * lore.h - as ilustrações animadas da abertura e da trilha.
 */
#ifndef APARA_LORE_H
#define APARA_LORE_H

#include "core.h"

/* Desenha a ilustração da página (0..LORE_PAGES-1) em 320 x 180. */
void lore_draw_scene(int page, float t);
/* Tela de título: só a paisagem. */
void lore_draw_title(float t);
/* Ilustração do final, depois do Oboro. */
void lore_draw_ending(float t);
/* Mapa da trilha em 320 x 180: a serra com os treze pontos. */
void lore_draw_trail(const Campaign *c, float t, int selected);
/* Posição de cada parada da trilha em 320 x 180. */
void lore_trail_point(int index, float *x, float *y);

#endif
