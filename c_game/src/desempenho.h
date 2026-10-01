/*
 * desempenho.h - onde vai o tempo de cada quadro (APARA_PERF, no main.c). Puro, sem raylib: guarda, quadro
 * a quadro, quanto durou cada seção do laço do jogo e resume em média, mediana, p95, p99 e máximo.
 * Seções de um quadro: o quadro inteiro (de um poll ao seguinte), a lógica (step), o mundo (a cena e os
 * lutadores desenhados nas texturas), a interface, a composição na janela e o EndDrawing (o swap, que
 * espera o vsync). Tempos em segundos.
 */
#ifndef APARA_DESEMPENHO_H
#define APARA_DESEMPENHO_H

#include <stdbool.h>

typedef enum { PERF_QUADRO, PERF_ATUALIZA, PERF_MUNDO, PERF_UI, PERF_COMPOE, PERF_SWAP, PERF_SECOES } PerfSecao;

typedef struct {
    int n;                       /* quadros do resumo */
    double media, p50, p95, p99, max;
} PerfResumo;

typedef struct {
    double *v[PERF_SECOES];      /* v[secao][quadro] */
    int n, cap, perdidos;        /* quadros guardados, capacidade, quadros que não couberam */
} Desempenho;

/* Guarda até `capacidade` quadros. false se faltar memória. */
bool perf_iniciar(Desempenho *p, int capacidade);
void perf_libera(Desempenho *p);

/* Um quadro: um tempo por seção. Um quadro além da capacidade só conta em `perdidos`. */
void perf_quadro(Desempenho *p, const double segundos[PERF_SECOES]);

/* O resumo de uma seção, sem os `pular` primeiros quadros (o aquecimento: carga preguiçosa, caches).
 * Mediana e percentis pelo posto mais próximo: o menor valor com ao menos q dos quadros até ele. */
PerfResumo perf_resumo(const Desempenho *p, PerfSecao secao, int pular);

/* Quantos quadros (sem os `pular` primeiros) passam de `limite` segundos nesta seção. */
int perf_acima(const Desempenho *p, PerfSecao secao, double limite, int pular);

const char *perf_nome(PerfSecao secao);

/* O tempo de CPU que o processo gastou até agora, em segundos (usuário e sistema; o Windows dá o do kernel como
 * sistema). false se o sistema não disser. Fica aqui, e não no main.c, porque o windows.h não convive com o raylib. */
bool perf_cpu_do_processo(double *usuario, double *sistema);

#endif
