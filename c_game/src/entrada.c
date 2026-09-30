#include "entrada.h"

#include "ajuste.h"

double entrada_no_quadro(double carimbo, double fim, double quadro, double corrido, double passo, bool *valido) {
    bool passoOk = isfinite(passo) && passo >= 0;
    double meio = passoOk ? passo * 0.5 : 0;
    if (valido) *valido = false;
    if (!passoOk || !isfinite(carimbo) || !isfinite(fim) || !isfinite(quadro) || !isfinite(corrido)) return meio;
    if (quadro <= 0 || quadro > AJ_PAUSA_POR_TRAVAMENTO) return meio;
    if (corrido <= 0 || corrido > quadro) return meio;
    double desdeOQuadro = carimbo - (fim - quadro);          /* 0 = começo do quadro, quadro = o poll */
    if (desdeOQuadro < -AJ_CARIMBO_MARGEM || desdeOQuadro > quadro + AJ_CARIMBO_MARGEM) return meio;
    double x = carimbo - (fim - corrido);                    /* a partir do começo do que correu */
    if (x < 0) x = 0;                                        /* congelado pelo hitstop: vale no começo do passo */
    if (x > corrido) x = corrido;
    double t = x / corrido * passo;
    if (t < 0) t = 0;
    if (t > passo) t = passo;
    if (valido) *valido = true;
    return t;
}
