/*
 * carimbo_win.c - o programa de teste da camada de carimbo do Windows (src/entrada_win.c), para o
 * tests/teste_carimbo_windows.sh: abre uma janela "aparar" (o tests/xclique.c a procura por esse título e
 * lhe dá o foco), liga a entrada bruta e escreve, por `segundos`, cada carimbo que chega:
 *   iniciar=true|false
 *   carimbo <instante, s> 
 *   total=N
 * Só compila para Windows (MinGW).
 */
#include "../src/entrada_plat.h"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

int main(int argc, char **argv) {
    const double segundos = argc > 1 ? atof(argv[1]) : 8;
    HWND janela = CreateWindowExW(0, L"STATIC", L"aparar teste", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 50, 50, 400, 300, NULL, NULL, GetModuleHandleW(NULL), NULL);
    entrada_preparar();
    const bool ok = janela && entrada_iniciar();
    printf("iniciar=%s\n", ok ? "true" : "false");
    fflush(stdout);
    if (!ok) return 1;
    const double fim = entrada_relogio() + segundos;
    int total = 0;
    while (entrada_relogio() < fim) {
        MSG m;
        while (PeekMessageW(&m, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&m); DispatchMessageW(&m); }
        double c[8];
        const int n = entrada_coletar(c, 8, entrada_relogio());
        for (int i = 0; i < n; i++) { printf("carimbo %.6f\n", c[i]); total++; }
        fflush(stdout);
        Sleep(1);
    }
    printf("total=%d\n", total);
    return 0;
}
