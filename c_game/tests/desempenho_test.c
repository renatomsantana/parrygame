/*
 * desempenho_test.c - a medida de quadros (src/desempenho.c), sem janela nem raylib. make test-desempenho
 * Cobre: média, mediana, p95, p99 e máximo com números que se conferem de cabeça (1 a 100 ms, fora de
 * ordem), o aquecimento que sai do resumo, a contagem de quadros acima de um limite, a capacidade cheia e
 * a série vazia, e o tempo de CPU do processo.
 */
#include "../src/desempenho.h"

#include <math.h>
#include <stdio.h>
#include <time.h>

static int checks = 0, failures = 0;
#define CHECK(cond, ...) do { \
    checks++; \
    if (!(cond)) { failures++; printf("FALHA %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } \
} while (0)

static bool perto(double a, double b) { return fabs(a - b) < 1e-9; }

static void quadro(Desempenho *p, double ms) {
    double s[PERF_SECOES];
    for (int i = 0; i < PERF_SECOES; i++) s[i] = ms * 0.001 * (i + 1);   /* cada seção um múltiplo, para não se confundirem */
    perf_quadro(p, s);
}

int main(void) {
    Desempenho p;
    CHECK(perf_iniciar(&p, 200), "iniciar");
    /* 1 a 100 ms, embaralhados por um passo que não repete: 37 é primo com 100 */
    for (int i = 0; i < 100; i++) quadro(&p, (double)((i * 37) % 100 + 1));
    PerfResumo r = perf_resumo(&p, PERF_QUADRO, 0);
    CHECK(r.n == 100, "100 quadros (%d)", r.n);
    CHECK(perto(r.media, 0.0505), "média 50,5 ms (%.6f)", r.media);
    CHECK(perto(r.p50, 0.050), "mediana 50 ms (%.6f)", r.p50);
    CHECK(perto(r.p95, 0.095), "p95 95 ms (%.6f)", r.p95);
    CHECK(perto(r.p99, 0.099), "p99 99 ms (%.6f)", r.p99);
    CHECK(perto(r.max, 0.100), "máximo 100 ms (%.6f)", r.max);
    r = perf_resumo(&p, PERF_SWAP, 0);
    CHECK(perto(r.media, 0.0505 * 6) && perto(r.max, 0.6), "cada seção tem a sua série (swap: média %.4f, máx %.4f)", r.media, r.max);
    /* o aquecimento sai: sem os 7 primeiros (1, 38, 75, 12, 49, 86, 23), sobram 93 quadros */
    double somaAquecimento = 0;
    int acimaAquecimento = 0;
    for (int i = 0; i < 7; i++) { double ms = (double)((i * 37) % 100 + 1); somaAquecimento += ms * 0.001; acimaAquecimento += ms > 16.6; }
    r = perf_resumo(&p, PERF_QUADRO, 7);
    CHECK(r.n == 93, "sem o aquecimento: 93 quadros (%d)", r.n);
    CHECK(perto(r.max, 0.100), "e o máximo continua 100 ms");
    CHECK(perto(r.media, (0.0505 * 100 - somaAquecimento) / 93), "a média sem o aquecimento (%.6f)", r.media);
    CHECK(perto(r.p50, 0.052), "e a mediana passa a ser 52 ms (%.6f)", r.p50);
    r = perf_resumo(&p, PERF_QUADRO, 1000);
    CHECK(r.n == 0 && r.max == 0, "pular mais que os quadros: resumo vazio");
    /* acima de um limite: 16,6 ms tem 84 quadros de 17 a 100 */
    CHECK(perf_acima(&p, PERF_QUADRO, 0.0166, 0) == 84, "84 quadros acima de 16,6 ms (%d)", perf_acima(&p, PERF_QUADRO, 0.0166, 0));
    CHECK(perf_acima(&p, PERF_QUADRO, 0.100, 0) == 0, "nenhum acima do máximo");
    CHECK(perf_acima(&p, PERF_QUADRO, 0.0166, 7) == 84 - acimaAquecimento, "sem o aquecimento: %d dos 7 primeiros passavam de 16,6 ms (%d)", acimaAquecimento, perf_acima(&p, PERF_QUADRO, 0.0166, 7));
    perf_libera(&p);

    /* capacidade cheia: o que não cabe é contado, e o resumo não muda */
    CHECK(perf_iniciar(&p, 5), "iniciar pequeno");
    for (int i = 1; i <= 8; i++) quadro(&p, i);
    CHECK(p.n == 5 && p.perdidos == 3, "capacidade 5: guardou %d, perdeu %d", p.n, p.perdidos);
    r = perf_resumo(&p, PERF_QUADRO, 0);
    CHECK(r.n == 5 && perto(r.max, 0.005), "resumo dos 5 primeiros (máx %.4f)", r.max);
    perf_libera(&p);

    /* série vazia e um quadro só */
    CHECK(perf_iniciar(&p, 4), "iniciar vazio");
    r = perf_resumo(&p, PERF_QUADRO, 0);
    CHECK(r.n == 0 && r.media == 0 && r.p99 == 0, "série vazia: tudo zero");
    quadro(&p, 7);
    r = perf_resumo(&p, PERF_QUADRO, 0);
    CHECK(r.n == 1 && perto(r.p50, 0.007) && perto(r.p99, 0.007) && perto(r.max, 0.007), "um quadro só");
    perf_libera(&p);
    CHECK(!perf_iniciar(&p, 0), "capacidade 0 não inicia");
    CHECK(perf_nome(PERF_SWAP)[0] == 's' && perf_nome((PerfSecao)99)[0] == '?', "nomes");

    /* o tempo de CPU do processo (getrusage no POSIX, GetProcessTimes no Windows): existe, não anda para trás, e cresce quando o processo trabalha */
    double u0 = -1, s0 = -1, u1 = -1, s1 = -1;
    CHECK(perf_cpu_do_processo(&u0, &s0) && u0 >= 0 && s0 >= 0, "o tempo de CPU do processo existe (%.3f, %.3f)", u0, s0);
    volatile double sorvedouro = 1;
    const clock_t inicio = clock();
    while (clock() - inicio < CLOCKS_PER_SEC / 10) sorvedouro = sorvedouro * 1.0000001 + 1e-9;      /* 100 ms trabalhando */
    CHECK(perf_cpu_do_processo(&u1, &s1) && u1 >= u0 && s1 >= s0, "o tempo de CPU não anda para trás");
    CHECK((u1 + s1) - (u0 + s0) >= 0.05, "cem milissegundos trabalhando somam ao menos 50 ms de CPU (%.3f s)", (u1 + s1) - (u0 + s0));

    printf("desempenho: %d verificações, %d falhas\n", checks, failures);
    return failures ? 1 : 0;
}
