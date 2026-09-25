/*
 * roster.c - A Trilha dos Doze Aprendizes: os doze aprendizes de Hattori Hanzo
 * que guardam o caminho e Oboro, o mestre das doze posturas. Preparações e janelas em segundos.
 */
#include "core.h"

#include <stddef.h>

#define W(...) .windups = {__VA_ARGS__}, .windupCount = sizeof((float[]){__VA_ARGS__}) / sizeof(float)
#define STANCE(name, perfect, good, ...) {name, perfect, good, W(__VA_ARGS__), 0, 0, 0, 0, false}

static const MasterProfile ROSTER[ROSTER_SIZE] = {
    {
        .id = 1, .name = "daichi", .style = "postura da terra", .title = "O que Segue a Tradição", .venue = "Celeiro ao entardecer",
        .special = "Espada pesada: o segundo golpe sempre demora.",
        .arena = ARENA_CELEIRO, .posture = 300, .hitsToFall = 50, .cueVisual = 1, .cueAudio = 1, .tint = 0xFFD8A8FF,
        .stances = {STANCE("", 0.090f, 0.220f, 1.20f, 1.00f, 1.30f)}, .stanceCount = 1,
        .moves = {
            {"rocha", 1, {0}, 2.0f, -1, 0, LOOK_HIGH},
            {"desabamento", 2, {1.00f}, 2.0f, -1, 0, LOOK_LOW},
        },
        .moveCount = 2,
        .intro = {{"daichi", "A tradição diz que eu sigo quem venceu o mestre. Ela não diz que eu preciso gostar."},
                  {"kojiro", "Nem que precisa perder de propósito."}}, .introCount = 2,
        .outro = {{"daichi", "Provado. A terra reconhece quem pisa firme. hanzo continua sendo o meu mestre."}}, .outroCount = 1,
        .sensei = {{"hanzo", "A terra demora a tremer. Quando treme, treme duas vezes."},
                   {"hanzo", "O segundo golpe dele sempre se atrasa. Não corra atrás."}}, .senseiCount = 2,
    },
    {
        .id = 2, .name = "genbu", .style = "postura da tartaruga", .title = "O que Tem Pena", .venue = "Jardim de pedra e vidro",
        .special = "Katana simples: demora a sair do casco, sai rápido.",
        .arena = ARENA_JARDIM, .posture = 300, .hitsToFall = 45, .cueVisual = 1, .cueAudio = 1, .tint = 0xD8E0C8FF,
        .stances = {STANCE("", 0.087f, 0.215f, 1.30f, 1.20f, 1.40f)}, .stanceCount = 1,
        .moves = {
            {"casco", 1, {0}, 2.0f, -1, 0, LOOK_THRUST},
            {"mordida", 2, {0.42f}, 2.0f, -1, 0, LOOK_THRUST},
        },
        .moveCount = 2,
        .intro = {{"genbu", "Sabe por que a tartaruga vive tanto? Ela não tem pressa de chegar a lugar nenhum. oboro tinha."},
                  {"kojiro", "Eu não tenho."}}, .introCount = 2,
        .outro = {{"genbu", "hanzo tentou ensinar isso a ele até o último dia. Vai, garoto. Vai com calma."}}, .outroCount = 1,
        .sensei = {{"hanzo", "A tartaruga demora a sair do casco. Quando sai, sai rápido."},
                   {"hanzo", "Paciência contra paciência: vence quem respira mais devagar."}}, .senseiCount = 2,
    },
    {
        .id = 3, .name = "raizo", .style = "postura do touro", .title = "O que Defende Oboro", .venue = "Pátio do dojo",
        .special = "Odachi pesada: golpes lentos, diretos, que avisam antes de chegar.",
        .arena = ARENA_DOJO, .posture = 330, .hitsToFall = 40, .cueVisual = 1, .cueAudio = 1, .tint = 0xE8D8C8FF,
        .stances = {STANCE("", 0.084f, 0.208f, 1.20f, 1.05f, 1.15f)}, .stanceCount = 1,
        .moves = {
            {"corte do touro", 1, {0}, 3.0f, -1, 0, LOOK_HIGH},
            {"investida dupla", 2, {0.85f}, 1.5f, -1, 0, LOOK_LOW},
        },
        .moveCount = 2,
        .intro = {{"raizo", "oboro ganhou cada coisa que tem, com suor. E você, ganhou o quê? Um velho que te achou na estrada?"},
                  {"kojiro", "Uma pergunta. Vim ver se alguém aqui sabe a resposta."}}, .introCount = 2,
        .outro = {{"raizo", "Não foi você que foi rápido. Fui eu que corri."}}, .outroCount = 1,
        .sensei = {{"hanzo", "O touro não esconde nada. O que ele mostra é o que ele faz."},
                   {"hanzo", "Força demais sempre avisa antes de chegar. Escute o peso."}}, .senseiCount = 2,
    },
    {
        .id = 4, .name = "shizuku", .style = "postura da água", .title = "A que Observa", .venue = "Cachoeira do Trovão",
        .special = "Florete: estocadas retas, uma, duas ou três, sem curva.",
        .arena = ARENA_CACHOEIRA, .posture = 330, .hitsToFall = 35, .cueVisual = 1, .cueAudio = 1, .tint = 0xC8E8FFFF,
        .stances = {STANCE("", 0.080f, 0.200f, 0.95f, 0.85f, 1.00f)}, .stanceCount = 1,
        .moves = {
            {"gota", 1, {0}, 3.0f, -1, 0, LOOK_THRUST},
            {"correnteza", 2, {0.50f}, 2.0f, -1, 0, LOOK_THRUST},
            {"queda d'água", 3, {0.50f, 0.45f}, 1.0f, -1, 0, LOOK_HIGH},
        },
        .moveCount = 3,
        .intro = {{"shizuku", "A água toma a forma de qualquer copo. Passei anos tentando tomar a forma do mestre. Nunca coube."},
                  {"kojiro", "Talvez ele não fosse um copo."}}, .introCount = 2,
        .outro = {{"shizuku", "Então é isso que ele via em você. Fico curiosa para saber até onde você vai."}}, .outroCount = 1,
        .sensei = {{"hanzo", "A água vem reta. Não procure curva onde não há."},
                   {"hanzo", "Duas gotas caem quase juntas. Conte a segunda."}}, .senseiCount = 2,
    },
    {
        .id = 5, .name = "garfiel", .style = "postura do tigre", .title = "O que Guarda o Portão", .venue = "Portão do tigre branco",
        .special = "Garras nas duas mãos: os golpes vêm aos pares, uma pata e logo a outra.",
        .arena = ARENA_TEMPLO, .posture = 360, .hitsToFall = 30, .healsOnHit = true, .cueVisual = 1, .cueAudio = 1, .tint = 0xFFF0D8FF,
        .stances = {STANCE("", 0.076f, 0.192f, 0.80f, 0.90f, 0.75f)}, .stanceCount = 1,
        .moves = {
            {"patada", 1, {0}, 2.0f, -1, 0, LOOK_HIGH},
            {"garras cruzadas", 2, {0.40f}, 2.0f, -1, 0, LOOK_LOW},
            {"bote do tigre", 3, {0.40f, 0.85f}, 1.0f, -1, 0, LOOK_HIGH},
        },
        .moveCount = 3,
        .intro = {{"garfiel", "O tigre branco guarda o portão do oeste. Eu guardo o que sobrou deste dojo. Ninguém passa sem sangrar um pouco."},
                  {"kojiro", "Então eu sangro."}}, .introCount = 2,
        .outro = {{"garfiel", "Hah! Passou. Vai, e se oboro te derrubar, levanta. Tigre não fica no chão."}}, .outroCount = 1,
        .sensei = {{"hanzo", "O tigre ataca com as duas patas. Quando uma chega, a outra já está no ar."},
                   {"hanzo", "Não recue diante do rugido. O golpe vem depois do grito."}}, .senseiCount = 2,
    },
    {
        .id = 6, .name = "karasu", .style = "postura do corvo", .title = "O que Aposta", .venue = "Cobertura na chuva",
        .special = "Duas wakizashi: sempre uma pausa antes do último golpe.",
        .arena = ARENA_COBERTURA, .posture = 360, .hitsToFall = 25, .healsOnHit = true, .cueVisual = 1, .cueAudio = 1, .tint = 0xC8C8D8FF,
        .stances = {STANCE("", 0.072f, 0.185f, 0.83f, 1.13f, 0.87f)}, .stanceCount = 1,
        .moves = {
            {"bicada", 1, {0}, 2.0f, -1, 0, LOOK_THRUST},
            {"garra", 2, {0.55f}, 2.0f, -1, 0, LOOK_HIGH},
            {"revoada", 3, {0.50f, 0.90f}, 1.0f, -1, 0, LOOK_LOW},
        },
        .moveCount = 3,
        .intro = {{"karasu", "Eu sigo quem ganha. Hoje é oboro. Não se ofenda se eu apostar contra você."},
                  {"kojiro", "Aposta. Eu não jogo."}}, .introCount = 2,
        .outro = {{"karasu", "Hm. Parece que vou ter que mudar de lado. De novo."}}, .outroCount = 1,
        .sensei = {{"hanzo", "O corvo sempre faz uma pausa antes de roubar. É nela que ele te pega."},
                   {"hanzo", "Quem aposta contra você também está esperando. Espere mais que ele."}}, .senseiCount = 2,
    },
    {
        .id = 7, .name = "hayate", .style = "postura do vento", .title = "O Impaciente", .venue = "Teto do trem noturno",
        .special = "Duas foices pequenas e o vento: o ritmo muda de direção sem avisar.",
        .arena = ARENA_TREM, .posture = 390, .hitsToFall = 22, .healsOnHit = true, .cueVisual = 1, .cueAudio = 1, .tint = 0xD0FFE8FF,
        .rhythmJitter = 0.12f,
        .stances = {STANCE("", 0.069f, 0.178f, 0.85f, 0.75f, 1.00f)}, .stanceCount = 1,
        .moves = {
            {"rajada", 1, {0}, 2.0f, -1, 0, LOOK_THRUST},
            {"redemoinho", 2, {0.45f}, 2.0f, -1, 0, LOOK_HIGH},
            {"vendaval", 3, {0.50f, 0.90f}, 1.0f, -1, 0, LOOK_LOW},
        },
        .moveCount = 3,
        .intro = {{"hayate", "hanzo passou a vida dizendo que oboro não estava pronto. oboro provou que estava. Agora prova você."},
                  {"kojiro", "Não vim provar nada a você."}}, .introCount = 2,
        .outro = {{"hayate", "Mais uma. Só mais uma... Não. Tudo bem. Você venceu."}}, .outroCount = 1,
        .sensei = {{"hanzo", "O vento muda de direção sem avisar. O ritmo dele também. Espere o ombro."},
                   {"hanzo", "Quem é impaciente começa cedo. Termine tarde."}}, .senseiCount = 2,
    },
    {
        .id = 8, .name = "enjin", .style = "postura da chama", .title = "O que Odeia", .venue = "Forja dentro da cratera",
        .special = "Sabre curvo: o fogo nunca queima uma vez só.",
        .arena = ARENA_FORJA, .posture = 390, .hitsToFall = 20, .healsOnHit = true, .cueVisual = 1, .cueAudio = 1, .tint = 0xFFB890FF,
        .stances = {STANCE("", 0.066f, 0.172f, 0.70f, 0.80f, 0.65f)}, .stanceCount = 1,
        .moves = {
            {"brasa", 2, {0.50f}, 2.0f, -1, 0, LOOK_HIGH},
            {"labareda", 3, {0.45f, 0.45f}, 2.0f, -1, 0, LOOK_HIGH},
            {"incêndio", 4, {0.45f, 0.45f, 0.80f}, 1.0f, -1, 0, LOOK_LOW},
        },
        .moveCount = 3,
        .intro = {{"enjin", "Eu odeio o homem que oboro virou. E odeio o velho por não ter impedido. Se você não aguentar o meu fogo, não aguenta o dele."},
                  {"kojiro", "Então queima."}}, .introCount = 2,
        .outro = {{"enjin", "Isso. É isso que derruba ele. Vai, e derruba por mim."}}, .outroCount = 1,
        .sensei = {{"hanzo", "O fogo nunca queima uma vez só."},
                   {"hanzo", "Não descanse o braço enquanto a chama ainda sobe."}}, .senseiCount = 2,
    },
    {
        .id = 9, .name = "suiren", .style = "postura do mar", .title = "O Amigo de Oboro", .venue = "Porto dos tambores",
        .special = "Lança: as ondas vêm cada vez mais rápido e depois recuam.",
        .arena = ARENA_PORTO, .posture = 420, .hitsToFall = 18, .healsOnHit = true, .cueVisual = 1, .cueAudio = 1, .tint = 0xC8D8FFFF,
        .accelSteps = 5, .accelFactor = 0.82f,
        .stances = {STANCE("", 0.063f, 0.166f, 1.20f, 1.05f, 1.15f)}, .stanceCount = 1,
        .moves = {
            {"onda", 1, {0}, 2.0f, -1, 0, LOOK_THRUST},
            {"ressaca", 2, {0.50f}, 2.0f, -1, 0, LOOK_THRUST},
            {"maremoto", 4, {0.55f, 0.50f, 0.45f}, 1.0f, -1, 0, LOOK_HIGH},
        },
        .moveCount = 3,
        .intro = {{"suiren", "Você tem os olhos que ele tinha quando chegou ao dojo. Antes de querer ser alguém."},
                  {"kojiro", "O que aconteceu com ele?"}}, .introCount = 2,
        .outro = {{"suiren", "Quis ser o sucessor, e parou de ser ele mesmo. Não deixa isso acontecer com você."}}, .outroCount = 1,
        .sensei = {{"hanzo", "A maré sobe cada vez mais rápido, mas nenhuma maré sobe para sempre. Conte as ondas."},
                   {"hanzo", "Quando a onda recua, o mar respira. Respire junto."}}, .senseiCount = 2,
    },
    {
        .id = 10, .name = "arashi", .style = "postura da tempestade", .title = "O Orgulhoso", .venue = "Salão de espelhos",
        .special = "Duas espadas: raios curtos e colados, três de cada vez.",
        .arena = ARENA_SALAO, .posture = 420, .hitsToFall = 15, .healsOnHit = true, .cueVisual = 1, .cueAudio = 1, .tint = 0xE0D0F0FF,
        .stances = {STANCE("", 0.060f, 0.160f, 0.80f, 0.90f, 0.75f)}, .stanceCount = 1,
        .moves = {
            {"faísca", 1, {0}, 1.0f, -1, 0, LOOK_THRUST},
            {"trovoada", 2, {0.45f}, 2.0f, -1, 0, LOOK_HIGH},
            {"tormenta", 3, {0.40f, 0.40f}, 1.5f, -1, 0, LOOK_LOW},
        },
        .moveCount = 3,
        .intro = {{"arashi", "O velho envelheceu e foi superado. É assim que o mundo anda. Você é só o eco de quem já perdeu."},
                  {"kojiro", "Um eco ainda chega longe."}}, .introCount = 2,
        .outro = {{"arashi", "Impossível... Não. Não é impossível. Eu só não queria que fosse."}}, .outroCount = 1,
        .sensei = {{"hanzo", "A tempestade é orgulhosa: bate sempre no mesmo ritmo."},
                   {"hanzo", "Três raios seguidos. Não olhe o céu, olhe o chão."}}, .senseiCount = 2,
    },
    {
        .id = 11, .name = "yoru", .style = "postura da noite", .title = "O que Desconfia", .venue = "Bambuzal à meia-noite",
        .special = "Uma adaga em cada mão: às vezes as lanternas se apagam e só o som avisa.",
        .arena = ARENA_BAMBUZAL, .posture = 450, .hitsToFall = 12, .healsOnHit = true, .cueVisual = 1, .cueAudio = 1, .tint = 0xB8B0E0FF,
        .blackoutChance = 0.4f,
        .stances = {STANCE("", 0.058f, 0.155f, 0.90f, 0.80f, 1.00f)}, .stanceCount = 1,
        .moves = {
            {"sombra", 1, {0}, 2.0f, -1, 0, LOOK_LOW},
            {"presas", 2, {0.45f}, 2.0f, -1, 0, LOOK_HIGH},
            {"lua nova", 3, {0.45f, 0.80f}, 1.0f, -1, 0, LOOK_LOW},
        },
        .moveCount = 3,
        .intro = {{"yoru", "Todo mundo viu oboro vencer. Ninguém viu o que hanzo fez um instante antes."},
                  {"kojiro", "E o que ele fez?"}}, .introCount = 2,
        .outro = {{"yoru", "Abriu a guarda. De propósito. Guarde isso, garoto. E não conte a oboro que eu vi."}}, .outroCount = 1,
        .sensei = {{"hanzo", "Na noite, os olhos mentem. Os ouvidos não."},
                   {"hanzo", "Duas presas mordem juntas. Não comemore a primeira."}}, .senseiCount = 2,
    },
    {
        .id = 12, .name = "jinshi", .style = "postura da montanha", .title = "O que Não Perdoou", .venue = "Encosta da serra",
        .special = "Katana branca forjada com a lua: na montanha o som não chega, só o corpo avisa.",
        .arena = ARENA_SERRA, .posture = 450, .hitsToFall = 10, .healsOnHit = true, .cueVisual = 1, .cueAudio = 0, .tint = 0xE0D8C8FF,
        .stances = {STANCE("", 0.055f, 0.150f, 0.90f, 0.80f, 1.00f)}, .stanceCount = 1,
        .moves = {
            {"pedra", 1, {0}, 2.0f, -1, 0, LOOK_HIGH},
            {"avalanche", 3, {0.60f, 0.60f}, 2.0f, -1, 0, LOOK_LOW},
            {"cordilheira", 4, {0.50f, 0.50f, 0.90f}, 1.0f, -1, 0, LOOK_HIGH},
        },
        .moveCount = 3,
        .intro = {{"jinshi", "hanzo falhou com oboro. oboro falhou com todos nós. Eu fiquei na montanha esperando alguém que não falhasse."},
                  {"kojiro", "Eu ainda não terminei."}}, .introCount = 2,
        .outro = {{"jinshi", "Então termina. Talvez você seja a última chance que este dojo tem."}}, .outroCount = 1,
        .sensei = {{"hanzo", "Na montanha, o som demora a chegar. Não espere ouvir. Olhe."},
                   {"hanzo", "A pedra não pensa. Só cai. Decore a queda."}}, .senseiCount = 2,
    },
    {
        .id = 13, .name = "oboro", .style = "o mestre das doze posturas", .title = "O Mestre das Doze Posturas", .venue = "O dojo de Hanzo, no alto da serra",
        .special = "Domina as doze posturas e troca de uma para outra; três selos.",
        .arena = ARENA_CIDADELA, .posture = 360, .hitsToFall = 10, .healsOnHit = true, .specialChance = 0.25f,
        .cueVisual = 1, .cueAudio = 1, .tint = 0xE0C8FFFF, .isBigBoss = true,
        .stances = {
            STANCE("postura da terra", 0.070f, 0.180f, 1.05f, 0.90f, 1.15f),
            STANCE("postura da tartaruga", 0.068f, 0.176f, 1.15f, 1.05f, 1.25f),
            STANCE("postura do touro", 0.066f, 0.172f, 1.05f, 0.95f, 1.05f),
            STANCE("postura da água", 0.064f, 0.168f, 0.85f, 0.80f, 0.90f),
            STANCE("postura do tigre", 0.062f, 0.164f, 0.75f, 0.85f, 0.72f),
            STANCE("postura do corvo", 0.060f, 0.160f, 0.75f, 1.00f, 0.80f),
            STANCE("postura do vento", 0.058f, 0.155f, 0.75f, 0.70f, 0.90f),
            STANCE("postura da chama", 0.056f, 0.150f, 0.65f, 0.72f, 0.60f),
            STANCE("postura do mar", 0.053f, 0.145f, 1.05f, 0.95f, 1.00f),
            STANCE("postura da tempestade", 0.050f, 0.140f, 0.72f, 0.80f, 0.68f),
            STANCE("postura da noite", 0.048f, 0.135f, 0.80f, 0.75f, 0.90f),
            STANCE("postura da montanha", 0.045f, 0.130f, 0.80f, 0.72f, 0.90f),
        },
        .stanceCount = 12,
        .seals = {
            {"primeiro selo", 1.00f, 2},
            {"segundo selo", 1.00f, 2},
            {"terceiro selo", 0.90f, 2},
        },
        .sealCount = 3,
        /* Um eco de cada aprendiz, na postura dele; no terceiro selo, as doze de uma vez. */
        .moves = {
            {"eco da terra", 2, {1.00f}, 1.0f, 0, 0, LOOK_LOW},
            {"eco da tartaruga", 2, {0.42f}, 1.0f, 1, 0, LOOK_THRUST},
            {"eco do touro", 2, {0.85f}, 1.0f, 2, 0, LOOK_LOW},
            {"eco da água", 3, {0.50f, 0.45f}, 1.0f, 3, 0, LOOK_THRUST},
            {"eco do tigre", 3, {0.40f, 0.85f}, 1.0f, 4, 0, LOOK_HIGH},
            {"eco do corvo", 3, {0.50f, 0.90f}, 1.0f, 5, 0, LOOK_LOW},
            {"eco do vento", 3, {0.50f, 0.90f}, 1.0f, 6, 0, LOOK_LOW},
            {"eco da chama", 3, {0.45f, 0.45f}, 1.0f, 7, 0, LOOK_HIGH},
            {"eco do mar", 4, {0.55f, 0.50f, 0.45f}, 1.0f, 8, 0, LOOK_HIGH},
            {"eco da tempestade", 3, {0.40f, 0.40f}, 1.0f, 9, 0, LOOK_LOW},
            {"eco da noite", 3, {0.45f, 0.80f}, 1.0f, 10, 0, LOOK_HIGH},
            {"eco da montanha", 4, {0.50f, 0.50f, 0.90f}, 1.0f, 11, 0, LOOK_HIGH},
            {"doze posturas", 5, {0.40f, 0.40f, 0.40f, 0.90f}, 1.0f, -1, 2, LOOK_HIGH},
        },
        .moveCount = 13,
        .intro = {{"oboro", "Doze posturas. Aprendi todas. Dominei todas. E ele escolheu você, que não sabe o nome de nenhuma."},
                  {"kojiro", "Eu não aprendi posturas. Aprendi a esperar."},
                  {"oboro", "Então espera. Vou te mostrar as doze, uma de cada vez."}}, .introCount = 3,
        .outro = {{"oboro", "Ele me deu aquela abertura... e eu passei a vida achando que tinha vencido. Nunca entendi nada."}}, .outroCount = 1,
        .sensei = {{"hanzo", "Ele aprendeu as doze posturas, mas usa todas do mesmo jeito: com pressa."},
                   {"hanzo", "Quando ele muda de postura, é você quem continua o mesmo. Isso basta."}}, .senseiCount = 2,
    },
};

const MasterProfile *roster_get(int index) {
    if (index < 0 || index >= ROSTER_SIZE) return NULL;
    return &ROSTER[index];
}

int roster_size(void) { return ROSTER_SIZE; }

/* A abertura é narrada: cada entrada é um parágrafo que sobe pela tela.
 * Parágrafos entre aspas são citações e aparecem destacados. */
static const char *LORE[LORE_PAGES] = {
    "Hattori Hanzo criou a Arte do Aparar: uma espada feita de observação, de paciência e do contra-ataque "
    "que só existe no instante certo.",
    "Já velho, fundou um dojo no alto da serra e reuniu treze aprendizes. Procurava um sucessor.",
    "Entre todos, um se destacava. Oboro: talentoso, disciplinado, implacável. Hanzo viu nele o herdeiro e, "
    "sem perceber, alimentou essa crença durante anos.",
    "Mas quanto mais Oboro queria ser o sucessor, mais se afastava do caminho.",
    "“Quem luta para ser herdeiro esquece o que está herdando.”",
    "Depois de incontáveis desafios, Oboro enfim venceu o mestre e tomou o dojo, como manda a tradição. "
    "Só ele sabe o que viu naquele instante: Hanzo havia lhe concedido uma abertura.",
    "Ele venceu. Mas não era aquela a vitória que buscou a vida inteira. Consumido pela dúvida, passou anos "
    "estudando as posturas dos outros aprendizes. Aprendeu todas. Dominou todas. Nenhuma lhe deu a resposta.",
    "Anos depois, na estrada, Hanzo encontrou um menino chamado Kojiro. Pela primeira vez, acreditou estar "
    "diante de alguém capaz de compreender a Arte do Aparar.",
    "Agora Kojiro sobe a Trilha dos Doze Aprendizes. Não para vingar ninguém. Para mostrar que o estilo de "
    "Hanzo é inabalável.",
    "“Não persiga a força. A força vai embora. Tudo é uma questão de postura.”",
};

const char *lore_page(int index) {
    if (index < 0 || index >= LORE_PAGES) return "";
    return LORE[index];
}
