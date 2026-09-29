#include "fonte.h"

/* ASCII imprimível e Latin-1 (sem o hífen suave, que a Tiny5 não tem), mais a pontuação tipográfica
 * das falas: travessões, aspas curvas (abrir e fechar) e reticências. Um caractere novo nas strings
 * do jogo tem que entrar aqui; o teste avisa. */
static const int EXTRAS[] = {0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2026};

int fonte_pedidos(int *cps, int max) {
    int n = 0;
    for (int c = 32; c <= 126 && n < max; c++) cps[n++] = c;
    for (int c = 160; c <= 255 && n < max; c++)
        if (c != 0xAD) cps[n++] = c;
    for (unsigned i = 0; i < sizeof EXTRAS / sizeof EXTRAS[0] && n < max; i++) cps[n++] = EXTRAS[i];
    return n;
}
