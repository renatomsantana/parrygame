/*
 * robos.c - curva de dificuldade: cada robô enfrenta cada mestre N vezes e a
 * tabela sai em Markdown. Rodar: make robos (ou make robos LUTAS=1000).
 * Kojiro chega em cada mestre depois de vencer os anteriores, como na trilha.
 * Os robôs decidem em ms e apertam no instante exato: a taxa de quadros não muda
 * o resultado (make robos HZ=144). Com QUADROS=1 o aperto entra no meio do quadro,
 * como no jogo, e o resultado passa a depender da taxa.
 *
 *   robos_relatorio [lutas [hz [quadros]]]     a tabela (make robos)
 *   robos_relatorio --taxas [lutas]            make robos-taxas: a mesma luta a 30, 60, 120, 144 e
 *       240 Hz, com o aperto em ms exato, tem que dar o MESMO resultado em todas as taxas, coluna a
 *       coluna: vitória, perfeitos, bons e erros iguais (a duração só varia um quadro a 30 Hz, porque
 *       a luta acaba no quadro do golpe final). Sai com erro se divergir. Exceção conhecida: o enjin.
 *       As brasas são somadas por quadro antes de o quadro julgar o golpe que caiu dentro dele, então
 *       um parry perfeito que as apaga no meio de um quadro chega até um quadro de brasa tarde; isso
 *       muda ~0,4% das lutas dele entre as taxas, e o teste só exige que fique abaixo de 2%.
 *       Mostra também o casual com o clique no meio do quadro, que é o que o jogo faz hoje, ao lado
 *       da coluna em ms exato.
 */
#include "../src/robo.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { const char *nome; Robo r; } Coluna;

#define NCOL 7
static void colunas(Coluna *col) {
    Coluna base[NCOL] = {
        {"Perfeito", ROBO_DO_DEMO},
        {"Nunca defende", ROBO_SEM_DEFESA},
        {"Spam", ROBO_APERTA_SEM_PARAR},
        {"Reação 200", {0}},
        {"Reação 250", {0}},
        {"Reação 300", {0}},
        {"Humano casual", ROBO_HUMANO_CASUAL},
    };
    base[3].r = robo_reacao(0.200f);
    base[4].r = robo_reacao(0.250f);
    base[5].r = robo_reacao(0.300f);
    for (int c = 0; c < NCOL; c++) col[c] = base[c];
}

/* A mesma luta em todas as taxas de quadros. */
static int taxas(int n) {
    static const double HZ[] = {30, 60, 120, 144, 240};
    const int nh = (int)(sizeof HZ / sizeof HZ[0]), REF = 1;      /* a referência é 60 Hz */
    Coluna col[NCOL];
    colunas(col);
    long divergem = 0, comparacoes = 0, divergemBrasas = 0, comparacoesBrasas = 0;
    long meio[13][5], exato[13];
    memset(meio, 0, sizeof meio);
    memset(exato, 0, sizeof exato);
    for (int i = 0; i < roster_size(); i++) {
        const MasterProfile *m = roster_get(i);
        for (int c = 0; c < NCOL; c++) {
            for (int k = 0; k < n; k++) {
                uint32_t semente = 1000u + (uint32_t)k;
                RoboLuta ref = robo_lutar_hz(&col[c].r, m, i, semente, HZ[REF], false);
                for (int h = 0; h < nh; h++) {
                    if (h == REF) continue;
                    RoboLuta l = robo_lutar_hz(&col[c].r, m, i, semente, HZ[h], false);
                    bool igual = l.vitoria == ref.vitoria && l.perfeitos == ref.perfeitos && l.bons == ref.bons && l.erros == ref.erros &&
                                 fabs(l.duracao - ref.duracao) <= 1.0 / 30 + 1e-9;
                    if (m->burn > 0) { comparacoesBrasas++; divergemBrasas += !igual; }
                    else {
                        comparacoes++;
                        if (!igual && divergem++ < 5) printf("DIVERGE: %s, %s, semente %u, %.0f Hz x %.0f Hz\n", m->name, col[c].nome, semente, HZ[h], HZ[REF]);
                    }
                }
                if (c == NCOL - 1) {
                    exato[i] += ref.vitoria;
                    for (int h = 0; h < nh; h++) meio[i][h] += robo_lutar_hz(&col[c].r, m, i, semente, HZ[h], true).vitoria;
                }
            }
        }
    }
    bool brasasOk = comparacoesBrasas == 0 || 50 * divergemBrasas <= comparacoesBrasas;     /* até 2% */
    printf("Taxas de quadros, aperto em ms exato, %d lutas por mestre e coluna (a 30, 120, 144 e 240 Hz contra 60 Hz):\n", n);
    printf("- sem brasas: %ld comparações, %ld divergem %s\n", comparacoes, divergem, divergem ? "(FALHA)" : "(resultado idêntico em todas as taxas)");
    printf("- enjin (brasas somadas por quadro): %ld comparações, %ld divergem (%.2f%%) %s\n\n", comparacoesBrasas, divergemBrasas,
           comparacoesBrasas ? 100.0 * (double)divergemBrasas / (double)comparacoesBrasas : 0.0, brasasOk ? "(dentro do limite de 2%)" : "(FALHA: acima de 2%)");
    printf("Casual, vitórias (%%): o clique no meio do quadro (como o jogo hoje) x o instante exato\n\n| Mestre |");
    for (int h = 0; h < nh; h++) printf(" meio %.0f Hz |", HZ[h]);
    printf(" ms exato |\n|---|");
    for (int h = 0; h < nh + 1; h++) printf("---|");
    printf("\n");
    for (int i = 0; i < roster_size(); i++) {
        printf("| %s |", roster_get(i)->name);
        for (int h = 0; h < nh; h++) printf(" %.0f |", 100.0 * (double)meio[i][h] / n);
        printf(" %.0f |\n", 100.0 * (double)exato[i] / n);
    }
    return divergem || !brasasOk ? 1 : 0;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--taxas") == 0) return taxas(argc > 2 && atoi(argv[2]) > 0 ? atoi(argv[2]) : 100);
    int n = argc > 1 ? atoi(argv[1]) : 300;
    double hz = argc > 2 ? atof(argv[2]) : 60;
    bool quadros = argc > 3 && atoi(argv[3]) != 0;
    if (n < 1) n = 1;
    if (hz < 10) hz = 60;
    Coluna col[NCOL];
    colunas(col);
    const int nc = NCOL;
    printf("Vitórias em %d lutas por mestre (%%), %.0f Hz, aperto %s.\n\n| # | Mestre |", n, hz, quadros ? "no meio do quadro" : "no instante exato");
    for (int c = 0; c < nc; c++) printf(" %s |", col[c].nome);
    printf(" Casual: perf/bom/erro | Casual: duração |\n|---|---|");
    for (int c = 0; c < nc + 2; c++) printf("---|");
    printf("\n");
    for (int i = 0; i < roster_size(); i++) {
        const MasterProfile *m = roster_get(i);
        printf("| %d | %s |", i + 1, m->name);
        int P = 0, B = 0, E = 0;
        double T = 0;
        for (int c = 0; c < nc; c++) {
            int v = 0;
            for (int k = 0; k < n; k++) {
                RoboLuta l = robo_lutar_hz(&col[c].r, m, i, 1000u + (uint32_t)k, hz, quadros);
                v += l.vitoria;
                if (c == nc - 1) { P += l.perfeitos; B += l.bons; E += l.erros; T += l.duracao; }
            }
            printf(" %.0f |", 100.0 * v / n);
        }
        int tot = P + B + E > 0 ? P + B + E : 1;
        printf(" %d/%d/%d%% | %.0f s |\n", 100 * P / tot, 100 * B / tot, 100 * E / tot, T / n);
    }
    return 0;
}
