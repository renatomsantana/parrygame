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

/* Velocidade do deslocamento visual; não altera quadros, aviso ou contato. */
#define AJ_SHIZUKU_DASH_X          1.15f

/* ---- Calibração de latência (opções: tela de teste) -------------------- */
/* O atraso de vídeo entra no julgamento: o aperto conta esse tanto mais cedo. O de
 * áudio adianta o som do aviso para chegar junto com o brilho. Um golpe sem defesa só é
 * julgado depois da tolerância tardia mais o atraso, e o golpe seguinte da sequência só
 * começa depois: a lâmina dele parte no máximo o tempo que falta até o contato (nunca
 * menos de AJ_LAMINA_MIN), e o ritmo da sequência não atrasa com atraso nenhum. */
#define AJ_LATENCIA_MAX            0.120f   /* o maior atraso aceito */
#define AJ_CALIBRA_BATIDA          0.800f   /* a tela de teste pisca (ou toca) a cada isto */
#define AJ_CALIBRA_APERTOS             8    /* apertos que contam para a média */
#define AJ_CALIBRA_ACEITA          0.300f   /* aperto mais longe que isto da batida não conta */
/* Ritmo: o tempo parado entre os golpes. A espera antes do aviso (o mestre segurando a
 * preparação) e a pausa depois de cada sequência são só tempo morto: encurtar os dois
 * deixa a luta mais viva sem tocar em nada que se julga. Do aviso ao contato, as
 * janelas e o intervalo dentro da sequência não mudam. Antes do primeiro corte: espera
 * x1,0 e pausa 0,8 s; no segundo: x0,60 e 0,55 s; agora: x0,50 e 0,40 s. A pausa não
 * pode cair abaixo do fim da animação de recuperação do mestre (0,32 s), e tem de ser
 * maior que AJ_TARDE_JANELA mais uma folga (o "tarde" acaba dentro da pausa). */
#define AJ_ESPERA_X                 0.50f   /* a espera antes do aviso x isto (1 = como antes) */
/* Hayate e Jinshi têm ritmo irregular (traço aleatório na preparação) e o piso de
 * AJ_PREPARO_ANTES_DO_AVISO já pega uma sequência em cada dez neles: a espera deles encurta
 * menos (MasterProfile.waitScale, no roster.c). */
#define AJ_ESPERA_X_IRREGULAR       0.60f
#define AJ_PAUSA_SEQUENCIA         0.400f   /* pausa depois de cada sequência */
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

/* ---- Rastro fantasma do golpe (só visual) ------------------------------- */
/* Na partida da lâmina, o mestre deixa fantasmas do quadro que ele já está mostrando (nunca
 * o de contato: esse só aparece no impacto, no instante do julgamento), esticados para
 * trás, que crescem até o contato e somem nele. É só desenho: não toca em tempo de
 * julgamento, âncora, hitbox nem duração de quadro. */
#define AJ_DESLIZE_GOLPE             1      /* 0 = desligado (o corpo salta de uma vez no contato, como antes). A prancha traz o avanço do golpe pronto, no quadro de contato, sem quadro no meio: na partida da lâmina o mestre desliza esse avanço (só desenho; no contato ele está exatamente onde a prancha põe) */
#define AJ_DESLIZE_MIN            6.0f      /* px: um salto menor que isto fica como a prancha tem */
#define AJ_DESLIZE_MAX           40.0f      /* px: o maior avanço que se espalha */
#define AJ_RASTRO_FANTASMA           0      /* cópias do corpo: opcionais; os PNGs já têm rastros */
#define AJ_RASTRO_FANTASMAS          3      /* quantos, atrás do mestre */
#define AJ_RASTRO_ESPACO          5.0f      /* px de um para o outro, no fim da partida */
#define AJ_RASTRO_ALFA           0.72f      /* opacidade do primeiro, no fim da partida */
#define AJ_RASTRO_FIO_ALFA       0.84f      /* brilho do traço preso à arma, no fim da partida */

/* Substituição do pack comprado: garras laranja do Garfiel e seu eco. */
#define AJ_PACK_SLASH              1       /* APARA_SLASH=0 compara com a tira original */
#define AJ_PACK_SLASH_ANTES     0.10f       /* começa após a partida, até isto antes do contato */
#define AJ_PACK_SLASH_CAUDA     0.12f       /* relógio do duelo: acompanha o hitstop */
#define AJ_PACK_SLASH_ALFA      0.92f

/* ---- Robô do demo (--demo) e dos testes -------------------------------- */
#define AJ_ROBO_ANTECEDENCIA       0.030f   /* aperta no máximo este tempo antes do contato (menos que a menor janela perfeita) */
#define AJ_ROBO_SPAM_PERIODO       0.150f   /* o que martela o botão aperta a cada isto */
#define AJ_ROBO_CASUAL_REACAO      0.250f   /* o humano casual: só usa um sinal que chegue com esta folga */
#define AJ_ROBO_CASUAL_RITMO       0.080f   /*   erro ao medir um intervalo: esta fração dele */
#define AJ_ROBO_CASUAL_MAO         0.025f   /*   erro fixo da mão (desvio padrão, s) */
#define AJ_ROBO_REACAO_MAO         0.020f   /* o que só reage: erro da mão (desvio padrão, s) */
#define AJ_ROBO_PLANOS                4     /* apertos planejados que ainda não chegaram (robo_aperto_em) */
#define AJ_ROBO_LUTA_MAX          1800.0    /* uma luta de robô que passa disto é abandonada (s) */
#define AJ_ROBO_HZ_PADRAO           60.0    /* taxa de quadros do robô quando ninguém escolhe */

/* ---- Ritmo das cenas e das telas (tempo do jogo; nada disto entra no julgamento) ---------- */
/* Depois de quebrar um selo do oboro: o mestre se recompõe, e no 1º e no 2º selo a cena de fala
 * começa este tempo depois da quebra (tempo de jogo, na câmera lenta da quebra). */
#define AJ_QUEBRA_ATE_A_CENA        1.0f
/* De volta ao duelo depois da cena do selo, o próximo golpe só começa depois disto (o grito). */
#define AJ_PAUSA_APOS_CENA_SELO     1.8
#define AJ_ESPADA_CRAVADA_ESPERA    0.8f    /* a espada do mestre crava; depois disto vem a fala final ou a cena */
#define AJ_SILENCIO_QUEBRA          0.5f    /* a música abaixa depois de quebrar a postura */
#define AJ_SILENCIO_DESARME         1.0f    /* ...depois do desarme */
#define AJ_SILENCIO_MORTE_OBORO     1.5f    /* ...e no golpe que mata o oboro */
#define AJ_CENA_SAIDA_APRENDIZ      1.8f    /* kojiro sai antes de o assassino aparecer */
#define AJ_CENA_ONI_PREPARA         0.5f    /* entrada da figura mascarada antes de sacar o golpe */
#define AJ_CENA_ONI_DURACAO         2.5f    /* golpe e queda do aprendiz; não faz parte do duelo */
/* Quem aperta sem parar, sem ler, tem de ver o resultado: a palavra "derrota" e o pergaminho da vitória entram em fade
 * (AJ_FADE_RESULTADO) e têm de estar inteiros antes do primeiro clique que vale: na derrota, o título já inteiro com uma
 * margem de 0,06 s antes de as opções aceitarem clique (como sempre foi: 0,07 s); na vitória, o pergaminho inteiro por
 * ao menos 0,3 s. O core_test confere as duas contas. */
#define AJ_FADE_RESULTADO           (1.0f / 3)   /* as telas de derrota e de vitória entram em fade neste tempo */
#define AJ_DERROTA_TITULO           0.8f    /* tela de derrota: a palavra "derrota" aparece depois disto (antes 1,2) */
#define AJ_DERROTA_OPCOES           1.2f    /* ...e as opções, depois disto (só então aceitam clique) (antes 1,6) */
#define AJ_VITORIA_TRAVA            0.7f    /* tela de vitória: o clique só vale depois disto (antes 1,0) */
#define AJ_ESCOLHA_TRAVA            1.5f    /* a escolha do final (poupar ou matar): idem */
#define AJ_DICA_PRIMEIRO_DUELO      1.0f    /* a dica "como se apara" aparece depois disto, no primeiro duelo */
#define AJ_FINAL_TITULO             2.2f    /* o título do final aparece depois disto */
#define AJ_FINAL_TRAVA              3.5f    /* o clique que fecha o final só vale depois disto */
#define AJ_TRAVA_CLIQUE_TITULO      0.3f    /* título e trilha: um clique logo depois de trocar de tela não conta */
#define AJ_TRAVA_CLIQUE_TRILHA      0.4f
#define AJ_TRAVA_CLIQUE_CALIBRA     0.4f
#define AJ_TRAVA_CLIQUE_FALA        0.25f   /* falas e cenas: o clique que pula a fala só vale depois disto */
/* Um quadro mais longo que isto (arrastar a janela, um travamento) pausa o duelo em vez de engoli-lo. */
#define AJ_PAUSA_POR_TRAVAMENTO     0.2f

/* ---- Jogo automático (APARA_AUTO e --demo): o robô também clica nas telas ------------------ */
#define AJ_AUTO_CLIQUE_PERIODO      0.9f    /* fora do duelo, um clique a cada isto */
#define AJ_FPS_ALVO                 60     /* limite normal, mesmo em monitor de 120/144 Hz */
#define AJ_FPS_MARGEM_PRECISA       0.002  /* últimos 2 ms evitam ultrapassar o prazo por precisão do sleep */
#define AJ_CENARIO_QUADRO           0.125f /* animação opcional do fundo: 8 quadros/s, independente do combate */
#define AJ_AUTO_TEMPO_MAX           900.0f  /* o jogo automático para depois disto (s de jogo) */
#define AJ_AUTO_VISITA_FIM          1.5f    /* ...ou este tempo depois de entrar na cabana do hanzo */
#define AJ_AUTO_ESCOLHA_ESCOLHE     3.0f    /* a escolha do final: o robô escolhe depois disto */
#define AJ_AUTO_ESCOLHA_CONFIRMA    4.0f    /* ...e confirma depois disto */

/* ---- Carimbo do aperto: o instante de hardware do clique (src/entrada.c) ---------------------- */
/* Um carimbo até isto fora do quadro ainda vale (o clique chegou logo depois do poll anterior, ou o
 * relógio do sistema e o do jogo diferem por poeira); fora disto é ruído e o aperto cai no meio do
 * quadro, como sempre foi. */
#define AJ_CARIMBO_MARGEM           0.005
#define AJ_CARIMBO_MEDIDAS          20      /* --carimbo mede este tanto de cliques e sai */

/* ---- Tolerâncias numéricas (não são regras: só impedem que o ruído do ponto flutuante decida) ---- */
#define AJ_EPS_TEMPO                1e-9    /* dois instantes a menos disto de distância são o mesmo */
#define AJ_EPS_JANELA               1e-6    /* um aperto no limite exato de uma janela ainda vale */
#define AJ_EPS_PERFEITO             1e-4f   /* "falta um perfeito": a postura que sobra cabe num perfeito, com esta folga */
#define AJ_EPS_VIDA                 0.001f  /* vida ou postura abaixo disto é zero */
#define AJ_NUNCA                  (-100.0)  /* "ainda não apertou": um instante que nunca chega */
#define AJ_CALIBRA_MAX_APERTOS     64       /* a calibração nunca guarda mais apertos que isto */
/* Na primeira preparação de uma sequência, do começo até a lâmina partir sobra ao menos isto. */
#define AJ_PREPARO_MIN_PRIMEIRO     0.100f

#endif
