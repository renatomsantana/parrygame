/*
 * entrada_linux.c - carimbos no X11. Uma segunda conexão ao servidor pede os eventos brutos do
 * XInput2 (botão e tecla, direto do dispositivo, sem depender de qual janela está em foco) e uma thread
 * marca o instante em que cada um chega: uns 0,1 ms depois do hardware, e sem esperar o poll do GLFW,
 * que só acontece depois do swap do vsync. Só o botão esquerdo e as teclas Espaço, J e Enter contam (as
 * mesmas de pressed() no main.c); a repetição de tecla não. Quem decide se o carimbo pertence a um aperto
 * que o jogo leu é o main.c: o carimbo de um clique que a janela não recebeu (fora de foco) nunca é usado.
 * No Wayland puro não há evento bruto para receber: a fila fica vazia e todo aperto cai no meio do quadro.
 */
#define _POSIX_C_SOURCE 200809L
#include "entrada_plat.h"

#include "entrada_fila.h"

#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/keysym.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

static EntradaFila fila;
static pthread_mutex_t trava = PTHREAD_MUTEX_INITIALIZER;
static pthread_t leitora;
static atomic_int estado;    /* 0 começando, 1 recebendo, -1 sem XInput2 ou sem servidor */

double entrada_relogio(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static void empilha(double t) {
    pthread_mutex_lock(&trava);
    fila_empilha(&fila, t);
    pthread_mutex_unlock(&trava);
}

static void *le_eventos(void *arg) {
    (void)arg;
    Display *d = XOpenDisplay(NULL);
    if (!d) { estado = -1; return NULL; }
    int op, ev, err, maior = 2, menor = 0;
    if (!XQueryExtension(d, "XInputExtension", &op, &ev, &err) || XIQueryVersion(d, &maior, &menor) != Success) { XCloseDisplay(d); estado = -1; return NULL; }
    unsigned char mascara[XIMaskLen(XI_LASTEVENT)] = {0};
    XISetMask(mascara, XI_RawButtonPress);
    XISetMask(mascara, XI_RawKeyPress);
    XISetMask(mascara, XI_RawKeyRelease);
    XIEventMask sel = {XIAllMasterDevices, (int)sizeof mascara, mascara};
    XISelectEvents(d, DefaultRootWindow(d), &sel, 1);
    XFlush(d);
    /* os códigos de tecla da disposição de agora (o GLFW conta as teclas pela posição; numa disposição
     * estranha o pior que acontece é um aperto sem carimbo, que cai no meio do quadro) */
    const KeyCode teclas[] = {XKeysymToKeycode(d, XK_space), XKeysymToKeycode(d, XK_j), XKeysymToKeycode(d, XK_Return)};
    bool baixa[256] = {false};    /* a tecla está apertada: a repetição do teclado não é um aperto novo */
    estado = 1;
    for (;;) {
        XEvent e;
        XNextEvent(d, &e);
        if (e.xcookie.type != GenericEvent || e.xcookie.extension != op || !XGetEventData(d, &e.xcookie)) continue;
        double t = entrada_relogio();
        const XIRawEvent *r = e.xcookie.data;
        if (e.xcookie.evtype == XI_RawButtonPress) {
            if (r->detail == 1) empilha(t);
        } else if (r->detail >= 0 && r->detail < 256) {
            if (e.xcookie.evtype == XI_RawKeyRelease) baixa[r->detail] = false;
            else if (!baixa[r->detail]) {
                baixa[r->detail] = true;
                for (unsigned i = 0; i < sizeof teclas / sizeof teclas[0]; i++)
                    if (teclas[i] && r->detail == teclas[i]) empilha(t);
            }
        }
        XFreeEventData(d, &e.xcookie);
    }
    return NULL;
}

void entrada_preparar(void) { XInitThreads(); }

/* Espera a thread dizer se conseguiu (o jogo não abre mais devagar por isso: são uns milissegundos). */
bool entrada_iniciar(void) {
    if (pthread_create(&leitora, NULL, le_eventos, NULL) != 0) return false;
    pthread_detach(leitora);
    struct timespec um_ms = {0, 1000000};
    for (int i = 0; i < 500 && estado == 0; i++) nanosleep(&um_ms, NULL);
    return estado == 1;
}

int entrada_coletar(double *carimbos, int max, double ate) {
    pthread_mutex_lock(&trava);
    int n = fila_coleta(&fila, carimbos, max, ate);
    pthread_mutex_unlock(&trava);
    return n;
}
