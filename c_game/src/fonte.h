/*
 * fonte.h - o que o jogo pede à fonte da interface (Tiny5). Sem raylib: o teste
 * tests/fonte_test.c confere, contra a própria TTF, que todo caractere não ASCII das strings
 * do jogo está nesta lista e tem glifo na fonte. Sem glifo a raylib desenha um "?".
 */
#ifndef APARA_FONTE_H
#define APARA_FONTE_H

#define FONTE_UI_ARQUIVO "assets/fonts/Tiny5-Regular.ttf"
#define FONTE_UI_TAMANHO 9          /* "em" de 8 px, a grade da Tiny5 */
#define FONTE_PEDIDOS_MAX 256

/* Preenche `cps` com os pontos de código que o jogo pede à fonte; devolve quantos. */
int fonte_pedidos(int *cps, int max);

#endif
