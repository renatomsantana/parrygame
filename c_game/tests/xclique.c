/*
 * xclique.c - ferramenta do teste do carimbo do clique (tests/teste_carimbo.sh): manda cliques (ou Espaço)
 * de verdade para o display de $DISPLAY, pelo XTest, o mesmo caminho de um mouse ou teclado (o servidor
 * gera os eventos brutos que o jogo carimba), e escreve, para cada um, o intervalo em que ele aconteceu,
 * no CLOCK_MONOTONIC (o relógio dos carimbos do jogo no Linux):
 *   xclique <quantos> <periodo_ms> [espaco|repete|semfoco]   uma linha "INJECAO antes depois" por aperto
 *       (com 0 cliques só dá o foco à janela, ou o tira, no caso de semfoco, e sai: serve para esperar a janela existir)
 *       (repete: o Espaço aperta e "repete" três vezes sem soltar: é um aperto só)
 * "antes" é lido antes de pedir ao servidor, e "depois", quando o servidor confirmou que já processou o
 * pedido: o instante do hardware fica entre os dois. Os períodos variam de propósito, para os cliques
 * caírem em pontos diferentes do quadro do jogo. Sai com 0; 1 se não achou a janela; 2 se faltou X.
 */
#define _POSIX_C_SOURCE 200809L
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if __has_include(<X11/extensions/XTest.h>)
#include <X11/extensions/XTest.h>
#else
extern int XTestFakeButtonEvent(Display *, unsigned int, Bool, unsigned long);
extern int XTestFakeKeyEvent(Display *, unsigned int, Bool, unsigned long);
#endif

#define TITULO_DO_JOGO "aparar"   /* o começo do título da janela (InitWindow no main.c) */

static Window acha(Display *d, Window w) {
    char *nome = NULL;
    if (XFetchName(d, w, &nome) && nome) {
        int ok = strncmp(nome, TITULO_DO_JOGO, strlen(TITULO_DO_JOGO)) == 0;
        XFree(nome);
        if (ok) return w;
    }
    Window raiz, pai, *filhas = NULL;
    unsigned int n = 0;
    if (!XQueryTree(d, w, &raiz, &pai, &filhas, &n)) return 0;
    Window achada = 0;
    for (unsigned int i = 0; i < n && !achada; i++) achada = acha(d, filhas[i]);
    if (filhas) XFree(filhas);
    return achada;
}

static double agora(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static void dorme(double s) {
    struct timespec ts = {(time_t)s, (long)((s - (double)(time_t)s) * 1e9)};
    nanosleep(&ts, NULL);
}

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 20;
    double periodo = (argc > 2 ? atof(argv[2]) : 150) / 1000;
    int repete = argc > 3 && !strcmp(argv[3], "repete");     /* Espaço segurado: a tecla "repete" sem soltar */
    int espaco = repete || (argc > 3 && !strcmp(argv[3], "espaco"));
    Display *d = XOpenDisplay(NULL);
    if (!d) { fprintf(stderr, "xclique: sem display\n"); return 2; }
    /* Sem gerenciador de janelas ninguém dá o foco à janela do jogo, e o jogo pausa o duelo sem foco: dá aqui
     * (a menos que o teste peça o contrário, com o terceiro argumento "semfoco"). */
    Window w = acha(d, DefaultRootWindow(d));
    if (!w) { fprintf(stderr, "xclique: janela do jogo não encontrada\n"); return 1; }
    if (argc > 3 && !strcmp(argv[3], "semfoco")) XSetInputFocus(d, DefaultRootWindow(d), RevertToNone, CurrentTime);   /* tira o foco do jogo, qualquer que fosse */
    else XSetInputFocus(d, w, RevertToParent, CurrentTime);
    XWarpPointer(d, None, DefaultRootWindow(d), 0, 0, 0, 0, 200, 200);   /* o ponteiro dentro da janela do jogo */
    XSync(d, False);
    dorme(0.3);
    KeyCode tecla = XKeysymToKeycode(d, XK_space);
    unsigned int s = 12345;
    for (int i = 0; i < n; i++) {
        s = s * 1103515245u + 12345u;
        dorme(periodo * (0.6 + 0.8 * ((s >> 8) % 1000) / 1000.0));    /* de 0,6 a 1,4 períodos: fase diferente a cada aperto */
        double antes = agora();
        if (espaco) XTestFakeKeyEvent(d, tecla, True, 0); else XTestFakeButtonEvent(d, 1, True, 0);
        XSync(d, False);
        double depois = agora();
        printf("INJECAO %.6f %.6f\n", antes, depois);
        fflush(stdout);
        if (repete)
            for (int r = 0; r < 3; r++) { dorme(0.008); XTestFakeKeyEvent(d, tecla, True, 0); XSync(d, False); }
        dorme(0.03);
        if (espaco) XTestFakeKeyEvent(d, tecla, False, 0); else XTestFakeButtonEvent(d, 1, False, 0);
        XSync(d, False);
    }
    XCloseDisplay(d);
    return 0;
}
