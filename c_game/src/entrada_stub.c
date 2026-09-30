/* entrada_stub.c - plataforma sem carimbo (Wayland puro, Windows): o jogo cai no meio do quadro, como sempre. */
#define _POSIX_C_SOURCE 200809L
#include "entrada_plat.h"

#include <time.h>

double entrada_relogio(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

void entrada_preparar(void) {}
bool entrada_iniciar(void) { return false; }
int entrada_coletar(double *carimbos, int max, double ate) { (void)carimbos; (void)max; (void)ate; return 0; }
