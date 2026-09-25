/*
 * lore.c - ilustrações da abertura, do final e do mapa da trilha,
 * em pixel art procedural (320 x 180).
 */
#include "lore.h"

#include <math.h>

#include "raylib.h"
#include "sprites.h"

#define C(r, g, b) ((Color){r, g, b, 255})
#define CA(r, g, b, a) ((Color){r, g, b, a})

static float hash1(float i) {
    float s = sinf(i * 12.9898f) * 43758.5453f;
    return s - floorf(s);
}
static float fract(float x) { return x - floorf(x); }
static Color fade(Color c, float a) { c.a = (unsigned char)(c.a * (a < 0 ? 0 : (a > 1 ? 1 : a))); return c; }
static void rect(float x, float y, float w, float h, Color c) {
    DrawRectangle((int)floorf(x), (int)floorf(y), (int)ceilf(w), (int)ceilf(h), c);
}
static void sky(Color top, Color bot) { DrawRectangleGradientV(0, 0, 320, 180, top, bot); }
static void glow(float x, float y, float r, Color c) {
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleGradient((Vector2){x, y}, r, fade(c, 0.55f), fade(c, 0));
    EndBlendMode();
}
static void stars(int n, float t) {
    for (int i = 0; i < n; i++)
        DrawCircleV((Vector2){hash1(i) * 320, hash1(i + 50) * 110}, 0.35f, fade(WHITE, 0.4f + 0.6f * fabsf(sinf(t + i))));
}
static void mountain(float peakX, float peakY, float base, Color c) {
    for (int x = 0; x < 320; x++) {
        float d = fabsf(x - peakX);
        float y = peakY + d * 0.55f + sinf(x * 0.2f) * 2;
        if (y < base) rect(x, y, 1, base - y + 40, c);
    }
}
/* Silhueta humana simples (em pé), altura h. */
static void person(float x, float feet, float h, Color c, bool sword) {
    float head = h * 0.14f;
    DrawCircle((int)x, (int)(feet - h + head), head, c);
    rect(x - h * 0.13f, feet - h + head * 1.8f, h * 0.26f, h * 0.45f, c);
    rect(x - h * 0.12f, feet - h * 0.4f, h * 0.09f, h * 0.4f, c);
    rect(x + h * 0.03f, feet - h * 0.4f, h * 0.09f, h * 0.4f, c);
    if (sword) DrawLine((int)(x + h * 0.1f), (int)(feet - h * 0.55f), (int)(x + h * 0.5f), (int)(feet - h * 0.85f), c);
}
/* O personagem das pranchas, parado e calmo (IDLE, PARADO, o fim da corrida
 * com a lâmina baixa ou a guarda), com o contorno escuro do duelo. Sem as
 * pranchas, devolve false e a ilustração usa a silhueta. */
static bool sprite_person_lit(const char *id, float x, float feet, bool faceLeft, float t, Color tint, Color rim) {
    const SprSet *s = spr_get(id);
    if (!s) return false;
    const SprAnim *a = spr_anim(s, "IDLE");
    int frame = 0;
    if (a) frame = (int)(t / a->frameTime) % a->frames;
    else if ((a = spr_anim(s, "PARADO"))) frame = 0;
    else if ((a = spr_anim(s, "DASH"))) frame = a->frames - 1;
    else if ((a = spr_anim(s, "ATTACK_1"))) frame = 0;
    if (!a) return false;
    int breath = (!a->loop && fract(t / 1.8f) > 0.5f) ? 1 : 0;
    SprDraw o = {faceLeft, breath, true, C(24, 16, 20)};
    static const int off[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int k = 0; k < 4; k++) spr_draw(s, a, frame, (Vector2){x + off[k][0], feet + off[k][1]}, o);
    if (rim.a) {
        /* luz de contorno do lado da frente (a lua do título) */
        o.color = rim;
        spr_draw(s, a, frame, (Vector2){x + (faceLeft ? -1 : 1), feet - 1}, o);
    }
    o.flat = false;
    o.color = tint;
    spr_draw(s, a, frame, (Vector2){x, feet}, o);
    return true;
}

static bool sprite_person(const char *id, float x, float feet, bool faceLeft, float t) {
    return sprite_person_lit(id, x, feet, faceLeft, t, WHITE, CA(0, 0, 0, 0));
}

static void rain(float t, int n, Color c) {
    for (int i = 0; i < n; i++) {
        float x = fract(hash1(i) + t * 0.1f) * 340 - 10, y = fract(hash1(i + 7) + t * 1.4f) * 190 - 10;
        DrawLine((int)x, (int)y, (int)(x - 2), (int)(y + 6), c);
    }
}

static void dojo_gate(float x, float ground, Color c) {
    rect(x - 30, ground - 40, 4, 40, c);
    rect(x + 26, ground - 40, 4, 40, c);
    rect(x - 36, ground - 46, 72, 5, c);
    rect(x - 32, ground - 36, 64, 3, c);
}

void lore_draw_scene(int page, float t) {
    switch (page) {
        case 0: { /* Hanzo e Ren no pátio do dojo ao amanhecer */
            sky(C(255, 170, 120), C(255, 225, 190));
            DrawCircle(250, 118, 26, C(255, 240, 200));
            mountain(60, 90, 180, C(200, 120, 110));
            rect(0, 140, 320, 40, C(120, 80, 60));
            dojo_gate(160, 140, C(90, 40, 36));
            if (!sprite_person("hanzo", 138, 142, false, t)) person(130, 142, 56, C(40, 26, 30), true);  /* Hanzo */
            if (!sprite_person("kojiro", 182, 142, true, t + 0.7f)) person(186, 142, 34, C(220, 100, 40), true);  /* Kojiro */
            for (int i = 0; i < 18; i++) {
                float x = fract(hash1(i) + t * 0.05f) * 340 - 10, y = fract(hash1(i + 3) + t * 0.08f) * 180;
                DrawEllipse((int)x, (int)y, 2, 1, C(255, 180, 200));
            }
            break;
        }
        case 1: { /* Noite de chuva: Oboro saca, Hanzo não */
            float flash = fract(t * 0.35f) < 0.05f ? 1 : 0;
            sky(C(10, 12, 24), C(30, 34, 50));
            if (flash > 0) rect(0, 0, 320, 180, CA(220, 220, 255, 90));
            rect(0, 140, 320, 40, C(24, 26, 34));
            dojo_gate(160, 140, C(16, 16, 22));
            person(120, 142, 50, C(40, 30, 34), false);   /* Hanzo, espada na bainha */
            rect(128, 118, 16, 2, C(60, 40, 30));
            person(205, 142, 52, C(8, 8, 12), true);      /* Oboro */
            DrawCircle(207, 97, 1, C(255, 40, 60));
            rain(t, 90, CA(150, 160, 200, 140));
            break;
        }
        case 2: { /* Doze mestres guardam o caminho até o castelo */
            sky(C(4, 4, 10), C(20, 16, 34));
            mountain(160, 40, 180, C(14, 12, 24));
            rect(146, 24, 28, 16, C(20, 14, 24));
            DrawTriangle((Vector2){140, 26}, (Vector2){180, 26}, (Vector2){160, 14}, C(60, 20, 30));
            rect(156, 30, 8, 6, C(255, 170, 80));
            for (int i = 0; i < 12; i++) {
                float x = 30 + i * 22 + sinf(t * 2 + i) * 0.5f, feet = 140 - fabsf(i - 5.5f) * 4;
                person(x, feet, 24, C(4, 4, 8), true);
                glow(x + 7, feet - 18, 5 * (0.8f + 0.2f * sinf(t * 9 + i)), C(255, 120, 40));
            }
            break;
        }
        case 3: { /* Ren pega a katana do mestre */
            sky(C(40, 30, 60), C(120, 80, 100));
            rect(0, 140, 320, 40, C(50, 40, 50));
            rect(170, 120, 12, 22, C(90, 90, 100));     /* lápide */
            rect(166, 116, 20, 5, C(100, 100, 110));
            rect(160, 132, 4, 8, C(80, 70, 70));        /* lanterna */
            glow(162, 130, 10 * (0.85f + 0.15f * sinf(t * 8)), C(255, 170, 80));
            person(130, 142, 40, C(220, 100, 40), false);
            float lift = fminf(1, t * 0.5f);
            DrawLine(140, (int)(120 - lift * 20), (int)(152 + lift * 8), (int)(100 - lift * 30), C(220, 220, 230));
            break;
        }
        default: { /* A trilha até o castelo */
            sky(C(20, 16, 50), C(120, 80, 120));
            stars(40, t);
            mountain(250, 30, 180, C(40, 30, 60));
            rect(236, 18, 28, 16, C(30, 20, 40));
            DrawTriangle((Vector2){232, 20}, (Vector2){268, 20}, (Vector2){250, 8}, C(90, 30, 60));
            for (int i = 0; i < MASTER_COUNT; i++) {
                float x, y;
                lore_trail_point(i, &x, &y);
                glow(x, y, 5 + 1.5f * sinf(t * 2 + i), C(255, 150, 70));
            }
            person(40, 132, 22, C(220, 100, 40), true);
            break;
        }
    }
}

void lore_trail_point(int index, float *x, float *y) {
    /* Ziguezague subindo a serra até a cidadela. */
    float k = index / (float)(ROSTER_SIZE - 1);
    *x = 40 + (index % 2 ? 1 : -1) * 18 + k * 200 + sinf(index * 1.3f) * 10;
    *y = 118 - k * 96;
    if (index == ROSTER_SIZE - 1) { *x = 250; *y = 16; }
}

void lore_draw_trail(const Campaign *c, float t, int selected) {
    sky(C(16, 14, 40), C(90, 70, 110));
    stars(50, t);
    mountain(250, 20, 180, C(42, 34, 64));
    mountain(90, 80, 180, C(30, 26, 50));
    /* Cidadela no topo. */
    rect(236, 12, 28, 14, C(24, 16, 32));
    DrawTriangle((Vector2){232, 14}, (Vector2){268, 14}, (Vector2){250, 2}, C(90, 30, 60));
    /* Caminho. */
    for (int i = 0; i < ROSTER_SIZE - 1; i++) {
        float x0, y0, x1, y1;
        lore_trail_point(i, &x0, &y0);
        lore_trail_point(i + 1, &x1, &y1);
        bool done = campaign_is_cleared(c, i);
        for (int k = 0; k < 10; k += 2) {
            float a = k / 10.0f, b = (k + 1) / 10.0f;
            DrawLine((int)(x0 + (x1 - x0) * a), (int)(y0 + (y1 - y0) * a), (int)(x0 + (x1 - x0) * b), (int)(y0 + (y1 - y0) * b),
                     done ? C(255, 170, 80) : C(90, 80, 110));
        }
    }
    for (int i = 0; i < ROSTER_SIZE; i++) {
        float x, y;
        lore_trail_point(i, &x, &y);
        bool done = campaign_is_cleared(c, i);
        bool last = i == ROSTER_SIZE - 1;
        bool open = last ? campaign_big_boss_open(c) : true;
        Color col = last ? C(200, 60, 90) : (done ? C(255, 160, 70) : C(90, 84, 110));
        float r = last ? 5 : 3;
        if (i == selected) {
            float p = 0.5f + 0.5f * sinf(t * 5);
            DrawCircleLines((int)x, (int)y, r + 3 + p * 2, C(255, 220, 150));
        }
        DrawPoly((Vector2){x, y}, 4, r, 45, open ? col : C(40, 36, 50));
        if (done || i == selected) glow(x, y, 10, col);
    }
}

void lore_draw_ending(float t) {
    sky(C(255, 180, 120), C(255, 235, 200));
    DrawCircle(160, 110, 40 + sinf(t * 0.5f) * 2, C(255, 245, 220));
    mountain(160, 70, 180, C(200, 130, 120));
    rect(0, 150, 320, 30, C(120, 80, 70));
    /* Kojiro devolve a katana a Hanzo no alto da serra (sem as pranchas, a lápide e a silhueta) */
    rect(152, 144, 4, 8, C(80, 70, 70));
    glow(154, 142, 12 * (0.85f + 0.15f * sinf(t * 8)), C(255, 170, 80));
    if (!sprite_person("hanzo", 184, 152, true, t)) {
        rect(170, 130, 12, 22, C(110, 100, 100));
        rect(166, 126, 20, 5, C(120, 110, 110));
    }
    if (!sprite_person("kojiro", 126, 152, false, t + 0.7f)) person(120, 152, 44, C(220, 100, 40), true);
    for (int i = 0; i < 24; i++) {
        float x = fract(hash1(i) + t * 0.04f) * 340 - 10, y = fract(hash1(i + 3) + t * 0.06f) * 180;
        DrawEllipse((int)x, (int)y, 2, 1, C(255, 190, 210));
    }
}

/* Tela de título: noite na serra. No pico mais alto, pequeno, o dojo de Hanzo
 * com as janelas acesas; névoa no vale; em primeiro plano, o morro onde Kojiro
 * para, de costas, e olha para ele (lore_draw_title_hero). */

/* Desenho em pixel: uma letra por pixel, '.' é vazio. */
typedef struct { char c; Color col; } Ink;
static void art(const char *const *rows, int n, float x0, float y0, const Ink *pal) {
    for (int y = 0; y < n; y++)
        for (int x = 0; rows[y][x]; x++)
            for (const Ink *p = pal; p->c; p++)
                if (p->c == rows[y][x]) { rect(x0 + x, y0 + y, 1, 1, p->col); break; }
}

/* Dois telhados curvos de pontas erguidas, janelas acesas e a varanda. */
static const char *const DOJO[] = {
    ".........k.........",
    "......kkkrkkk......",
    "....krrrrrrrrrk....",
    "..kkRRRRRRRRRRRkk..",
    ".k..kmLmLmLmLmk..k.",
    "....kmLmLmLmLmk....",
    "..rrrrrrrrrrrrrrr..",
    "kkRRRRRRRRRRRRRRRkk",
    "k.kmmLLmLLmLLmmk..k",
    "..kmmLfmLfmLfmmk...",
    ".vvvvvvvvvvvvvvvv..",
    ".k.k...........k.k.",
};
static const Ink DOJO_PAL[] = {
    {'k', {22, 20, 36, 255}}, {'R', {30, 26, 46, 255}}, {'r', {72, 74, 116, 255}},
    {'m', {50, 38, 44, 255}}, {'L', {255, 200, 120, 255}}, {'f', {176, 112, 60, 255}},
    {'v', {60, 58, 88, 255}}, {0, {0, 0, 0, 0}},
};
static float hill_y(float x) { float d = (x - 70) / 88; return 124 + d * d * 34; }

void lore_draw_title(float t) {
    DrawRectangleGradientV(0, 0, 320, 124, C(6, 8, 24), C(48, 42, 88));
    rect(0, 124, 320, 56, C(48, 42, 88));
    for (int i = 0; i < 80; i++) {
        float x = floorf(hash1(i) * 320), y = floorf(hash1(i + 50) * 104);
        float b = 0.35f + 0.65f * fabsf(sinf(t * (0.6f + hash1(i + 9)) + i));
        rect(x, y, 1, 1, fade(C(230, 232, 255), b));
        if (i % 13 == 0) { rect(x - 1, y, 3, 1, fade(C(200, 206, 255), b * 0.5f)); rect(x, y - 1, 1, 3, fade(C(200, 206, 255), b * 0.5f)); }
    }
    /* Lua cheia com crateras e a borda de sombra. */
    glow(292, 26, 38, C(170, 180, 255));
    DrawCircle(292, 26, 12, C(238, 234, 216));
    DrawCircle(288, 26, 11, C(226, 222, 204));
    DrawCircle(293, 25, 10, C(240, 236, 220));
    rect(288, 21, 3, 2, C(212, 206, 190));
    rect(295, 28, 4, 3, C(214, 208, 192));
    rect(290, 32, 2, 2, C(212, 206, 190));
    rect(297, 20, 2, 2, C(220, 214, 198));
    /* Nuvens finas cruzando a lua devagar, com a borda de cima acesa. */
    for (int i = 0; i < 4; i++) {
        float w = 60 + hash1(i + 20) * 50, x = fract(hash1(i) + t * 0.006f * (1 + i % 2)) * (320 + w) - w, y = floorf(22 + i * 13 + hash1(i + 30) * 6);
        rect(x, y, w, 3, C(34, 34, 70));
        rect(x + 6, y - 1, w - 18, 1, C(84, 84, 128));
        rect(x + w * 0.3f, y + 3, w * 0.5f, 1, C(28, 28, 58));
    }
    /* Serra ao longe. */
    for (int x = 0; x < 320; x++) {
        float y = 104 + sinf(x * 0.045f) * 8 + sinf(x * 0.13f + 2) * 3 + fabsf(sinf(x * 0.021f + 1)) * -10;
        rect(x, y, 1, 180 - y, C(38, 38, 76));
        rect(x, y, 1, 1, C(58, 58, 102));
    }
    /* A montanha do dojo: encosta da esquerda íngreme, a da direita pegando a lua. */
    for (int x = 110; x < 320; x++) {
        float d = x - 230, y = 60 + (d < 0 ? -d * 0.78f : d * 0.5f) + sinf(x * 0.3f) * 1.2f + (d < 0 ? sinf(x * 0.09f) * 3 : 0);
        rect(x, y, 1, 180 - y, C(24, 24, 52));
        if (d > 0) rect(x, y, 1, 2, C(76, 80, 128));
        else rect(x, y, 1, 1, C(44, 44, 80));
    }
    /* Pinheiros na encosta. */
    for (int i = 0; i < 9; i++) {
        float x = 250 + i * 8 + hash1(i + 60) * 4, y = 76 + (x - 230) * 0.5f;
        DrawTriangle((Vector2){x, y - 7}, (Vector2){x - 3, y + 1}, (Vector2){x + 3, y + 1}, C(18, 20, 40));
    }
    /* O dojo de Hanzo no pico, pequeno, sobre uma base de pedra e entre pinheiros. */
    for (int x = 219; x <= 240; x++) {
        float d = x - 230, ground = 60 + (d < 0 ? -d * 0.78f : d * 0.5f);
        rect(x, 60, 1, ground - 59, C(34, 32, 58));
        if ((x + (int)ground) % 4 == 0) rect(x, 62 + (x % 3), 1, 1, C(26, 24, 46));
    }
    rect(219, 60, 22, 1, C(70, 70, 108));
    static const float pines[] = {205, 211, 216, 244, 249, 255};
    for (int i = 0; i < 6; i++) {
        float x = pines[i], d = x - 230, y = 60 + (d < 0 ? -d * 0.78f : d * 0.5f) + 1;
        float h = 6 + hash1(i + 40) * 3;
        DrawTriangle((Vector2){x, y - h}, (Vector2){x - 2.5f, y + 1}, (Vector2){x + 2.5f, y + 1}, C(16, 20, 38));
        rect(x, y - h + 1, 1, 1, C(56, 64, 104));
    }
    art(DOJO, sizeof DOJO / sizeof *DOJO, 220, 49, DOJO_PAL);
    /* a luz das janelas no chão da varanda */
    for (int k = 0; k < 3; k++) rect(224 + k * 3, 59, 2, 1, C(150, 104, 70));
    /* Névoa no vale, em faixas que andam. */
    for (int i = 0; i < 6; i++) {
        float w = 70 + hash1(i + 70) * 80, x = fract(hash1(i + 71) + t * 0.004f * (1 + i % 3)) * (320 + w) - w;
        float y = floorf(120 + i * 5 + hash1(i + 72) * 4);
        rect(x, y, w, 2, CA(150, 150, 200, 70));
        rect(x + w * 0.15f, y - 1, w * 0.6f, 1, CA(170, 170, 220, 50));
    }
    /* O morro em primeiro plano. */
    for (int x = 0; x < 180; x++) {
        float y = hill_y(x);
        if (y < 180) {
            rect(x, y, 1, 180 - y, C(14, 14, 30));
            rect(x, y, 1, 1, C(52, 56, 96));
        }
    }
    rect(170, 158, 150, 22, C(16, 16, 34));
    /* Capim balançando no morro, com a ponta pegando o luar. */
    for (int i = 0; i < 70; i++) {
        float x = floorf(hash1(i + 90) * 175), y = floorf(hill_y(x) + hash1(i + 91) * 6);
        float sw = sinf(t * 1.6f + x * 0.07f) * 2;
        DrawLine((int)x, (int)y, (int)(x + sw), (int)(y - 5), C(26, 28, 50));
        rect(x + sw, y - 5, 1, 1, C(84, 92, 140));
    }
    /* Pétalas no vento. */
    for (int i = 0; i < 14; i++) {
        float x = fract(hash1(i) + t * 0.035f) * 340 - 10, y = fract(hash1(i + 3) + t * 0.05f + sinf(t + i) * 0.01f) * 180;
        rect(x, y, 2, 1, C(236, 190, 214));
    }
}

/* Kojiro de costas no alto do morro, a espada na bainha, olhando para o dojo sob
 * o luar (desenhado depois da paleta do fundo, para não perder as cores dele).
 * O vento leva os fiapos do coque e a barra da hakama. */
static const char *const KOJIRO_COSTAS[] = {
    "...............h...h....",
    "..............h.h.h.....",
    "..............kHhh......",
    "...........h.kHHk.......",
    "............kHHHk.......",
    "............krrk........",
    "...........kHHHHHk......",
    "..........kHHHHHHhk.....",
    "..........kHHHHHHhhk....",
    "..........kHHHHHHHSk....",
    "..........kHHHHHHhSk....",
    "...........kHHHHHhk.....",
    "............ksSSk.......",
    "........kkkvWWWwwkkk....",
    ".......kvWWWWWWWWwwwk...",
    "......kvWWWWWWWWWWwwwk..",
    "......kvWWWWWWWWWWWwwk..",
    ".....kvvWWWvWWWWWWWwwwk.",
    ".....kvWWWWvWWWWWWWWwwk.",
    ".....kvWWWWvWWWWWWWWwwk.",
    ".....kvvWWkvWWWWWkWwwwk.",
    ".....kvvvkkkOOOOOkkwwk..",
    "......kkkOOOOOOOOOOokk..",
    "....bBBgkOOqPPPPqOook...",
    "..bBBk.kPPqpPPPPpqPPpk..",
    "bBBk..kPPPqPPPPPPqPPpk..",
    "Bk....kPPqPPPPPPPPqPPpk.",
    "......kPPqPPPPPPPPqPPppk.",
    ".....kPPPqPPPPPPPPPqPPpk.",
    ".....kPPqPPPPPPPPPPqPPppk",
    ".....kPPqPPPPPPPPPPqPPPpk",
    "....kPPPqPPPPPPPPPPPqPPpk",
    "....kPPqPPPPPkPPPPPPqPPpk",
    "....kPPqPPPPPkPPPPPPqPPppk",
    "...kPPPqPPPPkkPPPPPPPqPPpk",
    "...kPPqPPPPPkkPPPPPPPqPPpk",
    "...kPPqPPPPk.kPPPPPPPqPPpk",
    "...kkkkkkkk...kkkkkkkkkkkk",
};
/* os fiapos em outra posição, quando o vento sopra mais forte */
static const char *const FIAPOS_VENTO[] = {
    "................h..h.h..",
    "...............hh.h.....",
    "..............kHhh......",
    "............hkHHk.......",
};
static const Ink KOJIRO_PAL[] = {
    {'k', {14, 12, 22, 255}}, {'H', {26, 24, 40, 255}}, {'h', {92, 98, 150, 255}},
    {'r', {150, 40, 52, 255}}, {'s', {104, 82, 90, 255}}, {'S', {166, 140, 150, 255}},
    {'v', {80, 88, 128, 255}}, {'W', {124, 134, 176, 255}}, {'w', {182, 192, 228, 255}},
    {'O', {34, 28, 46, 255}}, {'o', {70, 62, 96, 255}},
    {'P', {30, 30, 48, 255}}, {'p', {62, 64, 98, 255}}, {'q', {44, 44, 68, 255}},
    {'B', {30, 16, 28, 255}}, {'b', {110, 80, 120, 255}}, {'g', {150, 128, 84, 255}},
    {0, {0, 0, 0, 0}},
};

void lore_draw_title_hero(float t) {
    int n = sizeof KOJIRO_COSTAS / sizeof *KOJIRO_COSTAS;
    float x = 58, feet = hill_y(70) + 1, top = feet - n;
    bool gust = fract(t * 0.45f) < 0.35f;
    /* respira: os ombros sobem um pixel e descem */
    int breath = fract(t / 2.6f) < 0.5f ? 0 : 1;
    art(KOJIRO_COSTAS + 22, n - 22, x, top + 22, KOJIRO_PAL);
    art(KOJIRO_COSTAS + 4, 18, x, top + 4 + breath, KOJIRO_PAL);
    art(gust ? FIAPOS_VENTO : KOJIRO_COSTAS, 4, x, top + breath, KOJIRO_PAL);
}
