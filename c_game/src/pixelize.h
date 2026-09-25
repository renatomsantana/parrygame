/*
 * pixelize.h - fundo em pixel art de verdade: cada cena ganha uma paleta curta
 * (as cores que mais aparecem nela, por corte mediano) e o fundo é redesenhado só
 * com essas cores, com dithering ordenado 4 x 4 no lugar de degradê e brilho
 * suave. Os lutadores e a interface não passam por aqui.
 */
#ifndef APARA_PIXELIZE_H
#define APARA_PIXELIZE_H

void pix_init(int w, int h);
void pix_shutdown(void);

/* Desenhe o fundo entre as duas: vai para uma textura à parte. `key` identifica a
 * cena (a paleta é calculada na primeira vez e guardada). */
void pix_capture_begin(void);
void pix_capture_end(int key);

/* Pinta o fundo capturado no alvo atual, só com as cores da paleta da cena. */
void pix_draw(void);

#endif
