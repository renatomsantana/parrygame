/*
 * ajuste.h - as constantes globais de equilíbrio e de sensação do duelo, num
 * lugar só. O que é de cada mestre (janelas de parry, preparações, sequências,
 * dano, traços) fica no roster.c.
 *
 * Tempos em segundos; vida e postura em pontos. Mexeu num número daqui? Rode
 * `make test` (regras e janelas viáveis) e `make robos` (curva de dificuldade).
 */
#ifndef APARA_AJUSTE_H
#define APARA_AJUSTE_H

/* ---- Vida de kojiro -------------------------------------------------- */
#define AJ_VIDA_INICIAL            250.0f   /* vida de kojiro, a mesma contra todos: quantos erros ele
                                               aguenta é de cada mestre (hitsToFall, no roster.c) */

/* ---- O que cada resultado do parry faz ------------------------------- */
#define AJ_PERFEITO_POSTURA         20.0f   /* perfeito: o mestre perde isto de postura */
#define AJ_PERFEITO_POSTURA_NIVEL    2.0f   /*   + isto por mestre já vencido */
#define AJ_PERFEITO_CURA             0.03f  /* perfeito: kojiro recupera esta fração da vida (3%) */
#define AJ_BOM_POSTURA               6.0f   /* bom: o mestre perde isto de postura */
#define AJ_BOM_POSTURA_NIVEL         0.5f   /*   + isto por mestre já vencido */
#define AJ_BOM_CUSTO                 4.0f   /* bom: o impacto ainda tira isto de vida */
#define AJ_ERRO_MESTRE_RECUPERA     20.0f   /* erro: o mestre com cura recupera isto de postura */
#define AJ_SELO_CURA                 1.00f  /* oboro: cada selo quebrado devolve esta fração da vida (1 = cheia) */

/* ---- Tempos do duelo ------------------------------------------------- */
#define AJ_LAMINA_PARTE            0.220f   /* a lâmina parte este tempo antes do contato */
#define AJ_LANCA_PARTE_X            1.75f   /* a estocada de longe parte 1,75 x mais cedo */
/* Lâmina variável: do garfiel em diante, a lâmina parte um tempo sorteado a cada golpe
 * antes do contato, para que reagir à partida não baste (vale o ritmo do aviso). O
 * aviso e o contato não mudam; nem os quadros do golpe, nem quanto cada um dura: muda
 * só quando começam o bote, o rastro e o assobio. A lança e a investida (o primeiro
 * golpe delas toca quadros na partida) partem sempre no tempo fixo. */
#define AJ_LAMINA_VARIAVEL             1    /* 0 = fixa, sempre AJ_LAMINA_PARTE (como antes) */
#define AJ_LAMINA_MIN              0.140f
#define AJ_LAMINA_MAX              0.320f
#define AJ_LAMINA_MAX_CADEIA       0.240f   /* dentro da sequência (mais que isto atrasaria o ritmo) */
#define AJ_LAMINA_VARIA_DESDE          5    /* id do primeiro mestre com a lâmina variável (garfiel) */
#define AJ_ENTRE_GESTOS            0.300f   /* fora da preparação, intervalo mínimo entre dois apertos */
/* Aperto cedo: antes do aviso, apertar não trava o golpe, mas dá uma recarga de no
 * máximo isto, que termina no aviso (nunca cobre a janela boa, que vem depois dele), e
 * a defesa daquele golpe não sai perfeita (no máximo bom). Depois do aviso vale uma
 * tentativa só. */
#define AJ_RECARGA_CEDO            0.500f
#define AJ_TARDE_JANELA            0.250f   /* um aperto até isto depois de um golpe que entrou é "tarde" */
/* Tolerância tardia: um aperto até isto depois do contato ainda defende, como bom
 * (nunca perfeito). Sem aperto, o golpe só entra quando ela acaba. */
#define AJ_TOLERANCIA_TARDIA       0.030f

/* ---- Calibração de latência (opções: tela de teste) -------------------- */
/* O atraso de vídeo entra no julgamento: o aperto conta esse tanto mais cedo. O de
 * áudio adianta o som do aviso para chegar junto com o brilho. Com mais de ~70 ms, as
 * sequências mais rápidas (400 ms) podem atrasar um pouco: o golpe seguinte só começa
 * depois de o anterior ser julgado. */
#define AJ_LATENCIA_MAX            0.120f   /* o maior atraso aceito */
#define AJ_CALIBRA_BATIDA          0.800f   /* a tela de teste pisca (ou toca) a cada isto */
#define AJ_CALIBRA_APERTOS             8    /* apertos que contam para a média */
#define AJ_CALIBRA_ACEITA          0.300f   /* aperto mais longe que isto da batida não conta */
#define AJ_PAUSA_SEQUENCIA         0.800f   /* pausa depois de cada sequência */
#define AJ_PAUSA_SELO              2.200f   /* pausa depois de quebrar um selo do oboro */
#define AJ_PAUSA_INICIO            0.650f   /* pausa antes do primeiro golpe */
#define AJ_PRESSA                   0.90f   /* com metade da postura, a preparação x isto */
#define AJ_PAUSA_NA_CADEIA         0.080f   /* pausa entre dois golpes da mesma sequência */
#define AJ_CADEIA_MIN              0.400f   /* menor intervalo entre dois contatos de uma sequência */
#define AJ_BRASAS_TEMPO              3.0f   /* enjin: segundos em brasas depois de um erro */

/* ---- O aviso: som e brilho na lâmina, sempre o mesmo tempo antes do contato ---- */
/* Cada mestre tem o seu (Stance.aviso, no roster.c), dentro destes limites. Na
 * sequência, o aviso de cada golpe depois do primeiro é o contato anterior (o
 * ritmo), com um brilho menor quando a preparação seguinte começa. */
#define AJ_AVISO_PRIMEIRO          0.450f   /* o aviso do daichi, o primeiro da trilha */
#define AJ_AVISO_MENOR             0.320f   /* nenhum aviso vem mais tarde que isto antes do contato */
#define AJ_AVISO_SO_BRILHO         0.350f   /* quem avisa só com o brilho (jinshi, sem som) avisa ao menos isto antes */
#define AJ_PREPARO_ANTES_DO_AVISO  0.100f   /* a preparação começa pelo menos isto antes do aviso */

/* ---- Sensação: hitstop (o duelo congela), tremor, câmera lenta, recuo -- */
#define AJ_HITSTOP_PERFEITO        0.090f
#define AJ_HITSTOP_BOM             0.045f
#define AJ_HITSTOP_ERRO            0.045f
#define AJ_HITSTOP_QUEBRA          0.160f   /* o parry que quebra a postura */
/* O hitstop congela o duelo. Dentro de uma sequência ele sai da preparação seguinte:
 * o próximo contato chega no mesmo tempo real, qualquer que seja o resultado. */
#define AJ_PREPARO_MIN_CADEIA      0.000f   /* na sequência, a preparação é pelo menos a partida da lâmina mais isto */

#define AJ_TREMOR_PERFEITO           1.5f   /* força (px) do tremor da tela */
#define AJ_TREMOR_PERFEITO_TEMPO    0.10f   /*   e quanto dura */
#define AJ_TREMOR_ERRO               3.0f
#define AJ_TREMOR_ERRO_TEMPO        0.20f
#define AJ_TREMOR_SEGUNDA_LAMINA     2.5f   /* golpe duplo: a segunda lâmina entrou */
#define AJ_TREMOR_SEGUNDA_TEMPO     0.18f
#define AJ_TREMOR_QUEBRA             4.0f
#define AJ_TREMOR_QUEBRA_TEMPO      0.35f
#define AJ_TREMOR_ESPADA_CRAVA       1.5f   /* a espada do mestre desarmado crava no chão */
#define AJ_TREMOR_ESPADA_TEMPO      0.15f

#define AJ_LENTA_QUEBRA              0.3f   /* velocidade do tempo depois de quebrar um selo */
#define AJ_LENTA_QUEBRA_TEMPO        0.8f   /*   por quantos segundos reais */
#define AJ_LENTA_VITORIA             0.3f   /* o desarme */
#define AJ_LENTA_VITORIA_TEMPO       1.0f
#define AJ_LENTA_QUEDA               0.4f   /* kojiro cai */
#define AJ_LENTA_QUEDA_TEMPO         0.9f

#define AJ_RECUO_PERFEITO_MESTRE     7.0f   /* px que cada um recua no choque */
#define AJ_RECUO_PERFEITO_KOJIRO     1.0f
#define AJ_RECUO_BOM_MESTRE          3.0f
#define AJ_RECUO_BOM_KOJIRO          4.0f
#define AJ_RECUO_ERRO_KOJIRO         9.0f
#define AJ_RECUO_SEGUNDA_LAMINA      8.0f
#define AJ_RECUO_QUEBRA_MESTRE      10.0f

/* ---- Robô do demo (--demo) e dos testes -------------------------------- */
#define AJ_ROBO_ANTECEDENCIA       0.030f   /* aperta no máximo este tempo antes do contato (menos que a menor janela perfeita) */

#endif
