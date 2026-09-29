/*
 * xtecla.c - ferramenta dos testes do jogo real sob xvfb (tests/teste_vitoria.sh): manda uma tecla
 * de verdade, ou o pedido de fechar a janela (o que o botão de fechar de um gerenciador de janelas
 * faz), para a janela do jogo, no display de $DISPLAY.
 *   xtecla esc | q | m | enter | espaco     aperta e solta a tecla (com um tempo entre as duas)
 *   xtecla fechar                           WM_DELETE_WINDOW
 * Sai com 0 se entregou, 1 se não achou a janela do jogo, 2 se faltou X.
 */
#define _POSIX_C_SOURCE 200809L
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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

/* A tecla Q faz o jogo sair na hora: o "solta" pode chegar a uma janela que já não existe. */
static int ignora_erro(Display *d, XErrorEvent *e) { (void)d; (void)e; return 0; }

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "uso: xtecla esc|q|m|enter|espaco|fechar\n"); return 2; }
    XSetErrorHandler(ignora_erro);
    Display *d = XOpenDisplay(NULL);
    if (!d) { fprintf(stderr, "xtecla: sem display\n"); return 2; }
    Window w = acha(d, DefaultRootWindow(d));
    if (!w) { fprintf(stderr, "xtecla: janela do jogo não encontrada\n"); XCloseDisplay(d); return 1; }
    if (!strcmp(argv[1], "fechar")) {
        XEvent e;
        memset(&e, 0, sizeof e);
        e.xclient.type = ClientMessage;
        e.xclient.window = w;
        e.xclient.message_type = XInternAtom(d, "WM_PROTOCOLS", False);
        e.xclient.format = 32;
        e.xclient.data.l[0] = (long)XInternAtom(d, "WM_DELETE_WINDOW", False);
        e.xclient.data.l[1] = CurrentTime;
        XSendEvent(d, w, False, NoEventMask, &e);
        XSync(d, False);
        XCloseDisplay(d);
        return 0;
    }
    KeySym ks = !strcmp(argv[1], "esc") ? XK_Escape : !strcmp(argv[1], "q") ? XK_q : !strcmp(argv[1], "m") ? XK_m
              : !strcmp(argv[1], "enter") ? XK_Return : !strcmp(argv[1], "espaco") ? XK_space : NoSymbol;
    if (ks == NoSymbol) { fprintf(stderr, "xtecla: tecla desconhecida: %s\n", argv[1]); XCloseDisplay(d); return 2; }
    KeyCode kc = XKeysymToKeycode(d, ks);
    XKeyEvent e;
    memset(&e, 0, sizeof e);
    e.display = d;
    e.window = w;
    e.root = DefaultRootWindow(d);
    e.subwindow = None;
    e.same_screen = True;
    e.keycode = kc;
    e.type = KeyPress;
    e.time = CurrentTime;
    XSendEvent(d, w, True, KeyPressMask, (XEvent *)&e);
    XSync(d, False);
    /* alguns quadros apertada: a raylib só vê "apertou" se a tecla ficou pra baixo num quadro */
    struct timespec pausa = {0, 120 * 1000 * 1000};
    nanosleep(&pausa, NULL);
    e.type = KeyRelease;
    XSendEvent(d, w, True, KeyReleaseMask, (XEvent *)&e);
    XSync(d, False);
    XCloseDisplay(d);
    return 0;
}
