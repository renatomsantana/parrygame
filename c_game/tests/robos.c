/*
 * robos.c - curva de dificuldade: cada robô enfrenta cada mestre N vezes e a
 * tabela sai em Markdown. Rodar: make robos (ou make robos LUTAS=1000).
 * Kojiro chega em cada mestre depois de vencer os anteriores, como na trilha.
 */
#include "../src/robo.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct { const char *nome; Robo r; } Coluna;

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 300;
    if (n < 1) n = 1;
    Coluna col[] = {
        {"Perfeito", ROBO_DO_DEMO},
        {"Nunca defende", ROBO_SEM_DEFESA},
        {"Spam", ROBO_APERTA_SEM_PARAR},
        {"Reação 200", {0}},
        {"Reação 250", {0}},
        {"Reação 300", {0}},
        {"Humano casual", ROBO_HUMANO_CASUAL},
    };
    col[3].r = robo_reacao(0.200f);
    col[4].r = robo_reacao(0.250f);
    col[5].r = robo_reacao(0.300f);
    int nc = (int)(sizeof col / sizeof col[0]);
    printf("Vitórias em %d lutas por mestre (%%).\n\n| # | Mestre |", n);
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
                RoboLuta l = robo_lutar(&col[c].r, m, i, 1000u + (uint32_t)k);
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
