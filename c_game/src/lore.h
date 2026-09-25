/*
 * lore.h - as ilustrações animadas: título, cabana de Hanzo e trilha.
 */
#ifndef APARA_LORE_H
#define APARA_LORE_H

#include "core.h"

/* A cabana de Hanzo na serra, onde Kojiro volta depois de cada vitória (e pede conselho). */
void lore_draw_cabin(float t);
/* Tela de título: a serra à noite com o dojo no pico (o fundo)... */
void lore_draw_title(float t);
/* ...e Kojiro no morro olhando para ele (por cima, fora da paleta do fundo). */
void lore_draw_title_hero(float t);
/* Mapa da trilha em 320 x 180: a serra com os treze pontos. */
void lore_draw_trail(const Campaign *c, float t, int selected);
/* Posição de cada parada da trilha em 320 x 180. */
void lore_trail_point(int index, float *x, float *y);

#endif
