/*
 * vozes.h - quantas vozes cada som que toca várias vezes seguidas precisa, sem raylib.
 *
 * Um Sound que é tocado de novo antes de acabar recomeça do início e corta a própria cauda. Os sons
 * dos golpes têm várias vozes (cópias do mesmo som) usadas em rodízio: cada disparo pega a próxima.
 * Nenhuma cauda é cortada quando há tantas vozes quantos disparos cabem dentro de uma cauda. O parry
 * perfeito tem a cauda mais longa (o reverb), e um golpe duplo perfeito o toca duas vezes.
 * O teste do rodízio está em tests/core_test.c (test_vozes).
 */
#ifndef APARA_VOZES_H
#define APARA_VOZES_H

#include "ajuste.h"

#define SOM_PERFEITO_CAUDA 1.6f                 /* segundos do som do parry perfeito, com o reverb */
#define SOM_PERFEITO_DISPAROS_POR_GOLPE 2       /* o golpe duplo perfeito toca o som duas vezes */
#define VOZES_PADRAO 4                          /* bom, erro e assobio: caudas curtas */
#define VOZES_MAX 8

/* Vozes para nenhum disparo cortar a cauda do anterior: os golpes de uma sequência chegam com pelo
 * menos `intervalo` segundos entre os contatos (AJ_CADEIA_MIN) e cada um toca `disparos` vezes. */
static inline int vozes_precisas(float cauda, float intervalo, int disparos) {
    int golpes = (int)(cauda / intervalo);
    if ((float)golpes * intervalo < cauda - 1e-4f) golpes++;
    return golpes * disparos;
}

/* As vozes do perfeito (constante de compilação; o teste confere que é igual a vozes_precisas). */
#define VOZES_PERFEITO_N ((int)(SOM_PERFEITO_CAUDA / AJ_CADEIA_MIN + 0.999f) * SOM_PERFEITO_DISPAROS_POR_GOLPE)
_Static_assert(VOZES_PERFEITO_N <= VOZES_MAX, "VOZES_MAX não comporta as vozes do perfeito");

#endif
