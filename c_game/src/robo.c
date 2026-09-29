/*
 * robo.c - os robôs do demo, dos testes e do relatório de dificuldade.
 */
#include "robo.h"

#include <math.h>
#include <string.h>

const Robo ROBO_DO_DEMO = {ROBO_PERFEITO, 0, 0, 0, 0};
const Robo ROBO_SEM_DEFESA = {ROBO_NUNCA, 0, 0, 0, 0};
const Robo ROBO_APERTA_SEM_PARAR = {ROBO_SPAM, 0, 0, 0, 0.15f};
const Robo ROBO_HUMANO_CASUAL = {ROBO_HUMANO, 0.25f, 0.08f, 0.025f, 0};

Robo robo_reacao(float segundos) {
    Robo r = {ROBO_REACAO, segundos, 0, 0.020f, 0};
    return r;
}

void robo_iniciar(RoboMente *m, const Robo *r, uint32_t semente) {
    memset(m, 0, sizeof *m);
    m->r = *r;
    rng_seed(&m->rng, semente ^ 0x9E3779B9u);
    m->ataque = -1;
    m->aperta = -1;
}

/* Normal padrão (Box-Muller). */
static double normal(Rng *g) {
    double u = rng_next(g), v = rng_next(g);
    if (u < 1e-12) u = 1e-12;
    return sqrt(-2 * log(u)) * cos(6.283185307179586 * v);
}

/* Decide, no começo do golpe, quando apertar. */
static double planejar(RoboMente *m, const Duel *d) {
    const Robo *r = &m->r;
    double contato = d->strikeAt;
    double inicio = contato - d->windupDuration;          /* começo da preparação */
    double aviso = duel_cue_time(d);
    bool avisoPercebido = d->m->cueAudio > 0 || d->m->cueVisual > 0;   /* o som ou o brilho */
    switch (r->tipo) {
        case ROBO_REACAO: {
            /* a lâmina partindo se vê e se ouve (o assobio do golpe) */
            double partida = contato - duel_strike_lead(d);
            double sinal = partida, reacao = r->reacao - ROBO_REACAO_SOM;
            if (avisoPercebido && aviso > partida) {
                sinal = aviso;
                reacao = r->reacao - (d->m->cueAudio > 0 ? ROBO_REACAO_SOM : 0);
            }
            return sinal + reacao + normal(&m->rng) * r->mao;
        }
        case ROBO_HUMANO: {
            /* o último sinal que ainda dá tempo de usar */
            double ancora = inicio;
            if (avisoPercebido && aviso > ancora && contato - aviso >= r->reacao) ancora = aviso;
            double T = contato - ancora;
            double mira = duel_stance(d)->perfectWindow * 0.5;
            double erro = sqrt(r->mao * r->mao + (r->ritmo * T) * (r->ritmo * T));
            double t = contato - mira + normal(&m->rng) * erro;
            if (t < ancora + r->reacao) t = ancora + r->reacao;
            return t;
        }
        default:
            return -1;
    }
}

bool robo_quer_apertar(RoboMente *m, const Duel *d, double dt) {
    if (d->phase == PH_FINISHED) return false;
    switch (m->r.tipo) {
        case ROBO_NUNCA:
            return false;
        case ROBO_PERFEITO:
            /* o aperto entra no meio do quadro (duel_step) */
            return d->phase == PH_WINDUP && !d->attempted && d->strikeAt - d->clock <= dt * 0.5 + AJ_ROBO_ANTECEDENCIA;
        case ROBO_SPAM:
            if (d->clock + dt * 0.5 < m->spam) return false;
            m->spam = d->clock + m->r.periodo;
            return true;
        default:
            break;
    }
    if (d->phase != PH_WINDUP) {
        m->aperta = -1;
        return false;
    }
    if (d->attacks != m->ataque) {
        m->ataque = d->attacks;
        m->aperta = planejar(m, d);
    }
    /* aperta no quadro em que o instante planejado cai (o aperto entra no meio dele) */
    if (m->aperta >= 0 && m->aperta < d->clock + dt) {
        m->aperta = -1;
        return true;
    }
    return false;
}

RoboLuta robo_lutar(const Robo *r, const MasterProfile *m, int vencidos, uint32_t semente) {
    Settings s;
    settings_default(&s);
    settings_for_level(&s, vencidos);
    Duel d;
    duel_init(&d, &s, m, semente);
    RoboMente mente;
    robo_iniciar(&mente, r, semente);
    RoboLuta out;
    memset(&out, 0, sizeof out);
    DuelEvent ev[MAX_EVENTS];
    while (d.phase != PH_FINISHED && d.clock < 1800) {
        bool aperta = robo_quer_apertar(&mente, &d, ROBO_QUADRO);
        duel_step(&d, ROBO_QUADRO, aperta);
        int n = duel_drain(&d, ev, MAX_EVENTS);
        for (int i = 0; i < n; i++) {
            if (ev[i].kind == EV_IMPACT) {
                if (ev[i].judgement == J_PERFEITO) out.perfeitos++;
                else if (ev[i].judgement == J_BOM) out.bons++;
                else out.erros++;
            }
            if (ev[i].kind == EV_FINISHED) out.vitoria = ev[i].flag;
        }
    }
    out.duracao = d.clock;
    out.vida = d.renPosture / s.renPosture;
    return out;
}
