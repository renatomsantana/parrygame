/*
 * roster.c - A Trilha dos Doze Mestres, o BIG BOSS e a lore de abertura.
 * Preparações em segundos. Janelas em segundos (perfeito / bom).
 */
#include "core.h"

#include <stddef.h>

#define W(...) .windups = {__VA_ARGS__}, .windupCount = sizeof((float[]){__VA_ARGS__}) / sizeof(float)

static const MasterProfile ROSTER[ROSTER_SIZE] = {
    {
        .id = 1, .style = "postura do touro", .name = "tetsu", .title = "Veterano do Dojo", .venue = "Pátio da velha guarda",
        .special = "Golpes rítmicos e diretos, sem fintas.",
        .arena = ARENA_DOJO, .posture = 300, .cueVisual = 1, .cueAudio = 1, .tint = 0xE8D8C8FF,
        .stances = {{"", 0.090f, 0.220f, W(1.10f, 0.97f, 1.03f), 0, 0, 0, 0, false}}, .stanceCount = 1,
        .moves = {
            {"chifrada", 1, {0}, 3.0f, -1, 0, LOOK_HIGH},
            {"dois chifres", 2, {0.80f}, 1.5f, -1, 0, LOOK_LOW},
        },
        .moveCount = 2,
        .intro = {{"tetsu", "Uma espada que só sabe esperar é uma espada que nunca foi posta à prova. Vamos ver o que hanzo te deixou."},
                  {"musashi", "O suficiente."}}, .introCount = 2,
        .outro = {{"tetsu", "Vinte anos tentando cortar mais rápido que ele... e você só precisou ficar parado."}}, .outroCount = 1,
        .sensei = {{"hanzo", "O touro não esconde nada. O que ele mostra é o que ele faz: não procure truques onde só existe força."},
                   {"hanzo", "Um velho capitão respira sempre no mesmo compasso. Respire com ele."}}, .senseiCount = 2, .hitsToFall = 50,
    },
    {
        .id = 2, .style = "postura do macaco", .name = "neon jax", .title = "Dono do Porão", .venue = "Clube subterrâneo, madrugada",
        .special = "Sincopado: a preparação sai do compasso.",
        .arena = ARENA_RAVE, .posture = 300, .cueVisual = 1, .cueAudio = 1, .tint = 0xB8C8FFFF, .rhythmJitter = 0.12f,
        .stances = {{"", 0.085f, 0.210f, W(0.90f, 0.80f, 1.10f), 0, 0, 0, 0, false}}, .stanceCount = 1,
        .moves = {
            {"tapa", 1, {0}, 3.0f, -1, 0, LOOK_THRUST},
            {"passo quebrado", 2, {0.55f}, 2.0f, -1, 0, LOOK_HIGH},
        },
        .moveCount = 2,
        .intro = {{"neon jax", "Lá em cima eles têm o céu. Aqui embaixo a gente tem o barulho. Pelo menos o barulho não julga ninguém."},
                  {"musashi", "O silêncio também não."}}, .introCount = 2,
        .outro = {{"neon jax", "Então era isso que o velho ouvia. O espaço entre uma batida e outra."}}, .outroCount = 1,
        .sensei = {{"hanzo", "Quem dança muda o passo para te enganar, mas a lâmina ainda precisa descer."},
                   {"hanzo", "Não escute a música dele. A música é o que ele quer que você siga."}}, .senseiCount = 2, .hitsToFall = 40,
    },
    {
        .id = 3, .style = "postura do urso", .name = "cavan", .title = "Titã do Campo", .venue = "Celeiro ao entardecer",
        .special = "Golpes pesados e lentos, com o segundo atrasado.",
        .arena = ARENA_CELEIRO, .posture = 330, .cueVisual = 1, .cueAudio = 1, .tint = 0xFFD8A8FF,
        .stances = {{"", 0.080f, 0.200f, W(1.20f, 1.00f, 1.33f), 0, 0, 0, 0, false}}, .stanceCount = 1,
        .moves = {
            {"patada", 1, {0}, 2.0f, -1, 0, LOOK_HIGH},
            {"peso morto", 2, {1.00f}, 2.0f, -1, 0, LOOK_LOW},
        },
        .moveCount = 2,
        .intro = {{"cavan", "A terra não pergunta quem é o mais forte. Ela só recolhe tudo o que cai."},
                  {"musashi", "Então ela vai ter que esperar."}}, .introCount = 2,
        .outro = {{"cavan", "Plantei força a vida inteira... e foi isso que eu colhi."}}, .outroCount = 1,
        .sensei = {{"hanzo", "O homem pesado bate duas vezes, mas a segunda sempre demora. Não corra atrás dela."},
                   {"hanzo", "A paciência do urso é longa. A sua precisa ser um pouco mais."}}, .senseiCount = 2, .hitsToFall = 35,
    },
    {
        .id = 4, .style = "postura da raposa", .name = "vance", .title = "Executivo S.A.", .venue = "Cobertura corporativa",
        .special = "Sequências secas, com uma pausa traiçoeira no meio.",
        .arena = ARENA_COBERTURA, .posture = 330, .cueVisual = 1, .cueAudio = 1, .tint = 0xC8D0E0FF,
        .stances = {{"", 0.075f, 0.190f, W(0.83f, 1.13f, 0.87f), 0, 0, 0, 0, false}}, .stanceCount = 1,
        .moves = {
            {"proposta", 1, {0}, 2.0f, -1, 0, LOOK_THRUST},
            {"contrato", 2, {0.55f}, 2.0f, -1, 0, LOOK_HIGH},
            {"cláusula", 3, {0.50f, 0.90f}, 1.0f, -1, 0, LOOK_LOW},
        },
        .moveCount = 3,
        .intro = {{"vance", "Todo homem tem um preço, musashi. O de hanzo foi um braço. Qual é o seu?"},
                  {"musashi", "Não estou à venda. Nem a espada dele."}}, .introCount = 2,
        .outro = {{"vance", "Um homem sem preço... eu não sei lutar contra isso."}}, .outroCount = 1,
        .sensei = {{"hanzo", "O mercador sempre faz uma pausa antes do último preço. É nela que ele te pega."},
                   {"hanzo", "Quem tem pressa de fechar o negócio sempre assina antes de ler."}}, .senseiCount = 2, .hitsToFall = 30,
    },
    {
        .id = 5, .healsOnHit = true, .style = "postura da névoa", .name = "kaelen", .title = "Pintor Visionário", .venue = "Galeria surrealista",
        .special = "Cortes cegantes: tinta esconde o sinal.",
        .arena = ARENA_GALERIA, .posture = 360, .cueVisual = 0.35f, .cueAudio = 0.35f, .tint = 0xF0B8F0FF,
        .stances = {{"", 0.072f, 0.185f, W(0.77f, 0.97f, 0.73f), 0, 0, 0, 0, false}}, .stanceCount = 1,
        .moves = {
            {"pincelada", 1, {0}, 2.0f, -1, 0, LOOK_LOW},
            {"respingo", 2, {0.60f}, 2.0f, -1, 0, LOOK_HIGH},
            {"borrão", 3, {0.45f, 0.70f}, 1.0f, -1, 0, LOOK_THRUST},
        },
        .moveCount = 3,
        .intro = {{"kaelen", "Hanzo dizia que a espada é um pincel sem tinta. Eu discordei, e fui embora pintar de vermelho."},
                  {"musashi", "E a tela continuou vazia."}}, .introCount = 2,
        .outro = {{"kaelen", "Você cortou sem sujar nada. Que traço limpo."}}, .outroCount = 1,
        .sensei = {{"hanzo", "Quando a tinta cobre os olhos, a mão dele ainda sabe onde está a espada."},
                   {"hanzo", "Não olhe a pintura. Olhe o pintor."}}, .senseiCount = 2, .hitsToFall = 25,
    },
    {
        .id = 6, .healsOnHit = true, .style = "postura da maré", .name = "taiko", .title = "Tamborileiro do Porto", .venue = "Porto noturno dos tambores",
        .special = "Acelerando: cada golpe do ciclo vem mais rápido.",
        .arena = ARENA_PORTO, .posture = 360, .cueVisual = 1, .cueAudio = 1, .tint = 0xFFC0A0FF,
        .accelSteps = 5, .accelFactor = 0.82f,
        .stances = {{"", 0.070f, 0.180f, W(1.20f, 1.05f, 1.15f), 0, 0, 0, 0, false}}, .stanceCount = 1,
        .moves = {
            {"batida", 1, {0}, 2.0f, -1, 0, LOOK_HIGH},
            {"dois tempos", 2, {0.50f}, 2.0f, -1, 0, LOOK_LOW},
            {"rufar", 4, {0.55f, 0.50f, 0.45f}, 1.0f, -1, 0, LOOK_HIGH},
        },
        .moveCount = 3,
        .intro = {{"taiko", "O mar bate nessa pedra há mil anos. A pedra continua aqui. Quem venceu?"},
                  {"musashi", "Nenhum dos dois quis vencer."}}, .introCount = 2,
        .outro = {{"taiko", "A pedra não vence o mar. Ela só não sai do lugar. Agora eu entendo."}}, .outroCount = 1,
        .sensei = {{"hanzo", "A maré sobe cada vez mais rápido, mas nenhuma maré sobe para sempre. Conte as ondas."},
                   {"hanzo", "Quando a onda recua, o mar respira. Respire junto."}}, .senseiCount = 2, .hitsToFall = 22,
    },
    {
        .id = 7, .healsOnHit = true, .style = "postura da garça", .name = "eleonor", .title = "Grã-Duquesa Esgrimista", .venue = "Salão de baile com espelhos",
        .special = "Estocadas rápidas e longas.",
        .arena = ARENA_SALAO, .posture = 390, .cueVisual = 1, .cueAudio = 1, .tint = 0xFFF0D0FF,
        .stances = {{"", 0.065f, 0.170f, W(0.70f, 0.83f, 0.63f), 0, 0, 0, 0, false}}, .stanceCount = 1,
        .moves = {
            {"estocada", 1, {0}, 2.0f, -1, 0, LOOK_THRUST},
            {"estocada dupla", 2, {0.42f}, 2.0f, -1, 0, LOOK_THRUST},
            {"valsa", 3, {0.45f, 0.45f}, 1.0f, -1, 0, LOOK_HIGH},
        },
        .moveCount = 3,
        .intro = {{"eleonor", "Aprendi a esgrima dos salões para esquecer a serra. Não esqueci. Ninguém esquece o primeiro mestre."},
                  {"musashi", "Então lembra dele agora."}}, .introCount = 2,
        .outro = {{"eleonor", "Que coisa cruel... ser vencida por uma lembrança."}}, .outroCount = 1,
        .sensei = {{"hanzo", "A agulha dela vai longe e volta rápido. Espere o braço se esticar inteiro."},
                   {"hanzo", "Cada ponto que ela dá tem o mesmo tempo. Costure junto."}}, .senseiCount = 2, .hitsToFall = 20,
    },
    {
        .id = 8, .healsOnHit = true, .style = "postura do trovão", .name = "kira", .title = "Maquinista do Expresso", .venue = "Teto do trem-bala noturno",
        .special = "Golpe duplo: dois contatos em sequência.",
        .arena = ARENA_TREM, .posture = 390, .cueVisual = 1, .cueAudio = 1, .tint = 0xD0FFE8FF,
        .stances = {{"", 0.062f, 0.165f, W(0.80f, 0.90f, 0.75f), 0, 0, 0, 0, false}}, .stanceCount = 1,
        .moves = {
            {"relâmpago", 1, {0}, 1.0f, -1, 0, LOOK_THRUST},
            {"trovão duplo", 2, {0.45f}, 2.0f, -1, 0, LOOK_HIGH},
            {"tempestade", 3, {0.40f, 0.40f}, 1.5f, -1, 0, LOOK_LOW},
        },
        .moveCount = 3,
        .intro = {{"kira", "Corri tanto para longe do dojo que achei que nunca mais fosse ouvir a voz do velho."},
                  {"musashi", "Ela chega. Às vezes demora."}}, .introCount = 2,
        .outro = {{"kira", "Chegou... bem no último vagão."}}, .outroCount = 1,
        .sensei = {{"hanzo", "Um trem nunca passa sozinho. Depois do primeiro vagão, vem o segundo."},
                   {"hanzo", "Quem comemora o primeiro choque é atropelado pelo segundo."}}, .senseiCount = 2, .hitsToFall = 18,
    },
    {
        .id = 9, .healsOnHit = true, .style = "postura da serpente", .name = "hayate", .title = "Monge da Cachoeira", .venue = "Cachoeira do Trovão",
        .special = "Sinal mudo: só o corpo dele avisa.",
        .arena = ARENA_CACHOEIRA, .posture = 420, .cueVisual = 1, .cueAudio = 0, .tint = 0xC8E8FFFF,
        .stances = {{"", 0.060f, 0.160f, W(0.75f, 0.85f, 0.70f), 0, 0, 0, 0, false}}, .stanceCount = 1,
        .moves = {
            {"bote", 1, {0}, 2.0f, -1, 0, LOOK_LOW},
            {"dupla picada", 2, {0.70f}, 2.0f, -1, 0, LOOK_THRUST},
            {"serpente", 3, {0.50f, 0.80f}, 1.0f, -1, 0, LOOK_LOW},
        },
        .moveCount = 3,
        .intro = {{"hayate", "Meditei vinte anos debaixo dessa água para esquecer o que é querer vencer. Ainda quero."},
                  {"musashi", "Eu também. Só não deixo a espada saber."}}, .introCount = 2,
        .outro = {{"hayate", "A água... finalmente ficou em silêncio."}}, .outroCount = 1,
        .sensei = {{"hanzo", "Quando a água cala o aço, o corpo dele fala mais alto."},
                   {"hanzo", "Os monges da montanha escutam com os olhos. Faça o mesmo."}}, .senseiCount = 2, .hitsToFall = 15,
    },
    {
        .id = 10, .healsOnHit = true, .style = "postura do lobo", .name = "yoru", .title = "Caçadora sem Lua", .venue = "Bambuzal à meia-noite",
        .special = "Apagão: as lanternas morrem e só o som avisa.",
        .arena = ARENA_BAMBUZAL, .posture = 420, .cueVisual = 1, .cueAudio = 1, .tint = 0xB8B0E0FF,
        .blackoutChance = 0.5f,
        .stances = {{"", 0.058f, 0.155f, W(0.70f, 0.80f, 0.66f), 0, 0, 0, 0, false}}, .stanceCount = 1,
        .moves = {
            {"mordida", 1, {0}, 2.0f, -1, 0, LOOK_HIGH},
            {"matilha", 2, {0.50f}, 2.0f, -1, 0, LOOK_LOW},
            {"caçada", 3, {0.60f, 0.40f}, 1.0f, -1, 0, LOOK_THRUST},
        },
        .moveCount = 3,
        .intro = {{"yoru", "Na manhã em que hanzo caiu, fui eu que fechei o portão. Não para prendê-lo. Para não ver."},
                  {"musashi", "Eu vi. Por isso estou aqui."}}, .introCount = 2,
        .outro = {{"yoru", "Talvez eu devesse ter olhado."}}, .outroCount = 1,
        .sensei = {{"hanzo", "Na noite em que eu caí, você ouviu os passos antes de ver as sombras. Lembre-se."},
                   {"hanzo", "No escuro, os olhos mentem. Os ouvidos não."}}, .senseiCount = 2, .hitsToFall = 12,
    },
    {
        .id = 11, .healsOnHit = true, .style = "postura do fogo", .name = "magna", .title = "Forjadora do Vulcão", .venue = "Forja dentro da cratera",
        .special = "Martelada tripla: três contatos seguidos.",
        .arena = ARENA_FORJA, .posture = 450, .cueVisual = 1, .cueAudio = 1, .tint = 0xFFB890FF,
        .stances = {{"", 0.055f, 0.150f, W(0.66f, 0.74f, 0.62f), 0, 0, 0, 0, false}}, .stanceCount = 1,
        .moves = {
            {"martelo", 2, {0.50f}, 2.0f, -1, 0, LOOK_HIGH},
            {"bigorna", 3, {0.45f, 0.45f}, 2.0f, -1, 0, LOOK_HIGH},
            {"forja", 4, {0.45f, 0.45f, 0.80f}, 1.0f, -1, 0, LOOK_LOW},
        },
        .moveCount = 3,
        .intro = {{"magna", "Eu forjei a lâmina que cortou o braço do seu mestre. O aço não tem culpa, garoto. Ele só obedece."},
                  {"musashi", "A mão que o segura tem."}}, .introCount = 2,
        .outro = {{"magna", "O fogo apaga. Eu devia ter aprendido isso antes."}}, .outroCount = 1,
        .sensei = {{"hanzo", "O ferreiro nunca bate uma vez só. O aço pede mais."},
                   {"hanzo", "Não descanse o braço enquanto o martelo ainda está no ar."}}, .senseiCount = 2, .hitsToFall = 10,
    },
    {
        .id = 12, .healsOnHit = true, .style = "postura do espelho", .name = "sombra", .title = "O Reflexo de musashi", .venue = "Jardim de Vidro, Altar da Alma",
        .special = "Copia o ritmo de musashi: sequências curtas e rápidas.",
        .arena = ARENA_JARDIM, .posture = 450, .cueVisual = 1, .cueAudio = 1, .tint = 0x505070FF, .useHeroSheet = true,
        .stances = {{"", 0.050f, 0.140f, W(0.60f, 0.73f, 0.57f), 0, 0, 0, 0, false}}, .stanceCount = 1,
        .moves = {
            {"reflexo", 1, {0}, 2.0f, -1, 0, LOOK_HIGH},
            {"eco", 2, {0.40f}, 2.0f, -1, 0, LOOK_LOW},
            {"espelho partido", 3, {0.40f, 0.70f}, 1.0f, -1, 0, LOOK_THRUST},
        },
        .moveCount = 3,
        .intro = {{"sombra", "Eu sou o que você teria sido se tivesse escolhido o céu em vez do chão."},
                  {"musashi", "O céu não precisa de mim. O chão, sim."}}, .introCount = 2,
        .outro = {{"sombra", "Então vai. O chão te espera lá em cima."}}, .outroCount = 1,
        .sensei = {{"hanzo", "Ele tem o seu ritmo, só que mais curto. Encurte a espera."},
                   {"hanzo", "O reflexo não pensa. Só repete. Decore o que ele repete."}}, .senseiCount = 2, .hitsToFall = 10,
    },
    {
        .id = 13, .healsOnHit = true, .style = "postura do dragão", .name = "oboro", .title = "BIG BOSS - O Primeiro Discípulo", .venue = "Cidadela da Liga, acima das nuvens",
        .special = "Três selos, duas posturas (tartaruga e tigre) e golpes compostos.",
        .arena = ARENA_CIDADELA, .posture = 360, .cueVisual = 1, .cueAudio = 1, .tint = 0xE0C8FFFF, .isBigBoss = true,
        .stances = {
            {"postura da tartaruga", 0.070f, 0.180f, W(1.05f, 0.95f, 1.15f), 0, 0, 0, 0, false},
            {"postura do tigre", 0.045f, 0.130f, W(0.60f, 0.55f, 0.65f), 0, 0, 0, 0, false},
        },
        .stanceCount = 2,
        .moves = {
            {"casco", 1, {0}, 2.0f, 0, 0, LOOK_HIGH},
            {"maré lenta", 2, {0.80f}, 2.0f, 0, 0, LOOK_LOW},
            {"garra", 2, {0.40f}, 2.0f, 1, 0, LOOK_THRUST},
            {"salto do tigre", 3, {0.40f, 0.40f}, 2.0f, 1, 0, LOOK_HIGH},
            {"fúria do tigre", 4, {0.40f, 0.40f, 0.60f}, 1.5f, 1, 1, LOOK_LOW},
            {"sopro do dragão", 5, {0.40f, 0.40f, 0.40f, 0.90f}, 1.0f, -1, 2, LOOK_HIGH},
        },
        .moveCount = 6,
        .seals = {
            {"primeiro selo", 1.00f, 3},
            {"segundo selo", 1.00f, 2},
            {"terceiro selo", 0.90f, 2},
        },
        .sealCount = 3,
        .intro = {{"oboro", "O mais forte debaixo do céu. Eu cheguei, musashi. E não existe nada aqui em cima."},
                  {"musashi", "Porque você subiu sozinho."},
                  {"oboro", "Então me mostra o que ele te ensinou. Três vezes, se for preciso."}}, .introCount = 3,
        .outro = {{"oboro", "Invencível... era isso que ele queria dizer. Ninguém é."}}, .outroCount = 1,
        .sensei = {{"hanzo", "oboro sempre foi dois: a tartaruga que espera e o tigre que não sabe esperar. Veja qual deles está diante de você."},
                   {"hanzo", "Quando ele brilha de raiva, o golpe pesa em dobro. A raiva também tem pressa."}}, .senseiCount = 2, .hitsToFall = 10, .specialChance = 0.25f,
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
    "Na serra, onde a neblina demora a ir embora, Hanzo teve treze discípulos. Ensinou a todos a mesma coisa: "
    "que a espada não se vence com força, e sim com postura.",
    "Doze nunca aceitaram. Treinavam para superá-lo, não para entendê-lo. Oboro, o mais velho, era o que mais "
    "queria: ser o mais forte debaixo do céu.",
    "\u201cQuem quer ser o mais forte debaixo do céu acaba em guerra com o céu inteiro.\u201d",
    "Numa manhã de chuva, Oboro pediu um duelo. Hanzo se curvou para cumprimentá-lo, e Oboro cortou antes que "
    "ele se erguesse. O braço do mestre caiu no barro do pátio.",
    "Pelo costume da serra, quem derrota o mestre toma o dojo. Os doze dobraram o joelho diante de Oboro e se "
    "espalharam pelo caminho até o castelo, cada um guardando um trecho da estrada.",
    "Musashi chegou ao dojo ainda criança, pequeno demais para segurar uma espada de verdade. Hanzo o observou "
    "varrer o pátio por um inverno inteiro e soube, antes de todos, que estava diante de um prodígio.",
    "Musashi não se curvou a Oboro. Não queria vingança. Queria outra coisa, mais difícil: mostrar que o estilo "
    "de Hanzo era inabalável, e que nenhum dos doze jamais o entendeu.",
    "Recolheu do barro a espada do mestre e desceu a serra. Hanzo, com o único braço que lhe restou, disse apenas "
    "o que repetia no pátio, toda manhã, antes do primeiro golpe:",
    "\u201cNão persiga a força. A força vai embora. Tudo é uma questão de postura.\u201d",
};

const char *lore_page(int index) {
    if (index < 0 || index >= LORE_PAGES) return "";
    return LORE[index];
}
