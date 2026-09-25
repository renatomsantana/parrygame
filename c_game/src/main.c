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
 *   --state S      title | lore | trail | ending (com --master N: sensei;
 *                  com --master N --duel: pause | defeat | finisher | cleared)
 *   --demo         um robô apara no tempo perfeito e avança as telas
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
#define GHOST_MAX 10
#define VFX_MAX 12            /* efeitos das folhas tocando ao mesmo tempo */
#define AFTER_MAX 8           /* silhuetas que o mestre deixa nos movimentos rápidos */
enum { LEAP_NONE, LEAP_DASH, LEAP_JUMP, LEAP_FAR };
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
    ST_TITLE, ST_LORE, ST_TRAIL, ST_INTRO, ST_DUEL, ST_FINISHER, ST_OUTRO, ST_CLEARED, ST_DEFEAT, ST_SENSEI, ST_ENDING
} State;

/* Paleta da interface. */
static const Color INK = {24, 19, 16, 255};
static const Color PAPER = {236, 222, 192, 255};
static const Color AGED_GOLD = {201, 160, 82, 255};
static const Color VERMILION = {184, 62, 40, 255};
static const Color OCHRE = {214, 140, 58, 255};
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
    /* shizuku: quimono azul-claro até o chão, cabelo longo, florete fino. */
    {.coat = RGB(150, 190, 214), .sleeve = RGB(230, 236, 240), .pants = RGB(70, 90, 120), .skin = RGB(240, 206, 180),
     .hair = RGB(30, 30, 50), .blade = RGB(240, 244, 255), .hat = HAT_LONG_HAIR, .size = 1, .bladeLen = 26,
     .robe = 2, .flare = 0.6f, .trim = RGB(236, 240, 246), .handle = RGB(190, 196, 210), .bladeWidth = 0.5f},
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
    /* suiren: azul-marinho com acabamento verde-água, cabelo longo, lança. */
    {.coat = RGB(40, 60, 110), .sleeve = RGB(200, 214, 230), .pants = RGB(30, 40, 70), .skin = RGB(226, 186, 154),
     .hair = RGB(20, 30, 50), .blade = RGB(226, 236, 246), .hat = HAT_LONG_HAIR, .size = 1.05f, .bladeLen = 34,
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
    /* jinshi: monge da montanha, túnica longa, chapéu de palha, a katana branca forjada com a lua. */
    {.coat = RGB(110, 96, 80), .sleeve = RGB(110, 96, 80), .pants = RGB(80, 70, 60), .skin = RGB(210, 164, 126),
     .hair = RGB(40, 30, 26), .blade = RGB(248, 250, 255), .hat = HAT_KASA, .size = 1.1f, .bladeLen = 22,
     .robe = 1.6f, .flare = 0.3f, .extra = RGB(60, 36, 24), .extras = EX_BEADS, .handle = RGB(226, 226, 236), .bladeWidth = 1},
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
    struct { Rig rig; float life; Color color; } ghosts[GHOST_MAX];
    int ghostHead;
    float ghostTimer;
    Rig ren, boss;
    Fighter renS, bossS;
    float bossStep, bossStepTo, bossStepSpeed; /* passo do mestre até o alcance do golpe */
    float bossStrikeStep;     /* onde ele precisa estar no contato */
    int leap, leapStage;      /* investida correndo ou salto em curso (LEAP_*) */
    float leapT, leapAt, leapAir; /* tempo na preparação; quando corre ou salta; tempo no ar */
    float hopT, hopLen, hopH; /* arco do pulo do mestre (salto, recuo, ameaça) */
    float landT;              /* amortecendo a queda do salto */
    struct { const SprAnim *a; int frame; Vector2 feet; float life; } after[AFTER_MAX];
    int afterHead;
    float afterTimer, lastStep;
    bool gritoPending;
    struct { const SprFx *fx; int row; Vector2 pos; float t, fps; bool flip, back, glow; } vfx[VFX_MAX];
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

    /* Coreografia. */
    bool bossWinding;
    float windupLen, windupTime;
    bool strikeFeint;
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

    float shownRen, shownBoss, ghostRen, ghostBoss;
    ArenaCtx ctx;

    bool demo;
    const char *shotFile;
    float shotTime;
    const char *recDir;       /* --rec: quadros a 30 por segundo, para GIFs */
    float recStart, recEnd;
    int recFrame;
} G;

/* ------------------------------------------------------------------ */
/* Utilidades                                                          */
/* ------------------------------------------------------------------ */

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static Color fadec(Color c, float a) { c.a = (unsigned char)(c.a * clampf(a, 0, 1)); return c; }
static float smooth(float t) { t = clampf(t, 0, 1); return t * t * (3 - 2 * t); }

static bool pressed(void) {
    /* No modo demonstração, o robô também avança falas e painéis. */
    if (G.demo && G.state != ST_DUEL && fmodf(G.stateTime, 0.9f) < GetFrameTime()) return true;
    if (G.demo && (G.shotFile || G.recDir)) return false; /* capturas: só o robô joga */
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_J) || IsKeyPressed(KEY_ENTER);
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

/* Quebra em linhas e mostra só os primeiros `visible` bytes (máquina de escrever). */
static void ink_wrapped(const char *s, float x, float y, float width, float size, Color c, int visible) {
    char line[512], word[256], trial[512];
    int lineLen = 0, used = 0;
    float ly = y;
    const char *p = s;
    line[0] = 0;
    while (*p) {
        int wl = 0;
        while (p[wl] && p[wl] != ' ') wl++;
        if (wl > 255) wl = 255;
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
            snprintf(line, sizeof line, "%s", trial);
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

static void save_game(void) {
    if (G.demo) return;
    FILE *f = fopen(SAVE_FILE, "w");
    if (!f) return;
    fprintf(f, "APARA-C 2\n%d %u %d %d\n", G.camp.index, G.camp.clearedMask, G.camp.completed, G.camp.loreSeen);
    fclose(f);
}

static bool load_game(void) {
    FILE *f = fopen(SAVE_FILE, "r");
    if (!f) return false;
    int idx = 0, done = 0, lore = 0, ver = 0;
    unsigned mask = 0;
    bool ok = fscanf(f, "APARA-C %d\n%d %u %d %d", &ver, &idx, &mask, &done, &lore) == 5;
    fclose(f);
    if (!ok || idx < 0 || idx >= ROSTER_SIZE) return false;
    G.camp.index = idx;
    G.camp.clearedMask = mask;
    G.camp.completed = done;
    G.camp.loreSeen = lore;
    return true;
}

/* ------------------------------------------------------------------ */
/* Troca de tela                                                       */
/* ------------------------------------------------------------------ */

static void set_state(State s) {
    G.state = s;
    G.stateTime = 0;
}

/* ------------------------------------------------------------------ */
/* Lutadores em pixel art                                              */
/* ------------------------------------------------------------------ */

static const SprAnim *fa(const Fighter *f, const char *name) { return spr_anim(f->set, name); }

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
    if (f->idle || !spr_done(&f->pl)) return;
    if (f->qn > 0) {
        FSeg sg = f->q[0];
        memmove(f->q, f->q + 1, sizeof(FSeg) * (size_t)(f->qn - 1));
        f->qn--;
        if (sg.cycle) spr_cycle(&f->pl, sg.a, sg.dur);
        else spr_play(&f->pl, sg.a, sg.from, sg.to, sg.dur);
    } else if (f->autoIdle) {
        fighter_idle(f);
    }
}

static void fighter_load(Fighter *f, const char *id) {
    memset(f, 0, sizeof *f);
    f->set = spr_get(id);
    /* sem golpe desenhado não dá para lutar: fica o boneco */
    if (f->set && !spr_anim(f->set, "ATTACK_1")) f->set = NULL;
    fighter_idle(f);
}

/* Efeitos das folhas do pack (assets/sprites/_fx): tocam uma vez. Os de energia
 * (brilho, raios, fogo) vão atrás dos lutadores e somam luz; poeira e sangue vão
 * na frente. Sem a folha, ficam só as partículas. */
enum { VFX_FRONT = 0, VFX_BACK = 1, VFX_GLOW = 2 };

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
    G.vfx[slot].pos = pos;
    G.vfx[slot].t = 0;
    G.vfx[slot].fps = fps;
    G.vfx[slot].flip = flip;
    G.vfx[slot].back = flags & VFX_BACK;
    G.vfx[slot].glow = flags & VFX_GLOW;
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
        spr_fx_draw(G.vfx[i].fx, G.vfx[i].row, (int)(G.vfx[i].t * G.vfx[i].fps), G.vfx[i].pos, G.vfx[i].flip, WHITE);
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
    fighter_load(&G.bossS, G.m ? G.m->name : "");
    G.bossStep = G.bossStepTo = 0;
    G.bossStepSpeed = 0;
    G.leap = LEAP_NONE;
    G.hopT = G.hopLen = 0;
    memset(G.after, 0, sizeof G.after);
    G.gritoPending = false;
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

static void start_duel(void) {
    settings_default(&G.settings);
    settings_for_level(&G.settings, campaign_defeated(&G.camp));
    G.special = false;
    duel_init(&G.duel, &G.settings, G.m, (uint32_t)time(NULL) ^ (uint32_t)(G.camp.index * 7919));
    setup_actors();
    fx_clear(&G.fx);
    vfx_clear();
    G.shownRen = G.ghostRen = G.settings.renPosture;
    G.shownBoss = G.ghostBoss = G.m->posture;
    G.hitstop = 0;
    G.slowmo = 1;
    G.slowmoTime = 0;
    G.blackoutTarget = 0;
    G.ctx.blackout = 0;
    G.ctx.seal = 0;
    G.lightningTimer = 3;
    audio_music_intensity(0);
    banner(G.m->style, PAPER);
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
    fx_popup(&G.fx, "desarmado", (Vector2){160, 44}, 1.2f, PAPER);
    G.duo = 0.3f;
    G.slash = 0.35f;
    G.silence = 1.0f;
    G.slowmo = 0.3f;
    G.slowmoTime = 1.0f;
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
        vfx("70", 4, (Vector2){tip.x, GROUND_LOW - 8}, false, VFX_FRONT, 20);
        fx_burst(&G.fx, P_SPARK, tip, 6, 70, 0.8f, -1.57f, (Color){255, 240, 200, 255}, (Color){255, 190, 90, 255});
        fx_kick(&G.fx, 1.5f, 0.15f);
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

/* Empunhadura e ponta da arma do mestre (no sprite, uma estimativa pela altura). */
static void boss_blade(Vector2 *butt, Vector2 *tip) {
    if (!G.bossS.set) { rig_sword_line(&G.boss, butt, tip); return; }
    float bx = G.boss.x + G.boss.offsetX, h = (float)G.bossS.set->height;
    *butt = (Vector2){bx - 4, GROUND_LOW - h * 0.5f};
    *tip = (Vector2){bx - 4 - fmaxf(10, G.boss.look.bladeLen * 0.9f), GROUND_LOW - h * 0.75f};
}

/* Tipo do golpe k da sequência: o primeiro é o da sequência; os seguintes alternam. */
static MoveLook strike_look(void) {
    const Move *mv = duel_move(&G.duel);
    MoveLook look = mv ? mv->look : LOOK_HIGH;
    int k = G.duel.comboStrike;
    if (k == 0) return look;
    if (look == LOOK_HEAVY || look == LOOK_JUMP) return k % 2 ? LOOK_LOW : LOOK_HIGH;
    if (look == LOOK_THRUST || look == LOOK_DASH || look == LOOK_FAR) return k % 2 ? LOOK_HIGH : LOOK_THRUST;
    if (k % 2 == 0) return look;
    return look == LOOK_HIGH ? LOOK_LOW : LOOK_HIGH;
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
    /* as duas lâminas de uma vez: o corte cruzado */
    if (duel_strike_dual(&G.duel) && (a = fa(f, "ATTACK_3"))) return a;
    if (mv && mv->strikes >= 3 && G.duel.comboStrike == mv->strikes - 1 && (a = fa(f, "ESPECIAL"))) return a;
    /* a investida e a estocada de longe acabam na estocada; o salto desce com o corte alto */
    if (look == LOOK_DASH || look == LOOK_FAR) look = LOOK_THRUST;
    if (look == LOOK_JUMP) look = LOOK_HIGH;
    if (look == LOOK_THRUST && (a = fa(f, "DASH_ATTACK"))) return a;
    const char *base = look == LOOK_LOW ? "ATTACK_2" : (look == LOOK_THRUST ? "ATTACK_1" : "ATTACK_3");
    if (G.m->isBigBoss) {
        const char *stance = duel_stance(&G.duel)->name;
        for (int i = 0; i < MASTER_COUNT && stance; i++) {
            const MasterProfile *src = roster_get(i);
            if (strcmp(src->style, stance)) continue;
            snprintf(name, sizeof name, "%s_ECO_%s", base, src->name);
            for (char *u = name; *u; u++)
                if (*u >= 'a' && *u <= 'z') *u = (char)(*u - 32);
            if ((a = fa(f, name))) return a;
        }
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
    float w = G.windupLen;
    bool first = G.duel.comboStrike == 0;
    f_clear(f, false);
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
    f_add(f, a, 0, hold, first ? fminf(w * 0.6f, antic) : fminf(w, antic));
    G.bossStepTo = G.bossStep + (G.bossStrikeStep - G.bossStep) * 0.4f;
    float stepTime = first ? fminf(0.2f, w * 0.5f) : w * 0.8f;
    G.bossStepSpeed = fabsf(G.bossStepTo - G.bossStep) / fmaxf(0.06f, stepTime);
    if (first && fabsf(G.bossStepTo - G.bossStep) > 4) boss_hop(stepTime, 2);
}

/* A lâmina parte: os quadros entre o hold e o contato; o contato sai no impacto. */
static void sprite_launch(void) {
    Fighter *f = &G.bossS;
    if (!f->set || !f->strike) return;
    const SprAnim *a = f->strike;
    int hold = anim_hold(a), c = anim_contact(a);
    float lead = duel_strike_lead(&G.duel);
    G.bossStepTo = G.bossStrikeStep;
    G.bossStepSpeed = fabsf(G.bossStrikeStep - G.bossStep) / fmaxf(0.05f, lead - 0.02f);
    f_clear(f, false);
    int from = hold + 1;
    if (G.leap == LEAP_DASH) from = hold > 2 ? hold - 2 : 0;   /* da corrida direto para o golpe */
    if (c - 1 >= from) f_add(f, a, from, c - 1, lead);
    else f_add(f, a, hold, hold, lead);
}

/* O gesto de kojiro: a guarda (DEFEND) ou um corte rápido de encontro ao golpe. */
static void sprite_press(void) {
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
        f_add(b, a, c, c, 0.06f);
        if (e->flag) {
            /* postura quebrada: cambaleia e fica curvado até se recompor */
            if (hurt) f_add(b, hurt, 0, hurt->frames - 1, 0);
            else if (c + 1 < a->frames) f_add(b, a, c + 1, a->frames - 1, 0);
            b->autoIdle = false;
        } else if (e->judgement == J_PERFEITO && hurt) {
            f_add(b, hurt, 0, hurt->frames - 1, 0);
        } else if (c + 1 < a->frames) {
            f_add(b, a, c + 1, a->frames - 1, 0);
        }
        b->strike = NULL;
        if (G.leap == LEAP_JUMP) {   /* a poeira da queda, e os joelhos dobram */
            vfx("70", 4, (Vector2){G.boss.x + G.boss.offsetX, GROUND_LOW - 8}, true, VFX_FRONT, 24);
            G.landT = 0.2f;
        }
        /* entre os golpes de uma sequência ele fica onde está; no fim, volta */
        G.bossStepTo = G.duel.comboRemaining > 0 ? G.bossStep : 0;
        G.bossStepSpeed = 40;
        G.hopT = G.hopLen;
        G.leap = LEAP_NONE;
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
    b->trail = false;
    switch (e->judgement) {
        case J_PERFEITO:
            G.hitstop = e->flag ? G.settings.breakHitstop : G.settings.perfectHitstop;
            G.aberr = 1.5f;
            audio_play(SND_PERFECT, 1, 1 + (rand() % 5) * 0.02f);
            fx_burst(&G.fx, P_SPARK, at, 26, 170, 1.2f, -0.5f, (Color){255, 255, 230, 255}, (Color){255, 200, 90, 255});
            fx_burst(&G.fx, P_SPARK, at, 12, 130, 0.9f, 3.14f + 0.5f, (Color){255, 240, 200, 255}, (Color){255, 180, 60, 255});
            fx_ring(&G.fx, at, 180, 0.3f, 2, (Color){255, 245, 210, 230});
            fx_star(&G.fx, at, 16, 0.12f);
            vfx("652", 5, at, false, VFX_BACK | VFX_GLOW, 32);   /* raios de luz atrás do choque */
            fx_flash(&G.fx, (Color){255, 250, 235, 90}, 1);
            fx_kick(&G.fx, 1.5f, 0.1f);
            rig_pose(r, POSE_DEFLECT, 0.05f, EASE_OUT);
            rig_then(r, POSE_IDLE, 0.4f, EASE_INOUT);
            rig_pose(b, POSE_HURT, 0.07f, EASE_OUT);
            rig_then(b, POSE_IDLE, 0.5f, EASE_INOUT);
            b->flash = 1;
            b->flashColor = WHITE;
            G.bossKnock = 7;
            G.renKnock = 1;
            break;
        case J_BOM:
            G.hitstop = G.settings.goodHitstop;
            audio_play(SND_GOOD, 0.9f, 1);
            fx_burst(&G.fx, P_SPARK, at, 10, 110, 1.0f, -0.6f, (Color){255, 230, 120, 255}, (Color){255, 170, 50, 255});
            fx_flash(&G.fx, (Color){255, 230, 120, 40}, 1);
            vfx("63", 0, at, false, VFX_GLOW, 24);                /* faíscas douradas */
            rig_pose(r, POSE_DEFLECT, 0.06f, EASE_OUT);
            rig_then(r, POSE_IDLE, 0.4f, EASE_INOUT);
            rig_pose(b, POSE_FOLLOW, 0.08f, EASE_OUT);
            rig_then(b, POSE_IDLE, 0.45f, EASE_INOUT);
            G.renKnock = 4;
            G.bossKnock = 3;
            break;
        default: {
            G.hitstop = G.settings.badHitstop;
            G.aberr = 2.5f;
            audio_play(SND_BAD, 1, 1);
            Vector2 hit = {r->x + 4, GROUND_LOW - 28};
            fx_burst(&G.fx, P_SPARK, hit, 14, 140, 1.1f, 3.14f, (Color){255, 80, 60, 255}, (Color){255, 160, 90, 255});
            fx_burst(&G.fx, P_DUST, (Vector2){r->x, GROUND_LOW - 1}, 6, 40, 0.6f, 3.14f, (Color){200, 180, 160, 140}, (Color){120, 100, 90, 110});
            fx_flash(&G.fx, (Color){255, 40, 30, 80}, 1);
            vfx("71", 7, hit, false, VFX_FRONT, 28);              /* estouro vermelho em kojiro */
            fx_kick(&G.fx, 3, 0.2f);
            rig_pose(r, POSE_HURT, 0.06f, EASE_OUT);
            rig_then(r, POSE_IDLE, 0.45f, EASE_INOUT);
            r->flash = 1;
            r->flashColor = (Color){255, 80, 60, 255};
            rig_pose(b, POSE_FOLLOW, 0.1f, EASE_OUT);
            rig_then(b, POSE_IDLE, 0.5f, EASE_INOUT);
            G.renKnock = 9;
            G.renParryTime = -1;
            break;
        }
    }
    if ((e->i & 1) && e->judgement == J_PERFEITO) {
        /* as duas lâminas aparadas: o segundo tinido e o X de faíscas */
        audio_play(SND_PERFECT, 0.7f, 1.25f);
        fx_burst(&G.fx, P_SPARK, at, 12, 150, 0.5f, -2.3f, (Color){230, 240, 255, 255}, (Color){160, 190, 255, 255});
        fx_burst(&G.fx, P_SPARK, at, 12, 150, 0.5f, 2.3f, (Color){230, 240, 255, 255}, (Color){160, 190, 255, 255});
    }
    if ((e->i & 2) && e->judgement != J_RUIM) second_blade();
    if (e->flag) {
        G.aberr = 3.5f;
        G.desat = 1;
        G.crack = 0.14f;
        G.silence = 0.5f;
        audio_play(SND_BREAK, 0.9f, 1); /* cerâmica rachando e taiko; depois, meio segundo de silêncio */
        Vector2 c = {b->x, GROUND_LOW - 30};
        fx_burst(&G.fx, P_SHARD, c, 20, 160, 1.4f, -1.57f, (Color){230, 230, 255, 255}, (Color){180, 140, 255, 255});
        fx_ring(&G.fx, c, 320, 0.5f, 3, WHITE);
        vfx("184", 5, c, false, VFX_BACK | VFX_GLOW, 24);        /* onda de choque */
        fx_flash(&G.fx, WHITE, 1);
        fx_kick(&G.fx, 4, 0.35f);
        rig_pose(b, POSE_STAGGER, 0.15f, EASE_OUT);
        G.staggerTime = 0.01f;
        G.bossKnock = 10;
        G.slowmo = 0.3f;
        G.slowmoTime = 0.8f;
    }
}

/* Sinal próprio de cada vilão no começo de cada sequência: nunca dois iguais. */
static void tell_fx(void) {
    Rig *b = &G.boss;
    Vector2 butt, tip;
    boss_blade(&butt, &tip);
    Vector2 mid = {(butt.x + tip.x) / 2, (butt.y + tip.y) / 2};
    Vector2 feet = {b->x + b->offsetX - 9 * b->look.size, GROUND_LOW - 1};
    /* tom do gesto de cada mestre, na ordem da trilha */
    static const float pitch[ROSTER_SIZE] = {0.7f, 0.8f, 0.6f, 1.5f, 1.0f, 1.25f, 1.4f, 1.1f, 1.3f, 1.6f, 1.2f, 0.65f, 0.9f};
    switch (G.m->id) {
        case 1: fx_burst(&G.fx, P_SPARK, feet, 10, 70, 0.6f, -1.2f, (Color){255, 190, 110, 255}, (Color){255, 140, 60, 255}); break;
        case 2: fx_burst(&G.fx, P_GEM, tip, 8, 40, 1.2f, 1.57f, (Color){200, 236, 255, 255}, (Color){120, 190, 240, 255}); break;
        case 3: fx_burst(&G.fx, P_DUST, mid, 10, 20, 3.14f, 0, (Color){60, 40, 80, 170}, (Color){30, 20, 40, 150}); break;
        case 4: fx_burst(&G.fx, P_DUST, feet, 14, 50, 0.8f, -1.57f, (Color){170, 130, 90, 170}, (Color){110, 80, 50, 150}); break;
        case 5: fx_burst(&G.fx, P_PETAL, (Vector2){b->x + 20, GROUND_LOW - 40}, 10, 90, 0.4f, 3.14f, (Color){236, 240, 230, 220}, (Color){180, 220, 200, 200}); break;
        case 6: fx_burst(&G.fx, P_GEM, (Vector2){mid.x + 6, mid.y + 8}, 8, 30, 3.14f, 0, (Color){200, 230, 170, 255}, (Color){140, 170, 110, 255}); break;
        case 7: fx_burst(&G.fx, P_EMBER, mid, 16, 30, 3.14f, -1.57f, (Color){255, 190, 80, 255}, (Color){255, 90, 30, 255}); break;
        case 8: fx_burst(&G.fx, P_GEM, tip, 10, 50, 0.9f, -1.57f, (Color){170, 230, 240, 255}, (Color){90, 170, 220, 255}); break;
        case 9: fx_burst(&G.fx, P_PETAL, (Vector2){b->x, GROUND_LOW - 44}, 8, 50, 3.14f, -1.57f, (Color){30, 30, 40, 230}, (Color){60, 60, 80, 230}); break;
        case 10: fx_burst(&G.fx, P_SPARK, tip, 12, 90, 3.14f, 0, (Color){210, 190, 255, 255}, (Color){140, 110, 255, 255}); break;
        case 11: fx_burst(&G.fx, P_SHARD, feet, 10, 60, 0.7f, -1.57f, (Color){150, 146, 140, 255}, (Color){100, 96, 90, 255}); break;
        default: fx_burst(&G.fx, P_DUST, mid, 18, 16, 3.14f, 0, (Color){150, 90, 200, 150}, (Color){90, 50, 130, 130}); break;
    }
    audio_play(SND_GESTURE, 0.3f, pitch[(G.m->id - 1) % ROSTER_SIZE]);
    /* E o efeito do pack de cada um: onde nasce (no chão ou no corpo) e a cor. Oboro
     * usa o do aprendiz da postura em que está, em vermelho. */
    static const struct { const char *fx; int row; float y; int flags; } TELL[ROSTER_SIZE] = {
        {"70", 4, -8, VFX_FRONT},               /* daichi: poeira de terra */
        {"26", 3, -30, VFX_BACK | VFX_GLOW},    /* genbu: o casco, anel verde */
        {"14", 7, -30, VFX_BACK | VFX_GLOW},    /* raizo: rajada vermelha */
        {"06", 2, -30, VFX_BACK},               /* shizuku: respingo */
        {"64", 0, -30, VFX_BACK | VFX_GLOW},    /* garfiel: garras */
        {"64", 8, -30, VFX_BACK},               /* karasu: asas escuras */
        {"03", 3, -30, VFX_BACK | VFX_GLOW},    /* hayate: redemoinho */
        {"69", 0, -27, VFX_BACK | VFX_GLOW},    /* enjin: labareda */
        {"04", 2, -24, VFX_BACK},               /* suiren: onda */
        {"195", 2, -30, VFX_BACK | VFX_GLOW},   /* arashi: raios */
        {"197", 1, -30, VFX_BACK | VFX_GLOW},   /* yoru: estrela da noite */
        {"665", 5, -21, VFX_BACK},              /* jinshi: o pico da montanha */
        {"197", 7, -30, VFX_BACK | VFX_GLOW},   /* oboro */
    };
    int ti = (G.m->id - 1) % ROSTER_SIZE, row = TELL[ti].row;
    if (G.m->isBigBoss) {
        const char *stance = duel_stance(&G.duel)->name;
        for (int i = 0; i < MASTER_COUNT && stance; i++)
            if (!strcmp(roster_get(i)->style, stance)) { ti = i; row = 7; }
    }
    vfx(TELL[ti].fx, row, (Vector2){b->x + b->offsetX, GROUND_LOW + TELL[ti].y}, true, TELL[ti].flags, 22);
}

/* A vida de kojiro acaba: ele cai e o painel de derrota aparece. */
static void ren_falls(void) {
    rig_pose(&G.ren, POSE_FALLEN, 0.7f, EASE_OUT);
    sprite_fall();
    vfx("70", 4, (Vector2){G.ren.x + G.ren.offsetX, GROUND_LOW - 8}, false, VFX_FRONT, 20);
    G.ren.breath = 0;
    G.slowmo = 0.4f;
    G.slowmoTime = 0.9f;
    fx_popup(&G.fx, "kojiro caiu", (Vector2){160, 44}, 1.2f, VERMILION);
    audio_play(SND_DEFEAT, 0.9f, 1);
    G.defeatsHere++;
    G.defeatIndex = 0;
    set_state(ST_DEFEAT);
}

/* As duas lâminas vão vir juntas: brilham as duas e soa um tinido duplo. */
static void dual_tell(void) {
    Vector2 c = {G.boss.x + G.boss.offsetX, GROUND_LOW - 30};
    fx_star(&G.fx, (Vector2){c.x - 7, c.y - 5}, 11, 0.2f);
    fx_star(&G.fx, (Vector2){c.x + 3, c.y + 3}, 9, 0.2f);
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
    fx_burst(&G.fx, P_SPARK, hit, 12, 130, 1.1f, 3.14f, (Color){255, 80, 60, 255}, (Color){255, 160, 90, 255});
    fx_flash(&G.fx, (Color){255, 40, 30, 70}, 1);
    vfx("71", 7, hit, false, VFX_FRONT, 28);
    fx_kick(&G.fx, 2.5f, 0.18f);
    r->flash = 1;
    r->flashColor = (Color){255, 80, 60, 255};
    G.renKnock = fmaxf(G.renKnock, 8);
    G.hitstop = fmaxf(G.hitstop, G.settings.badHitstop);
}

static void handle_events(void) {
    DuelEvent ev[MAX_EVENTS];
    int n = duel_drain(&G.duel, ev, MAX_EVENTS);
    const MasterProfile *m = G.m;
    Rig *b = &G.boss, *r = &G.ren;
    for (int i = 0; i < n; i++) {
        const DuelEvent *e = &ev[i];
        switch (e->kind) {
            case EV_WINDUP:
                G.strikeFeint = e->flag;
                G.windupLen = fmaxf(0.1f, e->a - duel_strike_lead(&G.duel));
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
                if (duel_strike_dual(&G.duel)) dual_tell();
                G.blackoutTarget = G.duel.blackout ? 1 : 0;
                if (m->arena == ARENA_PORTO) { audio_play(SND_DRUM, 0.9f, 1); G.ctx.beat = 1; }
                break;
            case EV_FEINT_LAUNCH:
                G.bossWinding = false;
                if (duel_stance(&G.duel)->mimicParry) {
                    rig_pose(b, POSE_PARRY, 0.08f, EASE_OUT);
                    rig_then(b, POSE_WINDUP, 0.25f, EASE_INOUT);
                } else {
                    /* Parte como um golpe de verdade e trava no instante falso. */
                    rig_pose(b, POSE_FEINT, G.settings.attackLead, EASE_IN);
                    rig_then(b, windup_pose(strike_look()), 0.2f, EASE_OUT);
                }
                audio_play(SND_SWING, 0.5f, 1.2f);
                if (m->rhythmJitter > 0) boss_hop(0.3f, 9); /* hayate ameaça pular */
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
                audio_play(e->flag ? SND_CUE_FEINT : SND_CUE, m->cueAudio, 1);
                break;
            case EV_PRESS:
                rig_pose(r, G.duel.phase == PH_WINDUP ? parry_pose(strike_look()) : POSE_PARRY, 0.06f, EASE_OUT);
                sprite_press();
                G.renParryTime = 0;
                audio_play(SND_GESTURE, 0.8f, 1 + (rand() % 7) * 0.02f);
                break;
            case EV_IMPACT:
                on_impact(e);
                sprite_impact(e);
                G.special = false;
                G.renParryTime = -1;
                G.blackoutTarget = 0;
                break;
            case EV_STANCE:
                banner(duel_stance(&G.duel)->name, AGED_GOLD);
                break;
            case EV_SEAL: {
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
            case EV_FINISHED:
                if (e->flag) {
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

/* Guarda uma silhueta de quem está em movimento rápido. */
static void update_ghosts(float dt) {
    for (int i = 0; i < GHOST_MAX; i++) G.ghosts[i].life = fmaxf(0, G.ghosts[i].life - dt * 4);
    Rig *src = G.boss.trail ? &G.boss : ((G.ren.trail || (G.renParryTime >= 0 && G.renParryTime < 0.12f)) ? &G.ren : NULL);
    /* nas pranchas o rastro do golpe já vem desenhado */
    if (!src || (src == &G.boss && G.bossS.set) || (src == &G.ren && G.renS.set)) return;
    G.ghostTimer -= dt;
    if (G.ghostTimer > 0) return;
    G.ghostTimer = 0.03f;
    G.ghostHead = (G.ghostHead + 1) % GHOST_MAX;
    G.ghosts[G.ghostHead].rig = *src;
    G.ghosts[G.ghostHead].rig.flash = 0;
    G.ghosts[G.ghostHead].life = 1;
    G.ghosts[G.ghostHead].color = (G.ghostHead % 2) ? (Color){80, 220, 255, 255} : (Color){255, 70, 200, 255};
}

/* Pranchas e o passo do mestre até o alcance do golpe (e de volta ao lugar). */
static void fighters_update(float dt) {
    f_update(&G.renS, dt);
    f_update(&G.bossS, dt);
    if (!G.bossS.set) return;
    bool landed = G.hopT >= G.hopLen;
    /* investida e salto: depois do recuo (ou de agachar) ele arranca */
    if ((G.leap == LEAP_DASH || G.leap == LEAP_JUMP) && G.bossWinding) {
        G.leapT += dt;
        if (G.leapStage == 0 && G.leapT >= G.leapAt) {
            G.leapStage = 1;
            vfx("70", 4, (Vector2){G.boss.x + G.boss.offsetX, GROUND_LOW - 8}, true, VFX_FRONT, 22);
            if (G.leap == LEAP_DASH) {
                G.bossStepTo = G.bossStrikeStep + 10;
                G.bossStepSpeed = fabsf(G.bossStepTo - G.bossStep) / fmaxf(0.05f, G.windupLen - G.leapAt);
            } else {
                G.bossStepTo = G.bossStrikeStep;
                G.bossStepSpeed = fabsf(G.bossStepTo - G.bossStep) / fmaxf(0.05f, G.leapAir);
                boss_hop(G.leapAir, 24);
            }
        }
    }
    /* agacha antes do salto e firma o corpo no fim da preparação */
    G.landT = fmaxf(0, G.landT - dt);
    int squat = 0;
    if (G.leap == LEAP_JUMP && G.leapStage == 0) squat = 1 + (int)(2.99f * clampf(G.leapT / G.leapAt, 0, 1));
    else if (G.landT > 0) squat = G.landT > 0.1f ? 3 : 1;
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
            if (j) {
                f_clear(&G.bossS, true);
                f_add(&G.bossS, j, j->frames - 1, j->frames - 1, t);
            }
        } else {
            G.bossStepTo = 0;
            G.bossStepSpeed = fmaxf(G.bossStepSpeed, 30);
        }
    }
    float d = G.bossStepTo - G.bossStep, mv = G.bossStepSpeed * dt;
    G.bossStep = fabsf(d) <= mv ? G.bossStepTo : G.bossStep + (d > 0 ? mv : -mv);
}

/* Silhuetas que o mestre deixa para trás no bote, na corrida e no salto. */
static void update_after(float dt) {
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

static void update_actors(float dt) {
    Rig *b = &G.boss, *r = &G.ren;
    fighters_update(dt);
    rig_update(r, dt);
    rig_update(b, dt);
    /* Gesto sem golpe: Ren volta à guarda sozinho. */
    if (G.renParryTime >= 0) {
        G.renParryTime += dt;
        if (G.renParryTime > 0.3f) { rig_pose(r, POSE_IDLE, 0.22f, EASE_INOUT); G.renParryTime = -1; }
    }
    /* Golpe especial: o mestre arde em vermelhão enquanto arma. */
    if (G.special && G.state == ST_DUEL) {
        b->flash = 0.35f + 0.2f * sinf(G.time * 14);
        b->flashColor = VERMILION;
    }
    /* Tensão no fim da preparação. */
    if (G.bossWinding) {
        G.windupTime += dt;
        G.ctx.danger = clampf(G.windupTime / G.windupLen, 0, 1);
    } else {
        G.ctx.danger *= expf(-dt * 6);
    }
    /* BIG BOSS se recompõe depois de perder um selo. */
    if (G.staggerTime > 0) {
        G.staggerTime += dt;
        if (G.state == ST_DUEL && G.staggerTime > 1.3f) {
            rig_pose(b, POSE_IDLE, 0.5f, EASE_INOUT);
            G.staggerTime = 0;
            const SprAnim *grito = fa(&G.bossS, "SHOUT");   /* o grito do pack; sem ele, o montado */
            if (!grito) grito = fa(&G.bossS, "GRITO");
            if (G.gritoPending && grito) {
                /* oboro grita e volta em fúria */
                G.bossS.furia = true;
                f_clear(&G.bossS, true);
                f_add(&G.bossS, grito, 0, grito->frames - 1, 0);
            } else {
                fighter_idle(&G.bossS);
            }
            G.gritoPending = false;
        }
    }
    if (G.hopT < G.hopLen) {
        G.hopT = fminf(G.hopLen, G.hopT + dt);
        b->hopY = sinf(G.hopT / G.hopLen * PI) * G.hopH;
    } else {
        b->hopY = 0;
    }
    update_ghosts(dt);
    /* O cansaço segue a vida de kojiro e a postura do mestre. */
    if (G.state == ST_DUEL || G.state == ST_INTRO) {
        r->fatigue = G.state == ST_DUEL ? 1 - clampf(G.duel.renPosture / G.settings.renPosture, 0, 1) : 0;
        b->fatigue = G.state == ST_DUEL ? 1 - clampf(G.duel.bossPosture / G.m->posture, 0, 1) : 0;
    }
    G.renKnock *= expf(-dt * 9);
    G.bossKnock *= expf(-dt * 7);
    r->offsetX = -G.renKnock;
    b->offsetX = G.bossKnock + (G.bossS.set ? G.bossStep : 0);
    update_after(dt);
}

static void update_ctx(float dt) {
    G.ctx.t += dt;
    G.ctx.beat *= expf(-dt * 6);
    G.ctx.blackout += (G.blackoutTarget - G.ctx.blackout) * (1 - expf(-dt * (G.blackoutTarget > G.ctx.blackout ? 5 : 3)));
    G.ctx.lightning = fmaxf(0, G.ctx.lightning - dt * 3);
    /* tempestade: no dojo de oboro depois do primeiro selo, e a noite toda no castelo de arashi */
    bool storm = (G.m->arena == ARENA_CIDADELA && G.ctx.seal >= 1) || G.m->arena == ARENA_SALAO;
    if (storm) {
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

static void update_duel(float dtReal) {
    float dt = dtReal * G.slowmo;
    if (G.slowmoTime > 0) { G.slowmoTime -= dtReal; if (G.slowmoTime <= 0) G.slowmo = 1; }
    bool press = pressed();
    /* o robô aperta no meio do quadro (ver abaixo), logo antes do contato */
    if (G.demo && G.duel.phase == PH_WINDUP && !G.duel.attempted && G.duel.strikeAt - G.duel.clock <= dt * 0.5 + 0.04) press = true;

    /* Hitstop congela o duelo e as poses. */
    if (G.hitstop > 0) {
        G.hitstop -= dtReal;
        if (press) duel_press(&G.duel);
        handle_events();
        audio_music_duck(G.silence > 0 ? 1 : 0.6f);
        return;
    }
    audio_music_duck(G.silence > 0 ? 1 : 0);
    if (press) {
        /* O clique chegou em algum ponto do último quadro: aplicamos no meio. */
        duel_tick(&G.duel, dt * 0.5);
        duel_press(&G.duel);
        duel_tick(&G.duel, dt * 0.5);
    } else {
        duel_tick(&G.duel, dt);
    }
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
    if (G.sword.stuck && G.sword.stuckTime > 1.1f) start_lines(G.m->outro, G.m->outroCount, ST_OUTRO);
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

/* Lutador no estilo de ação 2D: contorno escuro de 1 px e um filete de neon do lado de trás. */
static void draw_fighter(const Rig *r, Color light, Color rim, bool dark) {
    static const int off[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    Color ink = {16, 12, 18, 255};
    Rig t = *r;
    for (int k = 0; k < 4; k++) {
        t.x = r->x + off[k][0];
        t.y = r->y + off[k][1];
        rig_draw_flat(&t, ink);
    }
    if (dark) {
        rig_draw_flat(r, (Color){24, 20, 36, 255});
        return;
    }
    t = *r;
    t.x = r->x + (r->faceLeft ? 1 : -1);
    t.y = r->y - 1;
    rig_draw_flat(&t, rim);
    rig_draw(r, light);
}

/* O mesmo contorno escuro e o filete de luz dos bonecos, em volta do sprite. */
static void draw_sprite_fighter(const Rig *r, const Fighter *f, Color light, Color rim, bool dark) {
    const SprAnim *a = f->pl.anim;
    if (!a) return;
    int frame = f->pl.frame;
    Vector2 feet = {r->x + r->offsetX, r->y - r->hopY};
    int breath = 0;
    if (f->idle && !a->loop) {
        /* quem não tem prancha parada respira: o tronco desce 1 px, mais rápido cansado */
        float period = 1.8f - 0.8f * r->fatigue;
        breath = fmodf(r->time, period) > period * 0.5f ? 1 : 0;
    }
    if (f->squat > breath) breath = f->squat;
    SprDraw o = {r->faceLeft, breath, true, (Color){16, 12, 18, 255}};
    static const int off[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int k = 0; k < 4; k++) spr_draw(f->set, a, frame, (Vector2){feet.x + off[k][0], feet.y + off[k][1]}, o);
    if (dark) {
        o.color = (Color){24, 20, 36, 255};
        spr_draw(f->set, a, frame, feet, o);
        return;
    }
    o.color = rim;
    spr_draw(f->set, a, frame, (Vector2){feet.x + (r->faceLeft ? 1 : -1), feet.y - 1}, o);
    o.flat = false;
    o.color = light;
    spr_draw(f->set, a, frame, feet, o);
    if (r->flash > 0) {
        o.flat = true;
        o.color = fadec(r->flashColor, fminf(1, r->flash) * 0.6f);
        spr_draw(f->set, a, frame, feet, o);
    }
}

static void draw_rigs(Color light) {
    BeginTextureMode(G.actors);
    ClearBackground(BLANK);
    bool dark = G.ctx.blackout > 0.5f;
    Color rim = arena_rim(G.m->arena);
    begin_actors();
    /* Rastros: silhuetas que ficam para trás, alternando ciano e magenta. */
    for (int n = GHOST_MAX; n >= 1; n--) {
        int i = (G.ghostHead - n + GHOST_MAX * 2) % GHOST_MAX;
        if (G.ghosts[i].life <= 0) continue;
        Color c = G.ghosts[i].color;
        c.a = (unsigned char)(150 * G.ghosts[i].life);
        rig_draw_flat(&G.ghosts[i].rig, c);
    }
    if (G.bossS.set && !dark) {
        /* as silhuetas do movimento, na cor do elemento de cada mestre */
        static const Color AFTER_TINT[ROSTER_SIZE] = {
            {230, 150, 80, 255}, {130, 210, 150, 255}, {235, 90, 70, 255}, {110, 180, 255, 255},
            {250, 240, 210, 255}, {120, 110, 190, 255}, {140, 240, 200, 255}, {255, 140, 50, 255},
            {90, 150, 240, 255}, {190, 150, 255, 255}, {130, 110, 220, 255}, {225, 225, 235, 255},
            {235, 60, 70, 255},
        };
        Color c = AFTER_TINT[(G.m->id - 1) % ROSTER_SIZE];
        for (int n = 1; n <= AFTER_MAX; n++) {       /* da mais antiga para a mais nova */
            int i = (G.afterHead + n) % AFTER_MAX;
            if (G.after[i].life <= 0 || !G.after[i].a) continue;
            SprDraw o = {G.boss.faceLeft, 0, true, fadec(c, 0.45f * G.after[i].life)};
            spr_draw(G.bossS.set, G.after[i].a, G.after[i].frame, G.after[i].feet, o);
        }
    }
    if (G.bossS.set) draw_sprite_fighter(&G.boss, &G.bossS, light, rim, dark);
    else draw_fighter(&G.boss, light, rim, dark);
    if (G.renS.set) draw_sprite_fighter(&G.ren, &G.renS, light, rim, false);
    else draw_fighter(&G.ren, light, rim, false);
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
    DrawEllipse((int)(r->x + r->offsetX), GROUND_LOW, w, 2, (Color){0, 0, 0, 80});
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
    draw_shadow(&G.boss);
    draw_shadow(&G.ren);
    vfx_draw(true);
    DrawTexturePro(G.actors.texture, (Rectangle){0, 0, LOW_W, -LOW_H}, (Rectangle){0, 0, LOW_W, LOW_H}, (Vector2){0, 0}, 0, WHITE);
    fx_draw_world(&G.fx);
    vfx_draw(false);
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

static void ending_scene(int page, float t) { (void)page; lore_draw_ending(t); }

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
        case ST_ENDING: draw_illustration(ending_scene, 0, G.stateTime, 0); break;   /* texto em cima */
        case ST_SENSEI: draw_illustration(lore_draw_scene, 0, G.time, 18); break;
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

/* Selo de oboro: losango de pixel, vermelho enquanto está de pé, apagado quando cai.
 * (x, y) é o canto de cima à esquerda do desenho de 5 x 5. */
static void ui_seal(float x, float y, bool broken) {
    static const char *SHAPE[5] = {"..#..", ".#o#.", "#ooo#", ".#o#.", "..#.."};
    Color fill = broken ? (Color){150, 128, 100, 255} : SEAL_RED;
    x = snap(x); y = snap(y);
    for (int j = 0; j < 5; j++)
        for (int i = 0; i < 5; i++)
            if (SHAPE[j][i] != '.') px_rect(x + i * PX, y + j * PX, PX, PX, SHAPE[j][i] == '#' ? INK_LINE : fill);
    if (!broken) px_rect(x + 2 * PX, y + PX, PX, PX, (Color){236, 130, 100, 255});   /* brilho */
}

#define SEAL_STEP 24.0f   /* distância entre os selos, em unidades da interface */

/* Placa de status no jeito RPG Maker: pergaminho pequeno com nome, selos e o medidor
 * (postura dos mestres, vida de kojiro). */
static void ui_status(Rectangle r, const char *name, const char *note, const char *gauge, int seals, int broken, float value, float ghost, float max, Color a, Color b) {
    parchment(r, 0.96f);
    ink_bold(name, r.x + 18, r.y + 12, 24, INK_TEXT);
    for (int i = 0; i < seals; i++)
        ui_seal(r.x + 18 + ui_width_f(G.uiBold, name, 24) + 12 + i * SEAL_STEP, r.y + 14, i < broken);
    if (note && note[0]) ink_right(note, r.x + r.width - 18, r.y + 12, 18, INK_SOFT);
    ink(gauge, r.x + 18, r.y + 46, 16, INK_SOFT);
    float gx = r.x + 18 + ui_width(gauge, 16) + 16;   /* o medidor começa depois da palavra */
    ui_gauge(gx, r.y + 50, r.x + r.width - 20 - gx, value, ghost, max, a, b);
}

static void ui_hud(void) {
    const MasterProfile *m = G.m;
    bool pressure = duel_under_pressure(&G.duel) && m->sealCount <= 1;
    const char *name = lower(m->name), *note = lower(m->sealCount > 1 ? duel_stance(&G.duel)->name : m->style);
    int seals = m->sealCount > 1 ? m->sealCount : 0;
    /* A placa cresce para caber nome, selos e postura na mesma linha. */
    float need = 18 + ui_width_f(G.uiBold, name, 24) + (seals ? 16 + seals * SEAL_STEP : 0) + 32 + ui_width(note, 18) + 18;
    float tw = snap(fmaxf(460, need));
    Rectangle top = {snap(UI_W / 2 - tw / 2), 16, tw, 76};
    ui_status(top, name, note, "postura", seals, G.duel.seal, G.shownBoss, G.ghostBoss, m->posture,
              pressure ? (Color){150, 40, 30, 255} : (Color){78, 62, 104, 255}, pressure ? (Color){200, 80, 50, 255} : (Color){134, 108, 160, 255});
    Rectangle bot = {UI_W / 2 - 230, UI_H - 92, 460, 76};
    bool low = G.shownRen <= G.settings.renPosture * 0.25f;
    /* kojiro não tem postura: tem vida, em vermelho, que pulsa quando está no fim */
    float pulse = low ? 0.5f + 0.5f * sinf(G.time * 8) : 0;
    ui_status(bot, "kojiro", NULL, "vida", 0, 0, G.shownRen, G.ghostRen, G.settings.renPosture,
              (Color){(unsigned char)(140 + 40 * pulse), 26, 30, 255}, (Color){(unsigned char)(212 + 30 * pulse), 62, 56, 255});

    if (G.bannerTime > 0) {
        float a = clampf(G.bannerTime * 2, 0, 1);
        const char *t = lower(G.banner);
        float w = ui_width_f(G.uiBold, t, 34) + 100;
        Rectangle r = {UI_W / 2 - w / 2, 262, w, 64};
        parchment(r, a);
        scroll_rods(r, a);
        ink_bold_center(t, UI_W / 2.0f, r.y + 14, 34, fadec(INK_TEXT, a));
    }
}

/* Falas. No cenário do duelo a caixa fica em cima, para os lutadores aparecerem
 * inteiros; nas ilustrações (hanzo), embaixo. */
static void ui_dialogue(const char *header, bool top) {
    if (top) ui_text(lower(header), UI_W - 48 - ui_width(lower(header), 24), UI_H - 64, 24, (Color){230, 216, 190, 220});
    else ui_text(lower(header), 48, 30, 24, (Color){230, 216, 190, 220});
    if (G.lineIndex >= G.lineCount) return;
    const Line *l = &G.lines[G.lineIndex];
    bool ren = strcmp(l->speaker, "kojiro") == 0;
    Rectangle r = top ? (Rectangle){80, 76, UI_W - 160, 168} : (Rectangle){80, 512, UI_W - 160, 180};
    parchment(r, 1);
    scroll_rods(r, 1);
    /* Caixa do nome separada, como no RPG Maker. */
    const char *who = lower(l->speaker);
    float nw = ui_width_f(G.uiBold, who, 26) + 44;
    Rectangle nb = {r.x + 24, r.y - 34, nw, 46};
    parchment(nb, 1);
    ink_bold(who, nb.x + 22, nb.y + 10, 26, ren ? (Color){170, 70, 24, 255} : SEAL_RED);
    int vis = utf8_visible(l->text, G.typeChars);
    ink_wrapped(l->text, r.x + 36, r.y + 34, r.width - 72, 28, INK_TEXT, vis);
    if (vis >= (int)strlen(l->text)) {
        float bob = sinf(G.time * 6) * 3;
        Vector2 c = {r.x + r.width - 40, r.y + r.height - 30 + bob};
        DrawTriangle((Vector2){c.x - 8, c.y - 5}, (Vector2){c.x, c.y + 5}, (Vector2){c.x + 8, c.y - 5}, SEAL_RED);
    }
}

static void ui_text_band(const char *textStr, int visible, float y) {
    Rectangle r = {80, y, UI_W - 160, 208};
    parchment(r, 1);
    scroll_rods(r, 1);
    ink_wrapped(textStr, r.x + 36, r.y + 24, r.width - 72, 26, INK_TEXT, visible);
}


/* Quebra um parágrafo em linhas que cabem em `width`. Devolve quantas. */
static int wrap_lines(const char *t, float size, float width, char lines[][256], int max) {
    int n = 0;
    char line[256] = "", trial[512];
    while (*t && n < max) {
        int wl = 0;
        while (t[wl] && t[wl] != ' ') wl++;
        snprintf(trial, sizeof trial, "%s%s%.*s", line, line[0] ? " " : "", wl, t);
        if (line[0] && ui_width(trial, size) > width) {
            snprintf(lines[n++], 256, "%s", line);
            snprintf(line, sizeof line, "%.*s", wl, t);
        } else {
            snprintf(line, sizeof line, "%s", trial);
        }
        t += wl;
        while (*t == ' ') t++;
    }
    if (line[0] && n < max) snprintf(lines[n++], 256, "%s", line);
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
    char lines[16][256];
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
    if (G.stateTime < 1.2f) return;
    float a = clampf((G.stateTime - 1.2f) * 3, 0, 1);
    DrawRectangle(0, 0, UI_W, UI_H, fadec((Color){20, 6, 4, 255}, 0.6f * a));
    draw_text_f(G.uiBold, "derrota", UI_W / 2.0f - ui_width_f(G.uiBold, "derrota", 80) / 2, 190, 80, fadec((Color){206, 70, 50, 255}, a), true);
    if (G.stateTime < 1.6f) return;
    const char *labels[3];
    int n = defeat_options(labels);
    parchment((Rectangle){UI_W / 2.0f - 220, 356, 440, 40 + n * 56.0f}, a);
    for (int i = 0; i < n; i++) {
        bool sensei = strcmp(labels[i], "conversar com hanzo") == 0;
        ui_menu_row(labels[i], UI_W / 2.0f, 380 + i * 56.0f, i == G.defeatIndex, sensei ? SEAL_RED : INK_TEXT, a);
    }
}

static void ui_cleared(void) {
    float a = clampf(G.stateTime * 3, 0, 1);
    DrawRectangle(0, 0, UI_W, UI_H, fadec(INK, 0.45f * a));
    Rectangle r = {UI_W / 2.0f - 300, 200, 600, 220};
    parchment(r, a);
    scroll_rods(r, a);
    ink_bold_center("aprendiz vencido", UI_W / 2.0f, r.y + 34, 50, fadec(INK_TEXT, a));
    ink_bold_center(lower(G.m->name), UI_W / 2.0f, r.y + 104, 34, fadec((Color){160, 66, 22, 255}, a));
    if (G.stateTime > 1.0f)
        ink_center(campaign_big_boss_open(&G.camp) ? "os doze caíram. oboro espera no dojo de hanzo." : "clique para seguir a trilha",
                   UI_W / 2.0f, r.y + 162, 24, fadec(INK_SOFT, a));
}


static const char *ENDING_TEXT =
    "Oboro caiu de joelhos, sem a katana de Hanzo. Pela primeira vez entendeu que aquela abertura não fora a derrota "
    "do mestre, e sim a última lição, a que ele se recusou a aprender. Kojiro subiu a serra e devolveu a katana a "
    "Hanzo. O velho a recebeu sem dizer nada. Não precisava.";

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
    const char *keys[] = {"ESC", "T", "F", "M", "Q"};
    const char *items[] = {"continuar", "voltar à trilha", G.fx.shakeEnabled ? "tremor ligado" : "tremor desligado", "voltar ao menu", "sair"};
    for (int i = 0; i < 5; i++) ui_key(keys[i], items[i], r.x + 120, r.y + 108 + i * 68.0f, INK_SOFT);
}

/* Primeiro duelo: como se apara, até o primeiro parry que pega. */
static void ui_first_hint(void) {
    if (G.camp.index != 0 || G.state != ST_DUEL || G.duel.perfects + G.duel.goods > 0) return;
    float a = clampf(G.stateTime - 1.0f, 0, 1);
    if (a <= 0) return;
    Color c = fadec((Color){236, 222, 192, 255}, a);
    float w = ui_width("aparar", 26) + 24 + 128 + 24 + ui_width("ou clique", 26);
    float x = UI_W / 2.0f - w / 2, y = 348;   /* acima das cabeças, abaixo da faixa da postura */
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

static void draw_ui(void) {
    switch (G.state) {
        case ST_TITLE: ui_title(); break;
        case ST_LORE: ui_narration(); break;
        case ST_TRAIL: ui_trail(); break;
        case ST_SENSEI: {
            DrawRectangleGradientV(0, 0, UI_W, UI_H, fadec(INK, 0.2f), fadec(INK, 0.6f));
            char head[96];
            snprintf(head, sizeof head, "hanzo fala sobre %s", G.m->name);
            ui_dialogue(head, false);
            break;
        }
        case ST_ENDING:
            /* o texto fica no céu e os dois, no chão da serra */
            ui_text_band(ENDING_TEXT, utf8_visible(ENDING_TEXT, G.typeChars), 40);
            if (G.typeChars > strlen(ENDING_TEXT)) ui_center("fim", UI_W / 2.0f, 272, 72, OCHRE);
            break;
        default:
            if (G.state == ST_DUEL || G.state == ST_DEFEAT || G.state == ST_FINISHER) ui_hud();
            ui_first_hint();
            fx_draw_popups(&G.fx, G.ui, UNIT);
            ui_slash();
            if (G.state == ST_INTRO || G.state == ST_OUTRO) ui_dialogue(G.m->venue, true);
            if (G.state == ST_DEFEAT) ui_defeat();
            if (G.state == ST_CLEARED) ui_cleared();
            break;
    }
    if (G.paused) ui_pause();
}

/* ------------------------------------------------------------------ */
/* Telas                                                               */
/* ------------------------------------------------------------------ */

static void update_title(void) {
    int options = G.hasSave ? 3 : 2;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) { G.menuIndex = (G.menuIndex + 1) % options; audio_play(SND_UI, 1, 1); }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) { G.menuIndex = (G.menuIndex + options - 1) % options; audio_play(SND_UI, 1, 1); }
    Vector2 v = mouse_ui();
    for (int i = 0; i < options; i++) {
        Rectangle r = {UI_W / 2.0f - 200, 414 + i * 60.0f, 400, 54};
        if (CheckCollisionPointRec(v, r) && (GetMouseDelta().x != 0 || GetMouseDelta().y != 0)) G.menuIndex = i;
    }
    if (!pressed() || G.stateTime < 0.3f) return;
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
    bool fast = IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyDown(KEY_SPACE) || (G.demo && !G.shotFile);
    G.loreScroll += dt * (fast ? 90 : 26);
    if (IsKeyDown(KEY_ESCAPE)) G.skipHold += dt;
    else G.skipHold = fmaxf(0, G.skipHold - dt * 3);
    if (G.skipHold >= SKIP_HOLD) { finish_lore(); return; }
    if (G.loreHeight > 0 && G.loreScroll > G.loreHeight + UI_H * 0.55f) finish_lore();
}

static void update_trail(void) {
    if (G.stateTime < 0.4f) return;
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
    if (!pressed() || G.stateTime < 0.25f) return;
    if (G.typeChars < len) { G.typeChars = (float)len; return; }
    audio_play(SND_UI, 0.8f, 1);
    G.lineIndex++;
    G.typeChars = 0;
    if (G.lineIndex >= G.lineCount) done();
}

static void intro_done(void) { start_duel(); }

static void outro_done(void) {
    campaign_mark_cleared(&G.camp, G.camp.index);
    if (G.m->isBigBoss) {
        campaign_advance(&G.camp);
        save_game();
        G.typeChars = 0;
        set_state(ST_ENDING);
        audio_music(MUSIC_LORE);
        return;
    }
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
    if (G.stateTime < 1.6f) return;
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

static void update_cleared(float dt) {
    rig_update(&G.ren, dt);
    rig_update(&G.boss, dt);
    f_update(&G.renS, dt);
    f_update(&G.bossS, dt);
    update_sword(dt);
    if (G.stateTime > 1.0f && pressed()) {
        campaign_advance(&G.camp);
        save_game();
        audio_music(MUSIC_TITLE);
        set_state(ST_TRAIL);
    }
}

static void update_ending(float dt) {
    G.typeChars += dt * 30;
    if (G.stateTime > 2 && pressed()) {
        if (G.typeChars < (float)strlen(ENDING_TEXT)) { G.typeChars = (float)strlen(ENDING_TEXT); return; }
        G.hasSave = true;
        G.menuIndex = 0;
        set_state(ST_TITLE);
        audio_music(MUSIC_TITLE);
    }
}

/* ------------------------------------------------------------------ */

static bool in_arena_state(void) {
    return G.state == ST_INTRO || G.state == ST_DUEL || G.state == ST_FINISHER || G.state == ST_OUTRO ||
           G.state == ST_CLEARED || G.state == ST_DEFEAT;
}

static void parse_args(int argc, char **argv, int *startMaster, bool *direct, const char **startState) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--master") && i + 1 < argc) *startMaster = atoi(argv[++i]) - 1;
        else if (!strcmp(argv[i], "--duel")) *direct = true;
        else if (!strcmp(argv[i], "--demo")) G.demo = true;
        else if (!strcmp(argv[i], "--state") && i + 1 < argc) *startState = argv[++i];
        else if (!strcmp(argv[i], "--shot") && i + 2 < argc) { G.shotFile = argv[++i]; G.shotTime = (float)atof(argv[++i]); }
        else if (!strcmp(argv[i], "--rec") && i + 3 < argc) {
            G.recDir = argv[++i];
            G.recStart = (float)atof(argv[++i]);
            G.recEnd = (float)atof(argv[++i]);
        }
    }
}

static void step(float dtReal) {
    switch (G.state) {
        case ST_TITLE: update_title(); break;
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
        case ST_ENDING: update_ending(dtReal); break;
    }
    if (in_arena_state()) {
        update_ctx(dtReal * (G.hitstop > 0 ? 0.1f : 1));
        fx_update(&G.fx, dtReal * (G.hitstop > 0 ? 0.25f : 1));
        vfx_update(dtReal * (G.hitstop > 0 ? 0.25f : 1));
        update_hud_values(dtReal);
    }
}

static Font load_font(const char *path, int size) {
    int cps[256 - 32 + 3], n = 0;
    for (int c = 32; c < 256; c++) cps[n++] = c;
    cps[n++] = 0x2014; /* travessão */
    cps[n++] = 0x2026; /* reticências */
    cps[n++] = 0x201C;
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

int main(int argc, char **argv) {
    int startMaster = -1;
    bool direct = false;
    const char *startState = NULL;
    parse_args(argc, argv, &startMaster, &direct, &startState);

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(UI_W, UI_H, "aparar - a trilha dos doze aprendizes");
    SetExitKey(KEY_NULL);
    SetWindowMinSize(LOW_W, LOW_H);
    ChangeDirectory(GetApplicationDirectory());
    srand((unsigned)time(NULL));

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
    katana3d_load("assets/katana");
    spr_init();
    pix_init(RW, RH);
    G.ui = load_font("assets/fonts/Tiny5-Regular.ttf", 9);  /* 9 = "em" de 8 px, a grade da Tiny5 */
    G.uiBold = G.ui;
    audio_init();
    fx_init(&G.fx);
    settings_default(&G.settings);
    campaign_reset(&G.camp);
    G.hasSave = load_game();
    G.slowmo = 1;
    G.m = roster_get(G.camp.index);
    setup_actors();

    if (startMaster >= 0 && startMaster < ROSTER_SIZE) {
        for (int i = 0; i < startMaster; i++) campaign_mark_cleared(&G.camp, i);
        start_master(startMaster);
        if (startState && !strcmp(startState, "sensei")) start_sensei();
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
    } else if (startState && !strcmp(startState, "trail")) {
        set_state(ST_TRAIL);
        audio_music(MUSIC_TITLE);
    } else if (startState && !strcmp(startState, "ending")) {
        set_state(ST_ENDING);
        audio_music(MUSIC_LORE);
    } else {
        set_state(ST_TITLE);
        audio_music(MUSIC_TITLE);
    }

    double wall = 0;
    while (!WindowShouldClose()) {
        float dtReal = G.recDir ? 1.0f / 30 : GetFrameTime();
        wall += dtReal;
        /* Travamento longo: pausa em vez de engolir o golpe. */
        if (dtReal > 0.2f) {
            dtReal = 0;
            if (G.state == ST_DUEL && !G.shotFile && !G.recDir) G.paused = true;
        }
        if (!IsWindowFocused() && G.state == ST_DUEL && !G.shotFile && !G.recDir) G.paused = true;
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();
        if (IsKeyPressed(KEY_F)) G.fx.shakeEnabled = !G.fx.shakeEnabled;
        if (IsKeyPressed(KEY_ESCAPE) && in_arena_state()) G.paused = !G.paused;
        if (G.paused) {
            if (IsKeyPressed(KEY_T)) { G.paused = false; audio_music(MUSIC_TITLE); set_state(ST_TRAIL); }
            if (IsKeyPressed(KEY_M)) go_to_menu();
            if (IsKeyPressed(KEY_Q)) break;
            dtReal = 0;
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
        if (!G.paused) step(dtReal);
        draw_world();
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
        DrawTexturePro(G.uiLow.texture, (Rectangle){0, 0, LOW_W, -LOW_H}, dst, (Vector2){0, 0}, 0, WHITE);
        EndBlendMode();

        if (G.recDir && wall >= G.recStart) {
            char path[512];
            snprintf(path, sizeof path, "%s/q%04d.png", G.recDir, G.recFrame++);
            Image img = LoadImageFromScreen();
            ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
            ImageResize(&img, UI_W / 2, UI_H / 2);
            ExportImage(img, path);
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
    }

    audio_shutdown();
    katana3d_unload();
    UnloadShader(G.post);
    UnloadRenderTexture(G.scene);
    UnloadRenderTexture(G.actors);
    if (G.ui.texture.id != GetFontDefault().texture.id) UnloadFont(G.ui);
    UnloadRenderTexture(G.uiLow);
    spr_shutdown();
    pix_shutdown();
    CloseWindow();
    return 0;
}
