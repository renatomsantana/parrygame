/*
 * roster.c - A Trilha dos Doze Aprendizes: os doze aprendizes de Hattori Hanzo
 * que guardam o caminho e Oboro, o mestre das doze posturas. Preparações e janelas em segundos.
 */
#include "core.h"

#include <stddef.h>

#define STANCE(name, perfect, good, aviso) {name, perfect, good, aviso, 0}
/* as primeiras `n` sequências desta postura saem na ordem em que estão aqui */
#define STANCE_EM_ORDEM(name, perfect, good, aviso, n) {name, perfect, good, aviso, n}

static const MasterProfile ROSTER[ROSTER_SIZE] = {
    {
        .id = 1, .name = "daichi", .style = "postura da terra", .title = "O que Segue a Tradição", .venue = "Celeiro ao entardecer",
        .special = "Katana, devagar: cada golpe avisa muito antes de chegar.",
        .arena = ARENA_CELEIRO, .posture = 300, .hitsToFall = 8, .cueVisual = 1, .cueAudio = 1, .tint = 0xFFD8A8FF,
        .stances = {STANCE("", 0.090f, 0.220f, 0.450f)}, .stanceCount = 1,
        .moves = {
            {"rocha", 1, {0}, 2.0f, -1, 0, LOOK_HIGH, 1.55f},
            {"raiz", 1, {0}, 2.0f, -1, 0, LOOK_LOW, 1.35f},
            {"sulco", 1, {0}, 1.5f, -1, 0, LOOK_THRUST, 1.75f},
            {"desabamento", 2, {1.00f}, 1.5f, -1, 0, LOOK_LOW, 1.55f},
            {"arado", 1, {0}, 1.0f, -1, 0, LOOK_DASH, 1.35f},
            {"pedregulho", 2, {1.10f}, 1.0f, -1, 0, LOOK_JUMP, 1.75f},
            {"terremoto", 1, {0}, 1.0f, -1, 0, LOOK_HEAVY, 1.55f},
        },
        .moveCount = 7,
        .intro = {{"daichi", "Você é o garoto do hanzo. Dá pra ver pela guarda."},
                  {"daichi", "Eu respeitava muito o velho. Mas oboro venceu, e seguimos o novo mestre. A tradição manda."},
                  {"kojiro", "Ele traiu o mestre."},
                  {"daichi", "Pode ser. A tradição não pergunta."}}, .introCount = 4,
        .outro = {{"daichi", "A terra reconhece quem pisa firme."},
                  {"daichi", "Vou sentir falta do velho. Diz isso a ele, se ainda o vir."}}, .outroCount = 2,
        .sensei = {{"hanzo", "A terra demora a tremer. Quando treme, treme duas vezes."},
                   {"hanzo", "O segundo golpe dele sempre se atrasa. Não corra atrás."}}, .senseiCount = 2,
        .visit = {{"kojiro", "Daichi caiu."},
                  {"hanzo", "Daichi. Um bom menino. Obedecia a tudo, até a quem não devia. Nunca passou disso."},
                  {"hanzo", "O próximo é genbu, no jardim de pedras do mosteiro. Uma katana simples, e muita paciência."},
                  {"hanzo", "Ele demora a sair do casco. Quando sai, sai rápido."},
                  {"hanzo", "Não lute contra ele. Entenda ele. Depois, devore."}}, .visitCount = 5,
    },
    {
        .id = 2, .name = "genbu", .style = "postura da tartaruga", .title = "O que Tem Pena", .venue = "Jardim de pedras do mosteiro",
        .special = "Katana simples, já mais rápida: demora a sair do casco, e sai de jeitos diferentes.",
        .arena = ARENA_JARDIM, .posture = 330, .hitsToFall = 8, .cueVisual = 1, .cueAudio = 1, .tint = 0xD8E0C8FF,
        .stances = {STANCE("", 0.082f, 0.203f, 0.437f)}, .stanceCount = 1,
        .moves = {
            {"casco", 1, {0}, 2.0f, -1, 0, LOOK_THRUST, 1.15f},
            {"mordida", 2, {0.42f}, 2.0f, -1, 0, LOOK_THRUST, 1.00f},
            {"carapaça", 2, {0.60f}, 1.5f, -1, 0, LOOK_HIGH, 1.25f},
            {"concha", 1, {0}, 1.5f, -1, 0, LOOK_LOW, 1.15f},
            {"bote da tartaruga", 1, {0}, 1.2f, -1, 0, LOOK_DASH, 1.00f},
            {"maré lenta", 2, {0.75f}, 1.0f, -1, 0, LOOK_LOW, 1.25f},
            {"casco fechado", 2, {0.90f}, 1.0f, -1, 0, LOOK_HIGH, 1.15f},
        },
        .moveCount = 7,
        .intro = {{"genbu", "Senta um pouco, garoto. As pedras não têm pressa, e eu também não."},
                  {"genbu", "Cheguei a este mosteiro quase morto. oboro me carregou serra acima nas costas. Ele salvou minha vida."},
                  {"kojiro", "E eu vim tirar a dele."},
                  {"genbu", "Então vai ter que passar por mim. Devagar."}}, .introCount = 4,
        .outro = {{"genbu", "Pensa numa coisa no caminho: depois que ele assumiu, ninguém mais sumiu do dojo."}}, .outroCount = 1,
        .sensei = {{"hanzo", "A tartaruga demora a sair do casco. Quando sai, sai rápido."},
                   {"hanzo", "Paciência contra paciência: vence quem respira mais devagar."}}, .senseiCount = 2,
        .visit = {{"kojiro", "Genbu."},
                  {"hanzo", "O paciente. Chamava de paciência o que era só medo de agir."},
                  {"kojiro", "Ele disse que, depois de oboro, ninguém mais sumiu do dojo."},
                  {"hanzo", "Quem foi embora foi porque não aguentou. A arte não é para todos."},
                  {"hanzo", "Depois vem raizo, no pátio do dojo. Uma odachi pesada, golpes diretos. Ele avisa antes de bater."},
                  {"hanzo", "Cada um deles aprendeu com aquele homem. Não se deixe enganar pelo que disserem dele."}}, .visitCount = 6,
    },
    {
        .id = 3, .name = "raizo", .style = "postura da montanha", .title = "O Honrado", .venue = "Pátio do dojo",
        .special = "Odachi pesada: cortes como pedra caindo, lentos e diretos, que avisam antes de chegar.",
        .arena = ARENA_DOJO, .posture = 350, .hitsToFall = 7, .cueVisual = 1, .cueAudio = 1, .tint = 0xE8D8C8FF,
        .stances = {STANCE("", 0.075f, 0.185f, 0.424f)}, .stanceCount = 1,
        .moves = {
            {"corte do cume", 1, {0}, 3.0f, -1, 0, LOOK_HIGH, 1.20f},
            {"fenda dupla", 2, {0.85f}, 1.5f, -1, 0, LOOK_LOW, 1.05f},
            {"rasgo na laje", 1, {0}, 1.2f, -1, 0, LOOK_LOW, 1.15f},
            {"ponta da serra", 2, {1.00f}, 1.2f, -1, 0, LOOK_THRUST, 1.20f},
            {"avalanche", 1, {0}, 1.5f, -1, 0, LOOK_DASH, 1.05f},
            {"queda de pedras", 2, {0.70f}, 1.0f, -1, 0, LOOK_JUMP, 1.15f},
            {"montanha partida", 1, {0}, 1.0f, -1, 0, LOOK_HEAVY, 1.20f},
        },
        .moveCount = 7,
        .intro = {{"raizo", "Quinze invernos treinando neste pátio com o hanzo. Nunca vi mestre igual."},
                  {"raizo", "Mas ele perdeu. E um homem honrado segue quem venceu."},
                  {"kojiro", "Honra não é seguir traidor."},
                  {"raizo", "Então me mostra o que é. Com a espada."}}, .introCount = 4,
        .outro = {{"raizo", "Direto, sem desperdício. Ele te ensinou bem."},
                  {"raizo", "Vou sentir falta do velho."}}, .outroCount = 2,
        .sensei = {{"hanzo", "A montanha não esconde nada. Veja de onde a lâmina vai cair."},
                   {"hanzo", "Força demais sempre avisa antes de chegar. Escute o peso."}}, .senseiCount = 2,
        .visit = {{"kojiro", "Raizo também."},
                  {"hanzo", "Firme como uma montanha. Nunca aprendeu a ceder."},
                  {"hanzo", "Shizuku espera na cachoeira do trovão. Florete e gelo: estocadas curtas, rápidas, sem curva."},
                  {"hanzo", "Ela observa muito. Não deixe que ela te leia primeiro."},
                  {"hanzo", "Não lute contra ela. Entenda ela. Depois, devore."}}, .visitCount = 5,
    },
    {
        .id = 4, .name = "shizuku", .style = "postura do gelo", .title = "A que Observa", .venue = "Cachoeira do Trovão",
        .special = "Florete com geada: quatro leituras, sempre na linha central; a finta não toca.",
        .arena = ARENA_CACHOEIRA, .posture = 380, .hitsToFall = 7, .cueVisual = 1, .cueAudio = 1, .tint = 0xC8E8FFFF,
        .stances = {STANCE("", 0.068f, 0.171f, 0.411f)}, .stanceCount = 1,
        .moves = {
            {"floco", 1, {0}, 2.5f, -1, 0, LOOK_THRUST, 0.85f, 0, true, false},
            {"geada", 2, {0.40f}, 2.0f, -1, 0, LOOK_THRUST, 0.75f, 0, true, false},
            {"deslize", 1, {0}, 1.2f, -1, 0, LOOK_DASH, 0.85f, 0, true, false},
            {"finta de gelo", 1, {0}, 1.0f, -1, 0, LOOK_THRUST, 0.90f, 0, true, true},
        },
        .moveCount = 4,
        .intro = {{"shizuku", "Você segura a espada igual a ele. Os ombros também. Ele te fez no mesmo molde."},
                  {"shizuku", "O hanzo era um monstro, sabia? Treinava a gente até a última gota de sangue."},
                  {"kojiro", "Ele me salvou."},
                  {"shizuku", "Foi o que todos nós pensamos, um dia."}}, .introCount = 4,
        .outro = {{"shizuku", "Quando chegar lá em cima, olha pra ele. Não pra espada. Pra ele."}}, .outroCount = 1,
        .sensei = {{"hanzo", "O gelo vem reto. Não procure curva onde não há."},
                   {"hanzo", "Dois flocos caem quase juntos. Conte o segundo."}}, .senseiCount = 2,
        .visit = {{"kojiro", "Shizuku falou mal do senhor."},
                  {"hanzo", "Shizuku sempre pensou demais. Quem pensa demais não corta nada."},
                  {"hanzo", "O próximo é garfiel, no portão do tigre branco. Garras nas duas mãos, combos longos, e muito barulho."},
                  {"hanzo", "Cada vez que te acerta, ele respira de novo. Não deixe ele respirar."},
                  {"hanzo", "Cada um deles aprendeu com aquele homem. Não se deixe enganar pelo que disserem dele."}}, .visitCount = 5,
    },
    {
        .id = 5, .name = "garfiel", .style = "postura do tigre", .title = "O que Guarda o Portão", .venue = "Portão do tigre branco",
        .special = "Garras nas duas mãos: combos longos, de seis a oito golpes seguidos.",
        .arena = ARENA_TEMPLO, .posture = 530, .hitsToFall = 6, .healsOnHit = true, .cueVisual = 1, .cueAudio = 1, .tint = 0xFFF0D8FF,
        .stances = {STANCE("", 0.063f, 0.158f, 0.398f)}, .stanceCount = 1,
        .moves = {
            {"patada", 1, {0}, 1.5f, -1, 0, LOOK_HIGH, 0.80f},
            {"garras cruzadas", 2, {0.40f}, 1.5f, -1, 0, LOOK_LOW, 0.90f},
            {"rasgo", 3, {0.40f, 0.40f}, 1.5f, -1, 0, LOOK_LOW, 0.75f},
            {"bote do tigre", 3, {0.40f, 0.85f}, 1.0f, -1, 0, LOOK_HIGH, 0.80f},
            {"fúria do tigre", 6, {0.40f, 0.40f, 0.40f, 0.40f, 0.40f}, 1.2f, -1, 0, LOOK_HIGH, 0.90f},
            {"caçada", 7, {0.40f, 0.40f, 0.45f, 0.40f, 0.40f, 0.70f}, 1.0f, -1, 0, LOOK_DASH, 0.75f},
            {"rugido", 8, {0.40f, 0.40f, 0.40f, 0.40f, 0.40f, 0.40f, 0.80f}, 0.8f, -1, 0, LOOK_HIGH, 0.80f},
            {"pulo do gato", 2, {0.45f}, 1.0f, -1, 0, LOOK_JUMP, 0.90f},
            {"salto do tigre", 1, {0}, 0.8f, -1, 0, LOOK_HEAVY, 0.75f},
        },
        .moveCount = 9,
        .intro = {{"garfiel", "Hah! Então você é o cachorrinho novo do velho!"},
                  {"garfiel", "Escuta aqui: o oboro salvou minha vida. Todos nós devemos tudo a ele!"},
                  {"garfiel", "Ninguém passa por este portão pra machucar ele. Ninguém!"},
                  {"kojiro", "Eu passo."}}, .introCount = 4,
        .outro = {{"garfiel", "Tsc... Passou. Mas se encostar um dedo nele, o tigre desce a serra pra te buscar."}}, .outroCount = 1,
        .sensei = {{"hanzo", "O tigre ataca com as duas patas. Quando uma chega, a outra já está no ar."},
                   {"hanzo", "Não recue diante do rugido. O golpe vem depois do grito."}}, .senseiCount = 2,
        .visit = {{"kojiro", "Garfiel."},
                  {"hanzo", "Muito barulho pra pouca coisa. Sempre foi assim."},
                  {"hanzo", "Karasu te espera nos telhados da vila do castelo, na chuva. A katana numa mão; às vezes, a wakizashi na outra."},
                  {"hanzo", "Ele some em penas e volta onde você não olha."},
                  {"hanzo", "Não lute contra ele. Entenda ele. Depois, devore."}}, .visitCount = 5,
    },
    {
        .id = 6, .name = "karasu", .style = "postura do corvo", .title = "O que Aposta", .venue = "Telhados da vila do castelo, na chuva",
        .special = "Katana e wakizashi: quase sempre a espada longa numa mão só; de repente, as duas lâminas de uma vez.",
        .arena = ARENA_TELHADOS, .posture = 490, .hitsToFall = 7, .bladeMax = 0.239f, .healsOnHit = true, .cueVisual = 1, .cueAudio = 1, .tint = 0xC8C8D8FF,
        .stances = {STANCE("", 0.058f, 0.148f, 0.385f)}, .stanceCount = 1,
        .moves = {
            {"bicada", 1, {0}, 2.0f, -1, 0, LOOK_THRUST, 0.83f},
            {"garra", 2, {0.55f}, 2.0f, -1, 0, LOOK_HIGH, 1.13f},
            {"sumiço", 1, {0}, 1.5f, -1, 0, LOOK_WARP, 0.87f},
            {"corvo fantasma", 2, {0.55f}, 1.0f, -1, 0, LOOK_WARP, 0.83f},
            {"revoada", 3, {0.50f, 0.90f}, 1.0f, -1, 0, LOOK_LOW, 1.13f, 0x4},
            {"bando", 4, {0.45f, 0.45f, 0.45f}, 1.0f, -1, 0, LOOK_THRUST, 0.87f, 0x8},
            {"duas penas", 1, {0}, 0.7f, -1, 0, LOOK_HIGH, 0.83f, 0x1},
            {"voo rasante", 1, {0}, 1.2f, -1, 0, LOOK_DASH, 1.13f},
            {"asa quebrada", 2, {0.50f}, 1.0f, -1, 0, LOOK_JUMP, 0.87f},
            {"mergulho", 1, {0}, 0.8f, -1, 0, LOOK_HEAVY, 0.83f},
            {"cruz de penas", 1, {0}, 0.4f, -1, 0, LOOK_HIGH, 0.95f, 0x1},
            {"corte curto", 2, {0.40f}, 1.0f, -1, 0, LOOK_THRUST, 0.85f},
        },
        .moveCount = 12,
        .intro = {{"karasu", "Olha só, o novo favorito. O velho sempre tem um favorito. Nunca dura."},
                  {"karasu", "Ele era um monstro. E depois daquele duelo o oboro perdeu a luz. Tudo mudou. Uma pena, eu gostava de apostar nele."},
                  {"kojiro", "Aposta em mim."},
                  {"karasu", "Hah. Nem que me pagassem."}}, .introCount = 4,
        .outro = {{"karasu", "Tá bom, tá bom, mudei de aposta. Mas lembra: o velho sempre tem um favorito."}}, .outroCount = 1,
        .sensei = {{"hanzo", "O corvo some em penas e volta onde você não olha. Apare quando ele reaparecer, não quando sumir."},
                   {"hanzo", "Ele luta com uma mão só, até a outra aparecer. Aí são duas lâminas: só o perfeito segura."}}, .senseiCount = 2,
        .visit = {{"kojiro", "Karasu."},
                  {"hanzo", "Um corvo. Vive do que sobra dos outros. Não sobrou nada pra ele."},
                  {"hanzo", "Hayate está na ponte de corda sobre o desfiladeiro. Duas foices, rápido, e o ritmo muda sem avisar."},
                  {"hanzo", "Não decore o ritmo dele. Decore o corpo."},
                  {"hanzo", "Cada um deles aprendeu com aquele homem. Não se deixe enganar pelo que disserem dele."}}, .visitCount = 5,
    },
    {
        .id = 7, .name = "hayate", .style = "postura do vento", .title = "O Impaciente", .venue = "Ponte de corda sobre o desfiladeiro",
        .special = "Duas foices e o vento: rápido, o ritmo muda sem avisar, e às vezes as duas foices cortam juntas.",
        .arena = ARENA_PONTE, .posture = 620, .hitsToFall = 6, .healsOnHit = true, .cueVisual = 1, .cueAudio = 1, .tint = 0xD0FFE8FF,
        .rhythmJitter = 0.12f, .waitScale = AJ_ESPERA_X_IRREGULAR,
        .stances = {STANCE("", 0.049f, 0.138f, 0.372f)}, .stanceCount = 1,
        .moves = {
            {"rajada", 1, {0}, 2.0f, -1, 0, LOOK_THRUST, 0.75f},
            {"redemoinho", 2, {0.45f}, 2.0f, -1, 0, LOOK_HIGH, 0.65f},
            {"vendaval", 3, {0.50f, 0.90f}, 1.0f, -1, 0, LOOK_LOW, 0.85f},
            {"brisa cortante", 2, {0.40f}, 1.5f, -1, 0, LOOK_THRUST, 0.75f},
            {"tufão", 5, {0.40f, 0.40f, 0.40f, 0.60f}, 0.8f, -1, 0, LOOK_LOW, 0.65f},
            {"foices gêmeas", 2, {0.45f}, 1.0f, -1, 0, LOOK_HIGH, 0.85f, 0x2},
            {"lufada", 1, {0}, 1.2f, -1, 0, LOOK_DASH, 0.75f},
            {"folha ao vento", 2, {0.55f}, 1.0f, -1, 0, LOOK_JUMP, 0.65f},
            {"ciclone", 1, {0}, 0.8f, -1, 0, LOOK_HEAVY, 0.85f},
            {"gancho duplo", 2, {0.40f}, 1.0f, -1, 0, LOOK_HIGH, 0.70f},
            {"ceifada em X", 1, {0}, 0.25f, -1, 0, LOOK_LOW, 0.80f, 0x1},
            {"vento partido", 3, {0.40f, 0.45f}, 0.8f, -1, 0, LOOK_THRUST, 0.75f},
        },
        .moveCount = 12,
        .intro = {{"hayate", "Finalmente! Tô te esperando faz tempo. Vamos logo!"},
                  {"hayate", "Sabe o que o hanzo fazia? Treinava a gente até a última gota de sangue. E depois mais um pouco."},
                  {"kojiro", "Fala menos."},
                  {"hayate", "Hah! Então vem!"}}, .introCount = 4,
        .outro = {{"hayate", "Mais uma. Só mais uma... Não. Tudo bem. Você venceu."},
                  {"hayate", "Mas olha pro oboro antes de bater nele. Ele não é mais o que era."}}, .outroCount = 2,
        .sensei = {{"hanzo", "O vento muda de direção sem avisar. O ritmo dele também. Espere o ombro."},
                   {"hanzo", "Quem é impaciente começa cedo. Termine tarde."}}, .senseiCount = 2,
        .visit = {{"kojiro", "Hayate."},
                  {"hanzo", "Rápido pra tudo, menos pra entender."},
                  {"hanzo", "Agora, enjin, na forja dentro da cratera. A katana dele queima: quem leva um golpe fica em brasas."},
                  {"hanzo", "Se pegar fogo, apare perfeito, e a brasa apaga."},
                  {"hanzo", "Não lute contra ele. Entenda ele. Depois, devore."}}, .visitCount = 5,
    },
    {
        .id = 8, .name = "enjin", .style = "postura da chama", .title = "O que Odeia", .venue = "Forja dentro da cratera",
        .special = "Katana de fogo: quem leva um golpe fica em brasas e continua perdendo vida; o parry perfeito apaga.",
        .arena = ARENA_FORJA, .posture = 690, .hitsToFall = 6, .burn = 0.6f, .healsOnHit = true, .cueVisual = 1, .cueAudio = 1, .tint = 0xFFB890FF,
        .stances = {STANCE("", 0.046f, 0.131f, 0.359f)}, .stanceCount = 1,
        .moves = {
            {"brasa", 2, {0.50f}, 2.0f, -1, 0, LOOK_HIGH, 0.70f},
            {"labareda", 3, {0.45f, 0.45f}, 2.0f, -1, 0, LOOK_HIGH, 0.80f},
            {"incêndio", 4, {0.45f, 0.45f, 0.80f}, 1.0f, -1, 0, LOOK_LOW, 0.65f},
            {"erupção", 1, {0}, 0.8f, -1, 0, LOOK_HEAVY, 0.70f},
            {"fagulhas", 3, {0.40f, 0.40f}, 1.0f, -1, 0, LOOK_THRUST, 0.80f},
            {"chama viva", 1, {0}, 1.2f, -1, 0, LOOK_DASH, 0.65f},
            {"cinzas", 2, {0.45f}, 1.5f, -1, 0, LOOK_LOW, 0.70f},
            {"fogo alto", 3, {0.45f, 0.45f}, 1.0f, -1, 0, LOOK_JUMP, 0.80f},
            {"labareda larga", 1, {0}, 1.2f, -1, 0, LOOK_HIGH, 0.75f},
            {"ferro em brasa", 2, {0.55f}, 0.7f, -1, 0, LOOK_HIGH, 0.70f},
            {"chicote de chamas", 2, {0.45f}, 1.0f, -1, 0, LOOK_LOW, 0.65f},
        },
        .moveCount = 11,
        .intro = {{"enjin", "O velho mandou mais um. Claro que mandou."},
                  {"enjin", "Ele era um monstro. E o oboro diz que protege a gente."},
                  {"enjin", "Então me responde, garoto: quem usa aquela máscara hoje, hein?"},
                  {"kojiro", "Que máscara?"},
                  {"enjin", "Pergunta pra ele. Se tiver coragem."}}, .introCount = 5,
        .outro = {{"enjin", "Vai, queima tudo. Mas lá em cima, olha bem pra cara dele."}}, .outroCount = 1,
        .sensei = {{"hanzo", "O fogo nunca queima uma vez só."},
                   {"hanzo", "Se pegar fogo, não corra: apare perfeito, e a brasa apaga."}}, .senseiCount = 2,
        .visit = {{"kojiro", "Enjin falou de uma máscara. Uma máscara de oni."},
                  {"hanzo", "..."},
                  {"kojiro", "O homem que matou meu pai usava uma."},
                  {"hanzo", "Então você já sabe o que vai encontrar lá em cima."},
                  {"hanzo", "A próxima é suiren, no porto do farol. Uma lança: ataca de longe, em ondas."},
                  {"hanzo", "Cada um deles aprendeu com aquele homem. Não se deixe enganar pelo que disserem dele."}}, .visitCount = 6,
    },
    {
        .id = 9, .name = "suiren", .style = "postura do mar", .title = "A Amiga de Oboro", .venue = "Porto do farol",
        .special = "Lança: ataca de longe, e a ponta demora mais para chegar do que parece; as ondas aceleram e recuam.",
        .arena = ARENA_PORTO, .posture = 840, .hitsToFall = 6, .healsOnHit = true, .cueVisual = 1, .cueAudio = 1, .tint = 0xC8D8FFFF,
        .accelSteps = 5, .accelFactor = 0.82f,
        .stances = {STANCE("", 0.046f, 0.125f, 0.346f)}, .stanceCount = 1,
        .moves = {
            {"onda", 1, {0}, 2.0f, -1, 0, LOOK_THRUST, 1.20f},
            {"arpão", 1, {0}, 2.0f, -1, 0, LOOK_FAR, 1.05f},
            {"linha d'água", 2, {0.60f}, 1.5f, -1, 0, LOOK_FAR, 1.15f},
            {"maré longa", 3, {0.60f, 0.50f}, 1.0f, -1, 0, LOOK_FAR, 1.20f},
            {"maré baixa", 2, {0.45f}, 1.5f, -1, 0, LOOK_LOW, 1.05f},
            {"arrebentação", 3, {0.45f, 0.45f}, 1.0f, -1, 0, LOOK_THRUST, 1.15f},
            {"maremoto", 4, {0.55f, 0.50f, 0.45f}, 1.0f, -1, 0, LOOK_HIGH, 1.20f},
            {"espuma", 1, {0}, 1.0f, -1, 0, LOOK_DASH, 1.05f},
            {"salto da baleia", 2, {0.60f}, 1.0f, -1, 0, LOOK_JUMP, 1.15f},
            {"vagalhão", 1, {0}, 0.8f, -1, 0, LOOK_HEAVY, 1.20f},
        },
        .moveCount = 10,
        .intro = {{"suiren", "Você está cansado. Dá pra ver. Quer um pouco de água antes?"},
                  {"kojiro", "Não."},
                  {"suiren", "O oboro me tirou do mar quando eu era criança. Ele salvou minha vida. Todos nós devemos tudo a ele."},
                  {"suiren", "Não me faz escolher entre você e ele. Eu já escolhi."}}, .introCount = 4,
        .outro = {{"suiren", "Se chegar até ele... escuta o que ele tem pra dizer. Só isso. Por favor."}}, .outroCount = 1,
        .sensei = {{"hanzo", "A maré sobe cada vez mais rápido, mas nenhuma maré sobe para sempre. Conte as ondas."},
                   {"hanzo", "A lança parte de longe e demora a chegar. Não apare o susto, apare a ponta."}}, .senseiCount = 2,
        .visit = {{"kojiro", "Suiren."},
                  {"hanzo", "Gentil demais. Gentileza é só um jeito educado de ser fraco."},
                  {"hanzo", "Arashi está no salão do castelo, numa noite de tempestade. Duas katanas, e ele adora se ouvir."},
                  {"hanzo", "As duas lâminas descem juntas. Só o parry perfeito segura as duas."},
                  {"hanzo", "Não lute contra ele. Entenda ele. Depois, devore."}}, .visitCount = 5,
    },
    {
        .id = 10, .name = "arashi", .style = "postura da tempestade", .title = "O Orgulhoso", .venue = "Salão do castelo, noite de tempestade",
        .special = "Duas katanas, golpes pesados: as duas lâminas de uma vez, e só o parry perfeito segura as duas.",
        .arena = ARENA_SALAO, .posture = 550, .hitsToFall = 10, .damage = 1.2f, .bladeMax = 0.220f, .healsOnHit = true, .cueVisual = 1, .cueAudio = 1, .tint = 0xE0D0F0FF,
        .stances = {STANCE("", 0.046f, 0.121f, 0.333f)}, .stanceCount = 1,
        .moves = {
            {"faísca", 1, {0}, 1.0f, -1, 0, LOOK_THRUST, 0.80f},
            {"duas tempestades", 1, {0}, 1.5f, -1, 0, LOOK_HIGH, 0.90f, 0x1},
            {"trovoada", 2, {0.45f}, 2.0f, -1, 0, LOOK_HIGH, 0.75f, 0x2},
            {"tormenta", 3, {0.40f, 0.40f}, 1.5f, -1, 0, LOOK_LOW, 0.80f, 0x4},
            {"granizo", 4, {0.40f, 0.40f, 0.40f}, 0.8f, -1, 0, LOOK_THRUST, 0.90f},
            {"ventania", 3, {0.40f, 0.70f}, 1.0f, -1, 0, LOOK_LOW, 0.75f},
            {"trovão", 1, {0}, 1.2f, -1, 0, LOOK_DASH, 0.80f, 0x1},
            {"raio duplo", 2, {0.40f}, 1.0f, -1, 0, LOOK_JUMP, 0.90f, 0x3},
            {"céu partido", 5, {0.40f, 0.40f, 0.40f, 0.80f}, 0.6f, -1, 0, LOOK_HIGH, 0.75f, 0x10},
            {"relâmpago", 1, {0}, 0.8f, -1, 0, LOOK_HEAVY, 0.80f, 0x1},
        },
        .moveCount = 10,
        .intro = {{"arashi", "Então o velho arrumou outro cachorro. Achei que ele tinha desistido."},
                  {"arashi", "Eu era o melhor deste dojo, e mesmo assim o oboro teve que salvar minha vida. Foi a única dívida que eu tive."},
                  {"arashi", "Não vou deixar você cobrar."},
                  {"kojiro", "Não vim cobrar nada. Vim passar."}}, .introCount = 4,
        .outro = {{"arashi", "Impossível... Não. Não é impossível. Eu só não queria que fosse."}}, .outroCount = 1,
        .sensei = {{"hanzo", "A tempestade é orgulhosa: bate sempre no mesmo ritmo."},
                   {"hanzo", "Duas lâminas descem num instante só. Meio parry segura uma; a outra corta."}}, .senseiCount = 2,
        .visit = {{"kojiro", "Arashi."},
                  {"hanzo", "Orgulhoso, sem nunca ter tido do que se orgulhar."},
                  {"hanzo", "Faltam dois. Yoru, no bambuzal, à meia-noite. Duas adagas, e ele apaga as luzes."},
                  {"hanzo", "No escuro, os olhos mentem. Os ouvidos não."},
                  {"hanzo", "Cada um deles aprendeu com aquele homem. Não se deixe enganar pelo que disserem dele."}}, .visitCount = 5,
    },
    {
        .id = 11, .name = "yoru", .style = "postura da noite", .title = "O que Viu", .venue = "Bambuzal à meia-noite",
        .special = "Uma adaga em cada mão: quase sempre as lanternas se apagam e só o som avisa.",
        .arena = ARENA_BAMBUZAL, .posture = 970, .hitsToFall = 5, .healsOnHit = true, .cueVisual = 1, .cueAudio = 1, .tint = 0xB8B0E0FF,
        .blackoutChance = 0.7f,
        .stances = {STANCE("", 0.044f, 0.119f, 0.320f)}, .stanceCount = 1,
        .moves = {
            {"sombra", 1, {0}, 2.0f, -1, 0, LOOK_LOW, 0.90f},
            {"presas", 2, {0.45f}, 2.0f, -1, 0, LOOK_HIGH, 0.80f},
            {"lua nova", 3, {0.45f, 0.80f}, 1.0f, -1, 0, LOOK_LOW, 1.00f},
            {"eclipse", 1, {0}, 0.8f, -1, 0, LOOK_HEAVY, 0.90f},
            {"vultos", 3, {0.40f, 0.70f}, 1.0f, -1, 0, LOOK_THRUST, 0.80f},
            {"breu", 1, {0}, 1.2f, -1, 0, LOOK_DASH, 1.00f},
            {"coruja", 2, {0.45f}, 1.0f, -1, 0, LOOK_JUMP, 0.90f},
            {"meia-noite", 4, {0.40f, 0.40f, 0.90f}, 0.8f, -1, 0, LOOK_LOW, 0.80f},
            {"nevoeiro", 2, {0.70f}, 1.0f, -1, 0, LOOK_THRUST, 1.00f},
        },
        .moveCount = 9,
        .intro = {{"yoru", "Não precisa se esconder. Te ouvi subindo desde o pé da serra."},
                  {"yoru", "Eu vi aquele duelo, sabia? O último. Não foi justo."},
                  {"kojiro", "Eu sei."},
                  {"yoru", "Não. Você não sabe."}}, .introCount = 4,
        .outro = {{"yoru", "O oboro não está bem desde aquele dia. Ninguém aqui consegue alcançar ele. Talvez você consiga."},
                  {"yoru", "Derrota ele, garoto. Só não do jeito que o velho quer."}}, .outroCount = 2,
        .sensei = {{"hanzo", "Na noite, os olhos mentem. Os ouvidos não."},
                   {"hanzo", "Duas presas mordem juntas. Não comemore a primeira."}}, .senseiCount = 2,
        .visit = {{"kojiro", "Yoru disse que o duelo não foi justo."},
                  {"hanzo", "Yoru sempre viu coisas no escuro que não estavam lá."},
                  {"hanzo", "Resta jinshi, na encosta da serra. A katana branca e a postura da lua. Ele não faz som nenhum."},
                  {"hanzo", "Esqueça os ouvidos. Olhe o corpo, só o corpo."},
                  {"hanzo", "Não lute contra ele. Entenda ele. Depois, devore."}}, .visitCount = 5,
    },
    {
        .id = 12, .name = "jinshi", .style = "postura da lua", .title = "O que Espera", .venue = "Encosta da serra",
        .special = "Postura da lua, katana branca forjada com ela: o repertório mais variado, ritmo irregular, e na montanha o som não chega.",
        .arena = ARENA_SERRA, .posture = 630, .hitsToFall = 5, .bladeMax = 0.220f, .healsOnHit = true, .cueVisual = 1, .cueAudio = 0, .tint = 0xE0D8C8FF,
        .stances = {STANCE("", 0.043f, 0.119f, 0.350f)}, .rhythmJitter = 0.08f, .waitScale = AJ_ESPERA_X_IRREGULAR, .stanceCount = 1,
        .moves = {
            {"crescente", 1, {0}, 1.5f, -1, 0, LOOK_HIGH, 0.95f},
            {"minguante", 3, {0.60f, 0.60f}, 1.5f, -1, 0, LOOK_LOW, 0.70f},
            {"fases da lua", 4, {0.50f, 0.50f, 0.90f}, 1.0f, -1, 0, LOOK_HIGH, 1.20f},
            {"luar", 3, {0.45f, 1.00f}, 1.2f, -1, 0, LOOK_THRUST, 0.80f},
            {"lua cheia", 5, {0.50f, 0.50f, 0.50f, 0.90f}, 0.8f, -1, 0, LOOK_HIGH, 1.05f},
            {"lua branca", 6, {0.40f, 0.90f, 0.45f, 0.45f, 1.00f}, 0.8f, -1, 0, LOOK_THRUST, 0.95f},
            {"noite branca", 4, {1.00f, 0.40f, 0.40f}, 1.0f, -1, 0, LOOK_LOW, 0.70f},
            {"reflexo no lago", 1, {0}, 1.2f, -1, 0, LOOK_DASH, 1.20f},
            {"lua alta", 2, {0.70f}, 1.0f, -1, 0, LOOK_JUMP, 0.80f},
            {"halo", 1, {0}, 0.8f, -1, 0, LOOK_HEAVY, 1.05f},
        },
        .moveCount = 10,
        .intro = {{"jinshi", "..."},
                  {"jinshi", "Eu também vi aquele duelo. Não foi justo. E o oboro nunca mais foi o mesmo."},
                  {"jinshi", "Ninguém consegue tirar ele daquele transe. Eu tentei."},
                  {"kojiro", "Não vim ajudar ele."},
                  {"jinshi", "Eu sei. Mesmo assim, tem que ser você."}}, .introCount = 5,
        .outro = {{"jinshi", "Vai. Ele está te esperando há muito tempo. Mais do que você imagina."}}, .outroCount = 1,
        .sensei = {{"hanzo", "Na montanha, o som demora a chegar. Não espere ouvir. Olhe."},
                   {"hanzo", "A lua muda de forma, mas sempre volta. Decore as fases."}}, .senseiCount = 2,
        .visit = {{"kojiro", "Jinshi."},
                  {"hanzo", "O mais calado de todos. Quieto não quer dizer sábio."},
                  {"kojiro", "Só falta ele."},
                  {"hanzo", "Oboro, no meu dojo, no alto da serra. Ele devorou as doze posturas, e vai usar todas contra você."},
                  {"hanzo", "Não lute contra ele. Entenda ele. Depois, devore."}}, .visitCount = 5,
    },
    {
        .id = 13, .name = "oboro", .style = "o mestre das doze posturas", .title = "O Mestre das Doze Posturas", .venue = "O dojo de Hanzo, no alto da serra",
        .special = "Três selos, uma postura em cada: a de hanzo, a de quem devorou as doze, e a do oni, de máscara e lâmina em chamas.",
        .arena = ARENA_CIDADELA, .posture = 360, .hitsToFall = 5, .healsOnHit = true, .specialChance = 0.25f,
        .cueVisual = 1, .cueAudio = 1, .tint = 0xE0C8FFFF, .isBigBoss = true,
        /* Três posturas, uma por selo: a de hanzo, que ele aprendeu primeiro; a de quem
           devorou as doze; e a do oni, de máscara e com a lâmina em chamas, em que ele se perde. */
        .stances = {
            STANCE_EM_ORDEM("postura de hanzo", 0.062f, 0.144f, 0.380f, 1),        /* abre com a lição completa */
            STANCE_EM_ORDEM("devorador de posturas", 0.051f, 0.124f, 0.350f, 12),  /* os doze na ordem da trilha */
            STANCE("postura do oni", 0.042f, 0.108f, 0.320f),
        },
        .stanceCount = 3,
        /* selo: nome, espera antes do aviso x, troca de postura, postura, dano x, sem especial.
         * Erros até cair, com a vida cheia a cada selo (AJ_SELO_CURA): 5, 10 e 4. */
        .seals = {
            {"primeiro selo", 1.00f, 0, 360, 0, true},
            {"segundo selo", 1.00f, 0, 1800, 0.5f, true},  /* comprido: os doze inteiros, até só com perfeitos */
            {"terceiro selo", 0.85f, 0, 450, 1.25f, true},
        },
        .sealCount = 3,
        /* Os ecos copiam uma sequência de cada aprendiz (intervalos, aparência, golpe duplo e
         * preparação), com a janela e o aviso do oboro naquela fase. O teste confere. */
#define ECOS(p)                                                                               \
            {"eco da terra", 2, {1.00f}, 1.0f, p, 0, LOOK_LOW, 1.55f},                          \
            {"eco da tartaruga", 2, {0.42f}, 1.0f, p, 0, LOOK_THRUST, 1.00f},                   \
            {"eco da montanha", 2, {0.85f}, 1.0f, p, 0, LOOK_LOW, 1.05f},                      \
            {"eco do gelo", 2, {0.40f}, 1.0f, p, 0, LOOK_THRUST, 0.75f, 0, true, false},       \
            {"eco do tigre", 6, {0.40f, 0.40f, 0.40f, 0.40f, 0.40f}, 1.0f, p, 0, LOOK_HIGH, 0.90f}, \
            {"eco do corvo", 3, {0.50f, 0.90f}, 1.0f, p, 0, LOOK_LOW, 1.13f, 0x4},              \
            {"eco do vento", 2, {0.45f}, 1.0f, p, 0, LOOK_HIGH, 0.85f, 0x2},                    \
            {"eco da chama", 4, {0.45f, 0.45f, 0.80f}, 1.0f, p, 0, LOOK_LOW, 0.65f},            \
            {"eco do mar", 3, {0.60f, 0.50f}, 1.0f, p, 0, LOOK_FAR, 1.20f},                     \
            {"eco da tempestade", 3, {0.40f, 0.40f}, 1.0f, p, 0, LOOK_LOW, 0.80f, 0x4},         \
            {"eco da noite", 4, {0.40f, 0.40f, 0.90f}, 1.0f, p, 0, LOOK_LOW, 0.80f},            \
            {"eco da lua", 5, {0.50f, 0.50f, 0.50f, 0.90f}, 1.0f, p, 0, LOOK_HIGH, 1.05f}
        .moves = {
            /* postura de hanzo: a arte como o mestre ensinou; abre com a lição inteira */
            {"lição completa", 7, {0.60f, 0.50f, 0.50f, 0.70f, 0.45f, 0.45f}, 2.0f, 0, 0, LOOK_HIGH, 1.15f},
            {"corte do mestre", 1, {0}, 2.0f, 0, 0, LOOK_HIGH, 1.05f},
            {"lição", 2, {0.60f}, 1.5f, 0, 0, LOOK_LOW, 0.95f},
            {"estocada de hanzo", 1, {0}, 1.5f, 0, 0, LOOK_THRUST, 1.15f},
            {"três lições", 3, {0.50f, 0.60f}, 1.0f, 0, 0, LOOK_HIGH, 1.05f},
            {"passo de hanzo", 1, {0}, 1.0f, 0, 0, LOOK_DASH, 0.95f},
            {"salto do mestre", 2, {0.55f}, 1.0f, 0, 0, LOOK_JUMP, 1.15f},
            /* devorador de posturas: os doze, na ordem da trilha na primeira volta */
            ECOS(1),
            /* postura do oni: os mesmos doze, mais rápidos e mais pesados (selo 3) */
            ECOS(2),
        },
#undef ECOS
        .moveCount = 31,
        .intro = {{"oboro", "Então é você. O último que ele mandou."},
                  {"kojiro", "Você traiu o mestre."},
                  {"oboro", "Ele não é quem você pensa."},
                  {"kojiro", "Saca a espada."},
                  {"oboro", "Doze posturas, uma de cada aprendiz. Vou te mostrar todas."}}, .introCount = 5,
        .outroCount = 0,
        .sensei = {{"hanzo", "Confie em você mesmo. Use tudo que aprendeu."}}, .senseiCount = 1,
        .visitCount = 0,
    },
};

const MasterProfile *roster_get(int index) {
    if (index < 0 || index >= ROSTER_SIZE) return NULL;
    return &ROSTER[index];
}

int roster_size(void) { return ROSTER_SIZE; }

/* A abertura é narrada: cada entrada é um parágrafo que sobe pela tela.
 * Parágrafos entre aspas são citações e aparecem destacados. É a história como
 * Kojiro acredita nela, do jeito que Hanzo contou. */
static const char *LORE[LORE_PAGES] = {
    "Hattori Hanzo criou a Arte do Aparar: aparar para se defender, ler a postura do inimigo, desarmar sem ferir.",
    "No alto da serra, fundou um dojo e reuniu treze aprendizes. Procurava um sucessor.",
    "O mais promissor era Oboro. Durante anos, Hanzo acreditou nele como em nenhum outro.",
    "Oboro pagou com a lâmina. Desafiou o mestre, venceu e tomou o dojo, como manda a tradição. "
    "Hanzo desceu a serra sozinho.",
    "Anos depois, numa estrada, Hanzo encontrou um menino. Kojiro tinha visto o pai morrer pelas mãos de um "
    "homem com máscara de oni, e desde então não tinha paz.",
    "Hanzo o recolheu e o treinou na metade da arte que se ensina.",
    "“A outra metade não se ensina. Tem que vir de você.”",
    "Oboro destruiu tudo o que Hanzo construiu. Os doze aprendizes que ficaram com ele guardam o caminho até o dojo.",
    "Agora Kojiro sobe a Trilha dos Doze Aprendizes. Pelo mestre. Pela vingança.",
    "“Não lute contra ele. Entenda ele. Depois, devore.”",
};

const char *lore_page(int index) {
    if (index < 0 || index >= LORE_PAGES) return "";
    return LORE[index];
}

/* A luta final: a cada selo quebrado, oboro para e fala; de joelhos, tira a máscara.
 * Depois da escolha, os dois finais. Nos dois oboro morre. */
static const Beat SEAL_1[] = {
    {"oboro", "Ele te contou do treino? Até a última gota de sangue. Todo dia. Até alguém não levantar mais.", CUE_NONE},
    {"oboro", "E no último duelo... ele parou. No meio do golpe, parou de lutar. Eu venci um homem que não quis lutar.", CUE_NONE},
    {"kojiro", "Mentira.", CUE_NONE},
};
static const Beat SEAL_2[] = {
    {NULL, NULL, CUE_MASK_ON},
    {"kojiro", "Foi você.", CUE_NONE},
    {"oboro", "O homem dessa máscara matou meus pais também.", CUE_NONE},
    {"oboro", "Eu uso isso pra lembrar do que eu quase me tornei. E pra que o próximo garoto que ele mandasse "
              "viesse atrás de mim, e não deles.", CUE_NONE},
    {"oboro", "Eu só não esperava que fosse você.", CUE_NONE},
};
static const Beat KNEEL[] = {
    {NULL, NULL, CUE_MASK_OFF},
    {"oboro", "Eu achei isso no baú dele.", CUE_NONE},
};
static const Beat SIM[] = {
    {NULL, NULL, CUE_RAISE},
    {"oboro", "Então ele conseguiu.", CUE_NONE},
    {NULL, NULL, CUE_KILL},
    {"hanzo", "Era exatamente isso que eu queria que você fizesse.", CUE_HANZO_CLAP},
    {"hanzo", "Finalmente, alguém pra quem passar a tocha.", CUE_NONE},
    {"hanzo", "Você vai ser melhor do que ele. Oboro nunca foi o que eu queria, nem quando me venceu.", CUE_NONE},
    {"hanzo", "Os outros? Falhas, todos eles. Treze aprendizes, e nenhum deu o último passo.", CUE_NONE},
    {"hanzo", "Você deu.", CUE_HANZO_MASK},
    {"kojiro", "Por que o senhor parou de lutar, naquele duelo?", CUE_NONE},
    {"hanzo", "Vamos pra casa, kojiro.", CUE_NONE},
};
static const Beat NAO[] = {
    {NULL, NULL, CUE_LOWER},
    {"oboro", "Obrigado.", CUE_NONE},
    {"hanzo", "Você é mais uma falha, não é?", CUE_HANZO_IN},
    {"hanzo", "Todos os anos que eu te treinei, todo o meu esforço, todo o meu suor, e você é só mais uma falha.", CUE_NONE},
    {"hanzo", "Que nem essa praga aqui.", CUE_NONE},
    {NULL, NULL, CUE_HANZO_MASK},
    {NULL, NULL, CUE_HANZO_KILL},
    {"hanzo", "Você? Não vale o trabalho.", CUE_NONE},
    {NULL, NULL, CUE_CHASE},
    {"kojiro", "hanzo!", CUE_NONE},
};

const Beat *story_scene(SceneId id, int *count) {
#define SCENE(a) do { *count = (int)(sizeof a / sizeof a[0]); return a; } while (0)
    switch (id) {
        case SCENE_SEAL_1: SCENE(SEAL_1);
        case SCENE_SEAL_2: SCENE(SEAL_2);
        case SCENE_KNEEL: SCENE(KNEEL);
        case SCENE_SIM: SCENE(SIM);
        case SCENE_NAO: SCENE(NAO);
        default: break;
    }
#undef SCENE
    *count = 0;
    return NULL;
}
