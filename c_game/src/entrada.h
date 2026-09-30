/*
 * entrada.h - de que instante foi o clique, no relógio do núcleo. Puro, sem raylib.
 *
 * O jogo só sabe em que quadro o clique foi lido, e sem mais nada o põe no meio dele (duel_step):
 * ±½ quadro de erro, 8 ms a 60 Hz, quando a janela perfeita dos mestres finais tem 44 ms. Cada
 * plataforma pode dar o instante de hardware do clique (o carimbo: src/entrada_plat.h). Este módulo
 * converte o carimbo para o relógio do núcleo e decide se dá para confiar nele; quando não dá, o
 * aperto cai no meio do quadro, exatamente como antes.
 */
#ifndef APARA_ENTRADA_H
#define APARA_ENTRADA_H

#include <math.h>
#include <stdbool.h>

/* Um aperto que veio sem carimbo (gamepad, Wayland, plataforma sem suporte). */
#define ENTRADA_SEM_CARIMBO NAN

/*
 * Em que instante, dentro do passo que o núcleo vai dar, o aperto aconteceu.
 *
 *   carimbo    instante do clique, no relógio do sistema (segundos), ou ENTRADA_SEM_CARIMBO
 *   fim        instante do sistema em que o quadro foi lido (o poll): o fim do quadro
 *   quadro     duração real do quadro (o dt que o jogo mediu): o clique é deste intervalo
 *   corrido    a parte do quadro em que o duelo correu, no tempo real (quadro, ou o que sobrou
 *              dele depois de um hitstop); o começo dela é fim - corrido
 *   passo      o dt que o núcleo vai andar neste quadro (corrido x câmera lenta)
 *
 * Devolve os segundos, em [0, passo], a contar do começo do passo, para o duel_step_at. Um clique que
 * o hitstop congelou (antes do começo de `corrido`) vale 0, como sempre valeu. *valido diz se o carimbo
 * foi usado; se não, devolve passo/2, o mesmo instante de duel_step(d, passo, true).
 * Inválido: carimbo ausente ou não finito; quadro <= 0 ou maior que AJ_PAUSA_POR_TRAVAMENTO (a
 * pausa por travamento); corrido fora de (0, quadro]; passo não finito ou < 0; carimbo mais de
 * AJ_CARIMBO_MARGEM fora do quadro. Nunca devolve NaN nem um instante fora do passo.
 */
double entrada_no_quadro(double carimbo, double fim, double quadro, double corrido, double passo, bool *valido);

#endif
