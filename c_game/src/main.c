/*
 * main.c - APARAR: A Trilha dos Doze Aprendizes.
 * Tudo em pixel art de 320 x 180, ampliado por número inteiro, sem filtro.
 * A interface é montada em coordenadas de 1280 x 720 (4 por pixel), mas desenhada
 * numa camada de 320 x 180, presa na grade, com a fonte de pixel Tiny5 e paleta
 * antiga: tinta, papel envelhecido, dourado gasto e vermelhão.
 *
 * Controles: clique esquerdo, Espaço, J ou Enter = aparar / avançar.
 * Esc = pausa. F = liga/desliga o tremor de tela. F11 = tela cheia.
 *
 * Opções de teste (sem efeito no jogo normal):
 *   --master N     começa direto nas falas do mestre N (1 a 13)
 *   --duel         pula as falas e vai direto ao duelo
 *   --state S      title | lore | trail | ending | calibra (com --master N: sensei;
 *                  com --master N --duel: pause | defeat | finisher | cleared)
 *   --demo         um robô apara no tempo perfeito e avança as telas
 *   F3 (ou APARA_DEBUG=1): overlay de debug com janelas, os últimos apertos (resultado e
 *                  erro em ms) e o estado do duelo.
 *   --teste        o modo de teste: liga o F3, vai direto ao duelo (com --master N e --fase F)
 *                  e liga as teclas de teste, na luta: R recomeça, V enche a vida, P enche a
 *                  postura do mestre, 1 a 3 escolhem a fase do oboro, N e B vão para o próximo
 *                  mestre e o anterior. Só existem com --teste.
 *   --fase F       com --master 13: começa na fase F do oboro (1 a 3)
 *   Nada disso grava o progresso: --teste, --master, --duel, --state, --fase, --final e
 *   --demo jogam sem salvar (apara_save.txt fica como está).
 *   --shot F T     salva uma captura em F depois de T segundos e sai
 *   --rec D T0 T1  salva os quadros de T0 a T1 segundos em D (30 por segundo, tempo fixo)
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "arenas.h"
#include "audio.h"
#include "core.h"
#include "desempenho.h"
#include "entrada.h"
#include "entrada_plat.h"
#include "robo.h"
#include "salvar.h"
#include "fonte.h"
#include "fx.h"
#include "katana3d.h"
#include "lore.h"
#include "pixelize.h"
#include "raylib.h"
#include "rig.h"
#include "rlgl.h"
#include "sprites.h"

#define REN_X 124.0f
#define BOSS_X 190.0f
#define ACTOR_SCALE 1.25f     /* tamanho dos bonecos (sem as pranchas dos sprites) */
#define UI_W 1280
#define UI_H 720
#define UNIT 4.0f             /* px da interface por px do mundo */
#define RS 1                  /* tudo em pixel art de 320 x 180, ampliado por número inteiro */
#define RW (LOW_W * RS)
#define RH (LOW_H * RS)
#define SWORD_GRAVITY 380.0f
#define VFX_MAX 12            /* efeitos das folhas tocando ao mesmo tempo */
#define AFTER_MAX 8           /* silhuetas que o mestre deixa nos movimentos rápidos */
#define KARASU_WARP_RECUO 28.0f       /* foge para a direita antes de virar penas */
#define KARASU_WARP_ANTES_AVISO 0.04f /* reaparece antes do brilho/aviso existente */
#define KARASU_WARP_PENAS 0.035f      /* espaçamento do rastro durante o recuo */
#define KARASU_WARP_DISSOLVE 0.10f    /* o corpo apaga neste tempo (s) antes de virar penas: sem corte seco */
#define KARASU_WARP_FORMA 0.04f       /* e se forma neste tempo ao reaparecer: ele reaparece 40 ms antes do aviso (KARASU_WARP_ANTES_AVISO), então está inteiro no aviso */
#define KARASU_WARP_PENAS_S 240.0f    /* penas por segundo que se soltam do corpo, ou voltam para ele */
#define ARASHI_ID 10                  /* o relâmpago (o golpe pesado dele): raios caem do céu e, quem leva, fica meio paralisado. Só desenho: o núcleo julga como qualquer outro golpe */
#define RAIOS_MAX 6
#define RAIO_VIDA 0.26f               /* s que cada raio fica na tela */
#define PARALISIA_CHEIA 0.9f          /* s de choque no kojiro quando o golpe pega em cheio (erro: as duas lâminas entram) */
#define PARALISIA_METADE 0.5f         /* s quando só uma lâmina entra (aparo bom): meio paralisado */
#define PARALISIA_LENTIDAO 0.65f      /* quanto a queda do kojiro (o HURT) fica mais lenta com o choque cheio */
enum { LEAP_NONE, LEAP_DASH, LEAP_JUMP, LEAP_FAR, LEAP_WARP, LEAP_FEINT };
#define PIX_TITLE 40          /* paletas das telas fora do duelo (as dos cenários são o ArenaId) */
#define PIX_LORE 41
#define PIX_TRAIL 42
#define PIX_SCENE 43
#define PIX_ENDING 44
#define SKIP_HOLD 2.0f        /* segundos segurando Esc para pular a abertura */

static const char *POST_FS =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor; uniform sampler2D texture0;\n"
    "uniform float aberr; uniform vec2 res; uniform float desat; uniform float duo; out vec4 finalColor;\n"
    "void main() {\n"
    "    vec2 uv = fragTexCoord;\n"
    "    vec2 d = vec2(aberr / res.x, 0.0);\n"
    "    vec3 c = vec3(texture(texture0, uv + d).r, texture(texture0, uv).g, texture(texture0, uv - d).b);\n"
    "    float lum = dot(c, vec3(0.3, 0.59, 0.11));\n"
    "    c = mix(c, vec3(lum), desat);\n"
    "    c = mix(c, lum > 0.33 ? vec3(0.96, 0.94, 0.9) : vec3(0.12, 0.0, 0.02), duo);\n"
    "    c *= 1.0 - 0.07 * mod(floor(uv.y * res.y), 2.0);\n"
    "    vec2 v = uv - 0.5;\n"
    "    c *= 1.0 - dot(v, v) * (0.45 + desat * 1.4);\n"
    "    finalColor = vec4(c, 1.0);\n"
    "}\n";

typedef enum {
    ST_TITLE, ST_LORE, ST_TRAIL, ST_INTRO, ST_DUEL, ST_FINISHER, ST_OUTRO, ST_CLEARED, ST_DEFEAT, ST_SENSEI, ST_ENDING,
    ST_VISIT, ST_SCENE, ST_CHOICE, ST_CALIBRA
} State;

/* Paleta da interface. */
static const Color INK = {24, 19, 16, 255};
static const Color PAPER = {236, 222, 192, 255};
static const Color AGED_GOLD = {201, 160, 82, 255};
static const Color VERMILION = {184, 62, 40, 255};
static const Color INK_TEXT = {36, 22, 12, 255};
static const Color INK_SOFT = {76, 52, 32, 255};
static const Color INK_LINE = {52, 32, 18, 255};
static const Color SEAL_RED = {150, 44, 32, 255};

/* Visual de cada lutador como boneco, usado quando faltam as pranchas em
 * assets/sprites (e para a cor da arma que voa no desarme).
 * Roupa de acordo com o lugar de cada mestre; katana com lâmina, cabo e largura próprios. */
#define RGB(r, g, b) ((Color){r, g, b, 255})

static const Look REN_LOOK = {
    .coat = RGB(150, 56, 18), .sleeve = RGB(168, 70, 26), .pants = RGB(58, 38, 30), .skin = RGB(232, 186, 146),
    .hair = RGB(22, 18, 22), .blade = RGB(240, 240, 245), .hat = HAT_LONG_HAIR, .size = 1, .bladeLen = 20, .hairTail = true,
    .handle = RGB(96, 52, 28), .bladeWidth = 1};

static const Look MASTER_LOOKS[ROSTER_SIZE] = {
    /* daichi: cores de terra, chapéu de palha, hakama e espada pesada. */
    {.coat = RGB(120, 92, 58), .sleeve = RGB(150, 120, 80), .pants = RGB(70, 56, 40), .skin = RGB(214, 164, 120),
     .hair = RGB(60, 40, 26), .blade = RGB(196, 196, 196), .hat = HAT_KASA, .size = 1.15f, .bladeLen = 22,
     .robe = 0.5f, .pantsWidth = 1.5f, .handle = RGB(90, 60, 36), .bladeWidth = 1.8f},
    /* genbu: velho de túnica verde-musgo e contas, uma katana simples. */
    {.coat = RGB(96, 110, 86), .sleeve = RGB(170, 170, 150), .pants = RGB(60, 64, 52), .skin = RGB(214, 170, 136),
     .hair = RGB(186, 186, 186), .blade = RGB(220, 224, 228), .hat = HAT_NONE, .size = 1.1f, .bladeLen = 19,
     .robe = 1.2f, .flare = 0.3f, .extra = RGB(90, 60, 36), .extras = EX_BEADS, .handle = RGB(60, 70, 50), .bladeWidth = 1},
    /* raizo: gi marrom, ombreiras, chapéu de palha, hakama larga e um espadão enorme. */
    {.coat = RGB(92, 58, 40), .sleeve = RGB(200, 188, 168), .pants = RGB(40, 36, 44), .skin = RGB(222, 176, 136),
     .hair = RGB(24, 20, 20), .blade = RGB(232, 232, 238), .hat = HAT_KASA, .size = 1.2f, .bladeLen = 30,
     .robe = 0.6f, .flare = 0.3f, .pantsWidth = 1.6f, .trim = RGB(150, 110, 60), .extra = RGB(96, 96, 106), .extras = EX_PAULDRONS,
     .handle = RGB(110, 30, 30), .bladeWidth = 1.25f},
    /* shizuku: quimono branco e azul-gelo até o chão, cabelo prateado, florete fino. */
    {.coat = RGB(214, 236, 248), .sleeve = RGB(246, 252, 255), .pants = RGB(138, 180, 214), .skin = RGB(246, 210, 184),
     .hair = RGB(164, 184, 212), .blade = RGB(246, 252, 255), .hat = HAT_LONG_HAIR, .size = 1, .bladeLen = 26,
     .robe = 2, .flare = 0.6f, .trim = RGB(158, 240, 255), .handle = RGB(190, 214, 236), .bladeWidth = 0.5f},
    /* garfiel: o tigre; roupa preta com detalhes vermelhos, cabelo loiro, uma garra em cada mão. */
    {.coat = RGB(34, 30, 38), .sleeve = RGB(44, 40, 48), .pants = RGB(24, 22, 28), .skin = RGB(234, 186, 146),
     .hair = RGB(232, 184, 60), .blade = RGB(250, 246, 236), .hat = HAT_NONE, .size = 1.1f, .bladeLen = 9,
     .robe = 0.4f, .pantsWidth = 1.3f, .trim = RGB(200, 40, 40), .extra = RGB(200, 40, 40), .extras = EX_SCARF,
     .handle = RGB(40, 40, 46), .bladeWidth = 1.3f, .offhand = OFF_DAGGER},
    /* karasu: sobretudo preto com capa de penas, uma wakizashi em cada mão. */
    {.coat = RGB(24, 24, 30), .sleeve = RGB(24, 24, 30), .pants = RGB(20, 20, 26), .skin = RGB(220, 190, 170),
     .hair = RGB(14, 14, 18), .blade = RGB(210, 214, 226), .hat = HAT_NONE, .size = 1, .bladeLen = 15,
     .robe = 1.1f, .trim = RGB(150, 30, 40), .extra = RGB(30, 30, 46), .extras = EX_CAPE, .handle = RGB(30, 30, 40),
     .bladeWidth = 1, .offhand = OFF_SWORD},
    /* hayate: jaqueta curta verde-água e cachecol branco ao vento, uma foice em cada mão. */
    {.coat = RGB(60, 150, 140), .sleeve = RGB(220, 230, 220), .pants = RGB(40, 50, 56), .skin = RGB(226, 184, 150),
     .hair = RGB(40, 30, 30), .blade = RGB(236, 244, 244), .hat = HAT_NONE, .size = 1, .bladeLen = 13,
     .robe = 0.3f, .trim = RGB(220, 240, 230), .extra = RGB(236, 236, 230), .extras = EX_SCARF,
     .handle = RGB(110, 80, 50), .bladeWidth = 1.1f, .offhand = OFF_DAGGER},
    /* enjin: vermelho com dourado, braços de fora, sabre em brasa. */
    {.coat = RGB(170, 40, 26), .sleeve = RGB(214, 150, 110), .pants = RGB(40, 26, 24), .skin = RGB(214, 150, 110),
     .hair = RGB(200, 70, 30), .blade = RGB(255, 184, 124), .hat = HAT_NONE, .size = 1.05f, .bladeLen = 21,
     .trim = RGB(220, 170, 70), .handle = RGB(50, 30, 24), .bladeWidth = 1.1f},
    /* suiren: quimono azul-claro, hakama azul-marinho, rabo de cavalo azul petróleo, lança. */
    {.coat = RGB(190, 226, 246), .sleeve = RGB(238, 248, 255), .pants = RGB(40, 74, 124), .skin = RGB(242, 194, 156),
     .hair = RGB(21, 80, 108), .blade = RGB(226, 236, 246), .hat = HAT_LONG_HAIR, .size = 1.05f, .bladeLen = 34,
     .robe = 0.9f, .trim = RGB(90, 200, 190), .weapon = WEAPON_SPEAR, .bladeWidth = 1},
    /* arashi: violeta e prata, ombreiras, cabelo branco, duas espadas. */
    {.coat = RGB(84, 70, 110), .sleeve = RGB(60, 50, 84), .pants = RGB(30, 26, 40), .skin = RGB(236, 204, 184),
     .hair = RGB(232, 232, 240), .blade = RGB(236, 236, 250), .hat = HAT_NONE, .size = 1.05f, .bladeLen = 19,
     .robe = 1, .pantsWidth = 1.3f, .trim = RGB(200, 200, 216), .extra = RGB(170, 170, 186), .extras = EX_PAULDRONS,
     .handle = RGB(60, 50, 84), .bladeWidth = 0.9f, .offhand = OFF_SWORD},
    /* yoru: capuz e roupa de noite, cachecol vinho, duas adagas. */
    {.coat = RGB(28, 28, 40), .sleeve = RGB(28, 28, 40), .pants = RGB(18, 18, 26), .skin = RGB(200, 186, 176),
     .hair = RGB(20, 20, 32), .blade = RGB(200, 204, 220), .hat = HAT_HOOD, .size = 1, .bladeLen = 10,
     .robe = 0.4f, .extra = RGB(120, 26, 36), .extras = EX_SCARF, .handle = RGB(20, 20, 26), .bladeWidth = 0.9f,
     .offhand = OFF_DAGGER},
    /* jinshi: a postura da lua; roupa roxa com verde, cabelo roxo comprido, a katana bem branca. */
    {.coat = RGB(160, 120, 210), .sleeve = RGB(200, 170, 232), .pants = RGB(30, 70, 46), .skin = RGB(242, 194, 156),
     .hair = RGB(78, 30, 104), .blade = RGB(255, 255, 255), .hat = HAT_LONG_HAIR, .size = 1, .bladeLen = 24,
     .robe = 1.2f, .flare = 0.4f, .trim = RGB(90, 208, 138), .handle = RGB(47, 138, 90), .bladeWidth = 1},
    /* oboro: o uniforme da escola em preto e roxo, cabelo solto, capa, a katana de hanzo. */
    {.coat = RGB(26, 20, 30), .sleeve = RGB(60, 34, 80), .pants = RGB(20, 16, 24), .skin = RGB(226, 200, 188),
     .hair = RGB(16, 12, 20), .blade = RGB(200, 200, 220), .hat = HAT_LONG_HAIR, .size = 1.15f, .bladeLen = 22,
     .robe = 1.1f, .flare = 0.3f, .pantsWidth = 1.3f, .trim = RGB(130, 80, 170), .extra = RGB(80, 40, 110),
     .extras = EX_PAULDRONS | EX_CAPE, .handle = RGB(30, 20, 30), .bladeWidth = 1},
};

static KatanaStyle katana_style(const Look *l) {
    KatanaStyle k = {l->blade, l->handle.a ? l->handle : RGB(60, 40, 34), l->bladeWidth > 0 ? l->bladeWidth : 1};
    return k;
}

/* Lutador em pixel art: as pranchas de assets/sprites tocadas no ritmo do duelo.
 * Sem as pranchas (set == NULL) o lutador é o boneco de rig.c. */
typedef struct { const SprAnim *a; int from, to; float dur; bool cycle; } FSeg;
typedef struct {
    const SprSet *set;
    SprPlayer pl;
    FSeg q[4];
    int qn;
    bool fresh;               /* o próximo f_add toca na hora */
    bool idle;                /* parado na guarda */
    bool autoIdle;            /* volta para a guarda quando a fila acaba */
    bool furia;               /* oboro depois do grito */
    int squat;                /* px que o tronco desce: pegar impulso, amortecer a queda */
    const SprAnim *strike;    /* golpe (ou parry) em curso */
} Fighter;

/* A espada do mestre voando depois do desarme. */
typedef struct {
    bool active, stuck;
    Vector2 pos, vel;
    float angle, spin, len, t, flight, landY, target, stuckTime;
    Color blade;
    KatanaStyle style;
} FlySword;

/* Um raio do relâmpago do arashi: cai em x depois de `espera` s e fica `vida` s na tela. */
typedef struct { float x, espera, vida; unsigned semente; bool principal; } Raio;

/* F3: um aperto anotado (resultado, quando, erro em ms). */
typedef struct {
    char res[12], quando[40], erro[80];
    Color cor;
} Anotacao;

static struct {
    RenderTexture2D scene, actors, uiLow;
    Font ui, uiBold;

    Shader post;
    int locAberr, locRes, locDesat, locDuo;
    float aberr;              /* aberração cromática (px), decai sozinha */
    float desat;              /* quebra de postura: tela sem cor e bordas escuras */
    float duo;                /* execução: a tela em duas cores */
    float slash;              /* execução: o traço de corte atravessando a tela */
    float crack;              /* rachadura branca no mestre */
    float silence;            /* a trilha some por um instante */
    Rig ren, boss;
    Fighter renS, bossS;
    float bossStep, bossStepTo, bossStepSpeed; /* passo do mestre até o alcance do golpe */
    float bossStrikeStep;     /* onde ele precisa estar no contato */
    int leap, leapStage;      /* investida correndo ou salto em curso (LEAP_*) */
    float leapT, leapAt, leapAir; /* tempo na preparação; quando corre ou salta; tempo no ar */
    float warpPenasT;          /* cadência visual das penas no recuo do Karasu */
    float feintFrom;           /* posição antes da ameaça de florete sem contato */
    float hopT, hopLen, hopH; /* arco do pulo do mestre (salto, recuo, ameaça) */
    float hopFrom, hopTo;
    bool hopTravel;           /* um caminho contínuo no ar, sem novo bote no lançamento */
    float landT;              /* amortecendo a queda do salto */
    bool bossHidden;          /* karasu virou penas: some até reaparecer na frente de kojiro */
    float bossDissolve;       /* 0 = inteiro, 1 = só penas: o corpo apaga e volta aos poucos (zero é o normal) */
    float warpSolta;          /* penas por soltar ou juntar (acumulador, para a cadência não depender do quadro) */
    Raio raios[RAIOS_MAX];    /* o relâmpago do arashi */
    float paralisia, paralisiaTotal, paralisiaForca, paralisiaFaisca;   /* o choque no kojiro: o que falta (s), a duração, de 0 a 1, e a cadência das faíscas */
    struct { const SprAnim *a; int frame; Vector2 feet; float life; } after[AFTER_MAX];
    int afterHead;
    float afterTimer, lastStep;
    float auraLeft, auraTick;
    int auraEcho;
    bool gritoPending;
    struct { const SprFx *fx; int row; Vector2 pos; float t, fps, scale; bool flip, back, glow, sword, body, offhand; Color tint; } vfx[VFX_MAX];
    Fx fx;
    FlySword sword;

    Settings settings;
    Duel duel;
    Campaign camp;
    const MasterProfile *m;

    State state;
    float stateTime;
    float time;
    bool paused;

    const Line *lines;
    int lineCount, lineIndex;
    float typeChars;
    int lorePage;
    float loreScroll, loreHeight, skipHold;

    int menuIndex;
    bool hasSave;
    char avisoSave[192];      /* o que dizer ao jogador sobre o save (não gravou, estava corrompido) */
    float avisoSaveAte;       /* G.time até quando a faixa fica na tela */

    /* Coreografia. */
    bool bossWinding;
    float windupLen, windupTime;
    float windupSpr;          /* a preparação como se a lâmina partisse no tempo fixo: os quadros
                                 tocam nela, então a lâmina variável não muda nenhum quadro */
    float renParryTime;       /* tempo desde o gesto; -1 = nenhum pendente */
    float renKnock, bossKnock;
    float bossHome;
    float hitstop;
    float slowmo, slowmoTime;
    float staggerTime;
    float blackoutTarget;
    float lightningTimer;
    float bannerTime;
    char banner[64];
    Color bannerColor;
    float renStepFrom;
    bool special;             /* golpe especial em preparação */
    int defeatsHere;          /* derrotas seguidas contra o mestre atual */
    int defeatIndex;          /* opção escolhida no painel de derrota */

    /* A luta final (story_scene), a escolha e os finais. */
    SceneId sceneId;
    const Beat *beats;
    int beatCount, beatIndex;
    float beatTime, beatPrev; /* desde que o momento da cena começou (e no quadro anterior) */
    bool cueFired;            /* a ação do momento já aconteceu */
    float cueFrom;            /* onde kojiro estava quando a ação começou */
    bool sealTold[MAX_SEALS]; /* oboro já falou depois deste selo */
    bool masked;              /* oboro de máscara: a terceira forma */
    bool maskOnGround;        /* a máscara que ele tirou, no chão */
    float maskDrop;           /* 0..1: caindo */
    int choice;               /* -1 nenhuma, 0 sim, 1 não */
    SceneId ending;           /* SCENE_SIM ou SCENE_NAO, depois da escolha */
    int demoChoice;           /* --final: o que o robô escolhe (0 sim, 1 não) */
    bool windOnly;            /* da escolha em diante: sem trovões, só o vento */
    struct {
        bool on;
        float x, from, to, t, len; /* anda de from até to em len segundos */
        float alpha;
        bool faceLeft;
        Fighter f;
    } hz;                     /* hanzo em cena, nos finais */

    float shownRen, shownBoss, ghostRen, ghostBoss;
    ArenaCtx ctx;

    bool demo;
    bool debug;               /* overlay de debug: F3 ou APARA_DEBUG=1 */
    bool teste;               /* uma opção de teste (--teste, --master, --duel, --state, --fase, --final): o progresso não é salvo */
    bool teclas;              /* --teste: as teclas de teste (R V P 1 2 3 N B) funcionam */
    bool autoJogo;            /* APARA_AUTO (tests/teste_save.sh): joga sozinho como o demo, mas salvando */
    bool lento;               /* APARA_VITORIA_LENTA: depois de salvar a vitória, o jogo anda em tempo real (tests/teste_vitoria.sh) */
    bool rastro;              /* o rastro fantasma do golpe (AJ_RASTRO_FANTASMA; APARA_RASTRO=0/1 só para os testes) */
    bool logImpactos;         /* APARA_LOG_IMPACTOS (tests/teste_rastro.sh): escreve cada impacto e os fantasmas desenhados */
    long fantasmasDesenhados;
    int teclaFalsa;           /* APARA_TECLAS (o mesmo teste): a tecla de teste que o teste "aperta" agora */
    /* O carimbo do clique (entrada.h): o instante de hardware que o sistema deu, no lugar do meio do quadro. */
    bool usaCarimbo;          /* o sistema dá carimbo e nada o desligou (demo, jogo automático, capturas, APARA_SEM_CARIMBO) */
    bool logCarimbos;         /* APARA_LOG_CARIMBOS (tests/teste_carimbo.sh): escreve cada carimbo e cada aperto do duelo */
    bool modoCarimbo;         /* --carimbo: só mede o atraso do poll em relação ao clique */
    double poll;              /* o relógio dos carimbos (entrada_relogio), lido assim que o quadro começa: o fim do quadro */
    double quadro;            /* a duração real deste quadro, s (0 se o quadro não conta: pausa, travamento) */
    double carimbo;           /* o carimbo do aperto deste quadro, ou ENTRADA_SEM_CARIMBO */
    long carimbados, semCarimbo;   /* F3: apertos do duelo que usaram o carimbo e que caíram no meio do quadro */
    double atrasoPoll;        /* F3: s do último clique carimbado até o poll que o leu */
    /* Desempenho (APARA_PERF=segundos: joga esse tempo, escreve o relatório e sai; desempenho.h) */
    bool perf;
    double perfSegundos, perfInicio, perfUltimaCarga;
    Desempenho perfDados;
    unsigned char *perfEstado;   /* o estado do jogo em cada quadro medido, para achar a causa de um pico */
    float *perfTempo;            /* e o G.time dele */
    char perfCarga[16][48];      /* a carga, etapa a etapa: nome e milissegundos */
    double perfCargaMs[16];
    int perfCargas;
    int carimboN;             /* --carimbo: cliques medidos e o atraso somado, mínimo e máximo (s) */
    double carimboSoma, carimboMin, carimboMax;
    int startSeal;            /* a próxima luta começa neste selo (--fase, teclas 1 a 3) */
    Anotacao aperto[6];       /* F3: os últimos apertos, do mais novo ao mais velho */
    int apertos;
    float latVideo, latAudio; /* calibração (s): atraso de vídeo e de áudio do jogador */
    struct {
        int modo;             /* 0 vídeo, 1 áudio, 2 resultado */
        float t;              /* segundos desde o começo do teste */
        int proxima;          /* próxima batida a tocar */
        int n;
        float off[AJ_CALIBRA_APERTOS];
        float video, audio, ultimo;
        bool falhou;
        State voltar;
        bool pausado;
    } cal;
    RoboMente robo;           /* --demo: o robô perfeito dos testes */
    Robo roboEscolhido;       /* o robô do demo: o perfeito, ou o de APARA_ROBO (cedo, tarde, casual, spam, nunca) */
    float cliquePeriodo;      /* fora do duelo, o robô clica a cada isto (AJ_AUTO_CLIQUE_PERIODO; APARA_CLIQUE_PERIODO) */
    float cliqueFlash;        /* s que faltam para apagar o ponto de "clique" (só com APARA_CLIQUE_PERIODO) */
    double recDt;             /* --rec: o passo fixo de cada quadro (1/30; APARA_REC_FPS) */
    FILE *recRaw;             /* APARA_REC_RAW: os quadros em RGB cru, para o ffmpeg ler */
    const char *shotFile;
    float shotTime;
    const char *recDir;       /* --rec: quadros com passo fixo (30 por segundo, ou APARA_REC_FPS), para GIFs e vídeos */
    float recStart, recEnd;
    int recFrame;
} G;

static void start_scene(SceneId id);

/* ------------------------------------------------------------------ */
/* Utilidades                                                          */
/* ------------------------------------------------------------------ */

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static Color fadec(Color c, float a) { c.a = (unsigned char)(c.a * clampf(a, 0, 1)); return c; }
static float smooth(float t) { t = clampf(t, 0, 1); return t * t * (3 - 2 * t); }

static bool pressed_key_mouse(void) {
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_J) || IsKeyPressed(KEY_ENTER);
}

static bool pressed(void) {
    /* No modo demonstração, o robô também avança falas e painéis. */
    if ((G.demo || G.autoJogo) && G.state != ST_DUEL && fmodf(G.stateTime, G.cliquePeriodo) < (G.recDir ? (float)G.recDt : GetFrameTime())) {
        G.cliqueFlash = 0.05f;
        return true;
    }
    if (G.demo && (G.shotFile || G.recDir)) return false; /* capturas: só o robô joga */
    return pressed_key_mouse() || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
}

/* Não corta um caractere UTF-8 ao meio. */
static int utf8_visible(const char *s, float chars) {
    int n = (int)chars, len = (int)strlen(s);
    if (n >= len) return len;
    while (n > 0 && ((unsigned char)s[n] & 0xC0) == 0x80) n++;
    return n;
}

static void banner(const char *s, Color c) {
    snprintf(G.banner, sizeof G.banner, "%s", s);
    G.bannerColor = c;
    G.bannerTime = 1.6f;
}

/* ------------------------------------------------------------------ */
/* Interface em pixel art                                              */
/* ------------------------------------------------------------------ */
/* A interface continua pensada em 1280 x 720, mas é desenhada numa tela de
 * 320 x 180 (1 pixel = 4 unidades) e ampliada sem filtro, como o mundo. A
 * fonte é a Tiny5, nítida no tamanho de 8 px de "em" (9 px de altura de linha na
 * raylib, que mede ascendente + descendente) e nos múltiplos: 9 px = 36 unidades. */
#define PX 4.0f                                   /* unidades da interface por pixel */

static float snap(float v) { return floorf(v / PX + 0.5f) * PX; }

/* Tamanho pedido -> tamanho da fonte de pixel (9, 18, 27, 36 px de linha...). */
static float px_size(float size) {
    float k = floorf(size / 36.0f + 0.7f);
    return (k < 1 ? 1 : k) * 36.0f;
}
static float ui_spacing(float size) { (void)size; return 0; }  /* a Tiny5 já traz 1 px entre as letras */

static float ui_width_f(Font f, const char *s, float size) { return MeasureTextEx(f, s, px_size(size), ui_spacing(size)).x; }
static float ui_width(const char *s, float size) { return ui_width_f(G.ui, s, size); }

/* Texto sobre a cena: sombra escura de 1 px. Texto sobre pergaminho: tinta, sem sombra.
 * O texto fica centrado na altura que o layout pediu e preso na grade de pixels. */
static void draw_text_f(Font f, const char *s, float x, float y, float size, Color c, bool shadow) {
    float ps = px_size(size), sp = ui_spacing(size), k = ps / 36.0f;
    Vector2 p = {snap(x), snap(y + (size - ps) * 0.5f)};
    if (shadow) DrawTextEx(f, s, (Vector2){p.x + PX * k, p.y + PX * k}, ps, sp, fadec((Color){12, 8, 6, 255}, c.a / 255.0f * 0.85f));
    DrawTextEx(f, s, p, ps, sp, c);
}

static void ui_text(const char *s, float x, float y, float size, Color c) { draw_text_f(G.ui, s, x, y, size, c, true); }
static void ui_center(const char *s, float cx, float y, float size, Color c) { ui_text(s, cx - ui_width(s, size) / 2, y, size, c); }

/* Tinta no pergaminho; bold para nomes e títulos. */
static void ink(const char *s, float x, float y, float size, Color c) { draw_text_f(G.ui, s, x, y, size, c, false); }
/* Na fonte de pixel o destaque vem do tamanho e da cor (negrito borra letras de 1 px). */
static void ink_bold(const char *s, float x, float y, float size, Color c) { draw_text_f(G.uiBold, s, x, y, size, c, false); }
static void ink_center(const char *s, float cx, float y, float size, Color c) { ink(s, cx - ui_width(s, size) / 2, y, size, c); }
static void ink_bold_center(const char *s, float cx, float y, float size, Color c) {
    ink_bold(s, cx - ui_width_f(G.uiBold, s, size) / 2, y, size, c);
}
static void ink_right(const char *s, float rx, float y, float size, Color c) { ink(s, rx - ui_width(s, size), y, size, c); }

/* Tudo em minúsculo: nomes, rótulos e títulos. */
static void to_lower_utf8(char *dst, const char *src, size_t cap) {
    size_t n = 0;
    for (const unsigned char *p = (const unsigned char *)src; *p && n + 2 < cap; p++) {
        unsigned char c = *p;
        if (c >= 'A' && c <= 'Z') dst[n++] = (char)(c + 32);
        else if (c == 0xC3 && p[1] >= 0x80 && p[1] <= 0x9E && p[1] != 0x97) { dst[n++] = (char)c; dst[n++] = (char)(p[1] + 0x20); p++; }
        else dst[n++] = (char)c;
    }
    dst[n] = 0;
}

static const char *lower(const char *s) {
    static char buf[4][256];
    static int slot;
    slot = (slot + 1) % 4;
    to_lower_utf8(buf[slot], s, sizeof buf[slot]);
    return buf[slot];
}

/* Uma linha quebrada do texto cabe em LINHA_MAX bytes (~140 letras de 9 px em 1280 px, mesmo em UTF-8);
 * uma palavra, em PALAVRA_MAX. O buffer de junção guarda a linha, o espaço e a palavra: nada trunca. */
#define LINHA_MAX 512
#define PALAVRA_MAX 256
#define JUNTA_MAX (LINHA_MAX + PALAVRA_MAX + 2)

/* Quebra em linhas e mostra só os primeiros `visible` bytes (máquina de escrever). */
static void ink_wrapped(const char *s, float x, float y, float width, float size, Color c, int visible) {
    char line[LINHA_MAX], word[PALAVRA_MAX], trial[JUNTA_MAX];
    int lineLen = 0, used = 0;
    float ly = y;
    const char *p = s;
    line[0] = 0;
    while (*p) {
        int wl = 0;
        while (p[wl] && p[wl] != ' ') wl++;
        if (wl > PALAVRA_MAX - 1) wl = PALAVRA_MAX - 1;
        memcpy(word, p, (size_t)wl);
        word[wl] = 0;
        snprintf(trial, sizeof trial, "%s%s%s", line, lineLen ? " " : "", word);
        if (lineLen && ui_width(trial, size) > width) {
            int show = visible - used;
            if (show <= 0) return;
            if (show < lineLen) line[show] = 0;
            ink(line, x, ly, size, c);
            used += lineLen + 1;
            ly += px_size(size) + PX;   /* linha de 9 px e 1 px de respiro */
            snprintf(line, sizeof line, "%s", word);
            lineLen = wl;
        } else {
            snprintf(line, sizeof line, "%.*s", (int)sizeof line - 1, trial);
            lineLen = (int)strlen(line);
        }
        p += wl;
        while (*p == ' ') p++;
    }
    int show = visible - used;
    if (show > 0) {
        if (show < lineLen) line[show] = 0;
        ink(line, x, ly, size, c);
    }
}

/* ------------------------------------------------------------------ */
/* Pergaminho                                                          */
/* ------------------------------------------------------------------ */

static float hashf(int x, int y) {
    unsigned h = (unsigned)x * 374761393u + (unsigned)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)((h ^ (h >> 16)) & 0xFFFF) / 65535.0f;
}

/* Retângulo preso na grade de pixels. */
static void px_rect(float x, float y, float w, float h, Color c) {
    float x0 = snap(x), y0 = snap(y), x1 = snap(x + w), y1 = snap(y + h);
    if (x1 > x0 && y1 > y0) DrawRectangleRec((Rectangle){x0, y0, x1 - x0, y1 - y0}, c);
}

/* Janela de papel no jeito RPG Maker, em pixel: cantos cortados, borda de tinta de
 * 1 px, luz em cima e sombra embaixo, filete interno e fibras do papel. */
static void parchment(Rectangle r, float alpha) {
    float x = snap(r.x), y = snap(r.y), w = snap(r.width), h = snap(r.height);
    Color ink = fadec(INK_LINE, alpha), paper = fadec((Color){212, 186, 138, 255}, alpha);
    px_rect(x + PX, y + h, w - PX, PX, fadec((Color){10, 6, 4, 255}, 0.45f * alpha));   /* sombra */
    px_rect(x + w, y + PX, PX, h - PX, fadec((Color){10, 6, 4, 255}, 0.45f * alpha));
    px_rect(x + PX, y, w - 2 * PX, h, ink);
    px_rect(x, y + PX, w, h - 2 * PX, ink);
    px_rect(x + PX, y + PX, w - 2 * PX, h - 2 * PX, paper);
    px_rect(x + 2 * PX, y + PX, w - 4 * PX, PX, fadec((Color){238, 220, 180, 255}, alpha));
    px_rect(x + 2 * PX, y + h - 2 * PX, w - 4 * PX, PX, fadec((Color){170, 140, 94, 255}, alpha));
    Color line = fadec((Color){150, 116, 76, 255}, 0.7f * alpha);            /* filete interno */
    px_rect(x + 3 * PX, y + 3 * PX, w - 6 * PX, PX, line);
    px_rect(x + 3 * PX, y + h - 4 * PX, w - 6 * PX, PX, line);
    px_rect(x + 3 * PX, y + 4 * PX, PX, h - 8 * PX, line);
    px_rect(x + w - 4 * PX, y + 4 * PX, PX, h - 8 * PX, line);
    /* fibras: pixels um pouco mais escuros, sempre nos mesmos lugares do papel */
    Color fiber = fadec((Color){190, 162, 116, 255}, alpha);
    for (float py = y + 5 * PX; py < y + h - 5 * PX; py += PX)
        for (float px = x + 5 * PX; px < x + w - 5 * PX; px += PX)
            if (hashf((int)(px / PX) * 7 + (int)(w / PX), (int)(py / PX) * 13 + (int)(h / PX)) < 0.05f) px_rect(px, py, PX, PX, fiber);
}

/* Bastões de madeira nas pontas, como um rolo aberto. */
static void scroll_rods(Rectangle r, float alpha) {
    for (int side = 0; side < 2; side++) {
        float x = snap(side ? r.x + r.width - 2 * PX : r.x - 2 * PX), y = snap(r.y - 2 * PX), h = snap(r.height + 4 * PX);
        px_rect(x, y, 4 * PX, h, fadec((Color){60, 36, 20, 255}, alpha));
        px_rect(x + PX, y, PX, h, fadec((Color){150, 100, 58, 255}, alpha));
        px_rect(x + 2 * PX, y, PX, h, fadec((Color){110, 70, 38, 255}, alpha));
        for (int e = 0; e < 2; e++) {
            float ky = e ? y + h - PX : y - 2 * PX;
            px_rect(x - PX, ky, 6 * PX, 3 * PX, fadec((Color){150, 44, 32, 255}, alpha));
            px_rect(x, ky, 2 * PX, PX, fadec((Color){206, 90, 60, 255}, alpha));
        }
    }
}

/* Cursor de seleção: faixa escura translúcida e a seta à esquerda. */
static void ui_cursor(Rectangle r, float alpha) {
    float pulse = 0.6f + 0.4f * sinf(G.time * 5);
    px_rect(r.x, r.y, r.width, r.height, fadec((Color){90, 56, 30, 255}, 0.22f * pulse * alpha));
    float cy = snap(r.y + r.height / 2), cx = snap(r.x + 12 + (sinf(G.time * 6) > 0 ? PX : 0));
    for (int k = 0; k < 4; k++) px_rect(cx + k * PX, cy - (3 - k) * PX, PX, (7 - 2 * k) * PX, fadec(SEAL_RED, alpha));
}

/* Gauge no jeito RPG Maker: trilho escuro, preenchimento em degradê, rastro claro. */
static void ui_gauge(float x, float y, float w, float value, float ghost, float max, Color a, Color b) {
    float h = 3 * PX, k = clampf(value / max, 0, 1), g = clampf(ghost / max, 0, 1);
    x = snap(x); y = snap(y); w = snap(w);
    px_rect(x - PX, y - PX, w + 2 * PX, h + 2 * PX, INK_LINE);
    px_rect(x, y, w, h, (Color){46, 34, 26, 255});
    px_rect(x, y, w * g, h, (Color){246, 232, 196, 170});
    float fill = snap(w * k);
    if (fill > 0) {
        DrawRectangleGradientH((int)x, (int)y, (int)fill, (int)h, a, b);
        px_rect(x, y, fill, PX, (Color){255, 245, 220, 90});
    }
}

/* Linha de menu dentro de uma janela de pergaminho. */
static void ui_menu_row(const char *label, float cx, float y, bool selected, Color c, float alpha) {
    float w = 360;
    if (selected) ui_cursor((Rectangle){cx - w / 2, y - 8, w, 48}, alpha);
    ink_center(label, cx, y, 30, fadec(selected ? c : INK_SOFT, alpha));
}

static Vector2 mouse_ui(void) {
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float scale = fminf(sw / RW, sh / RH);
    if (scale >= 1) scale = floorf(scale);
    float ox = floorf((sw - RW * scale) / 2), oy = floorf((sh - RH * scale) / 2);
    Vector2 m = GetMousePosition();
    float u = RW * scale / UI_W;
    return (Vector2){(m.x - ox) / u, (m.y - oy) / u};
}

/* ------------------------------------------------------------------ */
/* Progresso salvo                                                     */
/* ------------------------------------------------------------------ */

#define SAVE_FILE "apara_save.txt"

/* Testes do jogo real (APARA_AUTO): uma linha por marco, para o script saber onde o jogo está. */
static void marco_de_teste(const char *nome) {
    if (G.autoJogo) fprintf(stderr, "TESTE_MARCO %s t=%.3f\n", nome, G.time);
    if (G.autoJogo && !strcmp(nome, "vitoria_salva") && getenv("APARA_VITORIA_LENTA")) G.lento = true;
}

/* Uma faixa no canto avisa o jogador (e o terminal), em vez de perder o progresso calado. */
#define AVISO_SAVE_SEGUNDOS 12.0f

static void avisa_save(const char *texto) {
    fprintf(stderr, "apara: %s\n", texto);
    snprintf(G.avisoSave, sizeof G.avisoSave, "%s", texto);
    G.avisoSaveAte = G.time + AVISO_SAVE_SEGUNDOS;
}

static void save_game(void) {
    if (G.demo || G.teste) return;
    char erro[128], texto[192];
    if (save_gravar(SAVE_FILE, &G.camp, erro, sizeof erro)) return;
    snprintf(texto, sizeof texto, "não consegui gravar o progresso (%s): %s", SAVE_FILE, erro);
    avisa_save(texto);
    marco_de_teste("save_falhou");
}

/* Opções que não são progresso: a calibração de latência. */
#define OPTIONS_FILE "apara_opcoes.txt"

static void save_options(void) {
    if (G.demo) return;
    FILE *f = fopen(OPTIONS_FILE, "w");
    if (!f) return;
    fprintf(f, "atraso_video_ms %d\natraso_audio_ms %d\n", (int)lroundf(G.latVideo * 1000), (int)lroundf(G.latAudio * 1000));
    fclose(f);
}

static void load_options(void) {
    FILE *f = fopen(OPTIONS_FILE, "r");
    if (!f) return;
    int v = 0, a = 0;
    if (fscanf(f, "atraso_video_ms %d atraso_audio_ms %d", &v, &a) == 2) {
        G.latVideo = clampf(v / 1000.0f, 0, AJ_LATENCIA_MAX);
        G.latAudio = clampf(a / 1000.0f, 0, AJ_LATENCIA_MAX);
    }
    fclose(f);
}

static bool load_game(void) {
    Campaign lida;
    char aviso[192];
    switch (save_ler(SAVE_FILE, &lida, aviso, sizeof aviso)) {
        case SAVE_OK:
            G.camp.index = lida.index;
            G.camp.clearedMask = lida.clearedMask;
            G.camp.completed = lida.completed;
            G.camp.loreSeen = lida.loreSeen;
            return true;
        case SAVE_NAO_EXISTE:
            return false;
        case SAVE_CORROMPIDO:
            avisa_save(aviso);
            marco_de_teste("save_corrompido");
            return false;
        default:
            avisa_save(aviso);
            marco_de_teste("save_ilegivel");
            return false;
    }
}

/* O mestre atual caiu: a vitória vale desde o golpe final (marca, avança a trilha e salva), sem
 * esperar o clique na tela de vitória nem o fim da cena. Pode ser chamada de novo pela mesma
 * vitória (campaign_win não avança duas vezes). */
static void registra_vitoria(void) {
    campaign_win(&G.camp, G.m->id - 1);
    save_game();
    marco_de_teste("vitoria_salva");
}

/* ------------------------------------------------------------------ */
/* Troca de tela                                                       */
/* ------------------------------------------------------------------ */

static void set_state(State s) {
    char marco[24];
    snprintf(marco, sizeof marco, "estado_%d", (int)s);
    marco_de_teste(marco);
    G.state = s;
    G.stateTime = 0;
}

/* ------------------------------------------------------------------ */
/* Lutadores em pixel art                                              */
/* ------------------------------------------------------------------ */

static const SprAnim *fa(const Fighter *f, const char *name) { return spr_anim(f->set, name); }

/* O aprendiz de quem oboro devorou este golpe: "eco da terra" é a "postura da terra". -1 se não é eco. */
static int echo_of(const Move *mv) {
    if (!mv || strncmp(mv->name, "eco ", 4)) return -1;
    for (int i = 0; i < MASTER_COUNT; i++)
        if (!strcmp(mv->name + 4, roster_get(i)->style + 8)) return i;
    return -1;
}

static Color posture_color(int echo) {
    static const Color colors[MASTER_COUNT] = {
        {235,165,75,255}, {110,220,140,255}, {245,85,65,255}, {100,205,255,255},
        {255,225,135,255}, {155,110,240,255}, {100,245,180,255}, {255,125,35,255},
        {65,145,255,255}, {100,205,255,255}, {165,85,250,255}, {205,180,255,255}
    };
    return echo >= 0 && echo < MASTER_COUNT ? colors[echo] : (Color){235,60,70,255};
}

static int anim_contact(const SprAnim *a) {
    if (!a) return 0;
    return a->contact >= 0 ? a->contact : a->frames / 2;
}
static int anim_hold(const SprAnim *a) {
    if (!a) return 0;
    int c = anim_contact(a);
    int h = a->hold >= 0 ? a->hold : c - 1;
    return h < 0 ? 0 : (h > c ? c : h);
}

/* Guarda parada: IDLE (a da fúria, para oboro depois do grito), PARADO, ou o
 * primeiro quadro do corte, que é a guarda de quem não tem prancha parada. */
static void fighter_idle(Fighter *f) {
    if (!f->set) return;
    const SprAnim *a = f->furia ? fa(f, "IDLE_FURIA") : NULL;
    if (!a) a = fa(f, "IDLE");
    if (a) spr_loop(&f->pl, a);
    else {
        a = fa(f, "PARADO");
        if (!a) a = fa(f, "ATTACK_1");
        spr_play(&f->pl, a, 0, 0, 1);
    }
    f->qn = 0;
    f->fresh = false;
    f->idle = true;
    f->autoIdle = false;
    f->strike = NULL;
}

/* Nova sequência de trechos: o primeiro toca já, os outros entram em fila. */
static void f_clear(Fighter *f, bool autoIdle) {
    f->qn = 0;
    f->fresh = true;
    f->idle = false;
    f->autoIdle = autoIdle;
}

static void f_seg(Fighter *f, FSeg sg) {
    if (!f->set || !sg.a) return;
    if (f->fresh) {
        if (sg.cycle) spr_cycle(&f->pl, sg.a, sg.dur);
        else spr_play(&f->pl, sg.a, sg.from, sg.to, sg.dur);
        f->fresh = false;
    } else if (f->qn < 4) {
        f->q[f->qn++] = sg;
    }
}

static void f_add(Fighter *f, const SprAnim *a, int from, int to, float dur) { f_seg(f, (FSeg){a, from, to, dur, false}); }
/* A animação em laço (a corrida) por `dur` segundos. */
static void f_add_cycle(Fighter *f, const SprAnim *a, float dur) { f_seg(f, (FSeg){a, 0, 0, dur, true}); }

static void f_update(Fighter *f, float dt) {
    if (!f->set) return;
    spr_update(&f->pl, dt);
    while (!f->idle && spr_done(&f->pl)) {
        float end = f->pl.loop ? f->pl.limit : f->pl.dur;
        float carry = fmaxf(0, f->pl.t - end);
        if (f->qn > 0) {
            FSeg sg = f->q[0];
            memmove(f->q, f->q + 1, sizeof(FSeg) * (size_t)(f->qn - 1));
            f->qn--;
            if (sg.cycle) spr_cycle(&f->pl, sg.a, sg.dur);
            else spr_play(&f->pl, sg.a, sg.from, sg.to, sg.dur);
            spr_update(&f->pl, carry);
        } else {
            if (f->autoIdle) { fighter_idle(f); spr_update(&f->pl, carry); }
            break;
        }
    }
}

static void fighter_load(Fighter *f, const char *id) {
    memset(f, 0, sizeof *f);
    f->set = spr_get(id);
    /* sem golpe desenhado não dá para lutar: fica o boneco */
    if (f->set && !spr_anim(f->set, "ATTACK_1")) f->set = NULL;
    fighter_idle(f);
}

/* Kojiro fora da luta (a conversa antes do duelo, o mestre falando): a katana na
 * bainha, na cintura. Sem a tira EMBAINHADO, o parado de sempre. */
static void ren_sheathed(void) {
    Fighter *f = &G.renS;
    const SprAnim *a = fa(f, "EMBAINHADO");
    if (!a) return;
    spr_loop(&f->pl, a);
    f->qn = 0;
    f->fresh = false;
    f->idle = true;
}

/* No começo de cada luta, Kojiro saca: a tira DESEMBAINHAR termina no quadro 0 do
 * IDLE, a guarda em que ele fica depois (autoIdle). Cada quadro tem o seu tempo
 * (`tempos` no sprite.txt: o preparo devagar, o saque num quadro curto, a pausa no
 * brilho); ao todo, menos que a pausa antes do primeiro golpe do mestre
 * (firstWindupDelay). Os sons saem em ren_draw_sounds, no quadro certo. */
static void ren_draw_sword(void) {
    Fighter *f = &G.renS;
    const SprAnim *a = fa(f, "DESEMBAINHAR");
    if (!a) return;
    f_clear(f, true);
    f_add(f, a, 0, a->frames - 1, 0);
}

/* Os sons do saque, no quadro em que acontecem: o clique da tsuba quando o polegar a
 * empurra (quadro 1) e o shing quando a lâmina sai da bainha (quadro 2). */
static void ren_draw_sounds(void) {
    static int last = -1;
    const SprAnim *a = fa(&G.renS, "DESEMBAINHAR");
    if (!a || G.renS.pl.anim != a) {
        last = -1;
        return;
    }
    int k = G.renS.pl.frame;
    if (k == last) return;
    if (k == 1) audio_play(SND_KOIGUCHI, 0.7f, 1);
    if (k == 2) audio_play(SND_SAQUE, 0.75f, 1);
    last = k;
}

/* Efeitos das folhas do pack (assets/sprites/_fx): tocam uma vez. Os de energia
 * (brilho, raios, fogo) vão atrás dos lutadores e somam luz; poeira e sangue vão
 * na frente. Sem a folha, ficam só as partículas. */
enum { VFX_FRONT = 0, VFX_BACK = 1, VFX_GLOW = 2, VFX_SWORD = 4, VFX_BODY = 8, VFX_OFFHAND = 16 };

static void boss_blade(Vector2 *butt, Vector2 *tip);

static Vector2 vfx_origin(bool sword, bool body) {
    if (sword) { Vector2 h, t; boss_blade(&h, &t); return t; }
    if (body) return (Vector2){G.boss.x + G.boss.offsetX, G.boss.y - G.boss.hopY};
    return (Vector2){0, 0};
}

static Vector2 vfx_position(int i) {
    Vector2 o = vfx_origin(G.vfx[i].sword || G.vfx[i].offhand, G.vfx[i].body);
    if (G.vfx[i].offhand) spr_offhand_point(&G.bossS.pl,
        (Vector2){G.boss.x + G.boss.offsetX, G.boss.y - G.boss.hopY},
        G.boss.faceLeft, (int)G.bossS.squat, &o);
    return (Vector2){G.vfx[i].pos.x + o.x, G.vfx[i].pos.y + o.y};
}

static void vfx(const char *name, int row, Vector2 pos, bool flip, int flags, float fps) {
    const SprFx *f = spr_fx(name);
    if (!f) return;
    int slot = 0;
    for (int i = 0; i < VFX_MAX; i++) {
        if (!G.vfx[i].fx) { slot = i; break; }
        if (G.vfx[i].t > G.vfx[slot].t) slot = i;   /* sem vaga: troca o mais antigo */
    }
    G.vfx[slot].fx = f;
    G.vfx[slot].row = row;
    G.vfx[slot].sword = (flags & VFX_SWORD) != 0;
    G.vfx[slot].body = (flags & VFX_BODY) != 0;
    G.vfx[slot].offhand = (flags & VFX_OFFHAND) != 0;
    Vector2 origin = vfx_origin(G.vfx[slot].sword || G.vfx[slot].offhand, G.vfx[slot].body);
    if (G.vfx[slot].offhand) spr_offhand_point(&G.bossS.pl,
        (Vector2){G.boss.x + G.boss.offsetX, G.boss.y - G.boss.hopY},
        G.boss.faceLeft, (int)G.bossS.squat, &origin);
    G.vfx[slot].pos = (Vector2){pos.x - origin.x, pos.y - origin.y};
    G.vfx[slot].t = 0;
    G.vfx[slot].fps = fps;
    G.vfx[slot].flip = flip;
    G.vfx[slot].back = flags & VFX_BACK;
    G.vfx[slot].glow = flags & VFX_GLOW;
    G.vfx[slot].scale = 1;
    G.vfx[slot].tint = WHITE;
}

/* Poeira dos pés (o bote, a queda, o passo): a folha de fumaça do pack, que é branca e
 * grande, sai com pouco mais da metade do tamanho, na cor do chão do cenário e um
 * pouco transparente, com a base no chão. */
static void dust(float x, bool flip, float fps) {
    const SprFx *f = spr_fx("70");
    if (!f) return;
    float scale = 0.55f;
    vfx("70", 4, (Vector2){x, GROUND_LOW - 8 + f->cell * (1 - scale) * 0.22f}, flip, VFX_FRONT, fps);
    for (int i = 0; i < VFX_MAX; i++)
        if (G.vfx[i].fx == f && G.vfx[i].t == 0) {
            Color c = G.m ? arena_dust(G.m->arena) : WHITE;
            c.a = 200;
            G.vfx[i].scale = scale;
            G.vfx[i].tint = c;
        }
}

static void vfx_update(float dt) {
    for (int i = 0; i < VFX_MAX; i++) {
        if (!G.vfx[i].fx) continue;
        G.vfx[i].t += dt;
        if ((int)(G.vfx[i].t * G.vfx[i].fps) >= G.vfx[i].fx->frames) G.vfx[i].fx = NULL;
    }
}

static void vfx_draw(bool back) {
    for (int i = 0; i < VFX_MAX; i++) {
        if (!G.vfx[i].fx || G.vfx[i].back != back) continue;
        if (G.vfx[i].glow) BeginBlendMode(BLEND_ADDITIVE);
        spr_fx_draw_scaled(G.vfx[i].fx, G.vfx[i].row, (int)(G.vfx[i].t * G.vfx[i].fps), vfx_position(i), G.vfx[i].flip,
                           G.vfx[i].tint, G.vfx[i].scale);
        if (G.vfx[i].glow) EndBlendMode();
    }
}

static void vfx_clear(void) { memset(G.vfx, 0, sizeof G.vfx); }

/* Onde a lâmina de kojiro espera o golpe, a partir dos pés dele. */
static int ren_guard_x(void) { return G.renS.set && G.renS.set->hasGuard ? G.renS.set->guardX : 12; }

static void setup_actors(void) {
    int idx = G.camp.index < ROSTER_SIZE ? G.camp.index : ROSTER_SIZE - 1;
    Look rl = REN_LOOK, bl = MASTER_LOOKS[idx];
    rl.size *= ACTOR_SCALE;
    bl.size *= ACTOR_SCALE;
    G.bossHome = BOSS_X + (bl.size - ACTOR_SCALE) * 30;
    rig_init(&G.ren, &rl, REN_X, GROUND_LOW, false);
    rig_init(&G.boss, &bl, G.bossHome, GROUND_LOW, true);
    G.boss.time = 1.3f; /* respiração fora de fase com a de Ren */
    G.ren.hideBlade = G.boss.hideBlade = katana3d_ready();
    G.bossWinding = false;
    G.renParryTime = -1;
    G.renKnock = G.bossKnock = 0;
    G.staggerTime = 0;
    memset(&G.sword, 0, sizeof G.sword);
    fighter_load(&G.renS, "kojiro");
    ren_sheathed();
    fighter_load(&G.bossS, G.m ? G.m->name : "");
    G.bossStep = G.bossStepTo = 0;
    G.bossStepSpeed = 0;
    G.leap = LEAP_NONE;
    G.bossHidden = false;
    G.bossDissolve = 0;
    memset(G.raios, 0, sizeof G.raios);
    G.paralisia = G.paralisiaTotal = G.paralisiaForca = G.paralisiaFaisca = 0;
    G.hopT = G.hopLen = 0;
    G.hopTravel = false;
    memset(G.after, 0, sizeof G.after);
    G.gritoPending = false;
    G.auraLeft = G.auraTick = 0;
}

static void start_master(int index) {
    G.camp.index = index;
    G.m = roster_get(index);
    G.defeatsHere = 0;
    setup_actors();
    fx_clear(&G.fx);
    vfx_clear();
    memset(&G.ctx, 0, sizeof G.ctx);
    audio_music(G.m->arena);
    audio_music_intensity(0);
}

static void start_lines(const Line *lines, int count, State s) {
    G.lines = lines;
    G.lineCount = count;
    G.lineIndex = 0;
    G.typeChars = 0;
    set_state(s);
}

static void teste_selo(int selo);
static void desenha_rastro_do_golpe(bool escuro, bool fio);
static Color cor_rastro(void);

static void start_duel(void) {
    settings_default(&G.settings);
    settings_for_level(&G.settings, campaign_defeated(&G.camp));
    G.settings.latency = G.latVideo;                  /* calibração: ver update_calibra */
    G.settings.audioLead = G.latAudio - G.latVideo;
    G.special = false;
    duel_init(&G.duel, &G.settings, G.m, getenv("APARA_SEMENTE") ? (uint32_t)atoi(getenv("APARA_SEMENTE")) : (uint32_t)time(NULL) ^ (uint32_t)(G.camp.index * 7919));
    robo_iniciar(&G.robo, &G.roboEscolhido, 1);
    setup_actors();
    ren_draw_sword();
    fx_clear(&G.fx);
    vfx_clear();
    G.shownRen = G.ghostRen = G.settings.renPosture;
    G.shownBoss = G.ghostBoss = duel_posture_max(&G.duel);
    G.hitstop = 0;
    G.slowmo = 1;
    G.slowmoTime = 0;
    G.blackoutTarget = 0;
    G.ctx.blackout = 0;
    G.ctx.seal = 0;
    G.lightningTimer = 3;
    memset(G.sealTold, 0, sizeof G.sealTold);
    G.masked = G.maskOnGround = G.windOnly = false;
    G.hz.on = false;
    audio_music_intensity(0);
    banner(G.m->isBigBoss ? duel_stance(&G.duel)->name : G.m->style, PAPER);
    G.apertos = 0;
    if (G.startSeal > 0 && G.m->sealCount > 1) teste_selo(G.startSeal);
    G.startSeal = 0;
    set_state(ST_DUEL);
}

/* ------------------------------------------------------------------ */
/* Desarme: a espada do mestre voa, gira e crava no chão               */
/* ------------------------------------------------------------------ */

static void boss_blade(Vector2 *butt, Vector2 *tip);

static void start_disarm(void) {
    Rig *b = &G.boss, *r = &G.ren;
    FlySword *s = &G.sword;
    Vector2 h, t;
    boss_blade(&h, &t);
    s->active = true;
    s->stuck = false;
    s->len = sqrtf((t.x - h.x) * (t.x - h.x) + (t.y - h.y) * (t.y - h.y));
    s->pos = (Vector2){(h.x + t.x) / 2, (h.y + t.y) / 2};
    s->angle = fmodf(atan2f(t.y - h.y, t.x - h.x) * RAD2DEG + 360, 360);
    s->vel = (Vector2){58, -175}; /* para cima e para trás do mestre */
    s->target = 100;               /* ponta para baixo, levemente inclinada */
    s->landY = GROUND_LOW + 3 - sinf(s->target * DEG2RAD) * s->len / 2;
    float dy = s->landY - s->pos.y;
    s->flight = (-s->vel.y + sqrtf(s->vel.y * s->vel.y + 2 * SWORD_GRAVITY * dy)) / SWORD_GRAVITY;
    s->spin = (s->target + 720 - s->angle) / s->flight; /* duas voltas e meia até cravar */
    s->t = 0;
    s->blade = b->look.blade;
    s->style = katana_style(&b->look);
    b->noSword = true;
    b->trail = false;

    rig_pose(b, POSE_DISARMED, 0.12f, EASE_OUT);
    rig_then(b, POSE_KNEEL, 0.9f, EASE_INOUT);
    if (G.bossS.set) {
        /* sem a arma, de joelhos (ou curvado, quem não cai) */
        const SprAnim *d = fa(&G.bossS, "DESARMADO");
        f_clear(&G.bossS, false);
        if (d) f_add(&G.bossS, d, 0, d->frames - 1, 0.7f);
        else fighter_idle(&G.bossS);
    }
    b->breath = 0.4f;
    rig_pose(r, POSE_DEFLECT, 0.05f, EASE_OUT);
    G.renStepFrom = r->offsetX;

    audio_play(SND_SWING, 1, 1.4f);
    G.bannerTime = 0;   /* a faixa da postura sai: o "desarmado" fica sozinho */
    fx_popup(&G.fx, "desarmado", (Vector2){160, 56}, 1.2f, PAPER);
    G.duo = 0.3f;
    G.slash = 0.35f;
    G.silence = AJ_SILENCIO_DESARME;
    G.slowmo = AJ_LENTA_VITORIA;
    G.slowmoTime = AJ_LENTA_VITORIA_TEMPO;
    set_state(ST_FINISHER);
}

static void update_sword(float dt) {
    FlySword *s = &G.sword;
    if (!s->active) return;
    if (s->stuck) { s->stuckTime += dt; return; }
    s->t += dt;
    s->vel.y += SWORD_GRAVITY * dt;
    s->pos.x += s->vel.x * dt;
    s->pos.y += s->vel.y * dt;
    s->angle += s->spin * dt;
    if (s->t >= s->flight) {
        s->stuck = true;
        s->stuckTime = 0;
        s->pos.y = s->landY;
        s->angle = s->target;
        Vector2 tip = {s->pos.x + cosf(s->target * DEG2RAD) * s->len / 2, GROUND_LOW};
        fx_burst(&G.fx, P_DUST, tip, 10, 50, 0.9f, -1.57f, (Color){210, 190, 160, 170}, (Color){140, 120, 100, 120});
        dust(tip.x, false, 20);
        fx_burst(&G.fx, P_SPARK, tip, 6, 70, 0.8f, -1.57f, (Color){255, 240, 200, 255}, (Color){255, 190, 90, 255});
        fx_kick(&G.fx, AJ_TREMOR_ESPADA_CRAVA, AJ_TREMOR_ESPADA_TEMPO);
        audio_play(SND_THUD, 0.7f, 1);
    }
}

/* Posição da arma voando: centro, direção e as duas pontas. */
static void fly_ends(const FlySword *s, Vector2 *butt, Vector2 *tip) {
    float wobble = s->stuck ? sinf(s->stuckTime * 38) * expf(-s->stuckTime * 5) * 7 : 0;
    float a = (s->angle + wobble) * DEG2RAD;
    Vector2 dir = {cosf(a), sinf(a)};
    Vector2 c = s->pos;
    if (s->stuck) {
        Vector2 t = {s->pos.x + cosf(s->target * DEG2RAD) * s->len / 2, s->pos.y + sinf(s->target * DEG2RAD) * s->len / 2};
        c = (Vector2){t.x - dir.x * s->len / 2, t.y - dir.y * s->len / 2};
    }
    *butt = (Vector2){c.x - dir.x * s->len / 2, c.y - dir.y * s->len / 2};
    *tip = (Vector2){c.x + dir.x * s->len / 2, c.y + dir.y * s->len / 2};
}

/* Lança e cajado voando: haste desenhada em 2D. */
static void draw_pole_flying(void) {
    FlySword *s = &G.sword;
    if (!s->active || G.boss.look.weapon == WEAPON_KATANA) return;
    Vector2 b, t;
    fly_ends(s, &b, &t);
    bool spear = G.boss.look.weapon == WEAPON_SPEAR;
    DrawLineEx(b, t, 1.8f, spear ? (Color){116, 80, 48, 255} : (Color){70, 72, 80, 255});
    if (spear) DrawCircleV(t, 1.8f, G.boss.look.blade);
}

/* A espada do desarme em 3D: gira no ar e no próprio eixo, e crava pela ponta. */
static void draw_sword_3d(Color light) {
    FlySword *s = &G.sword;
    if (!s->active || G.boss.look.weapon != WEAPON_KATANA) return;
    float wobble = s->stuck ? sinf(s->stuckTime * 38) * expf(-s->stuckTime * 5) * 7 : 0;
    float a = (s->angle + wobble) * DEG2RAD;
    Vector2 dir = {cosf(a), sinf(a)};
    Vector2 c = s->pos;
    if (s->stuck) {
        Vector2 tip = {s->pos.x + cosf(s->target * DEG2RAD) * s->len / 2, s->pos.y + sinf(s->target * DEG2RAD) * s->len / 2};
        c = (Vector2){tip.x - dir.x * s->len / 2, tip.y - dir.y * s->len / 2};
    }
    Vector2 butt = {c.x - dir.x * s->len / 2, c.y - dir.y * s->len / 2};
    Vector2 tip = {c.x + dir.x * s->len / 2, c.y + dir.y * s->len / 2};
    float roll = s->stuck ? 0 : s->t * 900;
    katana3d_draw(butt, tip, roll, &s->style, light);
}

static void draw_sword(void) {
    FlySword *s = &G.sword;
    if (!s->active) return;
    float wobble = s->stuck ? sinf(s->stuckTime * 38) * expf(-s->stuckTime * 5) * 7 : 0;
    int ghosts = s->stuck ? 0 : 3;
    for (int g = ghosts; g >= 0; g--) {
        float a = (s->angle - s->spin * 0.012f * g + wobble) * DEG2RAD;
        /* Cravada, gira em volta da ponta; no ar, em volta do centro. */
        Vector2 dir = {cosf(a), sinf(a)};
        Vector2 c = s->pos;
        if (s->stuck) {
            Vector2 tip = {s->pos.x + cosf(s->target * DEG2RAD) * s->len / 2, s->pos.y + sinf(s->target * DEG2RAD) * s->len / 2};
            c = (Vector2){tip.x - dir.x * s->len / 2, tip.y - dir.y * s->len / 2};
        }
        Vector2 hilt = {roundf(c.x - dir.x * s->len / 2), roundf(c.y - dir.y * s->len / 2)};
        Vector2 guard = {roundf(hilt.x + dir.x * s->len * 0.2f), roundf(hilt.y + dir.y * s->len * 0.2f)};
        Vector2 tip = {roundf(c.x + dir.x * s->len / 2), roundf(c.y + dir.y * s->len / 2)};
        float alpha = g == 0 ? 1 : 0.25f / g;
        DrawLineEx(hilt, guard, 2.5f, fadec((Color){50, 30, 30, 255}, alpha));
        DrawLineEx(guard, tip, 1.6f, fadec(s->blade, alpha));
        if (g == 0) DrawCircleV(guard, 1.8f, (Color){170, 140, 70, 255});
    }
}

/* ------------------------------------------------------------------ */
/* Reação aos eventos do duelo                                         */
/* ------------------------------------------------------------------ */

/* Onde as lâminas se encontram: a guarda de kojiro, na altura do golpe do mestre. */
static Vector2 clash_point(void) {
    if (!G.renS.set) return rig_sword_mid(&G.ren);
    const SprAnim *a = G.bossS.strike;
    float y = a && a->hasReach ? GROUND_LOW + a->reachY : GROUND_LOW - 22;
    return (Vector2){G.ren.x + G.ren.offsetX + ren_guard_x(), y};
}

/* Origem dos efeitos: os pixels da lâmina do quadro atual, incluindo salto e recuo. */
static void boss_blade(Vector2 *butt, Vector2 *tip) {
    if (!G.bossS.set) { rig_sword_line(&G.boss, butt, tip); return; }
    float bx = G.boss.x + G.boss.offsetX, h = (float)G.bossS.set->height;
    Vector2 feet = {bx, G.boss.y - G.boss.hopY};
    *butt = (Vector2){bx - 4, feet.y - h * 0.5f + G.bossS.squat};
    if (!spr_weapon_point(&G.bossS.pl, feet, G.boss.faceLeft, G.bossS.squat, tip))
        *tip = (Vector2){bx - 4 - fmaxf(10, G.boss.look.bladeLen * 0.9f), feet.y - h * 0.75f + G.bossS.squat};
}

/* Tipo visual do golpe k da sequência. Algumas armas mantêm sempre a mesma direção. */
static MoveLook strike_look(void) {
    return move_contact_look(duel_move(&G.duel), G.duel.comboStrike);
}

/* Nos bonecos, o golpe forte e o salto usam as poses do golpe alto; a investida, as da estocada. */
static MoveLook rig_look(MoveLook l) { return l == LOOK_DASH || l == LOOK_FAR ? LOOK_THRUST : (l == LOOK_LOW || l == LOOK_THRUST ? l : LOOK_HIGH); }
static Pose windup_pose(MoveLook l) { l = rig_look(l); return l == LOOK_LOW ? POSE_WINDUP_LOW : (l == LOOK_THRUST ? POSE_WINDUP_THRUST : POSE_WINDUP); }
static Pose rearm_pose(MoveLook l) { l = rig_look(l); return l == LOOK_LOW ? POSE_REARM_LOW : (l == LOOK_THRUST ? POSE_WINDUP_THRUST : POSE_REARM_HIGH); }
static Pose contact_pose(MoveLook l) { l = rig_look(l); return l == LOOK_LOW ? POSE_CONTACT_LOW : (l == LOOK_THRUST ? POSE_CONTACT_THRUST : POSE_CONTACT); }
static Pose parry_pose(MoveLook l) { l = rig_look(l); return l == LOOK_LOW ? POSE_PARRY_LOW : (l == LOOK_THRUST ? POSE_PARRY_THRUST : POSE_PARRY); }

/* Golpe do mestre em pixel art. A preparação escolhe a prancha: alto desce
 * (ATTACK_3), baixo sobe (ATTACK_2), estocada é o corte reto ou a investida.
 * O último golpe das sequências longas é o especial; oboro ataca com o eco da
 * postura em que está e, depois do grito, com a fúria. */
static const SprAnim *boss_strike_anim(MoveLook look) {
    const Fighter *f = &G.bossS;
    const SprAnim *a = NULL;
    char name[64];
    if (G.special) {
        a = f->furia ? fa(f, "STRONG_ATTACK_FURIA") : NULL;
        if (!a) a = fa(f, "STRONG_ATTACK");
        if (!a) a = fa(f, "ESPECIAL");
        if (a) return a;
    }
    const Move *mv = duel_move(&G.duel);
    if (look == LOOK_HEAVY) {
        /* golpe forte: o salto com a pancada do pack; sem ele, o especial */
        a = f->furia ? fa(f, "STRONG_ATTACK_FURIA") : NULL;
        if (!a) a = fa(f, "STRONG_ATTACK");
        if (!a) a = fa(f, "ESPECIAL");
        if (a) return a;
        look = LOOK_HIGH;
    }
    /* No terceiro selo, os padrões continuam sendo os ecos, mas todas as ações
     * visuais vêm do pack FLAMING SWORD, inclusive duplos e finais de sequência. */
    if (G.m->isBigBoss && f->furia) {
        MoveLook fireLook = look;
        if (fireLook == LOOK_DASH || fireLook == LOOK_FAR) fireLook = LOOK_THRUST;
        if (fireLook == LOOK_JUMP || fireLook == LOOK_WARP) fireLook = LOOK_HIGH;
        const char *fireBase = duel_strike_dual(&G.duel) ? "ATTACK_3" :
            fireLook == LOOK_LOW ? "ATTACK_2" : fireLook == LOOK_THRUST ? "ATTACK_1" : "ATTACK_3";
        snprintf(name, sizeof name, "%s_FURIA", fireBase);
        if ((a = fa(f, name))) return a;
    }
    /* as duas lâminas de uma vez: o corte cruzado */
    if (duel_strike_dual(&G.duel) && (a = fa(f, "ATTACK_3"))) return a;
    if (mv && mv->strikes >= 3 && G.duel.comboStrike == mv->strikes - 1 && (a = fa(f, "ESPECIAL"))) return a;
    /* a investida e a estocada de longe acabam na estocada; o salto desce com o corte alto */
    if (look == LOOK_DASH || look == LOOK_FAR) look = LOOK_THRUST;
    if (look == LOOK_JUMP || look == LOOK_WARP) look = LOOK_HIGH;
    if (look == LOOK_THRUST && (a = fa(f, "DASH_ATTACK"))) return a;
    const char *base = look == LOOK_LOW ? "ATTACK_2" : (look == LOOK_THRUST ? "ATTACK_1" : "ATTACK_3");
    int echo = G.m->isBigBoss ? echo_of(mv) : -1;
    if (echo >= 0) {
        snprintf(name, sizeof name, "%s_ECO_%s", base, roster_get(echo)->name);
        for (char *u = name; *u; u++)
            if (*u >= 'a' && *u <= 'z') *u = (char)(*u - 32);
        if ((a = fa(f, name))) return a;
    }
    if (f->furia) {
        snprintf(name, sizeof name, "%s_FURIA", base);
        if ((a = fa(f, name))) return a;
    }
    if ((a = fa(f, base))) return a;
    return fa(f, "ATTACK_1");
}

static void boss_hop(float len, float h) {
    G.hopT = 0;
    G.hopLen = len;
    G.hopH = h;
    G.hopTravel = false;
}

static void update_hop(float dt) {
    if (G.hopT < G.hopLen) {
        G.hopT = fminf(G.hopLen, G.hopT + dt);
        float u = G.hopT / G.hopLen;
        G.boss.hopY = 4 * u * (1 - u) * G.hopH;
        if (G.hopTravel) G.bossStep = G.hopFrom + (G.hopTo - G.hopFrom) * smooth(u);
    } else {
        G.boss.hopY = 0;
        G.hopTravel = false;
    }
}

/* Penas escuras, opacas, no caminho do recuo; não encobrem o brilho da lâmina. */
static void feather_blur(void) {
    Vector2 at = {G.boss.x + G.boss.offsetX, GROUND_LOW - 26};
    fx_burst(&G.fx, P_FEATHER, at, 4, 34, 0.6f, 3.14f,
             (Color){22, 18, 31, 225}, (Color){77, 49, 70, 205});
}

/* Penas pretas e vermelhas: o corvo sumindo ou reaparecendo. */
static void feathers(void) {
    Vector2 at = {G.boss.x + G.boss.offsetX, GROUND_LOW - 26};
    fx_burst(&G.fx, P_FEATHER, at, 22, 80, 3.14f, -1.57f, (Color){18, 14, 29, 255}, (Color){65, 42, 62, 245});
    fx_burst(&G.fx, P_FEATHER, at, 6, 60, 3.14f, -1.57f, (Color){120, 22, 40, 240}, (Color){80, 14, 31, 230});
    if (IsWindowReady()) vfx("64", 8, at, true, VFX_BACK, 26);
    audio_play(SND_SWING, 0.45f, 0.7f);
}

static const SprAnim *boss_run(void) {
    const SprAnim *a = G.bossS.furia ? fa(&G.bossS, "RUN_FURIA") : NULL;
    return a ? a : fa(&G.bossS, "RUN");
}

/* Preparação: os quadros até o `hold` e ele parado ali até a lâmina partir.
 * No começo da sequência ele dá um passo curto e rápido para dentro e firma os
 * pés; o bote (o resto do caminho até a ponta da arma encontrar a guarda de
 * kojiro) vem quando a lâmina parte. A investida recua num pulinho e vem
 * correndo; o salto agacha, sobe e desce cortando, e toca o chão no contato. */
static void sprite_windup(void) {
    Fighter *f = &G.bossS;
    G.leap = LEAP_NONE;
    if (!f->set) return;
    MoveLook look = strike_look();
    const SprAnim *a = boss_strike_anim(look);
    int hold = anim_hold(a);
    float w = G.windupSpr;
    bool first = G.duel.comboStrike == 0;
    const SprAnim *previous = f->pl.anim;
    int resume = f->pl.frame + 1;
    bool follow = !first && previous && strstr(previous->name, "ATTACK") && resume < previous->frames;
    f_clear(f, false);
    if (follow) {
        float tail = fminf((previous->frames - resume) * previous->frameTime, w * 0.45f);
        f_add(f, previous, resume, previous->frames - 1, tail);
        w -= tail;
    }
    f->strike = a;
    float reach = a->hasReach ? (float)a->reachX : 36;
    G.bossStrikeStep = clampf(REN_X + ren_guard_x() + reach - G.bossHome, -70, 16);
    G.leapT = 0;
    G.leapStage = 0;
    const SprAnim *run = boss_run(), *jump = fa(f, "JUMP");
    if (first && look == LOOK_DASH && run && w > 0.3f) {
        G.leap = LEAP_DASH;
        G.leapAt = w * 0.3f;
        f_add(f, a, 0, 0, G.leapAt);
        f_add_cycle(f, run, w - G.leapAt);
        /* recua o bastante para sempre haver uns 30 px de corrida, mesmo com a lança */
        G.bossStepTo = fmaxf(G.bossStep, clampf(G.bossStrikeStep + 40, 20, 48));
        G.bossStepSpeed = fabsf(G.bossStepTo - G.bossStep) / (G.leapAt * 0.8f);
        boss_hop(G.leapAt * 0.8f, 5);
        return;
    }
    if (first && look == LOOK_JUMP && w > 0.3f) {
        G.leap = LEAP_JUMP;
        G.leapAt = w * 0.25f;
        G.leapAir = w - G.leapAt + G.settings.attackLead;
        float rise = G.leapAir * 0.5f;
        /* agacha na guarda; no ar, o salto do pack (se tiver) até o alto do arco */
        f_add(f, a, 0, 0, G.leapAt);
        if (jump) f_add(f, jump, 0, jump->frames > 2 ? jump->frames - 2 : jump->frames - 1, rise);
        else f_add(f, a, 0, 0, rise);
        f_add(f, a, 0, hold, fmaxf(0.02f, w - G.leapAt - rise));
        G.bossStepTo = G.bossStep;
        return;
    }
    /* a preparação anda devagar até o hold (cada quadro pelo menos 0,12 s) e segura */
    float antic = fmaxf((hold + 1) * a->frameTime * 1.8f, (hold + 1) * 0.12f);
    if (first && look == LOOK_WARP && w > 0.3f) {
        /* O corvo recua para a direita, vira penas e reaparece na pose de corte.
         * As duas marcas usam o relógio REAL da preparação; a lâmina variável
         * só altera a pose, sem mover o aviso nem o contato do núcleo. */
        G.leap = LEAP_WARP;
        float cue = (float)(duel_cue_time(&G.duel) - (G.duel.strikeAt - G.duel.windupDuration));
        G.leapAt = fmaxf(0.05f, fminf(G.windupLen * 0.38f, cue - 0.12f));
        G.leapAir = fminf(G.windupLen - 0.02f,
                          fmaxf(G.leapAt + 0.07f, cue - KARASU_WARP_ANTES_AVISO));
        G.warpPenasT = 0;
        f_add(f, a, 0, hold, G.leapAt);
        f_add(f, a, hold, hold, w - G.leapAt);
        G.bossStepTo = G.bossStep + KARASU_WARP_RECUO;
        G.bossStepSpeed = KARASU_WARP_RECUO / G.leapAt;
        return;
    }
    if (first && look == LOOK_FAR) {
        /* a lança: ele se afasta, recolhe a ponta e espera; o bote atravessa a distância */
        G.leap = LEAP_FAR;
        G.leapAt = 1e9f;
        f_add(f, a, 0, hold, fminf(w * 0.5f, antic));
        G.bossStepTo = fmaxf(G.bossStep, fminf(G.bossStrikeStep + 36, 48));
        G.bossStepSpeed = fabsf(G.bossStepTo - G.bossStep) / fmaxf(0.1f, w * 0.4f);
        boss_hop(fmaxf(0.1f, w * 0.3f), 3);
        return;
    }
    if (first && duel_move(&G.duel) && duel_move(&G.duel)->feint) {
        /* Um passo falso antes do aviso; recua e só então arma a estocada real.
         * Não há quadro de contato nem mudança no relógio ou na hitbox. */
        float beforeCue = fmaxf(0, w - (duel_aviso(&G.duel) - duel_strike_lead_base(&G.duel)));
        G.leap = LEAP_FEINT;
        G.leapT = 0;
        G.leapAt = beforeCue * 0.35f;
        G.leapAir = beforeCue * 0.75f;
        G.feintFrom = G.bossStep;
        G.bossStepTo = G.bossStep - 8;
        G.bossStepSpeed = 8 / fmaxf(0.05f, G.leapAt);
        f_add(f, a, 0, hold, fminf(w * 0.6f, antic));
        return;
    }
    f_add(f, a, 0, hold, first ? fminf(w * 0.6f, antic) : fminf(w, antic));
    G.bossStepTo = G.bossStep + (G.bossStrikeStep - G.bossStep) * 0.4f;
    float stepTime = first ? fminf(0.2f, w * 0.5f) : w * 0.8f;
    G.bossStepSpeed = fabsf(G.bossStepTo - G.bossStep) / fmaxf(0.06f, stepTime);
    if (first && fabsf(G.bossStepTo - G.bossStep) > 4) boss_hop(stepTime, 2);
}

/* O gesto avança até o contato no tempo de viagem já decidido pelo núcleo.
 * Nenhuma janela ou instante de impacto é alterado. */
static void sprite_launch(void) {
    Fighter *f = &G.bossS;
    if (!f->set || !f->strike) return;
    const SprAnim *a = f->strike;
    int c = anim_contact(a);
    float lead = duel_strike_lead(&G.duel);
    if (G.bossHidden) { G.bossHidden = false; feathers(); }
    if (!(G.leap == LEAP_JUMP && G.hopTravel)) {
        G.bossStepTo = G.bossStrikeStep;
        G.bossStepSpeed = fabsf(G.bossStrikeStep - G.bossStep) / fmaxf(0.05f, lead - 0.02f);
    }
    /* Repassar a antecipação evita congelar os packs cujo hold é c - 1. */
    if (c > 0) {
        f_clear(f, false);
        f_add(f, a, 0, c - 1, lead);
    }
}

/* O gesto de kojiro: a guarda (DEFEND) ou um corte rápido de encontro ao golpe. */
static void sprite_press(void) {
    G.paralisia = 0;          /* meio paralisado: o aperto sempre vale, e com ele o choque larga */
    Fighter *f = &G.renS;
    if (!f->set) return;
    const SprAnim *a = fa(f, "DEFEND");
    f_clear(f, true);
    if (a) {
        int c = anim_contact(a);
        f_add(f, a, 0, c, 0.05f);
        f_add(f, a, c, c, 0.25f);
        if (c + 1 < a->frames) f_add(f, a, c + 1, a->frames - 1, 0);
    } else {
        MoveLook l = G.duel.phase == PH_WINDUP ? strike_look() : LOOK_HIGH;
        /* corte reto contra o alto e a estocada (termina de volta na guarda); para baixo contra o baixo */
        a = fa(f, l == LOOK_LOW ? "ATTACK_3" : "ATTACK_1");
        if (!a) a = fa(f, "ATTACK_1");
        int h = anim_hold(a), c = anim_contact(a);
        f_add(f, a, h, c, 0.06f);
        if (c + 1 < a->frames) f_add(f, a, c + 1, a->frames - 1, (a->frames - 1 - c) * 0.07f);
    }
    f->strike = a;
}

/* Choque: os dois param no quadro de contato (o hitstop segura) e seguem. */
static void sprite_impact(const DuelEvent *e) {
    Fighter *b = &G.bossS, *r = &G.renS;
    if (b->set) {
        const SprAnim *a = b->strike ? b->strike : fa(b, "ATTACK_1");
        const SprAnim *hurt = b->furia ? fa(b, "HURT_FURIA") : NULL;
        if (!hurt) hurt = fa(b, "HURT");
        int c = anim_contact(a);
        f_clear(b, true);
        f_add(b, a, c, c, a->frameTime);
        if (e->flag) {
            /* postura quebrada: cambaleia e fica curvado até se recompor */
            if (hurt) f_add(b, hurt, 0, hurt->frames - 1, 0);
            else if (c + 1 < a->frames) f_add(b, a, c + 1, a->frames - 1, 0);
            b->autoIdle = false;

        } else if (c + 1 < a->frames) {
            f_add(b, a, c + 1, a->frames - 1, 0);
        }
        b->strike = NULL;
        if (G.leap == LEAP_JUMP) {   /* a poeira da queda, e os joelhos dobram */
            dust(G.boss.x + G.boss.offsetX, true, 24);
            G.landT = 0.2f;
        }
        /* entre os golpes de uma sequência ele fica onde está; no fim, volta */
        G.bossStepTo = G.duel.comboRemaining > 0 ? G.bossStep : 0;
        G.bossStepSpeed = 40;
        G.hopT = G.hopLen;
        G.hopTravel = false;
        G.leap = LEAP_NONE;
        G.bossHidden = false;
    }
    if (r->set) {
        if ((e->judgement == J_PERFEITO || e->judgement == J_BOM) && !(e->i & 2)) {
            const SprAnim *a = r->strike ? r->strike : fa(r, "ATTACK_1");
            int c = anim_contact(a);
            f_clear(r, true);
            f_add(r, a, c, c, 0.08f);
            if (c + 1 < a->frames) f_add(r, a, c + 1, a->frames - 1, 0);
        } else {
            const SprAnim *hurt = fa(r, "HURT");
            if (hurt) {
                f_clear(r, true);
                f_add(r, hurt, 0, hurt->frames - 1, 0);
            } else {
                fighter_idle(r);   /* o clarão vermelho e o recuo contam o golpe */
            }
        }
        r->strike = NULL;
    }
}

/* Kojiro cai: a DEATH do pack, ou agachado com a espada (Samurai #3). */
static void sprite_fall(void) {
    Fighter *r = &G.renS;
    if (!r->set) return;
    const SprAnim *d = fa(r, "DEATH");
    f_clear(r, false);
    if (d) f_add(r, d, 0, d->stop >= 0 ? d->stop : d->frames - 1, 0);
    else if ((d = fa(r, "DASH_ATTACK"))) f_add(r, d, 0, 0, 0.1f);
}

static void second_blade(void);

static void on_impact(const DuelEvent *e) {
    Vector2 at = clash_point();
    Rig *r = &G.ren, *b = &G.boss;
    /* o núcleo diz quanto congelar (e desconta isso do próximo golpe da sequência) */
    G.hitstop = G.duel.lastHitstop;
    b->trail = false;
    switch (e->judgement) {
        case J_PERFEITO:
            audio_play(SND_PERFECT, 1, 1 + (rand() % 5) * 0.02f);
            fx_burst(&G.fx, P_SPARK, at, 4, 55, 1.2f, -0.5f, (Color){220, 205, 175, 255}, (Color){155, 135, 105, 255});
            fx_kick(&G.fx, AJ_TREMOR_PERFEITO, AJ_TREMOR_PERFEITO_TEMPO);
            rig_pose(r, POSE_DEFLECT, 0.05f, EASE_OUT);
            rig_then(r, POSE_IDLE, 0.4f, EASE_INOUT);
            rig_pose(b, POSE_HURT, 0.07f, EASE_OUT);
            rig_then(b, POSE_IDLE, 0.5f, EASE_INOUT);

            G.bossKnock = AJ_RECUO_PERFEITO_MESTRE;
            G.renKnock = AJ_RECUO_PERFEITO_KOJIRO;
            break;
        case J_BOM:
            audio_play(SND_GOOD, 0.9f, 1);
            fx_burst(&G.fx, P_SPARK, at, 3, 45, 1.0f, -0.6f, (Color){205, 190, 160, 255}, (Color){145, 125, 100, 255});
            rig_pose(r, POSE_DEFLECT, 0.06f, EASE_OUT);
            rig_then(r, POSE_IDLE, 0.4f, EASE_INOUT);
            rig_pose(b, POSE_FOLLOW, 0.08f, EASE_OUT);
            rig_then(b, POSE_IDLE, 0.45f, EASE_INOUT);
            G.renKnock = AJ_RECUO_BOM_KOJIRO;
            G.bossKnock = AJ_RECUO_BOM_MESTRE;
            break;
        default: {
            G.aberr = 2.5f;
            audio_play(SND_BAD, 1, 1);
            Vector2 hit = {r->x + 4, GROUND_LOW - 28};
            fx_arc(&G.fx, hit, 8, -1.1f, 1.8f, 0.09f, 1, (Color){255, 228, 210, 220});
            fx_burst(&G.fx, P_DUST, (Vector2){r->x, GROUND_LOW - 1}, 6, 40, 0.6f, 3.14f, (Color){200, 180, 160, 140}, (Color){120, 100, 90, 110});
            fx_flash(&G.fx, (Color){255, 40, 30, 80}, 1);
            fx_kick(&G.fx, AJ_TREMOR_ERRO, AJ_TREMOR_ERRO_TEMPO);
            rig_pose(r, POSE_HURT, 0.06f, EASE_OUT);
            rig_then(r, POSE_IDLE, 0.45f, EASE_INOUT);
            r->flash = 1;
            r->flashColor = (Color){255, 80, 60, 255};
            rig_pose(b, POSE_FOLLOW, 0.1f, EASE_OUT);
            rig_then(b, POSE_IDLE, 0.5f, EASE_INOUT);
            G.renKnock = AJ_RECUO_ERRO_KOJIRO;
            G.renParryTime = -1;
            break;
        }
    }
    if ((e->i & 1) && e->judgement == J_PERFEITO) {
        /* as duas lâminas aparadas: o segundo tinido e o X de faíscas */
        audio_play(SND_PERFECT, 0.7f, 1.25f);
        fx_burst(&G.fx, P_SPARK, at, 2, 55, 0.5f, -2.3f, (Color){230, 240, 255, 255}, (Color){160, 190, 255, 255});
        fx_burst(&G.fx, P_SPARK, at, 2, 55, 0.5f, 2.3f, (Color){230, 240, 255, 255}, (Color){160, 190, 255, 255});
    }
    if ((e->i & 2) && e->judgement != J_RUIM) second_blade();
    if (e->flag) {
        G.aberr = 3.5f;
        G.desat = 1;
        G.crack = 0.14f;
        G.silence = AJ_SILENCIO_QUEBRA;
        audio_play(SND_BREAK, 0.9f, 1); /* cerâmica rachando e taiko; depois, meio segundo de silêncio */
        Vector2 c = {b->x, GROUND_LOW - 30};
        fx_burst(&G.fx, P_SHARD, c, 20, 160, 1.4f, -1.57f, (Color){230, 230, 255, 255}, (Color){180, 140, 255, 255});

        fx_kick(&G.fx, AJ_TREMOR_QUEBRA, AJ_TREMOR_QUEBRA_TEMPO);
        rig_pose(b, POSE_STAGGER, 0.15f, EASE_OUT);
        G.staggerTime = 0.01f;
        G.bossKnock = AJ_RECUO_QUEBRA_MESTRE;
        G.slowmo = AJ_LENTA_QUEBRA;
        G.slowmoTime = AJ_LENTA_QUEBRA_TEMPO;
    }
}

/* O relâmpago do arashi (o golpe pesado dele, LOOK_HEAVY): quando o golpe chega, raios caem do céu em volta de quem aparou ou, se pegou, de kojiro. Quem leva fica
 * meio paralisado: treme, pisca em azul e cai devagar. É só o que se vê: o núcleo julga o relâmpago como qualquer outro golpe de duas lâminas, e o aperto seguinte vale igual. */
static bool golpe_do_raio(void) {
    if (!G.m || G.m->id != ARASHI_ID || G.duel.m != G.m) return false;
    const Move *mv = duel_move(&G.duel);
    return mv && mv->look == LOOK_HEAVY;
}

static void raio_acende(Raio *r) {
    r->espera = 0;
    r->vida = RAIO_VIDA;
    const Color claro = {225, 238, 255, 255}, azul = {120, 170, 255, 255};
    fx_burst(&G.fx, P_SPARK, (Vector2){r->x, GROUND_LOW - 1}, r->principal ? 7 : 4, 75, 1.5f, -1.57f, claro, azul);
    if (r->principal) fx_burst(&G.fx, P_DUST, (Vector2){r->x, GROUND_LOW - 1}, 5, 35, 1.2f, -1.57f, (Color){170, 180, 200, 140}, (Color){100, 108, 130, 110});
}

static void raio_cai(float x, float espera, bool principal) {
    for (int i = 0; i < RAIOS_MAX; i++) {
        Raio *r = &G.raios[i];
        if (r->espera > 0 || r->vida > 0) continue;
        *r = (Raio){x, espera, 0, (unsigned)(x * 31.0f) * 2654435761u + (unsigned)i * 40503u, principal};
        if (espera <= 0) raio_acende(r);
        return;
    }
}

/* O golpe chegou: os raios, o trovão e, para quem levou, o choque. */
static void impacto_do_raio(const DuelEvent *e) {
    if (!golpe_do_raio()) return;
    const bool aparou = e->judgement == J_PERFEITO;
    const float alvo = aparou ? clash_point().x : G.ren.x + G.ren.offsetX;
    raio_cai(alvo, 0, true);
    raio_cai(alvo - 30, 0.05f, false);
    raio_cai(alvo + 38, 0.10f, false);
    G.ctx.lightning = 1;
    audio_play(SND_THUNDER, 0.9f, 1);
    fx_flash(&G.fx, (Color){170, 200, 255, 255}, aparou ? 0.3f : 0.6f);
    const float forca = e->judgement == J_RUIM ? 1.0f : (e->judgement == J_BOM && (e->i & 2)) ? 0.5f : 0.0f;
    if (forca <= 0) return;
    G.paralisiaForca = forca;
    G.paralisia = G.paralisiaTotal = forca >= 1 ? PARALISIA_CHEIA : PARALISIA_METADE;
    fx_popup(&G.fx, "paralisado", (Vector2){G.ren.x + G.ren.offsetX, GROUND_LOW - 58}, 0.9f, (Color){150, 205, 255, 255});
}

static void raios_update(float dt) {
    for (int i = 0; i < RAIOS_MAX; i++) {
        Raio *r = &G.raios[i];
        if (r->espera > 0) { r->espera -= dt; if (r->espera <= 0) raio_acende(r); }
        else if (r->vida > 0) r->vida -= dt;
    }
    if (G.paralisia <= 0) return;
    G.paralisia = fmaxf(0, G.paralisia - dt);
    G.paralisiaFaisca -= dt;
    if (G.paralisiaFaisca <= 0 && G.paralisia > 0) {
        G.paralisiaFaisca = 0.07f;
        const Vector2 at = {G.ren.x + G.ren.offsetX + frand(-6, 6), GROUND_LOW - frand(8, 38)};
        fx_burst(&G.fx, P_SPARK, at, 2, 40, 3.14f, 0, (Color){225, 238, 255, 255}, (Color){120, 170, 255, 255});
    }
}

/* A velocidade da queda do kojiro: devagar com o choque (só a animação de dor; aparar e golpear seguem no ritmo de sempre). */
static float paralisia_ritmo(void) {
    if (G.paralisia <= 0 || !G.renS.pl.anim || strcmp(G.renS.pl.anim->name, "HURT") != 0) return 1;
    return 1 - PARALISIA_LENTIDAO * G.paralisiaForca;
}

static const struct { const char *fx; int row; float y; int flags; float scale; } TELL[ROSTER_SIZE] = {
        {"70", 4, -8, VFX_FRONT, 0.6f},         /* daichi: poeira de terra (na cor do chão) */
        {"26", 3, -30, VFX_BACK | VFX_GLOW | VFX_BODY},    /* genbu: o casco acompanha o corpo */
        {"70", 4, -8, VFX_FRONT, 0.65f},        /* raizo: pó ocre aos pés */
        {"06", 2, -30, VFX_BACK},               /* shizuku: respingo */
        {"64", 0, -30, VFX_BACK | VFX_GLOW},    /* garfiel: garras */
        {"64", 8, -30, VFX_BACK},               /* karasu: asas escuras */
        {"03", 3, -30, VFX_BACK | VFX_GLOW},    /* hayate: redemoinho */
        {"69", 0, -27, VFX_BACK | VFX_GLOW},    /* enjin: labareda */
        {"04", 2, -24, VFX_BACK},               /* suiren: onda */
        {"195", 2, -30, VFX_BACK | VFX_GLOW | VFX_SWORD, 0.45f}, /* arashi: raios na lâmina */
        {"197", 1, -30, VFX_BACK | VFX_GLOW},   /* yoru: estrela da noite */
        {"665", 5, -21, VFX_BACK | VFX_GLOW, 0.5f}, /* jinshi: o brilho da lua (pequeno, lilás, luz somada) */
        {"197", 7, -30, VFX_BACK | VFX_GLOW},   /* oboro */
    };

/* Consulta todas as folhas usadas por um aviso antes de entrar no jogo. */
static void preload_runtime_art(void) {
    for (int i = 0; i < ROSTER_SIZE; i++) spr_fx(TELL[i].fx);
    spr_ui_preload();
}

/* A odachi do Raizo avisa o peso de cada sequência: a posição e a direção das
 * lascas mudam com o corte. Tudo parte da lâmina; só o avanço/salto levanta pó
 * também dos pés. Não participa do relógio nem da colisão do golpe. */
static void raizo_tell(Vector2 tip, Vector2 feet) {
    const Move *mv = duel_move(&G.duel);
    if (!mv) return;
    Color stone = {188, 181, 166, 235}, ochre = {205, 160, 91, 225};
    switch (mv->look) {
        case LOOK_HIGH:   /* o cume: lascas sobem antes do corte vertical */
            fx_burst(&G.fx, P_SHARD, tip, 8, 34, 0.55f, -1.57f, stone, ochre);
            break;
        case LOOK_LOW:    /* a fenda dupla denuncia dois cortes; a laje, só um */
            fx_burst(&G.fx, P_SHARD, tip, mv->strikes > 1 ? 8 : 5, 32, 0.65f, 1.57f, stone, ochre);
            if (mv->strikes > 1) fx_burst(&G.fx, P_DUST, feet, 5, 20, 0.55f, 3.14f, stone, ochre);
            break;
        case LOOK_THRUST: /* a ponta aponta para a guarda */
            fx_burst(&G.fx, P_SHARD, tip, 7, 38, 0.45f, 3.14f, stone, ochre);
            break;
        case LOOK_DASH:   /* a avalanche desloca a base antes da lâmina */
            fx_burst(&G.fx, P_SHARD, tip, 6, 36, 0.75f, 3.14f, stone, ochre);
            fx_burst(&G.fx, P_DUST, feet, 8, 27, 0.65f, 3.14f, stone, ochre);
            break;
        case LOOK_JUMP:   /* pedras caem com o salto */
            fx_burst(&G.fx, P_SHARD, tip, 9, 30, 0.70f, 1.57f, stone, ochre);
            fx_burst(&G.fx, P_DUST, feet, 5, 20, 0.65f, -1.57f, stone, ochre);
            break;
        case LOOK_HEAVY:  /* a montanha se abre no golpe forte */
            fx_burst(&G.fx, P_SHARD, tip, 12, 39, 0.85f, -1.57f, stone, ochre);
            fx_ring(&G.fx, tip, 34, 0.23f, 1, stone);
            break;
        default: break;
    }
}

/* O yoru apagou as luzes nesta sequência: só as adagas dele aparecem, então nada de brilho, faísca ou raio em volta. */
static bool yoru_no_escuro(void) { return G.m && G.m->id == 11 && G.duel.blackout; }

/* Partículas do aviso ligadas ao mestre atual. O roster mudou de ordem ao longo
 * do projeto: manter esta escolha por nome/ID impede que vento solte brasas ou
 * que a katana de fogo solte água. São só desenho; o aviso do core é o mesmo. */
static void tell_particles(int id, Vector2 tip, Vector2 mid, Vector2 feet) {
    switch (id) {
        case 1: /* Daichi: terra */
            fx_burst(&G.fx, P_DUST, feet, 10, 35, 0.65f, -1.57f, (Color){206, 172, 122, 210}, (Color){130, 95, 62, 190}); break;
        case 2: /* Genbu: casco de pedra */
            fx_burst(&G.fx, P_SHARD, feet, 7, 24, 0.70f, -1.57f, (Color){165, 176, 150, 210}, (Color){108, 123, 106, 190}); break;
        case 4: /* Shizuku: gelo */
            fx_burst(&G.fx, P_SHARD, tip, 8, 32, 0.55f, -1.57f, (Color){218, 245, 255, 230}, (Color){132, 196, 230, 210}); break;
        case 5: /* Garfiel: garras */
            fx_burst(&G.fx, P_SPARK, tip, 8, 46, 0.55f, 3.14f, (Color){247, 224, 178, 225}, (Color){205, 176, 116, 200}); break;
        case 6: /* Karasu: penas */
            fx_burst(&G.fx, P_FEATHER, mid, 8, 30, 0.8f, -1.57f, (Color){40, 31, 49, 230}, (Color){112, 37, 55, 210}); break;
        case 7: /* Hayate: folhas levadas pelo vento */
            fx_burst(&G.fx, P_PETAL, mid, 9, 46, 0.45f, 3.14f, (Color){190, 224, 160, 210}, (Color){105, 166, 111, 190}); break;
        case 8: /* Enjin: brasas */
            fx_burst(&G.fx, P_EMBER, tip, 13, 32, 0.8f, -1.57f, (Color){255, 190, 80, 240}, (Color){255, 90, 30, 220}); break;
        case 9: /* Suiren: gotas do mar */
            fx_burst(&G.fx, P_GEM, tip, 9, 36, 0.7f, -1.57f, (Color){170, 230, 240, 220}, (Color){90, 170, 220, 200}); break;
        case 10: /* Arashi: faíscas elétricas */
            fx_burst(&G.fx, P_SPARK, tip, 11, 65, 0.7f, 3.14f, (Color){222, 238, 255, 240}, (Color){117, 170, 255, 220}); break;
        case 11: /* Yoru: lascas discretas fora do apagão */
            fx_burst(&G.fx, P_SHARD, feet, 7, 27, 0.65f, -1.57f, (Color){112, 106, 128, 170}, (Color){62, 58, 83, 150}); break;
        case 12: /* Jinshi: luar */
            fx_burst(&G.fx, P_GEM, tip, 8, 26, 0.6f, -1.57f, (Color){219, 220, 245, 210}, (Color){157, 143, 201, 190}); break;
        default: /* Oboro sem eco: sombra da própria postura */
            fx_burst(&G.fx, P_DUST, mid, 12, 16, 0.9f, 0, (Color){150, 90, 200, 150}, (Color){90, 50, 130, 130}); break;
    }
}

/* Sinal próprio de cada vilão no começo de cada sequência: nunca dois iguais. */
static void tell_fx(void) {
    Rig *b = &G.boss;
    Vector2 butt, tip;
    boss_blade(&butt, &tip);
    Vector2 mid = {(butt.x + tip.x) / 2, (butt.y + tip.y) / 2};
    Vector2 feet = {b->x + b->offsetX - 9 * b->look.size, GROUND_LOW - 1};
    int echo = G.m->isBigBoss ? echo_of(duel_move(&G.duel)) : -1;
    /* tom do gesto de cada mestre, na ordem da trilha */
    static const float pitch[ROSTER_SIZE] = {0.7f, 0.8f, 0.6f, 1.5f, 1.0f, 1.25f, 1.4f, 1.1f, 1.3f, 1.6f, 1.2f, 0.65f, 0.9f};
    const bool escuro = yoru_no_escuro();
    if (escuro) { /* no apagão o aviso é só o som: nenhuma luz além das adagas */ }
    else if (G.m->id == 3 || echo == 2) raizo_tell(tip, feet);
    else tell_particles(G.m->id, tip, mid, feet);
    audio_play(SND_GESTURE, 0.3f, pitch[(G.m->id - 1) % ROSTER_SIZE]);
    if (escuro) return;
    /* E o efeito do pack de cada um: onde nasce (no chão ou no corpo) e a cor. Oboro
     * usa o do aprendiz da postura em que está, em vermelho. */
    int ti = (G.m->id - 1) % ROSTER_SIZE, row = TELL[ti].row;
    if (G.m->isBigBoss && G.masked) { ti = 7; row = TELL[7].row; }   /* o oni: a lâmina acende */
    else if (echo >= 0) { ti = echo; row = echo == 2 ? TELL[2].row : 7; }
    float sc = TELL[ti].scale > 0 ? TELL[ti].scale : 1;
    int flags = TELL[ti].flags;
    Vector2 pos = {b->x + b->offsetX, GROUND_LOW + TELL[ti].y * sc};
    if (ti > 1 && ti != 2) { flags |= VFX_SWORD; pos = tip; sc = fminf(sc, 0.5f); }
    const SprFx *sheet = spr_fx(TELL[ti].fx);
    float fps = 22;
    if ((flags & VFX_SWORD) && sheet)
        fps = sheet->frames / fmaxf(0.15f, (float)(duel_timeline(&G.duel).strike - G.duel.clock) + 0.08f);
    vfx(TELL[ti].fx, row, pos, true, flags, fps);
    if (ti == 9) {
        Vector2 other;
        if (spr_offhand_point(&G.bossS.pl,
                (Vector2){b->x + b->offsetX, b->y - b->hopY}, b->faceLeft, (int)G.bossS.squat, &other))
            vfx(TELL[ti].fx, row, other, true, VFX_BACK | VFX_GLOW | VFX_OFFHAND, fps);
    }
    if (sc < 1)
        for (int i = 0; i < VFX_MAX; i++)
            if (G.vfx[i].fx == spr_fx(TELL[ti].fx) && G.vfx[i].t == 0) {
                /* a fumaça branca do pack, menor e na paleta do cenário: a poeira na cor do
                   chão, o brilho da lua no lilás do céu da serra */
                G.vfx[i].scale = sc;
                G.vfx[i].tint = ti == 2 ? (Color){228, 220, 204, 255}
                               : ti == 11 ? (Color){150, 120, 170, 255}
                               : ti == 0 ? arena_dust(G.m->arena) : (Color){255, 255, 255, 210};
            }
}

/* A vida de kojiro acaba: ele cai e o painel de derrota aparece. */
static void ren_falls(void) {
    rig_pose(&G.ren, POSE_FALLEN, 0.7f, EASE_OUT);
    sprite_fall();
    dust(G.ren.x + G.ren.offsetX, false, 20);
    G.ren.breath = 0;
    G.slowmo = AJ_LENTA_QUEDA;
    G.slowmoTime = AJ_LENTA_QUEDA_TEMPO;
    G.bannerTime = 0;
    fx_popup(&G.fx, "kojiro caiu", (Vector2){160, 56}, 1.2f, VERMILION);
    audio_play(SND_DEFEAT, 0.9f, 1);
    G.defeatsHere++;
    G.defeatIndex = 0;
    set_state(ST_DEFEAT);
}

/* As duas lâminas vão vir juntas: brilham as duas e soa um tinido duplo. */
static void dual_tell(void) {
    Vector2 c = {G.boss.x + G.boss.offsetX, GROUND_LOW - 30};
    if (!yoru_no_escuro()) {
        fx_star(&G.fx, (Vector2){c.x - 7, c.y - 5}, 11, 0.2f);
        fx_star(&G.fx, (Vector2){c.x + 3, c.y + 3}, 9, 0.2f);
    }
    audio_play(SND_SWING, 0.35f, 1.7f);
    audio_play(SND_SWING, 0.3f, 1.9f);
    G.boss.flash = 0.7f;
    G.boss.flashColor = (Color){230, 236, 255, 255};
}

/* A segunda lâmina entrou (o parry não foi perfeito): kojiro sente o corte. */
static void second_blade(void) {
    Rig *r = &G.ren;
    Vector2 hit = {r->x + r->offsetX + 4, GROUND_LOW - 24};
    audio_play(SND_BAD, 0.9f, 1.1f);
    fx_arc(&G.fx, hit, 7, -1.1f, 1.8f, 0.09f, 1, (Color){255, 228, 210, 220});
    fx_flash(&G.fx, (Color){255, 40, 30, 70}, 1);
    fx_kick(&G.fx, AJ_TREMOR_SEGUNDA_LAMINA, AJ_TREMOR_SEGUNDA_TEMPO);
    r->flash = 1;
    r->flashColor = (Color){255, 80, 60, 255};
    G.renKnock = fmaxf(G.renKnock, AJ_RECUO_SEGUNDA_LAMINA);
}

/* Mensagem de aperto tarde; os detalhes de tempo ficam no F3. */
static void cedo_tarde(const char *quando) {
    Vector2 at = {G.ren.x + G.ren.offsetX, GROUND_LOW - 58};
    Color c = quando[0] == 'c' ? (Color){150, 205, 255, 255} : (Color){255, 170, 90, 255};
    fx_popup(&G.fx, quando, at, 0.9f, c);
}

/* O brilho do aviso na lâmina do mestre. O jinshi, que avisa sem som, ganha o brilho
 * da lua: maior, frio e com um anel. */
static void aviso_brilho(bool primeiro) {
    Vector2 h, t;
    boss_blade(&h, &t);
    Vector2 at = {h.x + (t.x - h.x) * 0.75f, h.y + (t.y - h.y) * 0.75f};
    if (G.m->id == 3 || (G.m->isBigBoss && echo_of(duel_move(&G.duel)) == 2)) {
        /* Lascas cinza-pedra e ocre no instante do aviso, presas à odachi. */
        fx_burst(&G.fx, P_SHARD, at, primeiro ? 7 : 4, 35, 0.70f, -1.57f,
                 (Color){196, 192, 183, 255}, (Color){223, 177, 104, 255});
        if (primeiro) fx_ring(&G.fx, at, 38, 0.18f, 1, (Color){219, 198, 158, 200});
        return;
    }
    bool lua = G.m->cueAudio <= 0;
    if (!primeiro) {
        fx_star(&G.fx, at, 8, 0.09f);
        return;
    }
    if (lua) fx_star_tint(&G.fx, at, 22, 0.2f, (Color){150, 190, 255, 255}, (Color){225, 238, 255, 255});
    else fx_star(&G.fx, at, 15, 0.14f);
    fx_ring(&G.fx, at, lua ? 70 : 45, 0.2f, 1, lua ? (Color){200, 225, 255, 220} : (Color){255, 245, 210, 180});
}

/* F3: o resultado de cada aperto e o erro em ms (o mais novo em cima). O erro é quanto
 * faltou para a janela perfeita; os números vêm do core (EV_PRESS e EV_IMPACT). */
static void anota_aperto(const DuelEvent *e) {
    if (e->kind == EV_PRESS && e->i != PRESS_CEDO && e->i != PRESS_TARDE) return;
    int n = (int)(sizeof G.aperto / sizeof G.aperto[0]);
    memmove(&G.aperto[1], &G.aperto[0], sizeof G.aperto[0] * (size_t)(n - 1));
    if (G.apertos < n) G.apertos++;
    Anotacao *l = &G.aperto[0];
    float a = fabsf(e->a) * 1000, b = e->b * 1000;
    snprintf(l->quando, sizeof l->quando, "%.0f ms %s do contato", a, duel_event_after_contact(e) ? "depois" : "antes");
    if (e->kind == EV_PRESS && e->i == PRESS_CEDO) {
        snprintf(l->res, sizeof l->res, "CEDO");
        snprintf(l->erro, sizeof l->erro, "%.0f ms antes do aviso: este golpe não sai perfeito", b);
        l->cor = (Color){150, 205, 255, 255};
        return;
    }
    if (e->kind == EV_PRESS) {
        snprintf(l->res, sizeof l->res, "TARDE");
        snprintf(l->erro, sizeof l->erro, "o golpe já tinha entrado");
        l->cor = (Color){255, 170, 90, 255};
        return;
    }
    snprintf(l->res, sizeof l->res, "%s", e->judgement == J_PERFEITO ? "PERFEITO" : e->judgement == J_BOM ? "BOM" : "ERRO");
    l->cor = e->judgement == J_PERFEITO ? GREEN : e->judgement == J_BOM ? GOLD : RED;
    if (e->a == -1.0f) {
        snprintf(l->quando, sizeof l->quando, "sem aperto");
        l->erro[0] = 0;
    } else if (b >= 0.5f) {
        snprintf(l->erro, sizeof l->erro, "erro %.0f ms cedo", b);
    } else if (b <= -0.5f) {
        snprintf(l->erro, sizeof l->erro, "erro %.0f ms tarde", -b);
    } else {
        snprintf(l->erro, sizeof l->erro, e->judgement == J_BOM ? "erro 0, mas apertou antes do aviso" : "erro 0");
    }
    if (e->i & 2) strncat(l->erro, "  (a 2ª lâmina entrou)", sizeof l->erro - strlen(l->erro) - 1);
}

static void begin_posture_aura(int echo);

static void handle_events(void) {
    DuelEvent ev[MAX_EVENTS];
    int n = duel_drain(&G.duel, ev, MAX_EVENTS);
    const MasterProfile *m = G.m;
    Rig *b = &G.boss, *r = &G.ren;
    for (int i = 0; i < n; i++) {
        const DuelEvent *e = &ev[i];
        switch (e->kind) {
            case EV_WINDUP:
                G.windupLen = fmaxf(0.1f, e->a - duel_strike_lead(&G.duel));
                G.windupSpr = fmaxf(0.1f, e->a - duel_strike_lead_base(&G.duel));
                G.windupTime = 0;
                G.bossWinding = true;
                G.staggerTime = 0;
                /* A preparação leva exatamente o tempo até a partida da lâmina.
                 * Cada tipo de sequência tem sua preparação: é assim que se lê o moveset. */
                if (G.duel.comboStrike == 0) {
                    rig_pose(b, windup_pose(strike_look()), G.windupLen, EASE_INOUT);
                    tell_fx();
                }
                else rig_pose(b, rearm_pose(strike_look()), G.windupLen, EASE_OUT);
                sprite_windup();
                if (m->isBigBoss) {
                    int echo = echo_of(duel_move(&G.duel));
                    if (G.duel.comboStrike == 0) begin_posture_aura(echo);
                    else if (echo >= 0) G.auraLeft = fmaxf(G.auraLeft, G.duel.windupDuration + 0.65f);
                }
                if (duel_strike_dual(&G.duel)) dual_tell();
                G.blackoutTarget = G.duel.blackout ? 1 : 0;
                if (m->arena == ARENA_PORTO) audio_play(SND_DRUM, 0.9f, 1);
                break;
            case EV_LAUNCH:
                G.bossWinding = false;
                /* O corte chega em POSE_CONTACT exatamente no instante do contato. */
                rig_pose(b, contact_pose(strike_look()), duel_strike_lead(&G.duel), EASE_IN);
                b->trail = true;
                sprite_launch();
                audio_play(SND_SWING, 0.9f, 1);
                break;
            case EV_CUE:
                /* o aviso: sempre o mesmo tempo antes do contato; na sequência, mais discreto.
                 * O som vem num evento próprio, adiantado pela calibração de áudio. */
                if (e->flag) audio_play(SND_CUE, m->cueAudio * (e->i == 0 ? 1 : 0.55f), 1);
                else aviso_brilho(e->i == 0);
                break;
            case EV_PRESS:
                if (e->i == PRESS_TARDE) cedo_tarde("tarde");
                anota_aperto(e);
                rig_pose(r, G.duel.phase == PH_WINDUP ? parry_pose(strike_look()) : POSE_PARRY, 0.06f, EASE_OUT);
                sprite_press();
                G.renParryTime = 0;
                audio_play(SND_GESTURE, 0.8f, 1 + (rand() % 7) * 0.02f);
                break;
            case EV_IMPACT:
                if (G.leap == LEAP_JUMP && G.hopTravel) {
                    G.bossStep = G.bossStrikeStep;
                    G.hopT = G.hopLen;
                    G.boss.hopY = 0;
                    G.boss.offsetX = G.bossKnock + G.bossStep;
                }
                anota_aperto(e);
                if (G.logImpactos)
                    fprintf(stderr, "IMPACTO contato %.5f julgamento %d antecedencia %.5f quadro %s %d\n", G.duel.lastStrikeAt, (int)e->judgement, e->a,
                            G.bossS.pl.anim ? G.bossS.pl.anim->name : "-", G.bossS.pl.frame);
                on_impact(e);
                impacto_do_raio(e);
                if (m->id == 3 || (m->isBigBoss && echo_of(duel_move(&G.duel)) == 2))
                    fx_burst(&G.fx, P_SHARD, clash_point(), 6, 43, 0.90f, 3.14f,
                             (Color){185, 182, 174, 220}, (Color){203, 157, 92, 220});
                sprite_impact(e);
                G.special = false;
                G.renParryTime = -1;
                G.blackoutTarget = 0;
                break;
            case EV_STANCE:
                /* no oboro, a postura nova aparece quando ele volta a lutar, depois da fala */
                if (!G.m->isBigBoss) banner(duel_stance(&G.duel)->name, AGED_GOLD);
                break;
            case EV_SEAL: {
                marco_de_teste("selo_quebrado");
                static const char *names[] = {"primeiro selo", "segundo selo", "terceiro selo"};
                banner(names[e->i < 3 ? e->i : 2], VERMILION);
                audio_play(SND_SEAL, 1, 1);
                audio_play(SND_THUNDER, 0.8f, 1);
                G.ctx.seal = e->i;
                G.ctx.lightning = 1;
                G.gritoPending = true;
                vfx("197", 7, (Vector2){G.boss.x + G.boss.offsetX, GROUND_LOW - 30}, true, VFX_BACK | VFX_GLOW, 20);
                audio_music_intensity(e->i / 2.0f);
                break;
            }
            case EV_SPECIAL:
                G.special = true;
                audio_play(SND_DRUM, 1, 0.7f);
                break;
            case EV_COMBO:
                break;
            case EV_ADVANTAGE:
                /* falta um perfeito: o próximo parry perfeito quebra a postura e é a execução */
                if (e->flag) {
                    fx_popup(&G.fx, "vantagem", (Vector2){b->x + b->offsetX, GROUND_LOW - 64}, 1.0f, (Color){255, 214, 120, 255});
                    audio_play(SND_GEM, 0.8f, 1.2f);
                }
                break;
            case EV_BURN: {
                Vector2 at = {r->x + r->offsetX, GROUND_LOW - 22};
                if (e->flag) {
                    /* a lâmina de fogo acendeu kojiro */
                    audio_play(SND_SWING, 0.5f, 0.55f);
                } else {
                    /* apagou: um fio de fumaça */
                    fx_burst(&G.fx, P_DUST, at, 10, 25, 1.0f, -1.57f, (Color){150, 146, 150, 150}, (Color){90, 86, 92, 120});
                    audio_play(SND_GESTURE, 0.4f, 0.55f);
                }
                break;
            }
            case EV_FINISHED:
                if (e->flag) {
                    registra_vitoria();
                    start_disarm();
                } else {
                    ren_falls();
                }
                break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Por quadro                                                          */
/* ------------------------------------------------------------------ */

/* O corvo se desfaz em penas e se refaz delas: o corpo apaga em vez de cortar, e as penas saem de pontos do corpo (ao sumir) ou voltam para ele
 * (ao reaparecer). Só desenho: o núcleo e os tempos do golpe não são tocados. */
static void karasu_penas(float dt, bool soltando) {
    G.warpSolta += dt * KARASU_WARP_PENAS_S;
    const Vector2 centro = {G.boss.x + G.boss.offsetX, GROUND_LOW - 22};
    while (G.warpSolta >= 1) {
        G.warpSolta -= 1;
        const float r = (float)rand() / (float)RAND_MAX, q = (float)rand() / (float)RAND_MAX;
        if (soltando) {
            /* de um ponto qualquer do corpo, para trás (a direita) e para cima */
            Vector2 at = {centro.x + (r - 0.5f) * 14, centro.y + (q - 0.5f) * 36};
            fx_burst(&G.fx, P_FEATHER, at, 1, 70, 0.8f, -0.5f, (Color){18, 14, 29, 255}, (Color){100, 26, 44, 245});
        } else {
            /* de um anel em volta, para o corpo */
            const float ang = r * 6.2832f, raio = 14 + q * 12;
            Vector2 at = {centro.x + cosf(ang) * raio, centro.y + sinf(ang) * raio * 1.3f};
            fx_burst(&G.fx, P_FEATHER, at, 1, 120, 0.15f, ang + 3.1416f, (Color){18, 14, 29, 255}, (Color){100, 26, 44, 245});
        }
    }
}

/* Pranchas e o passo do mestre até o alcance do golpe (e de volta ao lugar). */
static void fighters_update(float dt) {
    f_update(&G.renS, dt * paralisia_ritmo());
    ren_draw_sounds();
    f_update(&G.bossS, dt);
    if (!G.bossS.set) return;
    bool landed = G.hopT >= G.hopLen;
    /* investida e salto: depois do recuo (ou de agachar) ele arranca */
    if (G.leap == LEAP_WARP && G.bossWinding) {
        G.leapT += dt;
        if (G.leapStage == 0) {
            G.warpPenasT -= dt;
            if (G.warpPenasT <= 0) {
                G.warpPenasT = KARASU_WARP_PENAS;
                feather_blur();
            }
        }
        if (G.leapStage == 0 && G.leapT >= G.leapAt) {
            G.leapStage = 1;
            G.bossStep = G.bossStepTo; /* as penas partem do fim do recuo, à direita */
            G.boss.offsetX = G.bossKnock + G.bossStep;
            feathers();
            G.bossHidden = true;
            G.bossStep = G.bossStepTo = G.bossStrikeStep;     /* já está onde vai reaparecer */
        } else if (G.leapStage == 1 && G.leapT >= G.leapAir) {
            G.leapStage = 2;
            G.bossHidden = false;
            G.boss.offsetX = G.bossKnock + G.bossStep;
            feathers();
        }
    }
    {
        /* a dissolução: apaga nos últimos instantes do recuo e some; ao reaparecer, se forma. O que vem do núcleo (relógio, aviso, contato) é o mesmo. */
        const float dis = G.leap == LEAP_WARP ? fminf(KARASU_WARP_DISSOLVE, G.leapAt * 0.5f) : KARASU_WARP_DISSOLVE;
        bool apagando = false;
        if (G.leap == LEAP_WARP && G.bossWinding)
            apagando = G.leapStage == 1 || (G.leapStage == 0 && G.leapT >= G.leapAt - dis);
        const float antes = G.bossDissolve;
        G.bossDissolve = clampf(antes + (apagando ? dt / dis : -dt / KARASU_WARP_FORMA), 0, 1);
        if (apagando && G.bossDissolve < 1) karasu_penas(dt, true);
        else if (!apagando && G.bossDissolve > 0) karasu_penas(dt, false);
        else G.warpSolta = 0;
    }
    if ((G.leap == LEAP_DASH || G.leap == LEAP_JUMP) && G.bossWinding) {
        G.leapT += dt;
        if (G.leapStage == 0 && G.leapT >= G.leapAt) {
            G.leapStage = 1;
            dust(G.boss.x + G.boss.offsetX, true, 22);
            if (G.leap == LEAP_DASH) {
                G.bossStepTo = G.bossStrikeStep + 10;
                G.bossStepSpeed = fabsf(G.bossStepTo - G.bossStep) / fmaxf(0.05f, G.windupLen - G.leapAt);
            } else {
                G.leapAir = fmaxf(0.05f, (float)(duel_timeline(&G.duel).strike - G.duel.clock));
                boss_hop(G.leapAir, 24);
                G.hopFrom = G.bossStep;
                G.hopTo = G.bossStrikeStep;
                G.bossStepTo = G.hopTo;
                G.hopTravel = true;
            }
        }
    }
    if (G.leap == LEAP_FEINT && G.bossWinding) {
        G.leapT += dt;
        if (G.leapStage == 0 && G.leapT >= G.leapAt) {
            G.leapStage = 1;
            G.bossStepTo = G.feintFrom;
            G.bossStepSpeed = fabsf(G.bossStepTo - G.bossStep) / fmaxf(0.05f, G.leapAir - G.leapAt);
        } else if (G.leapStage == 1 && G.leapT >= G.leapAir) {
            G.leapStage = 2;
            G.bossStepTo = G.bossStep + (G.bossStrikeStep - G.bossStep) * 0.4f;
            G.bossStepSpeed = fabsf(G.bossStepTo - G.bossStep) / fmaxf(0.05f, G.windupLen - G.leapAir);
        }
    }
    /* agacha antes do salto e firma o corpo no fim da preparação */
    G.landT = fmaxf(0, G.landT - dt);
    int squat = 0;
    if (G.leap == LEAP_JUMP && G.leapStage == 0) squat = 1 + (int)(2.99f * clampf(G.leapT / G.leapAt, 0, 1));
    else if (G.landT > 0) squat = (int)roundf(3 * smooth(G.landT / 0.2f));
    else if (G.bossWinding && (!G.leap || G.leap == LEAP_FAR) && G.windupTime > G.windupLen * 0.5f) squat = 1;
    G.bossS.squat = squat;
    if (G.bossS.idle && !G.bossWinding && landed) {
        if (G.bossStep < -10) {
            /* longe do lugar: volta num pulo para trás */
            float t = 0.32f;
            const SprAnim *j = fa(&G.bossS, "JUMP");
            G.bossStepTo = 0;
            G.bossStepSpeed = -G.bossStep / t;
            boss_hop(t, 6);
            G.hopFrom = G.bossStep;
            G.hopTo = 0;
            G.hopTravel = true;
            if (j) {
                f_clear(&G.bossS, true);
                f_add(&G.bossS, j, 0, j->frames - 1, t);
            }
        } else {
            G.bossStepTo = 0;
            G.bossStepSpeed = fmaxf(G.bossStepSpeed, 30);
        }
    }
    float d = G.bossStepTo - G.bossStep, mv = G.bossStepSpeed * dt;
    if (!G.hopTravel || G.hopT >= G.hopLen)
        G.bossStep = fabsf(d) <= mv ? G.bossStepTo : G.bossStep + (d > 0 ? mv : -mv);
}

/* Silhuetas que o mestre deixa para trás no bote, na corrida e no salto. */
static void update_after(float dt) {
    if (G.m && G.m->id == 10) {
        memset(G.after, 0, sizeof G.after);
        G.lastStep = G.bossStep;
        return;
    }
    for (int i = 0; i < AFTER_MAX; i++) G.after[i].life = fmaxf(0, G.after[i].life - dt * 5);
    float v = dt > 0 ? fabsf(G.bossStep - G.lastStep) / dt : 0;
    G.lastStep = G.bossStep;
    bool fast = v > 60 || (G.hopT < G.hopLen && G.hopH > 10);
    G.afterTimer -= dt;
    if (!G.bossS.set || !G.bossS.pl.anim || !fast || G.afterTimer > 0) return;
    G.afterTimer = 0.035f;
    G.afterHead = (G.afterHead + 1) % AFTER_MAX;
    G.after[G.afterHead].a = G.bossS.pl.anim;
    G.after[G.afterHead].frame = G.bossS.pl.frame;
    G.after[G.afterHead].feet = (Vector2){G.boss.x + G.boss.offsetX, G.boss.y - G.boss.hopY};
    G.after[G.afterHead].life = 1;
}

static void begin_posture_aura(int echo) {
    if (echo < 0) return;
    G.auraEcho = echo;
    G.auraLeft = G.duel.windupDuration + 0.65f;
    G.auraTick = 0;
    banner(roster_get(echo)->style, posture_color(echo));
}

static void update_posture_aura(float dt) {
    if (!G.m || !G.m->isBigBoss || G.state != ST_DUEL || G.auraLeft <= 0) return;
    G.auraLeft = fmaxf(0, G.auraLeft - dt);
    G.auraTick -= dt;
    if (G.auraTick > 0) return;
    G.auraTick = 0.12f;
    Color c = posture_color(G.auraEcho);
    Vector2 body = {G.boss.x + G.boss.offsetX, G.boss.y - G.boss.hopY - 20};
    fx_burst(&G.fx, P_EMBER, body, 5, 26, 2.6f, -1.57f, c, fadec(c,0.45f));
    if (G.auraLeft > 0.45f && ((int)(G.auraLeft*8) % 3 == 0)) {
        vfx("70", 4, body, true, VFX_BACK | VFX_BODY | VFX_GLOW, 18);
        for (int i = 0; i < VFX_MAX; i++)
            if (G.vfx[i].fx == spr_fx("70") && G.vfx[i].body && G.vfx[i].t == 0) {
                G.vfx[i].scale = 0.65f;
                G.vfx[i].tint = fadec(c,0.55f);
            }
    }
}

/* O golpe chega ao contato num quadro só: a prancha traz o avanço do corpo pronto no quadro de contato (até 38 px, sem quadro no meio), e sem ajuda ele salta
 * no instante do choque. Na partida da lâmina o mestre passa a avançar esse tanto, devagar e acelerando (p ao quadrado), e no contato (o quadro de contato
 * entra com a partida zerada) o corpo está exatamente onde a prancha o põe. `p`: a partida de 0 a 1 (duel_launch_progress). Devolve o deslocamento em x
 * na tela, em px: quem olha para a esquerda avança para -x. */
static float deslize_do_golpe(const SprAnim *a, float p, bool faceLeft) {
    if (!AJ_DESLIZE_GOLPE || !a || p <= 0) return 0;
    const int c = anim_contact(a);
    if (c < 1) return 0;
    const float d = spr_salto_do_corpo(a, c - 1, c);
    if (fabsf(d) < AJ_DESLIZE_MIN) return 0;
    return (faceLeft ? -1.0f : 1.0f) * clampf(d, -AJ_DESLIZE_MAX, AJ_DESLIZE_MAX) * p * p;
}

static void update_actors(float dt) {
    Rig *b = &G.boss, *r = &G.ren;
    fighters_update(dt);
    update_posture_aura(dt);
    rig_update(r, dt);
    rig_update(b, dt);
    /* Gesto sem golpe: Ren volta à guarda sozinho. */
    if (G.renParryTime >= 0) {
        G.renParryTime += dt;
        if (G.renParryTime > 0.3f) { rig_pose(r, POSE_IDLE, 0.22f, EASE_INOUT); G.renParryTime = -1; }
    }
    /* Em brasas (enjin): kojiro solta brasas e pisca em laranja enquanto queima. */
    if (G.duel.burnLeft > 0 && G.state == ST_DUEL) {
        r->flash = fmaxf(r->flash, 0.22f + 0.12f * sinf(G.time * 18));
        r->flashColor = (Color){255, 120, 40, 255};
    }
    /* Golpe especial: o mestre arde em vermelhão enquanto arma. */
    if (G.special && G.state == ST_DUEL) {
        b->flash = 0.35f + 0.2f * sinf(G.time * 14);
        b->flashColor = VERMILION;
    }
    if (G.bossWinding) G.windupTime += dt;
    /* BIG BOSS se recompõe depois de perder um selo. */
    if (G.staggerTime > 0) {
        G.staggerTime += dt;
        int seal = G.duel.seal;
        if (G.state == ST_DUEL && G.staggerTime > AJ_QUEBRA_ATE_A_CENA && G.m->isBigBoss && seal >= 1 && seal <= 2 && !G.sealTold[seal]) {
            /* oboro para de lutar e fala; o grito vem depois */
            G.sealTold[seal] = true;
            marco_de_teste("cena_do_selo");
            start_scene(seal == 1 ? SCENE_SEAL_1 : SCENE_SEAL_2);
        } else if (G.state == ST_DUEL && G.staggerTime > AJ_QUEBRA_ATE_A_CENA) {
            rig_pose(b, POSE_IDLE, 0.5f, EASE_INOUT);
            G.staggerTime = 0;
            /* o grito do pack mostra a máscara: sem ela, o montado */
            const SprAnim *grito = G.masked || !G.m->isBigBoss ? fa(&G.bossS, "SHOUT") : NULL;
            if (!grito) grito = fa(&G.bossS, "GRITO");
            if (G.gritoPending && grito) {
                /* oboro grita; de máscara, volta em fúria, com a lâmina em chamas */
                G.bossS.furia = G.masked || !G.m->isBigBoss;
                f_clear(&G.bossS, true);
                f_add(&G.bossS, grito, 0, grito->frames - 1, 0);
            } else {
                fighter_idle(&G.bossS);
            }
            G.gritoPending = false;
        }
    }
    update_hop(dt);
    /* O cansaço segue a vida de kojiro e a postura do mestre. */
    if (G.state == ST_DUEL || G.state == ST_INTRO) {
        r->fatigue = G.state == ST_DUEL ? 1 - clampf(G.duel.renPosture / G.settings.renPosture, 0, 1) : 0;
        b->fatigue = G.state == ST_DUEL ? 1 - clampf(G.duel.bossPosture / duel_posture_max(&G.duel), 0, 1) : 0;
    }
    G.renKnock *= expf(-dt * 9);
    G.bossKnock *= expf(-dt * 7);
    r->offsetX = -G.renKnock;
    b->offsetX = G.bossKnock + (G.bossS.set ? G.bossStep + (G.state == ST_DUEL ? deslize_do_golpe(G.bossS.strike, duel_launch_progress(&G.duel), b->faceLeft) : 0) : 0);
    update_after(dt);
}

static void update_ctx(float dt) {
    G.ctx.t += dt;
    G.ctx.blackout += (G.blackoutTarget - G.ctx.blackout) * (1 - expf(-dt * (G.blackoutTarget > G.ctx.blackout ? 5 : 3)));
    G.ctx.lightning = fmaxf(0, G.ctx.lightning - dt * 3);
    /* tempestade: no dojo de oboro depois do primeiro selo, e a noite toda no castelo de arashi */
    bool storm = (G.m->arena == ARENA_CIDADELA && G.ctx.seal >= 1) || G.m->arena == ARENA_SALAO;
    if (storm && !G.windOnly) {
        G.lightningTimer -= dt;
        if (G.lightningTimer <= 0) {
            bool castle = G.m->arena == ARENA_SALAO;
            G.lightningTimer = castle ? 2.5f + frand(0, 4) : 3 + frand(0, 5);
            G.ctx.lightning = 1;
            G.ctx.bolt = frand(0, 1);
            audio_play(SND_THUNDER, castle ? 0.5f : 0.6f, frand(0.8f, 1.1f));
        }
    }
}

static void update_hud_values(float dt) {
    float k = 1 - expf(-dt * 18), g = 1 - expf(-dt * 2.5f);
    G.shownRen += (G.duel.renPosture - G.shownRen) * k;
    G.shownBoss += (G.duel.bossPosture - G.shownBoss) * k;
    if (G.ghostRen < G.shownRen) G.ghostRen = G.shownRen; else G.ghostRen += (G.shownRen - G.ghostRen) * g;
    if (G.ghostBoss < G.shownBoss) G.ghostBoss = G.shownBoss; else G.ghostBoss += (G.shownBoss - G.ghostBoss) * g;
}

/* Em que instante do passo `dt` o aperto deste quadro aconteceu, em segundos desde o começo do passo:
 * o do carimbo do clique, ou o meio do quadro (o de sempre) quando não há carimbo, ele não é confiável
 * (entrada_no_quadro) ou o jogo está em demonstração. `corrido` é a parte do quadro real em que o tempo
 * correu (o quadro todo, ou o que sobrou dele depois de um hitstop). */
static double instante_do_aperto(double corrido, double dt, bool podeCarimbar) {
    if (!G.usaCarimbo) return dt * 0.5;
    /* O monitor nativo carimba teclado e mouse, não o botão do controle.
     * Um evento de teclado no mesmo quadro não pode emprestar seu horário ao gamepad. */
    if (!podeCarimbar) { G.semCarimbo++; return dt * 0.5; }
    bool valido;
    double t = entrada_no_quadro(G.carimbo, G.poll, G.quadro, corrido, dt, &valido);
    if (valido) G.carimbados++; else G.semCarimbo++;
    if (G.logCarimbos)
        fprintf(stderr, "APERTO valido=%d t_ms=%.3f passo_ms=%.3f corrido_ms=%.3f atraso_ms=%.3f\n", valido, t * 1000, dt * 1000, corrido * 1000,
                (G.poll - G.carimbo) * 1000);
    return t;
}

static void update_duel(float dtReal) {
    float dt = dtReal * G.slowmo;
    double corrido = dtReal;
    if (G.slowmoTime > 0) { G.slowmoTime -= dtReal; if (G.slowmoTime <= 0) G.slowmo = 1; }
    bool press = pressed();
    /* o robô do demo é o mesmo dos testes (robo.c) */
    if ((G.demo || G.autoJogo) && robo_quer_apertar(&G.robo, &G.duel, dt)) press = true;

    /* Hitstop congela o duelo e as poses; o quadro em que ele acaba corre só o que sobra dele, e assim
     * o congelamento dura exatamente o que o núcleo descontou da sequência, em qualquer taxa. */
    if (G.hitstop > 0) {
        float sobra = hitstop_passo(&G.hitstop, dtReal);
        if (sobra <= 0) {
            if (press) duel_press(&G.duel);
            handle_events();
            audio_music_duck(G.silence > 0 ? 1 : 0.6f);
            return;
        }
        dt = sobra * G.slowmo;
        corrido = sobra;
    }
    audio_music_duck(G.silence > 0 ? 1 : 0);
    /* O clique chegou em algum ponto do último quadro: onde o carimbo diz, ou no meio dele. */
    duel_step_at(&G.duel, dt, press ? instante_do_aperto(corrido, dt, pressed_key_mouse()) : -1);
    handle_events();
    if (G.state == ST_DUEL || G.state == ST_DEFEAT) update_actors(dt);
}

static void update_finisher(float dt) {
    /* O mestre perde a espada: ela voa, gira e crava; ele cai de joelhos e Ren aponta a lâmina. */
    float t = G.stateTime;
    Rig *r = &G.ren, *b = &G.boss;
    if (t > 0.45f && r->to.sword != POSE_POINT.sword) {
        rig_pose(r, POSE_POINT, 0.6f, EASE_INOUT);
        /* kojiro avança e para com a lâmina baixa, apontada para o mestre */
        const SprAnim *dash = fa(&G.renS, "DASH");
        if (dash) {
            f_clear(&G.renS, false);
            f_add(&G.renS, dash, 0, dash->frames - 1, 0.6f);
        }
    }
    r->offsetX = G.renStepFrom + (14 - G.renStepFrom) * smooth((t - 0.45f) / 0.6f);
    G.bossKnock *= expf(-dt * 3);
    b->offsetX = G.bossKnock + (G.bossS.set ? G.bossStep : 0);
    rig_update(r, dt);
    rig_update(b, dt);
    f_update(&G.renS, dt);
    f_update(&G.bossS, dt);
    update_sword(dt);
    if (G.sword.stuck && G.sword.stuckTime > AJ_ESPADA_CRAVADA_ESPERA) {
        marco_de_teste("fala_do_vencido");
        if (G.m->isBigBoss) start_scene(SCENE_KNEEL);   /* de joelhos, ele tira a máscara */
        else start_lines(G.m->outro, G.m->outroCount, ST_OUTRO);
    }
}

/* ------------------------------------------------------------------ */
/* Cenas da luta final e dos finais (story_scene)                      */
/* ------------------------------------------------------------------ */

/* Quanto a ação de cada momento leva antes da fala (ou antes de passar sozinha). */
static float cue_len(Cue c) {
    switch (c) {
        case CUE_MASK_ON: return 1.4f;
        case CUE_MASK_OFF: return 1.5f;
        case CUE_RAISE: return 1.2f;
        case CUE_KILL: return 2.8f;
        case CUE_HANZO_CLAP: return 2.6f;
        case CUE_HANZO_MASK: return 2.0f;
        case CUE_LOWER: return 2.5f;
        case CUE_HANZO_IN: return 1.6f;
        case CUE_HANZO_KILL: return 2.6f;
        case CUE_CHASE: return 1.8f;
        default: return 0;
    }
}

/* A ação acontece uma vez, quando o relógio do momento passa por `when`. */
static bool crossed(float when) { return G.beatPrev <= when && G.beatTime > when; }

/* Oboro com ou sem a máscara de oni (duas pranchas; o quadro em curso segue). */
static void boss_wear(bool mask) {
    G.masked = mask;
    const SprSet *s = spr_get(mask ? "oboro_mascara" : "oboro");
    if (s && G.bossS.set) G.bossS.set = s;
}

/* Build de teste: a luta começa direto no selo `selo` do oboro, como se os anteriores
 * tivessem acabado de quebrar (sem as falas; na terceira fase, de máscara e em fúria). */
static void teste_selo(int selo) {
    duel_start_seal(&G.duel, selo);
    int f = G.duel.seal;
    G.ctx.seal = f;
    for (int i = 1; i <= f && i < MAX_SEALS; i++) G.sealTold[i] = true;
    if (f >= 2) {
        boss_wear(true);
        G.bossS.furia = true;
    }
    audio_music_intensity(f / 2.0f);
    G.shownBoss = G.ghostBoss = duel_posture_max(&G.duel);
    banner(duel_stance(&G.duel)->name, G.masked ? VERMILION : AGED_GOLD);
}

/* Recomeça a luta contra o mestre `indice`, no selo `selo`, com kojiro chegando como na
 * trilha (os anteriores vencidos). O progresso não é mais salvo nesta sessão. */
static void teste_luta(int indice, int selo) {
    G.teste = true;
    if (indice != G.camp.index) {
        campaign_reset(&G.camp);
        for (int i = 0; i < indice; i++) campaign_mark_cleared(&G.camp, i);
    }
    start_master(indice);
    G.startSeal = selo;
    start_duel();
}

/* A tecla de teste `k` foi apertada? (O teste automático "aperta" uma por vez: APARA_TECLAS.) */
static bool tecla_de_teste(int k) { return IsKeyPressed(k) || G.teclaFalsa == k; }

/* Teclas de teste, só com --teste, na luta ou na derrota. Nenhuma grava no progresso: com
 * --teste o jogo não salva (save_game). */
static void teclas_de_teste(void) {
    static const struct { float t; int k; } FALSAS[] = {{1.0f, KEY_R}, {1.6f, KEY_V}, {2.2f, KEY_P}, {2.8f, KEY_N}, {3.4f, KEY_B}, {4.0f, KEY_TWO}};
    static int falsa = 0;
    G.teclaFalsa = 0;
    if (getenv("APARA_TECLAS") && falsa < 6 && G.time >= FALSAS[falsa].t) G.teclaFalsa = FALSAS[falsa++].k;
    if (!G.teclas || G.demo || G.paused || !(G.state == ST_DUEL || G.state == ST_DEFEAT)) return;
    int n = roster_size(), fase = tecla_de_teste(KEY_ONE) ? 0 : tecla_de_teste(KEY_TWO) ? 1 : tecla_de_teste(KEY_THREE) ? 2 : -1;
    bool age = true;
    if (tecla_de_teste(KEY_R)) teste_luta(G.camp.index, G.duel.seal);
    else if (tecla_de_teste(KEY_N) || IsKeyPressed(KEY_PAGE_DOWN)) teste_luta((G.camp.index + 1) % n, 0);
    else if (tecla_de_teste(KEY_B) || IsKeyPressed(KEY_PAGE_UP)) teste_luta((G.camp.index + n - 1) % n, 0);
    else if (fase >= 0 && G.m->sealCount > 1) teste_luta(G.camp.index, fase);
    else if (G.state == ST_DUEL && tecla_de_teste(KEY_V)) duel_refill(&G.duel, true, false);
    else if (G.state == ST_DUEL && tecla_de_teste(KEY_P)) duel_refill(&G.duel, false, true);
    else age = false;
    if (age && G.autoJogo) fprintf(stderr, "TESTE_TECLA %d\n", G.teclaFalsa);
}

/* Kojiro anda (a corrida, devagar) ou corre por `dur` segundos. */
static void ren_walk(float dur, bool run) {
    const SprAnim *a = fa(&G.renS, "RUN");
    if (!a) return;
    f_clear(&G.renS, true);
    float lap = run ? a->frames * a->frameTime : a->frames * a->frameTime * 1.8f;
    for (float t = 0; t < dur - 0.01f; t += lap) f_add(&G.renS, a, 0, a->frames - 1, lap);
}

static void hanzo_walk(float to, float len) {
    G.hz.from = G.hz.x;
    G.hz.to = to;
    G.hz.t = 0;
    G.hz.len = len;
    /* A caminhada própria, quando o pack trouxer a tira WALK. Até lá, manter
     * exatamente o RUN lento que as cenas já usam. */
    const SprAnim *a = spr_anim(G.hz.f.set, "WALK");
    if (!a) a = spr_anim(G.hz.f.set, "RUN");
    if (!a || len <= 0) return;
    f_clear(&G.hz.f, true);
    float lap = a->frames * a->frameTime * 2;   /* um velho: passos lentos */
    for (float t = 0; t < len - 0.01f && G.hz.f.qn < 3; t += lap) f_add(&G.hz.f, a, 0, a->frames - 1, lap);
}

/* Hanzo entra em cena em x (sem as pranchas, só a fala). */
static void hanzo_enter(float x, bool faceLeft) {
    memset(&G.hz, 0, sizeof G.hz);
    G.hz.on = true;
    G.hz.x = G.hz.from = G.hz.to = x;
    G.hz.alpha = 1;
    G.hz.faceLeft = faceLeft;
    G.hz.f.set = spr_get("hanzo");
    fighter_idle(&G.hz.f);
}

/* O golpe que mata oboro: clarão, o traço de corte, e ele cai (o fim da DEATH, de joelhos até o chão). */
static void oboro_dies(void) {
    Rig *b = &G.boss;
    Vector2 at = {b->x + b->offsetX, GROUND_LOW - 16};
    const SprAnim *d = fa(&G.bossS, "DEATH");
    if (d) {
        f_clear(&G.bossS, false);
        f_add(&G.bossS, d, d->frames * 2 / 3, d->frames - 1, 1.4f);
    } else {
        rig_pose(b, POSE_FALLEN, 0.8f, EASE_OUT);
    }
    fx_flash(&G.fx, WHITE, 0.8f);
    fx_kick(&G.fx, 3, 0.25f);
    fx_burst(&G.fx, P_SHARD, at, 16, 70, 1.2f, -2.4f, (Color){130, 18, 26, 255}, (Color){60, 8, 12, 255});
    G.slash = 0.35f;
    G.duo = 0.5f;
    G.silence = AJ_SILENCIO_MORTE_OBORO;
    audio_play(SND_BREAK, 0.9f, 0.8f);
}

static void scene_cue(void) {
    Rig *r = &G.ren, *b = &G.boss;
    float t = G.beatTime, bx = b->x + b->offsetX;
    switch (G.beats[G.beatIndex].cue) {
        case CUE_MASK_ON:
            /* um raio, e quando a luz volta ele está de máscara */
            if (crossed(0.5f)) {
                boss_wear(true);
                fighter_idle(&G.bossS);
                G.ctx.lightning = 1;
                G.ctx.bolt = frand(0, 1);
                audio_play(SND_THUNDER, 0.9f, 0.8f);
                audio_play(SND_SEAL, 0.6f, 0.6f);
                fx_flash(&G.fx, (Color){190, 24, 34, 255}, 0.7f);
                fx_kick(&G.fx, 2, 0.3f);
                vfx("197", 7, (Vector2){bx, GROUND_LOW - 30}, true, VFX_BACK | VFX_GLOW, 20);
            }
            break;
        case CUE_MASK_OFF:
            if (crossed(0.6f)) {
                boss_wear(false);
                const SprAnim *d = fa(&G.bossS, "DESARMADO");
                if (d) {
                    f_clear(&G.bossS, false);
                    f_add(&G.bossS, d, d->frames - 1, d->frames - 1, 0.1f);
                }
                G.maskOnGround = true;
                G.maskDrop = 0;
                audio_play(SND_GESTURE, 0.6f, 0.7f);
            }
            if (crossed(0.95f)) audio_play(SND_THUD, 0.4f, 1.5f);
            break;
        case CUE_RAISE: {
            /* dois passos até ele, e a espada sobe */
            float to = bx - 32 - r->x;
            if (crossed(0)) ren_walk(0.6f, false);
            r->offsetX = G.cueFrom + (to - G.cueFrom) * smooth(t / 0.6f);
            const SprAnim *a = fa(&G.renS, "ATTACK_1");    /* a lâmina sobe por cima do ombro */
            if (crossed(0.6f) && a) {
                f_clear(&G.renS, false);
                f_add(&G.renS, a, 0, anim_hold(a), 0.5f);
            }
            break;
        }
        case CUE_KILL: {
            const SprAnim *a = fa(&G.renS, "ATTACK_1");
            if (crossed(0)) {
                if (a) {
                    f_clear(&G.renS, false);
                    f_add(&G.renS, a, anim_hold(a), a->frames - 1, 0.45f);
                }
                audio_play(SND_SWING, 1, 0.8f);
            }
            if (crossed(0.15f)) oboro_dies();
            break;
        }
        case CUE_HANZO_CLAP: {
            /* hanzo vem do dojo, devagar, batendo palmas */
            if (crossed(0)) {
                hanzo_enter(LOW_W + 24, true);
                hanzo_walk(bx + 44, 2.2f);
            }
            int n = (int)floorf((t - 0.3f) / 0.45f), np = (int)floorf((G.beatPrev - 0.3f) / 0.45f);
            if (t > 0.3f && t < 4.2f && n != np) audio_play(SND_CLAP, 0.55f, frand(0.95f, 1.05f));
            if (crossed(1.2f)) fighter_idle(&G.renS);
            break;
        }
        case CUE_HANZO_MASK:
            /* ele vai até a máscara, pega do chão e põe no rosto: um raio, e é o homem que matou o pai de kojiro */
            if (crossed(0)) hanzo_walk(bx + 18, 0.8f);
            if (crossed(1.0f)) {
                G.maskOnGround = false;
                const SprSet *m = spr_get("hanzo_mascara");
                if (m) G.hz.f.set = m;
                fighter_idle(&G.hz.f);
                G.ctx.lightning = 1;
                G.ctx.bolt = frand(0, 1);
                audio_play(SND_THUNDER, 0.8f, 0.8f);
                audio_play(SND_SEAL, 0.5f, 0.5f);
                fx_flash(&G.fx, (Color){190, 24, 34, 255}, 0.6f);
            }
            break;
        case CUE_LOWER:
            /* a espada desce; ele vira as costas e vai embora */
            if (crossed(0)) fighter_idle(&G.renS);
            if (crossed(0.7f)) {
                r->faceLeft = true;
                ren_walk(1.6f, false);
            }
            if (t > 0.7f) r->offsetX = G.cueFrom + (56 - r->x - G.cueFrom) * smooth((t - 0.7f) / 1.6f);
            if (crossed(2.3f)) fighter_idle(&G.renS);
            break;
        case CUE_HANZO_IN:
            /* hanzo sai do escuro atrás de oboro, com um raio; kojiro vira */
            if (crossed(0)) {
                hanzo_enter(bx + 46, true);
                G.ctx.lightning = 1;
                G.ctx.bolt = frand(0, 1);
                audio_play(SND_THUNDER, 0.7f, 0.9f);
            }
            G.hz.alpha = clampf(t / 0.9f, 0, 1);
            if (crossed(0.6f)) r->faceLeft = false;
            break;
        case CUE_HANZO_KILL:
            /* ele some e aparece do lado de oboro com a katana que era dele: um corte só */
            if (crossed(0.2f)) G.hz.alpha = 0;
            if (crossed(0.45f)) {
                G.hz.x = bx + 16;
                G.hz.alpha = 1;
                G.sword.active = false;
                oboro_dies();
            }
            break;
        case CUE_CHASE: {
            /* kojiro corre atrás dele; hanzo some numa nuvem, e ele para onde hanzo estava */
            float to = G.hz.x - 18 - r->x;
            if (crossed(0)) ren_walk(0.9f, true);
            r->offsetX = G.cueFrom + (to - G.cueFrom) * smooth(t / 0.9f);
            if (crossed(0.4f) && G.hz.on) {
                Vector2 at = {G.hz.x, GROUND_LOW - 16};
                fx_burst(&G.fx, P_DUST, at, 26, 40, 3.14f, -1.57f, (Color){170, 164, 170, 200}, (Color){90, 86, 96, 150});
                dust(G.hz.x, false, 20);
                audio_play(SND_GESTURE, 0.8f, 0.6f);
                G.hz.on = false;
            }
            if (crossed(0.9f)) fighter_idle(&G.renS);
            break;
        }
        default: break;
    }
}

/* Os três em cena, sem o duelo: poses, pranchas, a espada cravada, a máscara caindo. */
static void scene_actors(float dt) {
    rig_update(&G.ren, dt);
    rig_update(&G.boss, dt);
    f_update(&G.renS, dt);
    f_update(&G.bossS, dt);
    update_sword(dt);
    if (G.hz.on) {
        if (G.hz.t < G.hz.len) {
            G.hz.t = fminf(G.hz.len, G.hz.t + dt);
            G.hz.x = G.hz.from + (G.hz.to - G.hz.from) * smooth(G.hz.t / G.hz.len);
        }
        f_update(&G.hz.f, dt);
    }
    if (G.maskOnGround && G.maskDrop < 1) G.maskDrop = fminf(1, G.maskDrop + dt / 0.35f);
}

static void scene_done(void) {
    switch (G.sceneId) {
        case SCENE_SEAL_1:
        case SCENE_SEAL_2:
            /* de volta ao duelo: o grito vem agora, com tempo de acabar antes do próximo golpe */
            G.duel.phaseEnd = fmax(G.duel.phaseEnd, G.duel.clock + AJ_PAUSA_APOS_CENA_SELO);
            set_state(ST_DUEL);
            banner(duel_stance(&G.duel)->name, G.masked ? VERMILION : AGED_GOLD);
            break;
        case SCENE_KNEEL:
            /* a música corta; fica só o vento */
            marco_de_teste("escolha_final");
            G.choice = -1;
            G.windOnly = true;
            set_state(ST_CHOICE);
            audio_music(MUSIC_WIND);
            break;
        default:
            registra_vitoria();
            set_state(ST_ENDING);
            break;
    }
}

static void next_beat(void) {
    G.beatIndex++;
    G.beatTime = G.beatPrev = 0;
    G.typeChars = 0;
    G.cueFrom = G.ren.offsetX;
    if (G.beatIndex >= G.beatCount) scene_done();
}

static void start_scene(SceneId id) {
    G.sceneId = id;
    G.beats = story_scene(id, &G.beatCount);
    G.beatIndex = -1;
    set_state(ST_SCENE);
    next_beat();
}

static void update_scene(float dt) {
    audio_music_duck(G.silence > 0 ? 1 : 0.5f);
    G.beatPrev = G.beatTime;
    G.beatTime += dt;
    scene_cue();
    scene_actors(dt);
    const Beat *b = &G.beats[G.beatIndex];
    float lead = cue_len(b->cue);
    if (G.beatTime < lead) return;
    if (!b->text) { next_beat(); return; }
    int len = (int)strlen(b->text);
    float before = G.typeChars;
    G.typeChars += dt * 50;
    if ((int)G.typeChars / 3 != (int)before / 3 && G.typeChars < len) audio_play(SND_TYPE, 0.5f, strcmp(b->speaker, "kojiro") ? 0.8f : 1.1f);
    if (!pressed() || G.beatTime - lead < AJ_TRAVA_CLIQUE_FALA) return;
    if (G.typeChars < len) { G.typeChars = (float)len; return; }
    audio_play(SND_UI, 0.8f, 1);
    next_beat();
}

/* A escolha: sem nada marcado e sem tempo. */
static const Rectangle CHOICE_BOX[2] = {{UI_W / 2.0f - 300, 420, 240, 96}, {UI_W / 2.0f + 60, 420, 240, 96}};

static void update_choice(float dt) {
    scene_actors(dt);
    if (G.stateTime < AJ_ESCOLHA_TRAVA) return;
    int was = G.choice;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) G.choice = 0;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) G.choice = 1;
    Vector2 v = mouse_ui();
    int hover = -1;
    for (int i = 0; i < 2; i++)
        if (CheckCollisionPointRec(v, CHOICE_BOX[i])) hover = i;
    if (hover >= 0 && (GetMouseDelta().x != 0 || GetMouseDelta().y != 0)) G.choice = hover;
    bool confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_J);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hover >= 0) { G.choice = hover; confirm = true; }
    if ((G.demo || G.autoJogo) && G.stateTime > AJ_AUTO_ESCOLHA_ESCOLHE) { G.choice = G.demoChoice; confirm = G.stateTime > AJ_AUTO_ESCOLHA_CONFIRMA; }
    if (G.choice != was) audio_play(SND_UI, 0.8f, 1);
    if (!confirm || G.choice < 0) return;
    audio_play(SND_UI, 1, 0.7f);
    G.ending = G.choice == 0 ? SCENE_SIM : SCENE_NAO;
    start_scene(G.ending);
}

static void update_ending(float dt) {
    scene_actors(dt);
    if (G.stateTime > AJ_FINAL_TRAVA && pressed()) {
        G.hasSave = true;
        G.menuIndex = 0;
        set_state(ST_TITLE);
        audio_music(MUSIC_TITLE);
    }
}

/* ------------------------------------------------------------------ */
/* Mundo (320 x 180)                                                   */
/* ------------------------------------------------------------------ */

/* Câmera 2D que desenha as coordenadas do mundo (320 x 180) na textura grande. */
/* Câmera dos lutadores: pixel art de 320 x 180, sem ampliar. */
static void begin_actors(void) {
    Camera2D cam = {0};
    cam.zoom = 1;
    BeginMode2D(cam);
}

static void begin_world(Vector2 offset) {
    Camera2D cam = {0};
    cam.offset = (Vector2){offset.x * RS, offset.y * RS};
    cam.zoom = RS;
    BeginMode2D(cam);
}

/* O mesmo contorno escuro e o filete de luz dos bonecos, em volta do sprite. */
/* Quanto o tronco desce (a respiração): quem não tem prancha parada respira, mais rápido cansado. */
static int sprite_breath(const Rig *r, const Fighter *f) {
    const SprAnim *a = f->pl.anim;
    int breath = 0;
    if (f->idle && !a->loop) {
        float period = 1.8f - 0.8f * r->fatigue;
        breath = fmodf(r->time, period) > period * 0.5f ? 1 : 0;
    }
    return f->squat > breath ? f->squat : breath;
}

/* `apagar`: 0 = sob a luz, 1 = no escuro (a silhueta de um mestre no apagão), e entre os dois a passagem, sem salto.
 * `corpoApagado`: a opacidade que a silhueta guarda no escuro total (1 = a de sempre; o yoru some quase todo, e só as adagas ficam). `opac`: 1 = inteiro. */
static void draw_sprite_fighter(const Rig *r, const Fighter *f, Color light, Color rim, float apagar, float corpoApagado, float opac) {
    const SprAnim *a = f->pl.anim;
    if (!a) return;
    int frame = f->pl.frame;
    Vector2 feet = {r->x + r->offsetX, r->y - r->hopY};
    int breath = sprite_breath(r, f);
    const float aceso = (1 - clampf(apagar, 0, 1)) * opac, escuro = clampf(apagar, 0, 1) * corpoApagado * opac;   /* `opac`: o corvo se desfazendo em penas */
    SprDraw o = {r->faceLeft, breath, true, fadec((Color){16, 12, 18, 255}, aceso + escuro)};
    static const int off[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int k = 0; k < 4; k++) spr_draw(f->set, a, frame, (Vector2){feet.x + off[k][0], feet.y + off[k][1]}, o);
    if (escuro > 0.001f) {
        o.color = fadec((Color){24, 20, 36, 255}, escuro);
        spr_draw(f->set, a, frame, feet, o);
    }
    if (aceso <= 0.001f) return;
    o.color = fadec(rim, aceso);
    spr_draw(f->set, a, frame, (Vector2){feet.x + (r->faceLeft ? 1 : -1), feet.y - 1}, o);
    o.flat = false;
    o.color = fadec(light, aceso);
    spr_draw(f->set, a, frame, feet, o);
    if (r->flash > 0) {
        o.flat = true;
        o.color = fadec(r->flashColor, fminf(1, r->flash) * 0.6f * aceso);
        spr_draw(f->set, a, frame, feet, o);
    }
}

/* O apagão do yoru: o corpo some por inteiro e ficam só as duas adagas, nas cores delas (o aço da própria prancha, em spr_lamina), sem brilho nem raio em
 * volta; `k` é o apagão, de 0 a 1. */
static void desenha_laminas_acesas(float k) {
    const Fighter *f = &G.bossS;
    const SprAnim *a = f->pl.anim;
    if (!a || !f->set) return;
    const SprAnim *lamina = spr_lamina(f->set, a);
    if (!lamina) return;
    const Rig *r = &G.boss;
    Vector2 feet = {r->x + r->offsetX, r->y - r->hopY};
    SprDraw o = {r->faceLeft, sprite_breath(r, f), false, fadec(WHITE, k)};
    spr_draw(f->set, lamina, f->pl.frame, feet, o);
}

/* O choque do relâmpago no kojiro: a silhueta pisca em azul e branco por cima do corpo, e some nos últimos 0,25 s. */
static void desenha_choque(const Rig *r, const Fighter *f) {
    const SprAnim *a = f->pl.anim;
    if (!a) return;
    const float fim = clampf(G.paralisia / 0.25f, 0, 1);
    const Color cor = ((int)(G.time * 30) & 1) ? (Color){235, 245, 255, 255} : (Color){110, 160, 255, 255};
    SprDraw o = {r->faceLeft, sprite_breath(r, f), true, fadec(cor, 0.55f * G.paralisiaForca * fim)};
    spr_draw(f->set, a, f->pl.frame, (Vector2){r->x + r->offsetX, r->y - r->hopY}, o);
}

/* Os raios do relâmpago: do alto da tela ao chão, em zigue-zague (a forma é fixa por raio, não muda de quadro a quadro), com um clarão onde caem. */
static void desenha_raios(void) {
    for (int i = 0; i < RAIOS_MAX; i++) {
        const Raio *r = &G.raios[i];
        if (r->espera > 0 || r->vida <= 0) continue;
        const float k = r->vida / RAIO_VIDA;                                  /* 1 ao cair, 0 ao apagar */
        if (k < 0.55f && ((int)(r->vida * 60) % 3) == 0) continue;            /* perto do fim ele falha, como o raio de verdade */
        enum { N = 9 };
        Vector2 pt[N + 1];
        unsigned s = r->semente;
        pt[0] = (Vector2){r->x + 5, -2};
        for (int j = 1; j <= N; j++) {
            s = s * 1664525u + 1013904223u;
            pt[j] = (Vector2){r->x + (j == N ? 0 : (float)((int)((s >> 16) % 13) - 6)), floorf((float)j * GROUND_LOW / N)};
        }
        s = s * 1664525u + 1013904223u;
        const float lado = (s >> 16) & 1 ? 1.0f : -1.0f;
        const float corpo = r->principal ? 2.0f : 1.0f;                       /* o raio que cai em quem aparou ou apanhou é o mais grosso */
        for (int j = 1; j <= N; j++) DrawLineEx(pt[j - 1], pt[j], corpo + 2, fadec((Color){110, 160, 255, 255}, 0.34f * k));  /* o brilho em volta */
        if (r->principal) DrawLineEx(pt[4], (Vector2){pt[4].x + lado * 15, pt[4].y + 20}, 1, fadec((Color){170, 205, 255, 255}, 0.8f * k));   /* o ramo */
        for (int j = 1; j <= N; j++) DrawLineEx(pt[j - 1], pt[j], corpo, fadec((Color){240, 247, 255, 255}, k));          /* o fio */
        DrawEllipse((int)r->x, GROUND_LOW, 4 + 7 * k, 1.5f + k, fadec((Color){225, 238, 255, 255}, 0.7f * k));
    }
}

/* A máscara de oni que oboro tirou, caída do lado dele. */
static void draw_oni_mask(Color light) {
    static const char *M[] = {".a...a.", "aakkkaa", "akpkpka", "akkkkka", "arkkkra", ".akkka."};
    float x = G.boss.x + G.boss.offsetX + 12, y = GROUND_LOW - 6 - (1 - G.maskDrop * G.maskDrop) * 22;
    for (int j = 0; j < 6; j++)
        for (int i = 0; M[j][i]; i++) {
            char k = M[j][i];
            if (k == '.') continue;
            Color c = k == 'a' ? (Color){137, 30, 43, 255} : k == 'k' ? (Color){196, 36, 48, 255}
                    : k == 'p' ? (Color){255, 200, 37, 255} : (Color){255, 251, 232, 255};
            c = (Color){(unsigned char)(c.r * light.r / 255), (unsigned char)(c.g * light.g / 255), (unsigned char)(c.b * light.b / 255), 255};
            DrawRectangle((int)x + i, (int)y + j, 1, 1, c);
        }
}

/* Hanzo nos finais: o mesmo contorno dos lutadores, e some ou aparece pelo alfa. */
static void draw_hanzo(Color light, Color rim) {
    if (!G.hz.on || !G.hz.f.set || !G.hz.f.pl.anim || G.hz.alpha <= 0.01f) return;
    const SprAnim *a = G.hz.f.pl.anim;
    int frame = G.hz.f.pl.frame;
    Vector2 feet = {G.hz.x, GROUND_LOW};
    float al = G.hz.alpha;
    SprDraw o = {G.hz.faceLeft, 0, true, fadec((Color){16, 12, 18, 255}, al)};
    static const int off[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int k = 0; k < 4; k++) spr_draw(G.hz.f.set, a, frame, (Vector2){feet.x + off[k][0], feet.y + off[k][1]}, o);
    o.color = fadec(rim, al);
    spr_draw(G.hz.f.set, a, frame, (Vector2){feet.x + (G.hz.faceLeft ? 1 : -1), feet.y - 1}, o);
    o.flat = false;
    o.color = fadec(light, al);
    spr_draw(G.hz.f.set, a, frame, feet, o);
}

static void draw_rigs(Color light) {
    BeginTextureMode(G.actors);
    ClearBackground(BLANK);
    bool dark = G.ctx.blackout > 0.5f;
    Color rim = G.m->isBigBoss && G.auraLeft > 0 ? posture_color(G.auraEcho) : arena_rim(G.m->arena);
    begin_actors();
    if (G.rastro && G.bossS.set && G.m->id != 10 && !dark && !G.bossHidden) {
        /* As silhuetas da corrida usam a mesma cor que o golpe e somem com a chave. */
        Color c = cor_rastro();
        if (G.m->id == 6 && G.leap == LEAP_WARP) c = (Color){50, 39, 64, 255};
        for (int n = 1; n <= AFTER_MAX; n++) {       /* da mais antiga para a mais nova */
            int i = (G.afterHead + n) % AFTER_MAX;
            if (G.after[i].life <= 0 || !G.after[i].a) continue;
            SprDraw o = {G.boss.faceLeft, 0, true, fadec(c, 0.45f * G.after[i].life)};
            spr_draw(G.bossS.set, G.after[i].a, G.after[i].frame, G.after[i].feet, o);
        }
    }
    bool rastroGolpe = G.rastro && G.bossS.set && G.bossS.pl.anim && !G.bossHidden && G.state == ST_DUEL;
    if (rastroGolpe) desenha_rastro_do_golpe(dark, false);
    if (G.bossHidden) {
        /* sumiu em penas */
    } else if (G.bossS.set) {
        const bool yoru = G.m->id == 11;
        /* o yoru apaga com o apagão (sem o salto dos outros em 50%) e some por inteiro, só as adagas ficam; os outros mestres viram a silhueta de sempre */
        draw_sprite_fighter(&G.boss, &G.bossS, light, rim, yoru ? G.ctx.blackout : (dark ? 1.0f : 0.0f), yoru ? 0.0f : 1.0f, 1 - G.bossDissolve);
        if (yoru && G.ctx.blackout > 0.02f) desenha_laminas_acesas(clampf(G.ctx.blackout, 0, 1));
    }
    if (G.maskOnGround) draw_oni_mask(light);
    if (rastroGolpe) desenha_rastro_do_golpe(dark, true);
    draw_hanzo(light, rim);
    if (G.renS.set) {
        Rig ren = G.ren;
        if (G.paralisia > 0) ren.offsetX += ((int)(G.time * 28) & 1) ? 1.0f : -1.0f;       /* o choque faz tremer (1 px) */
        draw_sprite_fighter(&ren, &G.renS, light, rim, 0.0f, 1.0f, 1.0f);
        if (G.paralisia > 0) desenha_choque(&ren, &G.renS);
    }
    draw_pole_flying();
    if (G.crack > 0) {
        /* Rachadura branca atravessando o mestre de cima a baixo. */
        float x = G.boss.x + G.boss.offsetX, top = GROUND_LOW - 56 * G.boss.look.size;
        if (G.bossS.set) top = GROUND_LOW - G.bossS.set->height - 4;
        Vector2 prev = {x - 4, top};
        for (int k = 1; k <= 7; k++) {
            Vector2 p = {x + ((k % 2) ? 4.0f : -4.0f) + (k * 7 % 3) - 1, top + k * (GROUND_LOW - 4 - top) / 7};
            DrawLineEx(prev, p, 1.4f, (Color){255, 255, 255, 240});
            prev = p;
        }
    }
    EndMode2D();
    if (katana3d_ready()) {
        katana3d_begin(LOW_W, LOW_H);
        Vector2 butt, tip;
        /* Quem olha para a direita segura com meia volta: fio para baixo, ponta subindo. */
        KatanaStyle bs = katana_style(&G.boss.look), rs = katana_style(&G.ren.look);
        Color bossLight = dark ? (Color){40, 34, 54, 255} : light;
        if (!G.boss.noSword && G.boss.look.weapon == WEAPON_KATANA && !G.bossS.set) {
            rig_sword_line(&G.boss, &butt, &tip);
            katana3d_draw(butt, tip, G.boss.faceLeft ? 0 : 180, &bs, bossLight);
        }
        if (!G.bossS.set && rig_offhand_line(&G.boss, &butt, &tip)) {
            KatanaStyle os = bs;
            os.width *= 0.9f;
            katana3d_draw(butt, tip, G.boss.faceLeft ? 0 : 180, &os, bossLight);
        }
        if (!G.renS.set) {
            rig_sword_line(&G.ren, &butt, &tip);
            katana3d_draw(butt, tip, G.ren.faceLeft ? 0 : 180, &rs, light);
        }
        draw_sword_3d(light);
        katana3d_end();
    } else {
        begin_actors();
        draw_sword();
        EndMode2D();
    }
    EndTextureMode();
}

static void draw_shadow(const Rig *r) {
    float w = 11 * r->look.size * (1 - clampf(r->hopY / 20, 0, 0.6f));
    DrawEllipse((int)(r->x + r->offsetX), GROUND_LOW, w, 2, (Color){0, 0, 0, (unsigned char)(80 * (r == &G.boss ? 1 - G.bossDissolve : 1))});
}

static void draw_arena(void) {
    const MasterProfile *m = G.m;
    Color light = arena_light(m->arena, &G.ctx);
    draw_rigs(light);

    /* O fundo passa pela paleta curta do cenário, com dithering (pixelize.c). */
    Vector2 sh = fx_shake_offset(&G.fx);
    pix_capture_begin();
    begin_world(sh);
    arena_draw_back(m->arena, &G.ctx);
    EndMode2D();
    pix_capture_end((int)m->arena);

    BeginTextureMode(G.scene);
    ClearBackground(BLACK);
    pix_draw();
    begin_world(sh);
    /* Reflexo no chão polido: a camada dos lutadores espelhada no chão. */
    float refl = arena_reflection(m->arena);
    if (refl > 0) {
        BeginScissorMode(0, GROUND_LOW * RS, RW, (LOW_H - GROUND_LOW) * RS);
        DrawTexturePro(G.actors.texture, (Rectangle){0, 0, LOW_W, LOW_H}, (Rectangle){0, 2 * GROUND_LOW - LOW_H, LOW_W, LOW_H},
                       (Vector2){0, 0}, 0, fadec(WHITE, refl));
        EndScissorMode();
    }
    if (!G.bossHidden) draw_shadow(&G.boss);
    draw_shadow(&G.ren);
    if (G.hz.on && G.hz.alpha > 0.5f) DrawEllipse((int)G.hz.x, GROUND_LOW, 11, 2, (Color){0, 0, 0, 80});
    vfx_draw(true);
    DrawTexturePro(G.actors.texture, (Rectangle){0, 0, LOW_W, -LOW_H}, (Rectangle){0, 0, LOW_W, LOW_H}, (Vector2){0, 0}, 0, WHITE);
    fx_draw_world(&G.fx);
    vfx_draw(false);
    desenha_raios();
    arena_draw_front(m->arena, &G.ctx);
    EndMode2D();
    /* Vida de kojiro no fim: a borda pulsa. */
    if (G.state == ST_DUEL && G.duel.renPosture <= G.settings.renPosture * 0.25f) {
        float p = 0.5f + 0.5f * sinf(G.time * 7);
        for (int i = 0; i < 4; i++)
            DrawRectangleLinesEx((Rectangle){i * 6.0f, i * 6.0f, RW - i * 12.0f, RH - i * 12.0f}, 6, fadec((Color){170, 0, 0, 255}, (0.4f - i * 0.09f) * p));
    }
    fx_draw_flash(&G.fx, RW, RH);
    EndTextureMode();
}

/* As ilustrações sobem `lift` px para o chão ficar acima da caixa de texto de baixo. */
static void draw_illustration(void (*fn)(int, float), int page, float t, float lift) {
    BeginTextureMode(G.scene);
    ClearBackground((Color){20, 16, 14, 255});
    begin_world((Vector2){0, -lift});
    fn(page, t);
    EndMode2D();
    EndTextureMode();
}

static void cabin_scene(int page, float t) { (void)page; lore_draw_cabin(t); }

/* Uma paleta para a silhueta e a lâmina. Oboro toma a cor do eco que está usando. */
static int mestre_do_rastro(void) {
    /* na abertura da luta final o duelo ainda não foi iniciado (G.duel.m nulo): sem golpe, sem eco */
    int eco = G.m->isBigBoss && G.duel.m == G.m ? echo_of(duel_move(&G.duel)) : -1;
    return eco >= 0 ? eco : G.m->id - 1;
}

static Color cor_rastro(void) {
    static const Color COR[ROSTER_SIZE] = {
        {218, 178, 116, 255}, /* daichi: areia */
        {64, 122, 83, 255},   /* genbu: verde escuro */
        {207, 175, 93, 255},  /* raizo: pedra ocre */
        {170, 229, 252, 255}, /* shizuku: gelo */
        {249, 139, 62, 255},  /* garfiel: laranja */
        {91, 94, 105, 255},   /* karasu: preto com fio cinza */
        {136, 230, 167, 255}, /* hayate: verde claro */
        {237, 77, 52, 255},   /* enjin: brasa */
        {51, 106, 191, 255},  /* suiren: azul escuro */
        {89, 160, 251, 255},  /* arashi: raio azul */
        {164, 99, 203, 255},  /* yoru: roxo */
        {238, 239, 249, 255}, /* jinshi: branco */
        {181, 83, 211, 255},  /* oboro sem eco */
    };
    return COR[mestre_do_rastro()];
}

/* O traço acompanha o ponto da arma anotado para cada quadro do PNG Mattz.
 * Uma estocada fica longa e fina; um corte faz uma curva curta; garras viram
 * três riscos. É apenas desenho, sem criar contato nem mudar o sprite. */
static void fio_do_golpe(Vector2 arma, MoveLook look, float progresso, float comprimento, float largura, Color cor) {
    float direcao = G.boss.faceLeft ? -1.0f : 1.0f;
    Vector2 inicio, fim;
    if (look == LOOK_THRUST || look == LOOK_DASH || look == LOOK_FAR) {
        inicio = (Vector2){arma.x - direcao * comprimento * progresso, arma.y};
        fim = (Vector2){arma.x + direcao * comprimento * 0.25f * progresso, arma.y};
    } else {
        float altura = look == LOOK_LOW ? 1.0f : -1.0f;
        inicio = (Vector2){arma.x - direcao * comprimento * 0.62f * progresso,
                           arma.y + altura * comprimento * 0.38f * progresso};
        fim = (Vector2){arma.x + direcao * comprimento * 0.28f * progresso,
                        arma.y - altura * comprimento * 0.50f * progresso};
    }
    DrawLineEx(inicio, arma, largura, cor);
    DrawLineEx(arma, fim, fmaxf(1.0f, largura * 0.65f), fadec(cor, 0.72f));
}

/* Silhueta do quadro que está na tela e fio da arma. Tudo some no contato, que
 * continua saindo no instante do núcleo. No apagão do Yoru não sai nada: só as
 * adagas aparecem. */
static void desenha_rastro_do_golpe(bool escuro, bool so_fio) {
    float p = duel_launch_progress(&G.duel);
    if (p <= 0) return;
    if (escuro && G.m->id == 11) return;      /* no apagão do yoru só as adagas aparecem: nem o fio do golpe */
    int mestre = mestre_do_rastro();
    Color c = cor_rastro();
    Vector2 pes = {G.boss.x + G.boss.offsetX, G.boss.y - G.boss.hopY};
    float para_tras = G.boss.faceLeft ? 1.0f : -1.0f;
    if (!so_fio && !escuro) {
        for (int k = AJ_RASTRO_FANTASMAS; k >= 1; k--) {
            float fim = 1.0f - (float)(k - 1) / (float)AJ_RASTRO_FANTASMAS;
            SprDraw o = {G.boss.faceLeft, 0, true, fadec(c, AJ_RASTRO_ALFA * fim * p)};
            Vector2 at = {pes.x + para_tras * AJ_RASTRO_ESPACO * (float)k * p, pes.y};
            spr_draw(G.bossS.set, G.bossS.pl.anim, G.bossS.pl.frame, at, o);
            G.fantasmasDesenhados++;
        }
    }
    if (!so_fio) return;
    Vector2 empunhadura, lamina;
    boss_blade(&empunhadura, &lamina);
    (void)empunhadura;
    static const struct { float comprimento, largura; int riscos; bool duas; } ESTILO[ROSTER_SIZE] = {
        {15, 2, 1, false}, {14, 2, 1, false}, {22, 3, 1, false},
        {21, 1, 1, false}, {9, 1, 3, true}, {15, 2, 1, true},
        {11, 2, 1, true}, {17, 3, 1, false}, {23, 1, 1, false},
        {17, 2, 1, true}, {10, 2, 1, true}, {19, 2, 1, false},
        {17, 2, 1, false},
    };
    Color fio = fadec(c, AJ_RASTRO_FIO_ALFA * p * (escuro ? 0.38f : 1.0f));
    MoveLook look = strike_look();
    int riscos = ESTILO[mestre].riscos;
    for (int k = 0; k < riscos; k++) {
        Vector2 centro = {lamina.x, lamina.y + (k - (riscos - 1) * 0.5f) * 3.0f};
        fio_do_golpe(centro, look, p, ESTILO[mestre].comprimento, ESTILO[mestre].largura, fio);
    }
    if (ESTILO[mestre].duas || duel_strike_dual(&G.duel)) {
        Vector2 outra = {lamina.x + para_tras * 7, lamina.y - 5};
        spr_offhand_point(&G.bossS.pl, pes, G.boss.faceLeft, G.bossS.squat, &outra);
        fio_do_golpe(outra, look, p, ESTILO[mestre].comprimento * 0.78f,
                     ESTILO[mestre].largura, fio);
    }
    if (mestre == 5) { /* o fio escuro do karasu precisa aparecer sobre o telhado */
        fio_do_golpe(lamina, look, p, ESTILO[mestre].comprimento, 1,
                     fadec((Color){193, 197, 209, 255}, 0.48f * p));
    }
    if (mestre == 9) { /* a descarga segue ambas as lâminas */
        Vector2 raio = {lamina.x + para_tras * 5, lamina.y - 4};
        DrawLineEx(lamina, raio, 1, fadec((Color){193, 232, 255, 255}, 0.72f * p));
    }
}

static void draw_world(void) {
    switch (G.state) {
        case ST_TITLE:
            pix_capture_begin();
            begin_world((Vector2){0, 0});
            lore_draw_title(G.time);
            EndMode2D();
            pix_capture_end(PIX_TITLE);
            BeginTextureMode(G.scene);
            ClearBackground(BLACK);
            pix_draw();
            /* Kojiro no morro, olhando para o dojo de Hanzo no pico. */
            begin_world((Vector2){0, 0});
            lore_draw_title_hero(G.time);
            EndMode2D();
            EndTextureMode();
            break;
        case ST_LORE:
        case ST_CALIBRA:
            pix_capture_begin();
            begin_world((Vector2){0, 0});
            lore_draw_title(G.time);
            DrawRectangle(0, 0, LOW_W, LOW_H, (Color){10, 6, 8, 110});
            EndMode2D();
            pix_capture_end(PIX_LORE);
            BeginTextureMode(G.scene);
            ClearBackground(BLACK);
            pix_draw();
            EndTextureMode();
            break;
        case ST_SENSEI:
        case ST_VISIT: draw_illustration(cabin_scene, 0, G.time, 18); break;   /* a cabana de hanzo */
        case ST_TRAIL:
            pix_capture_begin();
            begin_world((Vector2){0, 0});
            lore_draw_trail(&G.camp, G.time, G.camp.index);
            EndMode2D();
            pix_capture_end(PIX_TRAIL);
            BeginTextureMode(G.scene);
            ClearBackground(BLACK);
            pix_draw();
            EndTextureMode();
            break;
        default: draw_arena(); break;
    }
}

/* ------------------------------------------------------------------ */
/* Interface (coordenadas de 1280 x 720, pixels de 320 x 180)         */
/* ------------------------------------------------------------------ */

/* Nome, descrição e medidor têm linhas próprias. As fases de Oboro não são anunciadas. */
static void ui_status(Rectangle r, const char *name, const char *note, const char *gauge, float value, float ghost, float max, Color a, Color b) {
    parchment(r, 0.96f);
    bool life = !strcmp(gauge, "vida");
    const float pad = 36;
    ink_bold(name, r.x + pad, r.y + 24, 28, INK_TEXT);
    if (note && note[0]) {
        if (life) ink_right(note, r.x + r.width - pad, r.y + 28, 18, INK_SOFT);
        else ink(note, r.x + pad, r.y + 60, 20, INK_SOFT);
    }
    float gy = r.y + (life ? 68 : 100);
    ink(gauge, r.x + pad, gy - 4, 18, INK_SOFT);
    float gx = r.x + pad + ui_width(gauge, 18) + 20;
    ui_gauge(gx, gy, r.x + r.width - pad - gx, value, ghost, max, a, b);
}

static void ui_hud(void) {
    const MasterProfile *m = G.m;
    bool pressure = duel_under_pressure(&G.duel) && m->sealCount <= 1;
    const char *name = lower(m->name), *note = lower(m->sealCount > 1 ? duel_stance(&G.duel)->name : m->style);
    /* vantagem: um perfeito agora quebra a postura (e é a execução) */
    bool vantagem = G.duel.advantage && G.state == ST_DUEL;
    if (vantagem) note = "vantagem: um perfeito quebra";
    float brilho = vantagem ? 0.5f + 0.5f * sinf(G.time * 10) : 0;
    float need = fmaxf(ui_width_f(G.uiBold, name, 28), ui_width(note, 20)) + 80;
    float tw = snap(fmaxf(600, need));
    Rectangle top = {snap(UI_W / 2 - tw / 2), 16, tw, 132};
    Color barA = pressure ? (Color){150, 40, 30, 255} : (Color){78, 62, 104, 255};
    Color barB = pressure ? (Color){200, 80, 50, 255} : (Color){134, 108, 160, 255};
    if (vantagem) {
        barA = (Color){(unsigned char)(170 + 60 * brilho), 130, 30, 255};
        barB = (Color){(unsigned char)(220 + 35 * brilho), 190, 70, 255};
    }
    ui_status(top, name, note, "postura", G.shownBoss, G.ghostBoss, duel_posture_max(&G.duel), barA, barB);
    Rectangle bot = {UI_W / 2 - 300, UI_H - 124, 600, 108};
    bool low = G.shownRen <= G.settings.renPosture * 0.25f;
    /* kojiro não tem postura: tem vida, em vermelho, que pulsa quando está no fim */
    float pulse = low ? 0.5f + 0.5f * sinf(G.time * 8) : 0;
    ui_status(bot, "kojiro", G.duel.burnLeft > 0 ? "em brasas" : NULL, "vida", G.shownRen, G.ghostRen, G.settings.renPosture,
              (Color){(unsigned char)(140 + 40 * pulse), 26, 30, 255}, (Color){(unsigned char)(212 + 30 * pulse), 62, 56, 255});

    if (G.bannerTime > 0) {
        float a = clampf(G.bannerTime * 2, 0, 1);
        const char *t = lower(G.banner);
        float w = ui_width_f(G.uiBold, t, 34) + 100;
        Rectangle r = {UI_W / 2 - w / 2, 164, w, 64};      /* abaixo da placa do mestre, acima dos saltos */
        parchment(r, a);
        scroll_rods(r, a);
        ink_bold_center(t, UI_W / 2.0f, r.y + 14, 34, fadec(INK_TEXT, a));
    }
}

/* Falas. No cenário do duelo a caixa fica em cima, para os lutadores aparecerem
 * inteiros; nas ilustrações (hanzo), embaixo. */
static void ui_say(const char *speaker, const char *text, bool top) {
    bool ren = strcmp(speaker, "kojiro") == 0;
    Rectangle r = top ? (Rectangle){80, 76, UI_W - 160, 168} : (Rectangle){80, 512, UI_W - 160, 180};
    parchment(r, 1);
    scroll_rods(r, 1);
    /* Caixa do nome separada, como no RPG Maker. */
    const char *who = lower(speaker);
    float nw = ui_width_f(G.uiBold, who, 26) + 44;
    Rectangle nb = {r.x + 24, r.y - 34, nw, 46};
    parchment(nb, 1);
    ink_bold(who, nb.x + 22, nb.y + 10, 26, ren ? (Color){170, 70, 24, 255} : SEAL_RED);
    int vis = utf8_visible(text, G.typeChars);
    ink_wrapped(text, r.x + 36, r.y + 34, r.width - 72, 28, INK_TEXT, vis);
    if (vis >= (int)strlen(text)) {
        float bob = sinf(G.time * 6) * 3;
        Vector2 c = {r.x + r.width - 40, r.y + r.height - 30 + bob};
        DrawTriangle((Vector2){c.x - 8, c.y - 5}, (Vector2){c.x, c.y + 5}, (Vector2){c.x + 8, c.y - 5}, SEAL_RED);
    }
}

static void ui_dialogue(const char *header, bool top) {
    if (top) ui_text(lower(header), UI_W - 48 - ui_width(lower(header), 24), UI_H - 64, 24, (Color){230, 216, 190, 220});
    else ui_text(lower(header), 48, 30, 24, (Color){230, 216, 190, 220});
    if (G.lineIndex < G.lineCount) ui_say(G.lines[G.lineIndex].speaker, G.lines[G.lineIndex].text, top);
}

/* A fala do momento da cena, depois que a ação dele acabou. */
static void ui_beat(void) {
    if (G.beatIndex < 0 || G.beatIndex >= G.beatCount) return;
    const Beat *b = &G.beats[G.beatIndex];
    if (b->text && G.beatTime >= cue_len(b->cue)) ui_say(b->speaker, b->text, true);
}

static void ui_choice(void) {
    float a = clampf(G.stateTime / 1.5f, 0, 1);
    DrawRectangle(0, 0, UI_W, UI_H, fadec((Color){6, 4, 8, 255}, 0.62f * a));
    const char *q = "DESEJA MATAR O OBORO?";
    draw_text_f(G.uiBold, q, UI_W / 2.0f - ui_width_f(G.uiBold, q, 64) / 2, 250, 64, fadec((Color){238, 214, 170, 255}, a), true);
    if (G.stateTime < AJ_ESCOLHA_TRAVA) return;
    float b = clampf((G.stateTime - AJ_ESCOLHA_TRAVA) * 2, 0, 1);
    static const char *OPT[2] = {"SIM", "NÃO"};
    for (int i = 0; i < 2; i++) {
        Rectangle r = CHOICE_BOX[i];
        parchment(r, b);
        if (G.choice == i) ui_cursor((Rectangle){r.x + 8, r.y + 8, r.width - 16, r.height - 16}, b);
        ink_bold_center(OPT[i], r.x + r.width / 2, r.y + 28, 40, fadec(G.choice == i ? SEAL_RED : INK_TEXT, b));
    }
}

/* O fim: a tela escurece e fica uma palavra. */
static void ui_ending(void) {
    float a = clampf(G.stateTime / 2.0f, 0, 1);
    DrawRectangle(0, 0, UI_W, UI_H, fadec(BLACK, a));
    if (G.stateTime < AJ_FINAL_TITULO) return;
    const char *t = G.ending == SCENE_SIM ? "fim" : "continua";
    float b = clampf((G.stateTime - AJ_FINAL_TITULO) / 1.2f, 0, 1);
    draw_text_f(G.uiBold, t, UI_W / 2.0f - ui_width_f(G.uiBold, t, 80) / 2, 300, 80, fadec((Color){238, 214, 170, 255}, b), false);
}

/* Quebra um parágrafo em linhas que cabem em `width`. Devolve quantas. */
static int wrap_lines(const char *t, float size, float width, char lines[][LINHA_MAX], int max) {
    int n = 0;
    char line[LINHA_MAX] = "", trial[JUNTA_MAX];
    while (*t && n < max) {
        int wl = 0;
        while (t[wl] && t[wl] != ' ' && wl < PALAVRA_MAX - 1) wl++;
        snprintf(trial, sizeof trial, "%s%s%.*s", line, line[0] ? " " : "", wl, t);
        if (line[0] && ui_width(trial, size) > width) {
            snprintf(lines[n++], LINHA_MAX, "%s", line);
            snprintf(line, sizeof line, "%.*s", wl, t);
        } else {
            snprintf(line, sizeof line, "%.*s", (int)sizeof line - 1, trial);
        }
        t += wl;
        while (*t == ' ') t++;
    }
    if (line[0] && n < max) snprintf(lines[n++], LINHA_MAX, "%s", line);
    return n;
}

/* Texto do narrador subindo pela tela, com as bordas esmaecidas. */
static void ui_narration(void) {
    /* Uma coluna alinhada à esquerda, como página de livro, no meio da tela. */
    float size = 26, lh = size * 1.7f, width = 760, x = UI_W / 2 - width / 2;
    DrawRectangleGradientH((int)x - 200, 0, 200, UI_H, fadec(INK, 0), fadec(INK, 0.45f));
    DrawRectangle((int)x, 0, (int)width, UI_H, fadec(INK, 0.45f));
    DrawRectangleGradientH((int)(x + width), 0, 200, UI_H, fadec(INK, 0.45f), fadec(INK, 0));
    float y = UI_H * 0.62f - G.loreScroll;
    char lines[16][LINHA_MAX];
    for (int p = 0; p < LORE_PAGES; p++) {
        int n = wrap_lines(lore_page(p), size, width, lines, 16);
        for (int i = 0; i < n; i++, y += lh) {
            /* Surge embaixo e some no alto. */
            float a = clampf((y - 90) / 160, 0, 1) * clampf((UI_H - 40 - y) / 160, 0, 1);
            bool quote = (unsigned char)lore_page(p)[0] == 0xE2; /* começa com aspas curvas */
            if (a > 0.01f) ui_text(lines[i], x + (quote ? 48 : 0), y, size, fadec(quote ? (Color){232, 196, 120, 255} : (Color){232, 220, 196, 255}, a));
        }
        y += lh * 1.1f;
    }
    G.loreHeight = y + G.loreScroll - UI_H * 0.62f;
    /* Anel de pular: só aparece enquanto Esc está pressionado. */
    if (G.skipHold > 0.02f) {
        Vector2 c = {UI_W - 70, UI_H - 70};
        float k = clampf(G.skipHold / SKIP_HOLD, 0, 1);
        DrawRing(c, 18, 23, 0, 360, 48, (Color){60, 50, 44, 160});
        DrawRing(c, 18, 23, -90, -90 + 360 * k, 48, (Color){210, 180, 130, 230});
        ui_center("esc", c.x, c.y - 11, 18, (Color){200, 186, 160, 200});
    }
}

static double calibra_batida(int k);

static void ui_calibra(void) {
    DrawRectangle(0, 0, UI_W, UI_H, fadec(INK, 0.82f));
    Color claro = {236, 222, 192, 255}, suave = {180, 168, 150, 255};
    ui_center("calibrar o atraso", UI_W / 2.0f, 70, 44, claro);
    char buf[160];
    if (G.cal.modo == 2) {
        snprintf(buf, sizeof buf, "vídeo: %.0f ms      áudio: %.0f ms", G.cal.video * 1000, G.cal.audio * 1000);
        ui_center(buf, UI_W / 2.0f, 260, 34, claro);
        snprintf(buf, sizeof buf, "o aperto conta %.0f ms mais cedo; o som do aviso vem %.0f ms antes do brilho",
                 G.cal.video * 1000, (G.cal.audio - G.cal.video) * 1000);
        ui_center(buf, UI_W / 2.0f, 330, 22, suave);
        ui_center("aperte para salvar      R refaz      Esc cancela", UI_W / 2.0f, 470, 24, claro);
        return;
    }
    ui_center(G.cal.modo == 0 ? "1 de 2: aperte quando o quadrado acender (sem som)" : "2 de 2: aperte junto com o clique (sem imagem)",
              UI_W / 2.0f, 140, 26, claro);
    ui_center("aperte no ritmo, não por reação; as duas primeiras batidas são para pegar o ritmo", UI_W / 2.0f, 180, 20, suave);
    if (G.cal.modo == 0) {
        double desde = G.cal.t - calibra_batida(G.cal.proxima - 1);
        bool aceso = G.cal.proxima > 0 && desde >= 0 && desde < 0.1;
        Rectangle q = {UI_W / 2.0f - 70, 270, 140, 140};
        DrawRectangleRec(q, aceso ? WHITE : (Color){40, 34, 36, 255});
        DrawRectangleLinesEx(q, 3, (Color){120, 110, 100, 255});
    } else {
        double desde = G.cal.t - calibra_batida(G.cal.proxima - 1);
        (void)desde;   /* no teste de áudio nada pisca: só o som */
        ui_center("( só o som )", UI_W / 2.0f, 320, 30, suave);
    }
    snprintf(buf, sizeof buf, "apertos %d de %d", G.cal.n, AJ_CALIBRA_APERTOS);
    ui_center(buf, UI_W / 2.0f, 450, 24, claro);
    if (G.cal.n > 0) {
        snprintf(buf, sizeof buf, "último: %+.0f ms", G.cal.ultimo * 1000);
        ui_center(buf, UI_W / 2.0f, 490, 22, suave);
    }
    if (G.cal.falhou) ui_center("apertos muito irregulares: de novo", UI_W / 2.0f, 530, 22, (Color){255, 170, 90, 255});
    ui_center("Esc cancela", UI_W / 2.0f, 620, 20, suave);
}

static void ui_title(void) {
    DrawRectangleGradientV(0, 360, UI_W, 360, fadec(INK, 0), fadec(INK, 0.5f));
    float bob = sinf(G.time * 1.2f) * 4;
    draw_text_f(G.uiBold, "aparar", UI_W / 2.0f - ui_width_f(G.uiBold, "aparar", 130) / 2, 96 + bob, 130, (Color){238, 214, 170, 255}, true);
    ui_center("a trilha dos doze aprendizes", UI_W / 2.0f, 250, 26, (Color){236, 220, 190, 230});
    int options = G.hasSave ? 3 : 2;
    Rectangle w = {UI_W / 2.0f - 220, 396, 440, 40 + options * 60.0f};
    parchment(w, 1);
    const char *all[] = {"continuar", "novo jogo", "ver a lore"};
    for (int i = 0; i < options; i++)
        ui_menu_row(all[G.hasSave ? i : i + 1], UI_W / 2.0f, 420 + i * 60.0f, i == G.menuIndex, INK_TEXT, 1);
    char buf[96];
    snprintf(buf, sizeof buf, "L  calibrar o atraso (vídeo %.0f ms, áudio %.0f ms)", G.latVideo * 1000, G.latAudio * 1000);
    ui_center(buf, UI_W / 2.0f, UI_H - 44.0f, 20, (Color){236, 220, 190, 200});
}


/* Botão de voltar ao menu, no canto da trilha. */
static const Rectangle MENU_BUTTON = {UI_W - 48 - 170, 22, 170, 46};

static void go_to_menu(void) {
    G.paused = false;
    G.hasSave = true;
    G.menuIndex = 0;
    save_game();
    audio_music(MUSIC_TITLE);
    set_state(ST_TITLE);
}

static void ui_trail(void) {
    const MasterProfile *m = roster_get(G.camp.index);
    char buf[128];
    snprintf(buf, sizeof buf, "vencidos %d de %d", campaign_defeated(&G.camp), MASTER_COUNT);
    Rectangle tag = {40, 18, ui_width(buf, 24) + 48, 48};
    parchment(tag, 1);
    ink(buf, tag.x + 24, tag.y + 12, 24, INK_TEXT);
    bool hover = CheckCollisionPointRec(mouse_ui(), MENU_BUTTON);
    parchment(MENU_BUTTON, 1);
    if (hover) ui_cursor((Rectangle){MENU_BUTTON.x + 8, MENU_BUTTON.y + 6, MENU_BUTTON.width - 16, MENU_BUTTON.height - 12}, 1);
    ink_center("menu", MENU_BUTTON.x + MENU_BUTTON.width / 2 + 6, MENU_BUTTON.y + 10, 24, INK_TEXT);

    Rectangle r = {70, 500, UI_W - 140, 190};
    parchment(r, 1);
    scroll_rods(r, 1);
    if (m->isBigBoss) snprintf(buf, sizeof buf, "último duelo");
    else snprintf(buf, sizeof buf, "aprendiz %d de %d", m->id, MASTER_COUNT);
    ink(buf, r.x + 36, r.y + 22, 22, INK_SOFT);
    ink_bold(lower(m->name), r.x + 36, r.y + 50, 48, m->isBigBoss ? SEAL_RED : (Color){160, 66, 22, 255});
    ink(lower(m->title), r.x + 36, r.y + 118, 24, INK_TEXT);
    float cx = r.x + r.width - 36;
    draw_text_f(G.uiBold, lower(m->style), cx - ui_width_f(G.uiBold, lower(m->style), 28), r.y + 60, 28, INK_TEXT, false);
    ink_right(lower(m->venue), cx, r.y + 110, 22, INK_SOFT);
    if (fmodf(G.time, 1.4f) < 1.0f) ink_right("clique para lutar", cx, r.y + 22, 22, INK_SOFT);
}


/* Opções da derrota: o sensei só aparece para quem já caiu duas vezes aqui. */
static int defeat_options(const char **labels) {
    int n = 0;
    labels[n++] = "tentar de novo";
    if (G.defeatsHere >= 2) labels[n++] = "conversar com hanzo";
    labels[n++] = "voltar à trilha";
    return n;
}

static void ui_defeat(void) {
    if (G.stateTime < AJ_DERROTA_TITULO) return;
    float a = clampf((G.stateTime - AJ_DERROTA_TITULO) / AJ_FADE_RESULTADO, 0, 1);
    DrawRectangle(0, 0, UI_W, UI_H, fadec((Color){20, 6, 4, 255}, 0.6f * a));
    draw_text_f(G.uiBold, "derrota", UI_W / 2.0f - ui_width_f(G.uiBold, "derrota", 80) / 2, 190, 80, fadec((Color){206, 70, 50, 255}, a), true);
    if (G.stateTime < AJ_DERROTA_OPCOES) return;
    const char *labels[3];
    int n = defeat_options(labels);
    parchment((Rectangle){UI_W / 2.0f - 220, 356, 440, 40 + n * 56.0f}, a);
    for (int i = 0; i < n; i++) {
        bool sensei = strcmp(labels[i], "conversar com hanzo") == 0;
        ui_menu_row(labels[i], UI_W / 2.0f, 380 + i * 56.0f, i == G.defeatIndex, sensei ? SEAL_RED : INK_TEXT, a);
    }
}

static void ui_cleared(void) {
    float a = clampf(G.stateTime / AJ_FADE_RESULTADO, 0, 1);
    DrawRectangle(0, 0, UI_W, UI_H, fadec(INK, 0.45f * a));
    Rectangle r = {UI_W / 2.0f - 300, 200, 600, 220};
    parchment(r, a);
    scroll_rods(r, a);
    ink_bold_center("aprendiz vencido", UI_W / 2.0f, r.y + 34, 50, fadec(INK_TEXT, a));
    ink_bold_center(lower(G.m->name), UI_W / 2.0f, r.y + 104, 34, fadec((Color){160, 66, 22, 255}, a));
    if (G.stateTime > AJ_VITORIA_TRAVA)
        ink_center(campaign_big_boss_open(&G.camp) ? "os doze caíram. oboro espera no dojo de hanzo." : "clique para seguir a trilha",
                   UI_W / 2.0f, r.y + 162, 24, fadec(INK_SOFT, a));
}



/* Tecla de pixel (a folha de teclas do pack) com o rótulo ao lado; sem a folha,
 * a tecla vai escrita. Devolve a largura usada. */
static float ui_key(const char *key, const char *label, float x, float y, Color c) {
    float w = spr_key(key, x, y, PX, false, WHITE);
    if (w <= 0) {
        ink(lower(key), x, y + 14, 26, c);
        w = ui_width(lower(key), 26);
    }
    if (!label) return w;
    float lx = x + fmaxf(w, 64) + 24;
    ink(label, lx, y + 14, 26, c);
    return lx + ui_width(label, 26) - x;
}

static void ui_pause(void) {
    DrawRectangle(0, 0, UI_W, UI_H, fadec(INK, 0.6f));
    Rectangle r = {UI_W / 2 - 250, 120, 500, 470};
    parchment(r, 1);
    scroll_rods(r, 1);
    ink_bold_center("pausa", UI_W / 2.0f, r.y + 30, 54, INK_TEXT);
    const char *keys[] = {"ESC", "T", "F", "L", "M", "Q"};
    const char *items[] = {"continuar", "voltar à trilha", G.fx.shakeEnabled ? "tremor ligado" : "tremor desligado", "calibrar o atraso",
                           "voltar ao menu", "sair"};
    for (int i = 0; i < 6; i++) ui_key(keys[i], items[i], r.x + 120, r.y + 100 + i * 58.0f, INK_SOFT);
}

/* Primeiro duelo: como se apara, até o primeiro parry que pega. */
static void ui_first_hint(void) {
    if (G.camp.index != 0 || G.state != ST_DUEL || G.duel.perfects + G.duel.goods > 0) return;
    float a = clampf(G.stateTime - AJ_DICA_PRIMEIRO_DUELO, 0, 1);
    if (a <= 0) return;
    Color c = fadec((Color){236, 222, 192, 255}, a);
    float w = ui_width("aparar", 26) + 24 + 128 + 24 + ui_width("ou clique", 26);
    float x = UI_W / 2.0f - w / 2, y = 196;   /* abaixo da faixa da postura, acima até dos saltos */
    ui_text("aparar", x, y + 14, 26, c);
    x += ui_width("aparar", 26) + 24;
    float kw = spr_key("SPACE", x, y, PX, fmodf(G.time, 1.2f) < 0.2f, fadec(WHITE, a));
    if (kw <= 0) { ui_text("espaço", x, y + 14, 26, c); kw = ui_width("espaço", 26); }
    ui_text("ou clique", x + kw + 24, y + 14, 26, c);
}


/* Execução: um traço branco atravessa a tela na diagonal. */
static void ui_slash(void) {
    if (G.slash <= 0) return;
    float t = 1 - G.slash / 0.35f, a = clampf(G.slash / 0.15f, 0, 1);
    Vector2 p0 = {-60, UI_H * 0.78f}, p1 = {UI_W + 60, UI_H * 0.2f};
    Vector2 head = {p0.x + (p1.x - p0.x) * fminf(1, t * 3), p0.y + (p1.y - p0.y) * fminf(1, t * 3)};
    DrawLineEx(p0, head, 10 * a, fadec(WHITE, 0.25f * a));
    DrawLineEx(p0, head, 3 * a, fadec(WHITE, a));
}

/* ------------------------------------------------------------------ */
/* Overlay de debug (F3 ou APARA_DEBUG=1)                              */
/* ------------------------------------------------------------------ */

/* Desenhado por cima de tudo, em pixels da tela (texto nítido), dentro de `dst`. */
static void ui_carimbo(Rectangle dst) {
    float u = dst.width / UI_W, x = dst.x + 40 * u, y = dst.y + 200 * u, lh = 34 * u;
    int fs = (int)fmaxf(12, 26 * u);
    DrawRectangleRec((Rectangle){x - 16 * u, y - 16 * u, 900 * u, 7 * lh}, (Color){0, 0, 0, 200});
    char ln[200];
    snprintf(ln, sizeof ln, "--carimbo: clique com o mouse ou aperte Espaço, J ou Enter (%d de %d)", G.carimboN, AJ_CARIMBO_MEDIDAS);
    DrawText(ln, (int)x, (int)y, fs, YELLOW); y += lh;
    if (!G.usaCarimbo) { DrawText("esta plataforma não dá o instante do clique: todo aperto vale no meio do quadro", (int)x, (int)y, fs, RED); return; }
    if (G.carimboN > 0) {
        snprintf(ln, sizeof ln, "o poll leu o clique depois de: mín %.1f ms   média %.1f ms   máx %.1f ms", G.carimboMin * 1000,
                 G.carimboSoma / G.carimboN * 1000, G.carimboMax * 1000);
        DrawText(ln, (int)x, (int)y, fs, WHITE);
    }
    y += lh;
    snprintf(ln, sizeof ln, "quadro %.1f ms: os valores devem ir de 0 até uns 1 quadro", GetFrameTime() * 1000);
    DrawText(ln, (int)x, (int)y, fs, LIGHTGRAY); y += lh;
    DrawText("se der sempre perto de 0, o carimbo está sendo tirado no poll, e não no clique", (int)x, (int)y, fs, LIGHTGRAY);
}

static void ui_debug(Rectangle dst) {
    if (G.modoCarimbo) { ui_carimbo(dst); return; }
    if (!G.debug || !(G.state == ST_DUEL || G.state == ST_DEFEAT || G.state == ST_FINISHER)) return;
    const Duel *d = &G.duel;
    float u = dst.width / UI_W;
    float x = dst.x + 16 * u, y = dst.y + 104 * u, w = 860 * u, lh = 22 * u;
    int fs = (int)fmaxf(10, 18 * u), fp = (int)fmaxf(9, 14 * u);
    int nAnot = (int)(sizeof G.aperto / sizeof G.aperto[0]);
    DrawRectangleRec((Rectangle){x - 8 * u, y - 8 * u, w + 16 * u, (9 + nAnot) * lh + 128 * u}, (Color){0, 0, 0, 185});
    char ln[200];
#define DBG_LINHA(cor, ...) do { snprintf(ln, sizeof ln, __VA_ARGS__); DrawText(ln, (int)x, (int)y, fs, cor); y += lh; } while (0)
    static const char *FASE[] = {"pronto", "preparação", "recuperação", "fim"};
    const Stance *st = duel_stance(d);
    const Move *mv = duel_move(d);
    Color branco = {235, 235, 235, 255}, cinza = {160, 160, 170, 255};
    DBG_LINHA(YELLOW, "DEBUG (F3)  %s  %s", G.m->name, st->name && st->name[0] ? st->name : "");
    DBG_LINHA(branco, "fase: %s   relógio %.2f s   quadro %.1f ms%s%s", FASE[d->phase], d->clock, (G.recDir ? G.recDt : GetFrameTime()) * 1000,
              d->earlyUsed && d->phase == PH_WINDUP ? "   apertou cedo: sem perfeito" : "",
              d->pressBlockedUntil > d->clock ? "   recarga" : "");
    DBG_LINHA(branco, "golpe: %s  %d de %d   preparação %.0f ms%s%s", mv ? mv->name : "-", d->comboStrike + 1,
              mv ? mv->strikes : 1, d->windupDuration * 1000, d->special ? "  ESPECIAL" : "", duel_strike_dual(d) ? "  DUPLO" : "");
    DBG_LINHA(branco, "janela: perfeita %.0f ms, boa %.0f ms   lâmina parte %.0f ms antes, aviso %.0f ms antes",
              st->perfectWindow * 1000, st->goodWindow * 1000, duel_strike_lead(d) * 1000, duel_aviso(d) * 1000);
    DBG_LINHA(branco, "mestre: postura %.0f / %.0f   selo %d de %d%s%s", d->bossPosture, duel_posture_max(d), d->seal + 1,
              d->m->sealCount > 0 ? d->m->sealCount : 1, duel_under_pressure(d) && d->m->sealCount <= 1 ? "   com pressa" : "",
              d->advantage ? "   VANTAGEM" : "");
    DBG_LINHA(branco, "kojiro: vida %.0f / %.0f%s   dano de um erro %.1f", d->renPosture, d->s.renPosture,
              d->burnLeft > 0 ? "  em brasas" : "", duel_ren_damage(d));
    DBG_LINHA(branco, "hitstop %.0f ms   câmera lenta %.2fx   perfeitos %d  bons %d  erros %d   atraso: vídeo %.0f, áudio %.0f ms",
              fmaxf(0, G.hitstop) * 1000, G.slowmo, d->perfects, d->goods, d->bads, G.latVideo * 1000, G.latAudio * 1000);
    if (G.usaCarimbo)
        DBG_LINHA(cinza, "carimbo do clique: %ld apertos no instante do clique, %ld no meio do quadro   último lido %.0f ms depois",
                  G.carimbados, G.semCarimbo, G.atrasoPoll * 1000);
    else
        DBG_LINHA(cinza, "carimbo do clique: nenhum (o aperto vale no meio do quadro)");
    /* os últimos apertos: o resultado, quando foi e o erro em ms */
    DBG_LINHA(cinza, "apertos, o mais novo em cima (erro: quanto faltou para a janela perfeita)");
    for (int i = 0; i < nAnot; i++) {
        if (i >= G.apertos) { y += lh; continue; }
        const Anotacao *l = &G.aperto[i];
        Color c = i == 0 ? l->cor : ColorAlpha(l->cor, 0.7f);
        DrawText(l->res, (int)x, (int)y, fs, c);
        DrawText(l->quando, (int)(x + 110 * u), (int)y, fs, c);
        DrawText(l->erro, (int)(x + 340 * u), (int)y, fs, c);
        y += lh;
    }
    /* linha do tempo do golpe: preparação, lâmina partindo, aviso, janelas e o agora */
    DuelTimeline t = duel_timeline(d);
    y += 6 * u;
    Rectangle bar = {x, y, w, 26 * u};
    DrawRectangleRec(bar, (Color){50, 50, 60, 255});
    if (t.active) {
        double t0 = t.start, t1 = t.strike + 0.15;
        float px = (float)(bar.width / (t1 - t0));
#define DBG_X(tt) (bar.x + (float)((tt) - t0) * px)
        DrawRectangleRec((Rectangle){DBG_X(t.launch), bar.y, DBG_X(t.strike) - DBG_X(t.launch), bar.height}, (Color){120, 70, 40, 255});
        DrawRectangleRec((Rectangle){DBG_X(t.goodFrom), bar.y + 4 * u, DBG_X(t.strike) - DBG_X(t.goodFrom), bar.height - 8 * u}, GOLD);
        DrawRectangleRec((Rectangle){DBG_X(t.perfectFrom), bar.y + 4 * u, DBG_X(t.strike) - DBG_X(t.perfectFrom), bar.height - 8 * u}, GREEN);
        DrawRectangleRec((Rectangle){DBG_X(t.cue) - 1 * u, bar.y - 4 * u, 3 * u, bar.height + 8 * u}, SKYBLUE);
        DrawRectangleRec((Rectangle){DBG_X(t.strike) - 1 * u, bar.y - 6 * u, 3 * u, bar.height + 12 * u}, RED);
        if (d->attempted && d->lastPress >= t0) DrawRectangleRec((Rectangle){DBG_X(d->lastPress) - 1 * u, bar.y, 3 * u, bar.height}, MAGENTA);
        DrawRectangleRec((Rectangle){DBG_X(t.now) - 1 * u, bar.y - 6 * u, 3 * u, bar.height + 12 * u}, WHITE);
        snprintf(ln, sizeof ln, "faltam %.0f ms", (t.strike - t.now) * 1000);
        DrawText(ln, (int)(bar.x + bar.width - 130 * u), (int)(bar.y + bar.height + 6 * u), fs, branco);
#undef DBG_X
    }
    DrawText("azul: aviso  marrom: lâmina partindo  verde/ouro: janelas  vermelho: contato  branco: agora  rosa: seu aperto",
             (int)x, (int)(bar.y + bar.height + 30 * u), fp, cinza);
    if (G.teste)
        DrawText(G.teclas ? "teclas: R recomeça  V vida cheia  P postura cheia  1 2 3 fase do oboro  N B próximo e anterior   |   modo de teste: o progresso não é salvo"
                          : "opções de teste: o progresso não é salvo",
                 (int)x, (int)(bar.y + bar.height + 50 * u), fp, (Color){255, 200, 120, 255});
#undef DBG_LINHA
}

/* O aviso do save, no canto de cima à esquerda (o placar do mestre fica no meio; a trilha põe a sua
 * etiqueta acima disto), em qualquer tela. */
static void ui_aviso_save(void) {
    if (G.avisoSaveAte <= G.time || !G.avisoSave[0]) return;
    const float size = 18, lh = size * 1.5f, w = 420;
    char linhas[4][LINHA_MAX];
    int n = wrap_lines(G.avisoSave, size, w - 32, linhas, 4);
    float a = clampf(G.avisoSaveAte - G.time, 0, 1);
    Rectangle r = {20, 80, w, 24 + n * lh};
    parchment(r, a);
    for (int i = 0; i < n; i++) ink(linhas[i], r.x + 16, r.y + 12 + i * lh, size, fadec(SEAL_RED, a));
}

static void draw_ui(void) {
    switch (G.state) {
        case ST_TITLE: ui_title(); break;
        case ST_CALIBRA: ui_calibra(); break;
        case ST_LORE: ui_narration(); break;
        case ST_TRAIL: ui_trail(); break;
        case ST_SENSEI: {
            DrawRectangleGradientV(0, 0, UI_W, UI_H, fadec(INK, 0.2f), fadec(INK, 0.6f));
            char head[96];
            snprintf(head, sizeof head, "hanzo fala sobre %s", G.m->name);
            ui_dialogue(head, false);
            break;
        }
        case ST_VISIT:
            DrawRectangleGradientV(0, 0, UI_W, UI_H, fadec(INK, 0.2f), fadec(INK, 0.6f));
            ui_dialogue("a cabana de hanzo, na serra", false);
            break;
        default:
            if (G.state == ST_DUEL || G.state == ST_DEFEAT || G.state == ST_FINISHER) ui_hud();
            ui_first_hint();
            fx_draw_popups(&G.fx, G.ui, UNIT);
            ui_slash();
            if (G.state == ST_INTRO || G.state == ST_OUTRO) ui_dialogue(G.m->venue, true);
            if (G.state == ST_SCENE) ui_beat();
            if (G.state == ST_CHOICE) ui_choice();
            if (G.state == ST_ENDING) ui_ending();
            if (G.state == ST_DEFEAT) ui_defeat();
            if (G.state == ST_CLEARED) ui_cleared();
            break;
    }
    ui_aviso_save();
    if (G.paused) ui_pause();
}

/* ------------------------------------------------------------------ */
/* Telas                                                               */
/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/* Calibração de latência (L no título ou na pausa)                    */
/* ------------------------------------------------------------------ */

/* Um quadrado pisca (vídeo) ou um clique toca (áudio) a cada AJ_CALIBRA_BATIDA; o
 * jogador aperta junto. A mediana de quanto os apertos chegam depois da batida é o
 * atraso daquele canal (calibration_result, no núcleo). */
static double calibra_batida(int k) { return 1.0 + k * AJ_CALIBRA_BATIDA; }

static void start_calibra(bool pausado) {
    G.cal.voltar = G.state;
    G.cal.pausado = pausado;
    G.cal.modo = 0;
    G.cal.t = 0;
    G.cal.proxima = 0;
    G.cal.n = 0;
    G.cal.falhou = false;
    G.cal.ultimo = 0;
    G.cal.video = G.latVideo;
    G.cal.audio = G.latAudio;
    G.paused = false;
    set_state(ST_CALIBRA);
}

static void end_calibra(bool salva) {
    if (salva) {
        G.latVideo = G.cal.video;
        G.latAudio = G.cal.audio;
        save_options();
        /* vale já no duelo em curso */
        G.settings.latency = G.duel.s.latency = G.latVideo;
        G.settings.audioLead = G.duel.s.audioLead = G.latAudio - G.latVideo;
    }
    G.state = G.cal.voltar;
    G.paused = G.cal.pausado;
}

static void update_calibra(float dt) {
    if (IsKeyPressed(KEY_ESCAPE)) { end_calibra(false); return; }
    if (G.cal.modo == 2) {
        if (IsKeyPressed(KEY_R)) { G.cal.modo = 0; G.cal.t = 0; G.cal.proxima = 0; G.cal.n = 0; return; }
        if (pressed() && G.stateTime > AJ_TRAVA_CLIQUE_CALIBRA) end_calibra(true);
        return;
    }
    G.cal.t += dt;
    /* a batida: no teste de áudio, o clique */
    while (calibra_batida(G.cal.proxima) <= G.cal.t) {
        if (G.cal.modo == 1) audio_play(SND_CUE, 1, 1);
        G.cal.proxima++;
    }
    if (!pressed()) return;
    double tp = (G.cal.t - dt) + instante_do_aperto(dt, dt, pressed_key_mouse());   /* o aperto chegou em algum ponto do quadro */
    int k = (int)lround((tp - 1.0) / AJ_CALIBRA_BATIDA);
    if (k < 2) return;                               /* as duas primeiras batidas são para pegar o ritmo */
    G.cal.ultimo = (float)(tp - calibra_batida(k));
    G.cal.off[G.cal.n++] = G.cal.ultimo;
    if (G.cal.n < AJ_CALIBRA_APERTOS) return;
    float r = calibration_result(G.cal.off, G.cal.n);
    G.cal.falhou = r < 0;
    if (r >= 0) {
        if (G.cal.modo == 0) G.cal.video = r;
        else G.cal.audio = r;
        G.cal.modo++;
        G.stateTime = 0;
    }
    G.cal.t = 0;
    G.cal.proxima = 0;
    G.cal.n = 0;
}

static void update_title(void) {
    if (IsKeyPressed(KEY_L)) { start_calibra(false); return; }
    int options = G.hasSave ? 3 : 2;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) { G.menuIndex = (G.menuIndex + 1) % options; audio_play(SND_UI, 1, 1); }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) { G.menuIndex = (G.menuIndex + options - 1) % options; audio_play(SND_UI, 1, 1); }
    Vector2 v = mouse_ui();
    for (int i = 0; i < options; i++) {
        Rectangle r = {UI_W / 2.0f - 200, 414 + i * 60.0f, 400, 54};
        if (CheckCollisionPointRec(v, r) && (GetMouseDelta().x != 0 || GetMouseDelta().y != 0)) G.menuIndex = i;
    }
    if (!pressed() || G.stateTime < AJ_TRAVA_CLIQUE_TITULO) return;
    audio_play(SND_UI, 1, 1.2f);
    int choice = G.hasSave ? G.menuIndex : G.menuIndex + 1; /* 0 continuar, 1 novo, 2 lore */
    if (choice == 0) {
        if (G.camp.completed) { campaign_reset(&G.camp); G.camp.loreSeen = true; }
        set_state(ST_TRAIL);
        audio_music(MUSIC_TITLE);
        return;
    }
    if (choice == 1) campaign_reset(&G.camp);
    G.lorePage = 0;
    G.typeChars = 0;
    G.loreScroll = 0;
    G.skipHold = 0;
    set_state(ST_LORE);
    audio_music(MUSIC_LORE);
}

static void finish_lore(void) {
    G.camp.loreSeen = true;
    save_game();
    set_state(ST_TRAIL);
    audio_music(MUSIC_TITLE);
}

/* A abertura: o texto sobe sozinho; segurar o clique acelera; segurar Esc enche o anel e pula. */
static void update_lore(float dt) {
    bool fast = IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyDown(KEY_SPACE) || ((G.demo || G.autoJogo) && !G.shotFile);
    G.loreScroll += dt * (fast ? 90 : 26);
    if (IsKeyDown(KEY_ESCAPE)) G.skipHold += dt;
    else G.skipHold = fmaxf(0, G.skipHold - dt * 3);
    if (G.skipHold >= SKIP_HOLD) { finish_lore(); return; }
    if (G.loreHeight > 0 && G.loreScroll > G.loreHeight + UI_H * 0.55f) finish_lore();
}

static void update_trail(void) {
    if (G.stateTime < AJ_TRAVA_CLIQUE_TRILHA) return;
    if (IsKeyPressed(KEY_ESCAPE) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse_ui(), MENU_BUTTON))) {
        audio_play(SND_UI, 1, 1);
        go_to_menu();
        return;
    }
    if (!pressed()) return;
    audio_play(SND_UI, 1, 1);
    start_master(G.camp.index);
    start_lines(G.m->intro, G.m->introCount, ST_INTRO);
}

static void update_lines(float dt, void (*done)(void)) {
    const Line *l = &G.lines[G.lineIndex];
    int len = (int)strlen(l->text);
    float before = G.typeChars;
    G.typeChars += dt * 50;
    if ((int)G.typeChars / 3 != (int)before / 3 && G.typeChars < len) audio_play(SND_TYPE, 0.5f, strcmp(l->speaker, "kojiro") ? 0.8f : 1.1f);
    if (!pressed() || G.stateTime < AJ_TRAVA_CLIQUE_FALA) return;
    if (G.typeChars < len) { G.typeChars = (float)len; return; }
    audio_play(SND_UI, 0.8f, 1);
    G.lineIndex++;
    G.typeChars = 0;
    if (G.lineIndex >= G.lineCount) done();
}

static void intro_done(void) { start_duel(); }

static void outro_done(void) {
    registra_vitoria();
    marco_de_teste("tela_de_vitoria");
    set_state(ST_CLEARED);
    audio_play(SND_VICTORY, 0.8f, 1);
}

static void start_sensei(void) {
    start_lines(G.m->sensei, G.m->senseiCount, ST_SENSEI);
    audio_music(MUSIC_LORE);
}

static void sensei_done(void) {
    audio_music(G.m->arena);
    start_duel();
}

static void update_defeat(float dt) {
    if (G.slowmoTime > 0) { G.slowmoTime -= dt; if (G.slowmoTime <= 0) G.slowmo = 1; }
    update_actors(dt * G.slowmo);
    if (G.stateTime < AJ_DERROTA_OPCOES) return;
    const char *labels[3];
    int n = defeat_options(labels);
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) { G.defeatIndex = (G.defeatIndex + 1) % n; audio_play(SND_UI, 1, 1); }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) { G.defeatIndex = (G.defeatIndex + n - 1) % n; audio_play(SND_UI, 1, 1); }
    Vector2 v = mouse_ui();
    for (int i = 0; i < n; i++) {
        Rectangle r = {UI_W / 2.0f - 220, 374 + i * 56.0f, 440, 52};
        if (CheckCollisionPointRec(v, r) && (GetMouseDelta().x != 0 || GetMouseDelta().y != 0)) G.defeatIndex = i;
    }
    const char *choice = NULL;
    if (IsKeyPressed(KEY_T)) choice = "voltar à trilha";
    else if (IsKeyPressed(KEY_H) && G.defeatsHere >= 2) choice = "conversar com hanzo";
    else if (pressed()) choice = labels[G.defeatIndex < n ? G.defeatIndex : 0];
    if (!choice) return;
    audio_play(SND_UI, 1, 1);
    if (!strcmp(choice, "tentar de novo")) start_duel();
    else if (!strcmp(choice, "conversar com hanzo")) start_sensei();
    else { audio_music(MUSIC_TITLE); set_state(ST_TRAIL); }
}

static void visit_done(void) {
    audio_music(MUSIC_TITLE);
    set_state(ST_TRAIL);
}

static void update_cleared(float dt) {
    rig_update(&G.ren, dt);
    rig_update(&G.boss, dt);
    f_update(&G.renS, dt);
    f_update(&G.bossS, dt);
    update_sword(dt);
    if (G.stateTime > AJ_VITORIA_TRAVA && pressed()) {
        if (G.m->visitCount > 0) {
            /* a cabana de hanzo: kojiro conta quem venceu, hanzo fala do próximo */
            start_lines(G.m->visit, G.m->visitCount, ST_VISIT);
            audio_music(MUSIC_LORE);
            return;
        }
        audio_music(MUSIC_TITLE);
        set_state(ST_TRAIL);
    }
}


/* ------------------------------------------------------------------ */

static bool in_arena_state(void) {
    return G.state == ST_INTRO || G.state == ST_DUEL || G.state == ST_FINISHER || G.state == ST_OUTRO ||
           G.state == ST_CLEARED || G.state == ST_DEFEAT || G.state == ST_SCENE || G.state == ST_CHOICE ||
           G.state == ST_ENDING;
}

static void parse_args(int argc, char **argv, int *startMaster, bool *direct, const char **startState) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--master") && i + 1 < argc) { *startMaster = atoi(argv[++i]) - 1; G.teste = true; }
        else if (!strcmp(argv[i], "--duel")) { *direct = true; G.teste = true; }
        else if (!strcmp(argv[i], "--teste")) { *direct = true; G.debug = G.teste = G.teclas = true; }
        else if (!strcmp(argv[i], "--fase") && i + 1 < argc) { G.startSeal = atoi(argv[++i]) - 1; G.teste = true; }
        else if (!strcmp(argv[i], "--demo")) G.demo = true;
        else if (!strcmp(argv[i], "--carimbo")) { G.modoCarimbo = true; G.teste = true; }
        else if (!strcmp(argv[i], "--state") && i + 1 < argc) { *startState = argv[++i]; G.teste = true; }
        else if (!strcmp(argv[i], "--final") && i + 1 < argc) { G.demoChoice = strcmp(argv[++i], "sim") ? 1 : 0; G.teste = true; }
        else if (!strcmp(argv[i], "--shot") && i + 2 < argc) { G.shotFile = argv[++i]; G.shotTime = (float)atof(argv[++i]); }
        else if (!strcmp(argv[i], "--rec") && i + 3 < argc) {
            G.recDir = argv[++i];
            G.recStart = (float)atof(argv[++i]);
            G.recEnd = (float)atof(argv[++i]);
        }
    }
    if (G.teste && *startMaster < 0) *startMaster = 0;
}

static void step(float dtReal) {
    switch (G.state) {
        case ST_TITLE: update_title(); break;
        case ST_CALIBRA: update_calibra(dtReal); break;
        case ST_LORE: update_lore(dtReal); break;
        case ST_TRAIL: update_trail(); break;
        case ST_INTRO:
            update_lines(dtReal, intro_done);
            update_actors(dtReal);
            break;
        case ST_DUEL: update_duel(dtReal); break;
        case ST_FINISHER: {
            float dt = dtReal;
            if (G.hitstop > 0) { G.hitstop -= dtReal; dt = 0; }
            if (G.slowmoTime > 0) { G.slowmoTime -= dtReal; dt *= G.slowmo; if (G.slowmoTime <= 0) G.slowmo = 1; }
            G.stateTime += dt - dtReal; /* o desarme corre no tempo lento */
            update_finisher(dt);
            break;
        }
        case ST_OUTRO:
            update_lines(dtReal, outro_done);
            rig_update(&G.ren, dtReal);
            rig_update(&G.boss, dtReal);
            f_update(&G.renS, dtReal);
            f_update(&G.bossS, dtReal);
            update_sword(dtReal);
            break;
        case ST_CLEARED: update_cleared(dtReal); break;
        case ST_DEFEAT: update_defeat(dtReal); break;
        case ST_SENSEI: update_lines(dtReal, sensei_done); break;
        case ST_VISIT: update_lines(dtReal, visit_done); break;
        case ST_SCENE: update_scene(dtReal); break;
        case ST_CHOICE: update_choice(dtReal); break;
        case ST_ENDING: update_ending(dtReal); break;
    }
    if (in_arena_state()) {
        update_ctx(dtReal * (G.hitstop > 0 ? 0.1f : 1));
        fx_update(&G.fx, dtReal * (G.hitstop > 0 ? 0.25f : 1));
        vfx_update(dtReal * (G.hitstop > 0 ? 0.25f : 1));
        raios_update(dtReal * (G.hitstop > 0 ? 0.25f : 1));
        update_hud_values(dtReal);
    }
}

static Font load_font(const char *path, int size) {
    int cps[FONTE_PEDIDOS_MAX];
    int n = fonte_pedidos(cps, FONTE_PEDIDOS_MAX);
    Font f = LoadFontEx(path, size, cps, n);
    if (f.texture.id == 0 || f.glyphCount == 0) return GetFontDefault();
    /* Fonte de pixel: cada pixel do atlas fica cheio ou vazio (o mesmo corte do
     * FONT_BITMAP da raylib), sem a borda suavizada, e ampliado sem filtro. */
    Image atlas = LoadImageFromTexture(f.texture);
    ImageFormat(&atlas, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *px = atlas.data;
    for (int i = 0; i < atlas.width * atlas.height; i++) px[i] = px[i].a >= 80 ? WHITE : BLANK;
    UnloadTexture(f.texture);
    f.texture = LoadTextureFromImage(atlas);
    UnloadImage(atlas);
    SetTextureFilter(f.texture, TEXTURE_FILTER_POINT);
    return f;
}

/* ---- Desempenho (APARA_PERF) ---- */

static double perf_agora(void) { return G.perf ? entrada_relogio() : 0; }

/* Uma etapa da carga terminou: quanto levou desde a anterior. */
static void perf_carga(const char *nome) {
    if (!G.perf || G.perfCargas >= 16) return;
    double t = entrada_relogio();
    snprintf(G.perfCarga[G.perfCargas], sizeof G.perfCarga[0], "%s", nome);
    G.perfCargaMs[G.perfCargas++] = (t - G.perfUltimaCarga) * 1000;
    G.perfUltimaCarga = t;
}

static void perf_relatorio(void) {
    const Desempenho *p = &G.perfDados;
    const int aquecimento = 30;                      /* os primeiros quadros pagam a carga preguiçosa */
    double total = 0;
    for (int i = 0; i < G.perfCargas; i++) total += G.perfCargaMs[i];
    fprintf(stderr, "PERF carga até o primeiro quadro (ms):");
    for (int i = 0; i < G.perfCargas; i++) fprintf(stderr, "  %s %.0f", G.perfCarga[i], G.perfCargaMs[i]);
    fprintf(stderr, "  | total %.0f\n", total);
    if (p->n > 0) {
        double pior = 0;
        for (int i = 1; i < p->n && i < aquecimento; i++) if (p->v[PERF_QUADRO][i] > pior) pior = p->v[PERF_QUADRO][i];
        fprintf(stderr, "PERF primeiro quadro (carga preguiçosa incluída): %.0f ms; o pior dos %d seguintes: %.0f ms\n", p->v[PERF_QUADRO][0] * 1000, aquecimento - 1, pior * 1000);
    }
    PerfResumo q = perf_resumo(p, PERF_QUADRO, aquecimento);
    fprintf(stderr, "PERF %s: %d quadros medidos (%d de aquecimento fora, %d perdidos), %.1f quadros por segundo\n",
            G.m ? G.m->name : "?", q.n, p->n < aquecimento ? p->n : aquecimento, p->perdidos, q.media > 0 ? 1.0 / q.media : 0);
    fprintf(stderr, "PERF seção        média    p50    p95    p99    máx   (ms)\n");
    for (int s = 0; s < PERF_SECOES; s++) {
        PerfResumo r = perf_resumo(p, (PerfSecao)s, aquecimento);
        fprintf(stderr, "PERF %-12s %6.2f %6.2f %6.2f %6.2f %6.2f\n", perf_nome((PerfSecao)s), r.media * 1000, r.p50 * 1000, r.p95 * 1000, r.p99 * 1000, r.max * 1000);
    }
    PerfResumo a = perf_resumo(p, PERF_ATUALIZA, aquecimento), w = perf_resumo(p, PERF_MUNDO, aquecimento), u = perf_resumo(p, PERF_UI, aquecimento),
              c = perf_resumo(p, PERF_COMPOE, aquecimento);
    fprintf(stderr, "PERF trabalho do jogo por quadro (lógica + mundo + interface + composição, sem o swap): média %.2f ms\n",
            (a.media + w.media + u.media + c.media) * 1000);
    int pausados = 0;
    for (int i = aquecimento; i < p->n; i++) pausados += (G.perfEstado[i] & 0x80) != 0;
    if (pausados > 0) fprintf(stderr, "PERF AVISO: %d quadros com o jogo pausado (a luta não andava): esta medida não vale\n", pausados);
    const double meta = 1.0 / 60 * 1.001;             /* 16,68 ms */
    int acima = perf_acima(p, PERF_QUADRO, meta, aquecimento), acima2 = perf_acima(p, PERF_QUADRO, 2 * meta, aquecimento);
    fprintf(stderr, "PERF quadros acima de %.1f ms: %d de %d (%.1f%%); acima de %.1f ms: %d\n", meta * 1000, acima, q.n, q.n ? 100.0 * acima / q.n : 0, 2 * meta * 1000, acima2);
    /* os cinco piores quadros, com o que o jogo fazia */
    static const char *ESTADO[] = {"título", "lore", "trilha", "intro", "duelo", "desarme", "fala final", "vitória", "derrota", "sensei", "final", "visita", "cena", "escolha", "calibração"};
    int piores[5], nPiores = 0;
    for (int k = 0; k < 5 && k < p->n - aquecimento; k++) {
        int pior = -1;
        for (int i = aquecimento; i < p->n; i++) {
            bool listado = false;
            for (int j = 0; j < nPiores; j++) listado = listado || piores[j] == i;
            if (!listado && (pior < 0 || p->v[PERF_QUADRO][i] > p->v[PERF_QUADRO][pior])) pior = i;
        }
        if (pior < 0) break;
        piores[nPiores++] = pior;
        int e = G.perfEstado[pior] & 0x7F;
        fprintf(stderr, "PERF pico %d: quadro %d, jogo %.2f s (%s): quadro %.1f ms = lógica %.1f + mundo %.1f + interface %.1f + composição %.1f + swap %.1f\n", k + 1, pior,
                G.perfTempo[pior], e < (int)(sizeof ESTADO / sizeof ESTADO[0]) ? ESTADO[e] : "?", p->v[PERF_QUADRO][pior] * 1000, p->v[PERF_ATUALIZA][pior] * 1000,
                p->v[PERF_MUNDO][pior] * 1000, p->v[PERF_UI][pior] * 1000, p->v[PERF_COMPOE][pior] * 1000, p->v[PERF_SWAP][pior] * 1000);
    }
    double usuario, sistema;
    if (perf_cpu_do_processo(&usuario, &sistema) && G.perfSegundos > 0) {
        fprintf(stderr, "PERF cpu do processo (com o driver de vídeo): %.0f%% de um núcleo (usuário %.1f s, sistema %.1f s em %.1f s)\n", 100 * (usuario + sistema) / (entrada_relogio() - G.perfInicio),
                usuario, sistema, entrada_relogio() - G.perfInicio);
    }
}

int main(int argc, char **argv) {
    int startMaster = -1;
    bool direct = false;
    const char *startState = NULL;
    G.demoChoice = 1;
    G.debug = getenv("APARA_DEBUG") && strcmp(getenv("APARA_DEBUG"), "0") != 0;
    G.autoJogo = getenv("APARA_AUTO") != NULL;
    G.rastro = getenv("APARA_RASTRO") ? atoi(getenv("APARA_RASTRO")) != 0 : AJ_RASTRO_FANTASMA != 0;
    G.logImpactos = getenv("APARA_LOG_IMPACTOS") != NULL;
    G.logCarimbos = getenv("APARA_LOG_CARIMBOS") != NULL;
    /* Ganchos dos vídeos e dos testes: o robô do demo, o clique fora do duelo e o passo do --rec */
    G.roboEscolhido = ROBO_DO_DEMO;
    if (getenv("APARA_ROBO")) {
        const char *r = getenv("APARA_ROBO");
        if (!strcmp(r, "cedo")) G.roboEscolhido = robo_deslocado(-0.60f);          /* antes do aviso de qualquer mestre */
        else if (!strcmp(r, "tarde")) G.roboEscolhido = robo_deslocado(0.10f);     /* depois de o golpe entrar */
        else if (!strcmp(r, "casual")) G.roboEscolhido = ROBO_HUMANO_CASUAL;
        else if (!strcmp(r, "spam")) G.roboEscolhido = ROBO_APERTA_SEM_PARAR;
        else if (!strcmp(r, "nunca")) G.roboEscolhido = ROBO_SEM_DEFESA;
    }
    G.cliquePeriodo = getenv("APARA_CLIQUE_PERIODO") && atof(getenv("APARA_CLIQUE_PERIODO")) > 0 ? (float)atof(getenv("APARA_CLIQUE_PERIODO")) : AJ_AUTO_CLIQUE_PERIODO;
    G.recDt = getenv("APARA_REC_FPS") && atof(getenv("APARA_REC_FPS")) >= 1 ? 1.0 / atof(getenv("APARA_REC_FPS")) : 1.0 / 30;
    parse_args(argc, argv, &startMaster, &direct, &startState);
    entrada_preparar();
    if (getenv("APARA_PERF") && atof(getenv("APARA_PERF")) > 0) {
        G.perfSegundos = atof(getenv("APARA_PERF"));
        int capacidade = (int)fmin(1e6, fmax(20000, G.perfSegundos * 1000));
        G.perf = perf_iniciar(&G.perfDados, capacidade);
        G.perfEstado = malloc((size_t)capacidade);
        G.perfTempo = malloc(sizeof(float) * (size_t)capacidade);
        G.perf = G.perf && G.perfEstado && G.perfTempo;
        G.perfUltimaCarga = entrada_relogio();
    }

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(UI_W, UI_H, "aparar - a trilha dos doze aprendizes");
    if (!IsWindowReady()) {
        fprintf(stderr, "Não foi possível abrir a janela gráfica.\n");
        return 1;
    }
    perf_carga("janela");
    SetExitKey(KEY_NULL);
    if (getenv("APARA_FPS")) SetTargetFPS(atoi(getenv("APARA_FPS")));   /* só para os testes e as medidas */
    /* O instante de hardware do clique só vale com um humano jogando: o demo, o jogo automático e as capturas
     * apertam por código, e os testes (tests/teste_*.sh) dependem de os cliques deles caírem no meio do quadro. */
    G.usaCarimbo = !G.demo && !G.autoJogo && !G.recDir && !G.shotFile && !getenv("APARA_SEM_CARIMBO") && entrada_iniciar();
    G.carimboMin = 1e9;
    SetWindowMinSize(LOW_W, LOW_H);
    ChangeDirectory(GetApplicationDirectory());
    srand(getenv("APARA_SEMENTE") ? (unsigned)atoi(getenv("APARA_SEMENTE")) : (unsigned)time(NULL));

    G.scene = LoadRenderTexture(RW, RH);
    G.actors = LoadRenderTexture(LOW_W, LOW_H);
    G.uiLow = LoadRenderTexture(LOW_W, LOW_H);
    SetTextureFilter(G.uiLow.texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(G.scene.texture, TEXTURE_FILTER_POINT);   /* pixel art: ampliação sem filtro */
    SetTextureFilter(G.actors.texture, TEXTURE_FILTER_POINT);
    G.post = LoadShaderFromMemory(NULL, POST_FS);
    G.locAberr = GetShaderLocation(G.post, "aberr");
    G.locRes = GetShaderLocation(G.post, "res");
    G.locDesat = GetShaderLocation(G.post, "desat");
    G.locDuo = GetShaderLocation(G.post, "duo");
    perf_carga("texturas e shader");
    katana3d_load("assets/katana");
    perf_carga("katana 3D");
    spr_init();
    preload_runtime_art();
    perf_carga("sprites");
    pix_init(RW, RH);
    perf_carga("pixelize");
    G.ui = load_font(FONTE_UI_ARQUIVO, FONTE_UI_TAMANHO);
    G.uiBold = G.ui;
    perf_carga("fonte");
    audio_init();
    perf_carga("áudio");
    fx_init(&G.fx);
    settings_default(&G.settings);
    campaign_reset(&G.camp);
    G.hasSave = load_game();
    load_options();
    G.slowmo = 1;
    G.m = roster_get(G.camp.index);
    setup_actors();
    perf_carga("save e primeiros lutadores");

    if (startMaster >= 0 && startMaster < ROSTER_SIZE) {
        for (int i = 0; i < startMaster; i++) campaign_mark_cleared(&G.camp, i);
        start_master(startMaster);
        if (startState && !strcmp(startState, "sensei")) start_sensei();
        else if (startState && !strcmp(startState, "visita")) { start_lines(G.m->visit, G.m->visitCount, ST_VISIT); audio_music(MUSIC_LORE); }
        else if (direct) {
            start_duel();
            if (startState && !strcmp(startState, "pause")) G.paused = true;
            else if (startState && !strcmp(startState, "defeat")) ren_falls();
            else if (startState && !strcmp(startState, "finisher")) start_disarm();
            else if (startState && !strcmp(startState, "cleared")) set_state(ST_CLEARED);
        } else start_lines(G.m->intro, G.m->introCount, ST_INTRO);
    } else if (startState && !strcmp(startState, "lore")) {
        set_state(ST_LORE);
        audio_music(MUSIC_LORE);
    } else if (startState && !strcmp(startState, "calibra")) {
        set_state(ST_TITLE);
        audio_music(MUSIC_TITLE);
        start_calibra(false);
    } else if (startState && !strcmp(startState, "trail")) {
        set_state(ST_TRAIL);
        audio_music(MUSIC_TITLE);
    } else {
        set_state(ST_TITLE);
        audio_music(MUSIC_TITLE);
    }

    perf_carga("primeira tela");
    double wall = 0;
    while (!WindowShouldClose()) {
        G.poll = entrada_relogio();
        if (G.perf && G.perfInicio == 0) G.perfInicio = G.poll;
        float dtReal = G.recDir ? (float)G.recDt : GetFrameTime();
        wall += dtReal;
        /* Travamento longo: pausa em vez de engolir o golpe. */
        if (dtReal > AJ_PAUSA_POR_TRAVAMENTO) {
            dtReal = 0;
            if (G.state == ST_DUEL && !G.shotFile && !G.recDir && !G.perf) G.paused = true;
        }
        if (!IsWindowFocused() && G.state == ST_DUEL && !G.shotFile && !G.recDir && !G.perf) G.paused = true;
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();
        if (IsKeyPressed(KEY_F3)) G.debug = !G.debug;
        teclas_de_teste();
        if (IsKeyPressed(KEY_F)) G.fx.shakeEnabled = !G.fx.shakeEnabled;
        if (IsKeyPressed(KEY_ESCAPE) && in_arena_state()) G.paused = !G.paused;
        if (G.paused) {
            if (IsKeyPressed(KEY_T)) { G.paused = false; audio_music(MUSIC_TITLE); set_state(ST_TRAIL); }
            if (IsKeyPressed(KEY_L)) start_calibra(true);
            if (IsKeyPressed(KEY_M)) go_to_menu();
            if (IsKeyPressed(KEY_Q)) break;
            dtReal = 0;
        }

        if (G.logCarimbos) {
            static bool pausaAntes;
            if (G.paused != pausaAntes) fprintf(stderr, "PAUSA %d\n", G.paused);
            pausaAntes = G.paused;
        }
        /* Os cliques que o sistema carimbou desde o quadro passado. Vale um por quadro, como pressed(); um
         * carimbo que sobra (um clique que nenhuma tecla lida aqui trouxe) só vale para o quadro dele. */
        G.quadro = dtReal;
        G.carimbo = ENTRADA_SEM_CARIMBO;
        if (G.usaCarimbo) {
            double c[8];
            int n = entrada_coletar(c, 8, G.poll);
            if (n > 0) { G.carimbo = c[0]; G.atrasoPoll = G.poll - c[0]; }
            for (int i = 0; i < n; i++) {
                if (G.logCarimbos) fprintf(stderr, "CARIMBO ts=%.6f poll=%.6f atraso_ms=%.3f\n", c[i], G.poll, (G.poll - c[i]) * 1000);
                if (G.modoCarimbo) {
                    double a = G.poll - c[i];
                    G.carimboN++;
                    G.carimboSoma += a;
                    if (a < G.carimboMin) G.carimboMin = a;
                    if (a > G.carimboMax) G.carimboMax = a;
                    printf("clique %2d: o poll leu %.1f ms depois\n", G.carimboN, a * 1000);
                    fflush(stdout);
                }
            }
            if (G.modoCarimbo && G.carimboN >= AJ_CARIMBO_MEDIDAS) {
                printf("carimbo: %d cliques, o poll leu de %.1f a %.1f ms depois (média %.1f ms, quadro %.1f ms)\n", G.carimboN, G.carimboMin * 1000,
                       G.carimboMax * 1000, G.carimboSoma / G.carimboN * 1000, GetFrameTime() * 1000);
                break;
            }
        }
        G.time += dtReal;
        G.stateTime += dtReal;
        G.bannerTime = fmaxf(0, G.bannerTime - dtReal);
        G.aberr = fmaxf(0, G.aberr - dtReal * 6);
        G.desat = fmaxf(0, G.desat - dtReal * 1.4f);
        G.duo = fmaxf(0, G.duo - dtReal);
        G.slash = fmaxf(0, G.slash - dtReal);
        G.crack = fmaxf(0, G.crack - dtReal);
        if (G.silence > 0) { G.silence -= dtReal; audio_music_duck(G.silence > 0 ? 1 : 0); }
        double pf0 = perf_agora();
        if (!G.paused && !G.modoCarimbo) step(dtReal);
        double pf1 = perf_agora();
        /* tests/teste_save.sh: depois de vencer o primeiro mestre (a cabana de hanzo), o jogo sai */
        if (G.autoJogo && ((G.state == ST_VISIT && G.stateTime > AJ_AUTO_VISITA_FIM) || G.time > AJ_AUTO_TEMPO_MAX)) break;
        draw_world();
        double pf2 = perf_agora();
        /* Interface em 320 x 180. Cor e alfa acumulados separados: a camada sai com
         * alfa pré-multiplicado e pousa certa por cima da cena. */
        BeginTextureMode(G.uiLow);
        ClearBackground(BLANK);
        rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD, RL_FUNC_ADD);
        BeginBlendMode(BLEND_CUSTOM_SEPARATE);
        rlPushMatrix();
        rlScalef(1 / PX, 1 / PX, 1);
        draw_ui();
        rlDrawRenderBatchActive();
        rlPopMatrix();
        EndBlendMode();
        EndTextureMode();
        double pf3 = perf_agora();

        /* Mundo: ampliação só por número inteiro e sem filtro. Sem mistura,
         * o alfa acumulado na textura não escurece a imagem. */
        BeginDrawing();
        ClearBackground(BLACK);
        float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
        float scale = fminf(sw / RW, sh / RH);
        if (scale >= 1) scale = floorf(scale);
        Rectangle dst = {floorf((sw - RW * scale) / 2), floorf((sh - RH * scale) / 2), RW * scale, RH * scale};
        rlDrawRenderBatchActive();
        rlDisableColorBlend();
        /* Pós-processo: aberração cromática nos impactos, scanlines e vinheta. */
        float res[2] = {LOW_W, LOW_H}; /* scanlines e aberração na escala dos lutadores */
        SetShaderValue(G.post, G.locAberr, &G.aberr, SHADER_UNIFORM_FLOAT);
        SetShaderValue(G.post, G.locRes, res, SHADER_UNIFORM_VEC2);
        float desat = clampf(G.desat, 0, 1), duo = clampf(G.duo * 3, 0, 1);
        SetShaderValue(G.post, G.locDesat, &desat, SHADER_UNIFORM_FLOAT);
        SetShaderValue(G.post, G.locDuo, &duo, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(G.post);
        DrawTexturePro(G.scene.texture, (Rectangle){0, 0, RW, -RH}, dst, (Vector2){0, 0}, 0, WHITE);
        EndShaderMode();
        rlDrawRenderBatchActive();
        rlEnableColorBlend();
        /* Interface: já desenhada em 320 x 180 (alfa pré-multiplicado), ampliada como o mundo. */
        BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
        if (!getenv("APARA_SEM_UI"))   /* capturas das provas: só a cena, sem a interface */
            DrawTexturePro(G.uiLow.texture, (Rectangle){0, 0, LOW_W, -LOW_H}, dst, (Vector2){0, 0}, 0, WHITE);
        EndBlendMode();
        ui_debug(dst);
        rlDrawRenderBatchActive();   /* as capturas leem a tela antes do EndDrawing */
        double pf4 = perf_agora();

        if (getenv("APARA_CLIQUE_PERIODO") && (G.demo || G.autoJogo) && G.state != ST_DUEL &&
            fmodf(G.stateTime, G.cliquePeriodo) < (G.recDir ? (float)G.recDt : GetFrameTime()))
            G.cliqueFlash = 0.05f;   /* o clique do robô existe mesmo quando a tela o ignora (a trava): o ponto mostra todos */
        if (G.cliqueFlash > 0) {
            /* APARA_CLIQUE_PERIODO: um ponto no canto mostra cada clique do robô (só nos vídeos dos testes) */
            if (getenv("APARA_CLIQUE_PERIODO")) {
                DrawCircle(GetScreenWidth() - 40, 40, 14, (Color){255, 60, 60, 255});
                rlDrawRenderBatchActive();   /* a captura lê a tela antes do EndDrawing: o ponto tem de sair do lote antes */
            }
            G.cliqueFlash -= (float)G.recDt;
        }
        if (G.recDir && wall >= G.recStart) {
            Image img = LoadImageFromScreen();
            ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
            if (getenv("APARA_REC_RAW")) {
                /* RGB cru, um quadro atrás do outro, na tela inteira: o ffmpeg lê e faz o vídeo (tools/gravar_video.sh) */
                if (!G.recRaw) {
                    G.recRaw = fopen(getenv("APARA_REC_RAW"), "wb");
                    fprintf(stderr, "REC_RAW %d %d\n", img.width, img.height);
                }
                if (G.recRaw) { fwrite(img.data, 1, (size_t)img.width * (size_t)img.height * 3, G.recRaw); fflush(G.recRaw); }
                G.recFrame++;
            } else {
                char path[512];
                snprintf(path, sizeof path, "%s/q%04d.png", G.recDir, G.recFrame++);
                ImageResize(&img, UI_W / 2, UI_H / 2);
                ExportImage(img, path);
            }
            UnloadImage(img);
            if (wall >= G.recEnd) { EndDrawing(); break; }
        }
        if (G.shotFile && wall >= G.shotTime) {
            Image img = LoadImageFromScreen();
            ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
            ImageResize(&img, UI_W, UI_H);
            ExportImage(img, G.shotFile);
            UnloadImage(img);
            EndDrawing();
            break;
        }
        EndDrawing();
        if (G.perf) {
            /* o EndDrawing termina no poll do quadro seguinte: daqui até o poll deste quadro é o quadro inteiro */
            double pf5 = entrada_relogio();
            const double sec[PERF_SECOES] = {pf5 - G.poll, pf1 - pf0, pf2 - pf1, pf3 - pf2, pf4 - pf3, pf5 - pf4};
            int i = G.perfDados.n;
            perf_quadro(&G.perfDados, sec);
            if (G.perfDados.n > i) { G.perfEstado[i] = (unsigned char)(G.state | (G.paused ? 0x80 : 0)); G.perfTempo[i] = (float)G.time; }
            if (pf5 - G.perfInicio >= G.perfSegundos) break;
        }
        if (G.lento) WaitTime(1.0 / 30);
    }
    if (G.recRaw) fclose(G.recRaw);
    if (G.perf) perf_relatorio();

    audio_shutdown();
    katana3d_unload();
    UnloadShader(G.post);
    UnloadRenderTexture(G.scene);
    UnloadRenderTexture(G.actors);
    if (G.ui.texture.id != GetFontDefault().texture.id) UnloadFont(G.ui);
    UnloadRenderTexture(G.uiLow);
    spr_shutdown();
    pix_shutdown();
    if (G.logImpactos) fprintf(stderr, "FANTASMAS %ld\n", G.fantasmasDesenhados);
    CloseWindow();
    return 0;
}
