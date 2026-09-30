/* entrada_fila.h - a fila pequena de carimbos, igual em todas as plataformas (quem chama trava, se precisar). */
#ifndef APARA_ENTRADA_FILA_H
#define APARA_ENTRADA_FILA_H

#define ENTRADA_FILA_MAX 32

typedef struct {
    double t[ENTRADA_FILA_MAX];
    int n;
} EntradaFila;

/* Cheia, descarta o mais velho: um carimbo que ninguém lê em 32 apertos já não serve. */
static inline void fila_empilha(EntradaFila *f, double t) {
    if (f->n == ENTRADA_FILA_MAX) {
        for (int i = 1; i < ENTRADA_FILA_MAX; i++) f->t[i - 1] = f->t[i];
        f->n--;
    }
    f->t[f->n++] = t;
}

static inline int fila_coleta(EntradaFila *f, double *saida, int max, double ate) {
    int dados = 0, resta = 0;
    for (int i = 0; i < f->n; i++) {
        if (f->t[i] <= ate && dados < max) saida[dados++] = f->t[i];
        else f->t[resta++] = f->t[i];
    }
    f->n = resta;
    return dados;
}

#endif
