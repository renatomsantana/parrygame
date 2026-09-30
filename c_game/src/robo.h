/*
 * robo.h - robôs que jogam o duelo. O perfeito é o do demo (--demo) e o dos
 * testes; os outros medem a curva de dificuldade (make robos). Não dependem da
 * raylib: jogam um quadro por vez, como o jogo.
 *
 * Os robôs são estimativa, não verdade: o humano deles não vê a arte, não sofre
 * com a escuridão do Yoru e não se distrai.
 */
#ifndef APARA_ROBO_H
#define APARA_ROBO_H

#include "core.h"

typedef enum {
    ROBO_PERFEITO,   /* aperta como o demo: no máximo AJ_ROBO_ANTECEDENCIA antes do contato */
    ROBO_NUNCA,      /* nunca aperta */
    ROBO_SPAM,       /* aperta a cada `periodo` segundos, sem olhar */
    ROBO_REACAO,     /* não decora nada: reage ao último sinal antes do contato (a lâmina
                        partindo, que se ouve no assobio do golpe, ou o aviso, o que vier
                        depois) depois de `reacao` s; ao som, 50 ms mais rápido */
    ROBO_HUMANO      /* decora o padrão: mede o tempo até o contato a partir do último sinal
                        que ainda dá tempo de usar (o começo da preparação ou o aviso) e
                        erra em proporção ao intervalo medido (`ritmo`) mais a mão (`mao`) */
} RoboTipo;

typedef struct {
    RoboTipo tipo;
    float reacao;    /* s até reagir a um sinal (REACAO, HUMANO) */
    float ritmo;     /* HUMANO: erro ao medir um intervalo, fração dele (0,08 = casual) */
    float mao;       /* REACAO, HUMANO: desvio-padrão da mão, s */
    float periodo;   /* SPAM: s entre apertos */
} Robo;

#define ROBO_QUADRO (1.0 / 60.0)      /* o robô joga em quadros de 60 Hz, como o jogo */
#define ROBO_REACAO_SOM 0.050         /* ao som se reage este tanto mais rápido que à imagem */

/* Os robôs do relatório. */
extern const Robo ROBO_DO_DEMO, ROBO_SEM_DEFESA, ROBO_APERTA_SEM_PARAR, ROBO_HUMANO_CASUAL;
Robo robo_reacao(float segundos);

/* O que o robô pensa durante uma luta. */
typedef struct {
    Robo r;
    Rng rng;
    int ataque;      /* último golpe planejado */
    double aperta;   /* instante planejado do aperto (-1 = nenhum) */
    double spam;     /* próximo aperto do spam */
    double pend[4];  /* robo_aperto_em: apertos planejados que ainda não chegaram */
    int npend;
} RoboMente;

void robo_iniciar(RoboMente *m, const Robo *r, uint32_t semente);
/* Chamado no começo de cada quadro, antes de duel_step: o robô aperta neste quadro? O aperto
 * entra no meio dele, como o do jogo (que só sabe em que quadro o clique veio). */
bool robo_quer_apertar(RoboMente *m, const Duel *d, double dt);
/* Decide em ms, sem depender de quadros: quando, dentro do quadro que começa em d->clock, o robô
 * aperta (segundos depois do começo do quadro; -1 = não aperta neste quadro). Vale para duel_step_at. */
double robo_aperto_em(RoboMente *m, const Duel *d, double dt);

typedef struct {
    bool vitoria;
    int perfeitos, bons, erros;
    double duracao;  /* s de duelo */
} RoboLuta;

/* Uma luta inteira contra `m`, com kojiro depois de `vencidos` mestres. O robô decide em ms e
 * aperta no instante exato, a `hz` quadros por segundo (o resultado não depende de `hz`).
 * `quadros`: o aperto entra no meio do quadro, como no jogo (o resultado passa a depender de `hz`). */
RoboLuta robo_lutar_hz(const Robo *r, const MasterProfile *m, int vencidos, uint32_t semente, double hz, bool quadros);
RoboLuta robo_lutar(const Robo *r, const MasterProfile *m, int vencidos, uint32_t semente);   /* 60 Hz, em ms */

#endif
