/*
 * lore.c - ilustrações da abertura, do final e do mapa da trilha,
 * em pixel art procedural (320 x 180).
 */
#include "lore.h"

#include <math.h>

#include "raylib.h"

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
            person(130, 142, 56, C(40, 26, 30), true);    /* Hanzo */
            person(186, 142, 34, C(220, 100, 40), true);  /* Ren, de laranja */
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
    float k = index / 12.0f;
    *x = 40 + (index % 2 ? 1 : -1) * 18 + k * 200 + sinf(index * 1.3f) * 10;
    *y = 118 - k * 96;
    if (index == 12) { *x = 250; *y = 16; }
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
        bool open = i == 12 ? campaign_big_boss_open(c) : true;
        Color col = i == 12 ? C(200, 60, 90) : (done ? C(255, 160, 70) : C(90, 84, 110));
        float r = i == 12 ? 5 : 3;
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
    rect(170, 130, 12, 22, C(110, 100, 100));   /* lápide de Hanzo */
    rect(166, 126, 20, 5, C(120, 110, 110));
    rect(158, 144, 4, 8, C(80, 70, 70));
    glow(160, 142, 12 * (0.85f + 0.15f * sinf(t * 8)), C(255, 170, 80));
    person(120, 152, 44, C(220, 100, 40), true);
    for (int i = 0; i < 24; i++) {
        float x = fract(hash1(i) + t * 0.04f) * 340 - 10, y = fract(hash1(i + 3) + t * 0.06f) * 180;
        DrawEllipse((int)x, (int)y, 2, 1, C(255, 190, 210));
    }
}

/* Tela de título: só a paisagem, sem personagens. Serra em camadas ao entardecer. */
void lore_draw_title(float t) {
    sky(C(58, 40, 70), C(255, 170, 120));
    /* Sol baixo com faixas. */
    DrawCircle(172, 112, 30, C(255, 214, 160));
    DrawCircle(172, 112, 24, C(255, 236, 196));
    for (int i = 0; i < 5; i++) rect(136, 104 + i * 6, 72, 1, C(250, 170, 120));
    /* Nuvens passando devagar. */
    for (int i = 0; i < 6; i++) {
        float x = fract(hash1(i) + t * 0.004f * (1 + i % 3)) * 380 - 30, y = 22 + hash1(i + 4) * 50;
        DrawEllipse((int)x, (int)y, 26 + hash1(i + 1) * 18, 4, C(236, 170, 150));
        DrawEllipse((int)(x + 12), (int)(y - 2), 16, 3, C(246, 196, 170));
    }
    /* Três camadas de serra, da mais clara à mais escura. */
    mountain(90, 58, 180, C(170, 110, 120));
    mountain(250, 72, 180, C(130, 80, 100));
    for (int x = 0; x < 320; x++) {
        float y = 128 + sinf(x * 0.035f) * 7 + sinf(x * 0.11f + 1) * 3;
        rect(x, y, 1, 60, C(80, 50, 64));
    }
    /* Pagode distante no morro. */
    rect(74, 96, 10, 16, C(70, 40, 54));
    for (int k = 0; k < 3; k++) {
        float w = 18 - k * 4, y = 96 - k * 7;
        DrawTriangle((Vector2){79 - w / 2, y}, (Vector2){79 + w / 2, y}, (Vector2){79, y - 5}, C(70, 40, 54));
    }
    /* Campo em primeiro plano com capim balançando. */
    rect(0, 150, 320, 30, C(46, 30, 36));
    for (int i = 0; i < 90; i++) {
        float x = hash1(i) * 320, y = 150 + hash1(i + 5) * 26;
        float sw = sinf(t * 1.4f + x * 0.06f) * 2;
        DrawLine((int)x, (int)y, (int)(x + sw), (int)(y - 6), C(70, 46, 50));
    }
    /* Pássaros e pétalas. */
    for (int i = 0; i < 3; i++) {
        float bx = fract(t * 0.015f + i * 0.33f) * 360 - 20, by = 40 + i * 8 + sinf(t + i) * 3, w = sinf(t * 7 + i) * 2;
        DrawLine((int)bx - 3, (int)(by - w), (int)bx, (int)by, C(70, 40, 60));
        DrawLine((int)bx, (int)by, (int)bx + 3, (int)(by - w), C(70, 40, 60));
    }
    for (int i = 0; i < 22; i++) {
        float x = fract(hash1(i) + t * 0.04f) * 340 - 10, y = fract(hash1(i + 3) + t * 0.06f) * 180;
        DrawEllipse((int)x, (int)y, 2, 1, C(255, 190, 205));
    }
}
