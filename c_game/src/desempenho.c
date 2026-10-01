#include "desempenho.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/resource.h>
#endif

bool perf_iniciar(Desempenho *p, int capacidade) {
    memset(p, 0, sizeof *p);
    if (capacidade < 1) return false;
    for (int s = 0; s < PERF_SECOES; s++) {
        p->v[s] = malloc(sizeof(double) * (size_t)capacidade);
        if (!p->v[s]) { perf_libera(p); return false; }
    }
    p->cap = capacidade;
    return true;
}

void perf_libera(Desempenho *p) {
    for (int s = 0; s < PERF_SECOES; s++) { free(p->v[s]); p->v[s] = NULL; }
    p->n = p->cap = p->perdidos = 0;
}

void perf_quadro(Desempenho *p, const double segundos[PERF_SECOES]) {
    if (p->n >= p->cap) { p->perdidos++; return; }
    for (int s = 0; s < PERF_SECOES; s++) p->v[s][p->n] = segundos[s];
    p->n++;
}

static int compara(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return x < y ? -1 : x > y;
}

/* O menor valor com ao menos `q` dos n quadros até ele (posto mais próximo). */
static double posto(const double *ordenado, int n, double q) {
    int i = (int)ceil(q * n) - 1;
    if (i < 0) i = 0;
    if (i > n - 1) i = n - 1;
    return ordenado[i];
}

PerfResumo perf_resumo(const Desempenho *p, PerfSecao secao, int pular) {
    PerfResumo r = {0, 0, 0, 0, 0, 0};
    if (pular < 0) pular = 0;
    int n = p->n - pular;
    if (n <= 0 || !p->v[secao]) return r;
    double *c = malloc(sizeof(double) * (size_t)n);
    if (!c) return r;
    memcpy(c, p->v[secao] + pular, sizeof(double) * (size_t)n);
    qsort(c, (size_t)n, sizeof(double), compara);
    double soma = 0;
    for (int i = 0; i < n; i++) soma += c[i];
    r.n = n;
    r.media = soma / n;
    r.p50 = posto(c, n, 0.50);
    r.p95 = posto(c, n, 0.95);
    r.p99 = posto(c, n, 0.99);
    r.max = c[n - 1];
    free(c);
    return r;
}

int perf_acima(const Desempenho *p, PerfSecao secao, double limite, int pular) {
    if (pular < 0) pular = 0;
    int k = 0;
    for (int i = pular; i < p->n; i++) k += p->v[secao][i] > limite;
    return k;
}

const char *perf_nome(PerfSecao secao) {
    static const char *NOME[PERF_SECOES] = {"quadro", "lógica", "mundo", "interface", "composição", "swap"};
    return secao >= 0 && secao < PERF_SECOES ? NOME[secao] : "?";
}

bool perf_cpu_do_processo(double *usuario, double *sistema) {
#ifdef _WIN32
    FILETIME criado, saiu, kernel, user;
    if (!GetProcessTimes(GetCurrentProcess(), &criado, &saiu, &kernel, &user)) return false;
    const double cem_ns = 1e-7;       /* o FILETIME conta em intervalos de 100 ns */
    *usuario = (double)(((unsigned long long)user.dwHighDateTime << 32) | user.dwLowDateTime) * cem_ns;
    *sistema = (double)(((unsigned long long)kernel.dwHighDateTime << 32) | kernel.dwLowDateTime) * cem_ns;
    return true;
#else
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) != 0) return false;
    *usuario = (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec * 1e-6;
    *sistema = (double)ru.ru_stime.tv_sec + (double)ru.ru_stime.tv_usec * 1e-6;
    return true;
#endif
}
