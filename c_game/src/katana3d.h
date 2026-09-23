/*
 * katana3d.h - a katana 3D (assets/katana) desenhada dentro do mundo em pixel art.
 * Câmera ortográfica em que 1 unidade = 1 pixel; a lâmina vai da empunhadura à
 * ponta exatamente onde o boneco segura a espada.
 */
#ifndef APARA_KATANA3D_H
#define APARA_KATANA3D_H

#include <stdbool.h>

#include "raylib.h"

bool katana3d_load(const char *dir);   /* false se o modelo não existir: o jogo usa a lâmina desenhada */
void katana3d_unload(void);
bool katana3d_ready(void);

/* Abre e fecha a passada 3D sobre a textura atual (largura e altura em pixels). */
void katana3d_begin(int width, int height);
void katana3d_end(void);

/* Estilo de cada katana: cor da lâmina (e guarda), cor do cabo e largura. */
typedef struct {
    Color blade, handle;
    float width;
} KatanaStyle;

/* butt: ponta do cabo; tip: ponta da lâmina; roll: giro no próprio eixo (graus); light: luz do cenário.
 * O modelo curva com o fio para cima quando roll = 0 e a ponta aponta para a direita:
 * quem olha para a direita segura com roll 180 (fio para baixo), quem olha para a esquerda com 0. */
void katana3d_draw(Vector2 butt, Vector2 tip, float roll, const KatanaStyle *style, Color light);

#endif
