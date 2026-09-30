/*
 * entrada_plat.h - de onde vêm os carimbos: o instante de hardware do clique esquerdo e das teclas de
 * aparar (Espaço, J, Enter), dado pelo sistema, sem depender de quando o jogo lê o quadro. Um arquivo por
 * plataforma, e o Makefile escolhe:
 *   entrada_mac.m     macOS: NSEvent.timestamp, num monitor local (só vê os eventos do próprio jogo)
 *   entrada_linux.c   X11: eventos brutos do XInput2, numa thread que marca a chegada
 *   entrada_stub.c    o resto (Wayland puro, Windows): sem carimbo, e todo aperto cai no meio do quadro
 * Quem lê os carimbos é o main.c, e quem os converte para o núcleo é o entrada.c.
 */
#ifndef APARA_ENTRADA_PLAT_H
#define APARA_ENTRADA_PLAT_H

#include <stdbool.h>

/* O relógio dos carimbos (segundos, monotônico, de qualquer origem). O jogo o lê no começo de cada quadro. */
double entrada_relogio(void);

/* Antes de InitWindow (o X11 pede XInitThreads antes de qualquer outra chamada). */
void entrada_preparar(void);

/* Depois de InitWindow: passa a carimbar. false = esta plataforma não dá carimbo. */
bool entrada_iniciar(void);

/* Tira da fila os carimbos que chegaram com instante <= ate, em ordem, e devolve quantos (no máximo
 * `max`). O que veio depois de `ate` fica para o quadro seguinte. */
int entrada_coletar(double *carimbos, int max, double ate);

#endif
