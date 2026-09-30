/*
 * entrada_mac.m - carimbos no macOS. Um monitor local de eventos (só os do próprio jogo, sem pedir
 * permissão nenhuma) lê NSEvent.timestamp, que é o instante de hardware do clique, em segundos desde o
 * boot: o mesmo relógio de NSProcessInfo.systemUptime, que é o que entrada_relogio() devolve. O monitor
 * roda dentro do poll do GLFW, tarde, mas o carimbo é o original. Só o botão esquerdo e as teclas Espaço,
 * J e Enter contam (as de pressed() no main.c); a repetição de tecla não.
 */
#import <Cocoa/Cocoa.h>

#include "entrada_plat.h"

#include "entrada_fila.h"

static EntradaFila fila;   /* só a thread principal a toca: o monitor roda dentro do poll */
static id monitor;

double entrada_relogio(void) { return [[NSProcessInfo processInfo] systemUptime]; }

void entrada_preparar(void) {}

bool entrada_iniciar(void) {
    if (monitor) return true;
    monitor = [NSEvent addLocalMonitorForEventsMatchingMask:(NSEventMaskLeftMouseDown | NSEventMaskKeyDown)
                                                    handler:^NSEvent *(NSEvent *e) {
        /* os códigos de tecla do macOS são posições: 49 espaço, 38 J, 36 Enter (o do teclado numérico não) */
        if (e.type == NSEventTypeKeyDown && (e.isARepeat || (e.keyCode != 49 && e.keyCode != 38 && e.keyCode != 36))) return e;
        fila_empilha(&fila, e.timestamp);
        return e;
    }];
    return monitor != nil;
}

int entrada_coletar(double *carimbos, int max, double ate) { return fila_coleta(&fila, carimbos, max, ate); }
