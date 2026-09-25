/*
 * arenas.c - catorze cenários em pixel art procedural.
 * Tudo é determinístico a partir do relógio: nada guarda estado.
 */
#include "arenas.h"

#include <math.h>

#define C(r, g, b) ((Color){r, g, b, 255})
#define CA(r, g, b, a) ((Color){r, g, b, a})
#define PI_F 3.14159265f

static float hash1(float i) {
    float s = sinf(i * 12.9898f) * 43758.5453f;
    return s - floorf(s);
}

static float fract(float x) { return x - floorf(x); }

static Color mix(Color a, Color b, float t) {
    t = t < 0 ? 0 : (t > 1 ? 1 : t);
    return (Color){(unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
                   (unsigned char)(a.b + (b.b - a.b) * t), (unsigned char)(a.a + (b.a - a.a) * t)};
}

static Color fade(Color c, float a) {
    c.a = (unsigned char)(c.a * (a < 0 ? 0 : (a > 1 ? 1 : a)));
    return c;
}

/* Brilho suave: círculo em degradê somado à cena. */
static void glow(float x, float y, float r, Color c) {
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleGradient((Vector2){x, y}, r, fade(c, 0.55f), fade(c, 0));
    EndBlendMode();
}

static void rect(float x, float y, float w, float h, Color c) { DrawRectangleRec((Rectangle){x, y, w, h}, c); }

/* Degradê vertical em qualquer retângulo (frações de pixel valem: o cenário é desenhado ampliado). */
static void vgrad(float x, float y, float w, float h, Color top, Color bot) {
    DrawRectangleGradientEx((Rectangle){x, y, w, h}, top, bot, bot, top);
}

/* Degradê horizontal: bom para volume de pilares e troncos. */
static void hgrad(float x, float y, float w, float h, Color left, Color right) {
    DrawRectangleGradientEx((Rectangle){x, y, w, h}, left, left, right, right);
}

static void line(float x1, float y1, float x2, float y2, float th, Color c) { DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, th, c); }

static void stars(int n, float seed, int maxY, float twinkle, float t) {
    for (int i = 0; i < n; i++) {
        float x = hash1(i + seed) * LOW_W, y = hash1(i * 3.1f + seed) * maxY;
        float b = 0.5f + 0.5f * sinf(t * twinkle * (1 + hash1(i + 7)) + i);
        DrawCircleV((Vector2){x, y}, 0.25f + 0.25f * hash1(i + 3), fade(C(230, 230, 255), 0.3f + 0.7f * b));
    }
}

/* Chão com sombra de contato perto da parede e escurecendo para a frente. */
static void floor_shade(Color near, Color far) {
    vgrad(0, GROUND_LOW - 6, LOW_W, LOW_H - GROUND_LOW + 6, near, far);
    vgrad(0, GROUND_LOW - 6, LOW_W, 5, CA(0, 0, 0, 90), CA(0, 0, 0, 0));
}

/* Tábua de madeira com veios finos. */
static void plank(float x, float y, float w, float h, Color base, float seed) {
    hgrad(x, y, w, h, mix(base, C(0, 0, 0), 0.25f), mix(base, C(255, 255, 255), 0.06f));
    for (int k = 0; k < 3; k++) {
        float gx = x + w * (0.2f + 0.3f * k) + sinf(seed + k) * w * 0.08f;
        line(gx, y, gx + sinf(seed * 2 + k) * 0.6f, y + h, 0.25f, fade(mix(base, C(0, 0, 0), 0.4f), 0.6f));
    }
}

/* Lanterna de papel com gomos e brilho. */
static void paper_lantern(float x, float y, float r, Color paper, float t, float seed) {
    float fl = 0.9f + 0.1f * sinf(t * 11 + seed) * sinf(t * 7 + seed * 2);
    glow(x, y, r * 5 * fl, C(255, 150, 70));
    DrawEllipse((int)x, (int)y, r, r * 1.25f, paper);
    DrawEllipse((int)x, (int)y, r * 0.55f, r * 1.1f, mix(paper, C(255, 240, 200), 0.5f));
    for (int k = -2; k <= 2; k++) line(x - r, y + k * r * 0.45f, x + r, y + k * r * 0.45f, 0.25f, fade(C(80, 30, 20), 0.5f));
    rect(x - r * 0.5f, y - r * 1.4f, r, r * 0.3f, C(40, 24, 18));
    rect(x - r * 0.5f, y + r * 1.15f, r, r * 0.3f, C(40, 24, 18));
}

/* ------------------------------------------------------------------ */
/* 1. Dojo                                                             */
/* ------------------------------------------------------------------ */
static void dojo(const ArenaCtx *c) {
    float t = c->t;
    vgrad(0, 0, LOW_W, GROUND_LOW, C(34, 22, 16), C(78, 52, 34));
    /* Shoji iluminados por trás, com a sombra do bambu lá fora. */
    for (int p = 0; p < 6; p++) {
        float x = 12 + p * 50;
        vgrad(x, 34, 44, 96, C(252, 236, 196), C(226, 196, 150));
        for (int b = 0; b < 3; b++) {
            float bx = x + 6 + b * 14 + sinf(t * 0.8f + p + b) * 2.5f;
            rect(bx, 34, 1.4f, 96, CA(120, 96, 60, 55));
            for (int k = 0; k < 5; k++) {
                float ly = 40 + k * 18 + b * 4, sw = sinf(t + k + b) * 1.5f;
                DrawTriangle((Vector2){bx, ly}, (Vector2){bx + 9 + sw, ly - 3}, (Vector2){bx + 8 + sw, ly - 1}, CA(110, 86, 52, 45));
            }
        }
        for (int gx = 0; gx <= 44; gx += 11) rect(x + gx - 0.4f, 34, 0.8f, 96, C(70, 42, 24));
        for (int gy = 0; gy <= 96; gy += 16) rect(x, 34 + gy - 0.4f, 44, 0.8f, C(70, 42, 24));
        vgrad(x, 110, 44, 20, CA(0, 0, 0, 0), CA(60, 30, 10, 40));
    }
    /* Vigas e pilares com volume e veio. */
    vgrad(0, 0, LOW_W, 26, C(22, 14, 10), C(40, 26, 18));
    plank(0, 24, LOW_W, 10, C(64, 40, 26), 1);
    plank(0, 130, LOW_W, 7, C(64, 40, 26), 2);
    for (int p = 0; p < 7; p++) plank(5 + p * 50, 20, 8, 130, C(58, 34, 22), p * 3.1f);
    /* Kamidana: prateleira com o pergaminho de caligrafia. */
    rect(146, 36, 28, 3, C(92, 60, 36));
    vgrad(152, 40, 16, 60, C(240, 230, 206), C(222, 208, 180));
    rect(150, 38.5f, 20, 2.5f, C(40, 22, 14));
    rect(150, 99, 20, 2.5f, C(40, 22, 14));
    line(158, 48, 162, 55, 1.6f, C(24, 16, 14));
    line(157, 60, 163, 60, 1.4f, C(24, 16, 14));
    line(160, 63, 159, 78, 1.8f, C(24, 16, 14));
    line(156, 72, 164, 70, 1.2f, C(24, 16, 14));
    DrawCircleV((Vector2){164, 90}, 1.6f, C(170, 40, 30));
    /* Suporte com espadas de madeira à direita. */
    rect(262, 96, 30, 2, C(60, 38, 24));
    rect(262, 112, 30, 2, C(60, 38, 24));
    for (int k = 0; k < 3; k++) line(266 + k * 8, 90, 268 + k * 8, 126, 1.2f, C(150, 112, 70));
    /* Lanternas de papel. */
    for (int l = 0; l < 2; l++) {
        float lx = l ? 262 : 58, sway = sinf(t * 1.3f + l) * 1.5f;
        line(lx, 0, lx + sway, 14, 0.4f, C(30, 20, 15));
        paper_lantern(lx + sway, 21, 5, C(220, 90, 50), t, l * 3);
    }
    /* Tatami com a borda de pano escuro e a trama. */
    floor_shade(C(176, 160, 100), C(120, 104, 60));
    for (int i = 0; i < 9; i++) {
        float x0 = i * 40 - 20;
        line(x0 + 20, GROUND_LOW - 6, x0 - 10, LOW_H, 1.6f, C(48, 40, 30));
    }
    line(0, 166, LOW_W, 166, 1.6f, C(48, 40, 30));
    for (int k = 0; k < 18; k++) line(0, GROUND_LOW - 4 + k * 1.7f, LOW_W, GROUND_LOW - 4 + k * 1.7f, 0.2f, CA(90, 80, 40, 60));
    vgrad(0, GROUND_LOW - 6, LOW_W, 2, C(48, 30, 18), C(48, 30, 18));
}

/* ------------------------------------------------------------------ */
/* 3. Celeiro ao entardecer: casa de fazenda e varais de arroz         */
/* ------------------------------------------------------------------ */
static void celeiro(const ArenaCtx *c) {
    float t = c->t;
    vgrad(0, 0, LOW_W, 60, C(54, 38, 90), C(178, 96, 110));
    vgrad(0, 60, LOW_W, 55, C(178, 96, 110), C(255, 176, 96));
    glow(250, 98, 70, C(255, 150, 60));
    DrawCircleGradient((Vector2){250, 98}, 20, C(255, 244, 200), C(255, 200, 120));
    /* Nuvens finas acesas por baixo. */
    for (int i = 0; i < 5; i++) {
        float x = fract(hash1(i) + t * 0.003f) * 380 - 30, y = 30 + i * 9;
        DrawEllipse((int)x, (int)y, 34 + i * 4, 2.2f, CA(255, 170, 140, 90));
    }
    /* Colinas em duas camadas com névoa. */
    for (int x = 0; x < LOW_W; x++) {
        float h = 96 + sinf(x * 0.02f) * 6 + sinf(x * 0.05f + 1) * 3;
        vgrad(x, h, 1.05f, 60, C(130, 76, 84), C(100, 60, 60));
        float h2 = 108 + sinf(x * 0.03f + 2) * 4;
        vgrad(x, h2, 1.05f, 60, C(92, 56, 46), C(70, 44, 36));
    }
    vgrad(0, 90, LOW_W, 30, CA(255, 170, 120, 40), CA(255, 170, 120, 0));
    /* Casa de fazenda (minka): telhado alto de palha, parede de reboco e esteios
       escuros, a janela de papel acesa e a varanda de madeira. */
    Color straw = C(184, 146, 74), strawDk = C(138, 102, 50), strawLt = C(214, 178, 98), wood = C(62, 42, 30);
    rect(18, 96, 84, 34, wood);
    for (int k = 0; k < 4; k++) rect(22 + k * 20, 99, 16, 18, C(214, 198, 164));   /* reboco */
    for (int k = 0; k < 4; k++) rect(22 + k * 20, 116, 16, 1, C(170, 150, 118));
    rect(42, 101, 16, 14, C(255, 206, 128));                                          /* janela de papel acesa */
    for (int k = 1; k < 4; k++) rect(42 + k * 4, 101, 1, 14, C(150, 96, 56));
    rect(42, 108, 16, 1, C(150, 96, 56));
    glow(50, 108, 12, C(255, 170, 80));
    rect(18, 122, 84, 8, C(84, 58, 40));                                               /* varanda */
    for (int k = 0; k < 7; k++) rect(20 + k * 12, 122, 1, 8, C(60, 40, 28));
    rect(18, 121, 84, 1, C(122, 90, 60));
    DrawTriangle((Vector2){6, 97}, (Vector2){114, 97}, (Vector2){80, 54}, straw);    /* telhado de palha */
    DrawTriangle((Vector2){6, 97}, (Vector2){80, 54}, (Vector2){40, 54}, straw);
    for (int r = 0; r < 11; r++) {                                                     /* camadas da palha */
        float y = 58 + r * 4, half = 20 + (y - 54) * 0.79f;
        rect(60 - half, y, half * 2, 1, strawDk);
        for (int k = 0; k < (int)half; k += 3) rect(60 - half + k * 2 + (r % 2) * 2, y + 1, 1, 2, r % 3 ? strawLt : strawDk);
    }
    rect(4, 96, 112, 2, C(104, 74, 38));                                                /* beiral */
    rect(34, 49, 52, 6, C(78, 56, 36));                                                 /* cumeeira */
    rect(34, 49, 52, 1, C(120, 88, 56));
    line(32, 44, 40, 52, 1, C(78, 56, 36));                                             /* chigi nas pontas */
    line(40, 44, 32, 52, 1, C(78, 56, 36));
    line(80, 44, 88, 52, 1, C(78, 56, 36));
    line(88, 44, 80, 52, 1, C(78, 56, 36));
    /* Varais de arroz (hasakake): esteios, a trave e os feixes pendurados secando. */
    for (int rk = 0; rk < 2; rk++) {
        float x0 = 132 + rk * 96, len = 76;
        for (int k = 0; k <= 3; k++) rect(x0 + k * len / 3, 104, 2, 28, C(96, 66, 40));
        rect(x0 - 2, 106, len + 6, 2, C(112, 78, 46));
        for (int k = 0; k < (int)len; k += 4) {
            float sx = x0 + k + 1, sh = 11 + hash1(k + rk * 7) * 3;
            rect(sx, 108, 3, sh, C(214, 170, 82));
            rect(sx, 108 + sh - 3, 3, 3, C(168, 124, 54));
            rect(sx + 1, 110, 1, sh - 4, C(236, 198, 110));
            rect(sx, 111, 3, 1, C(120, 84, 40));
        }
    }
    for (int i = 0; i < 4; i++) {
        float bx = fract(t * 0.02f + i * 0.27f) * 360 - 20, by = 30 + i * 7 + sinf(t + i) * 3, w = sinf(t * 8 + i) * 2;
        line(bx - 3, by - w, bx, by, 0.5f, C(60, 30, 50));
        line(bx, by, bx + 3, by - w, 0.5f, C(60, 30, 50));
    }
    /* Chão de terra e trigo com as pontas pegando luz. */
    floor_shade(C(150, 104, 62), C(96, 64, 38));
    for (int i = 0; i < 220; i++) {
        float x = hash1(i) * LOW_W, y = GROUND_LOW + 2 + hash1(i + 5) * 28;
        float sw = sinf(t * 1.6f + x * 0.05f) * 2;
        line(x, y, x + sw, y - 8, 0.35f, C(196, 150, 74));
        DrawCircleV((Vector2){x + sw, y - 8}, 0.6f, C(240, 200, 110));
    }
}

/* ------------------------------------------------------------------ */
/* 4. Cobertura de um arranha-céu, na chuva                             */
/* ------------------------------------------------------------------ */
static void cobertura(const ArenaCtx *c) {
    float t = c->t;
    vgrad(0, 0, LOW_W, 70, C(6, 8, 20), C(22, 20, 48));
    vgrad(0, 70, LOW_W, 60, C(22, 20, 48), C(70, 44, 80));
    stars(30, 4, 50, 1.5f, t);
    glow(62, 32, 26, C(120, 130, 190));
    DrawCircleV((Vector2){62, 32}, 9, C(226, 228, 236));
    DrawCircleV((Vector2){66, 29}, 8, C(12, 13, 31)); /* mesma cor do céu nessa altura: sobra a lua minguante */
    /* Três camadas de prédios, cada vez mais escuras e nítidas. */
    for (int layer = 0; layer < 3; layer++) {
        Color body = layer == 0 ? C(48, 40, 78) : (layer == 1 ? C(28, 26, 52) : C(14, 14, 30));
        float baseY = 118 + layer * 6;
        for (int b = 0; b < 16 - layer * 3; b++) {
            float w = 12 + hash1(b + layer * 30) * 20, h = 30 + hash1(b * 2 + layer * 30) * (70 - layer * 14);
            float x = b * (22 + layer * 6) - 8 + layer * 9, y = baseY - h;
            rect(x, y, w, h + 40, body);
            if (layer == 0) continue;
            for (float wy = y + 3; wy < baseY; wy += 3.2f)
                for (float wx = x + 2; wx < x + w - 2; wx += 2.6f) {
                    float on = hash1(wx * 0.37f + wy * 1.7f + floorf(t * 0.15f + hash1(wx + wy)));
                    if (on > 0.66f) rect(wx, wy, 1.2f, 1.6f, layer == 2 ? C(255, 214, 140) : C(150, 176, 240));
                }
            if (hash1(b + 50 + layer) > 0.55f && fract(t * 0.6f + b * 0.3f) < 0.45f) {
                DrawCircleV((Vector2){x + w / 2, y - 2}, 0.9f, C(255, 50, 50));
                glow(x + w / 2, y - 2, 5, C(255, 40, 40));
            }
        }
    }
    vgrad(0, 96, LOW_W, 30, CA(120, 60, 140, 0), CA(120, 60, 140, 60)); /* brilho da cidade na névoa */
    /* Telhado: mureta, grade, caixa d'água, ar-condicionado e antena. */
    vgrad(0, 126, LOW_W, 18, C(46, 46, 58), C(30, 30, 40));
    rect(0, 126, LOW_W, 1, C(90, 92, 110));
    for (int x = 0; x < LOW_W; x += 8) rect(x, 114, 0.7f, 12, C(80, 84, 100));
    rect(0, 114, LOW_W, 0.8f, C(100, 104, 124));
    rect(0, 120, LOW_W, 0.5f, C(80, 84, 100));
    /* Caixa d'água. */
    for (int k = 0; k < 4; k++) line(18 + k * 7, 124, 20 + k * 5, 100, 0.8f, C(40, 36, 44));
    hgrad(12, 76, 34, 26, C(60, 44, 40), C(96, 70, 58));
    for (int k = 0; k < 5; k++) rect(12, 80 + k * 5, 34, 0.5f, C(40, 30, 28));
    DrawTriangle((Vector2){10, 77}, (Vector2){48, 77}, (Vector2){29, 68}, C(52, 38, 36));
    /* Ar-condicionado com ventiladores girando. */
    for (int u = 0; u < 2; u++) {
        float ux = 250 + u * 30;
        hgrad(ux, 108, 24, 18, C(70, 74, 86), C(110, 114, 128));
        DrawCircleV((Vector2){ux + 12, 117}, 6, C(36, 38, 46));
        for (int k = 0; k < 3; k++) {
            float a = t * 12 + k * 2.09f + u;
            line(ux + 12, 117, ux + 12 + cosf(a) * 5, 117 + sinf(a) * 5, 1, C(90, 94, 106));
        }
    }
    /* Antena com luz piscando. */
    line(222, 126, 222, 58, 1, C(60, 60, 72));
    for (int k = 0; k < 6; k++) line(219, 70 + k * 9, 225, 70 + k * 9, 0.5f, C(60, 60, 72));
    if (fract(t * 0.7f) < 0.5f) { DrawCircleV((Vector2){222, 57}, 1.3f, C(255, 60, 60)); glow(222, 57, 9, C(255, 40, 40)); }
    /* Chão de concreto molhado com reflexos e poças. */
    floor_shade(C(52, 52, 66), C(24, 24, 32));
    for (int i = 0; i < 12; i++) {
        float x = hash1(i + 3) * LOW_W, w = 8 + hash1(i) * 26, y = GROUND_LOW + 2 + hash1(i + 7) * 24;
        DrawEllipse((int)(x + w / 2), (int)y, w / 2, 1.2f, CA(120, 130, 190, 60));
        DrawEllipse((int)(x + w / 2), (int)y, w / 3, 0.5f, CA(255, 200, 150, 50));
    }
    for (int i = 0; i < 14; i++) {
        float ph = fract(t * 1.3f + hash1(i)), x = hash1(i + 11) * LOW_W, y = GROUND_LOW + 2 + hash1(i + 13) * 26;
        DrawEllipseLines((int)x, (int)y, ph * 3, ph * 0.8f, fade(C(170, 180, 220), 1 - ph));
    }
}

/* ------------------------------------------------------------------ */
/* 6. Porto dos tambores                                               */
/* ------------------------------------------------------------------ */
static void porto(const ArenaCtx *c) {
    float t = c->t;
    vgrad(0, 0, LOW_W, 100, C(8, 12, 34), C(44, 54, 96));
    stars(40, 11, 70, 1, t);
    glow(230, 40, 50, C(140, 150, 200));
    DrawCircleGradient((Vector2){230, 40}, 14, C(252, 246, 226), C(230, 226, 210));
    /* Farol distante com o facho girando. */
    rect(28, 70, 5, 26, C(30, 30, 44));
    DrawCircleV((Vector2){30.5f, 69}, 2, C(255, 230, 160));
    BeginBlendMode(BLEND_ADDITIVE);
    float a = sinf(t * 0.7f) * 0.9f;
    DrawTriangle((Vector2){30.5f, 69}, (Vector2){30.5f + cosf(a) * 140, 69 + sinf(a) * 10 - 8}, (Vector2){30.5f + cosf(a) * 140, 69 + sinf(a) * 10 + 8}, CA(255, 230, 170, 20));
    EndBlendMode();
    /* Mar com o reflexo da lua tremendo. */
    vgrad(0, 96, LOW_W, 54, C(16, 26, 60), C(8, 12, 30));
    for (int y = 98; y < 148; y += 2) {
        float w = 6 + (y - 96) * 0.6f, off = sinf(t * 2 + y) * 3;
        rect(230 - w / 2 + off, y, w, 0.7f, fade(C(236, 232, 210), 0.6f - (y - 96) / 90.0f));
        for (int k = 0; k < 3; k++) rect(fract(hash1(y + k) + t * 0.02f) * LOW_W, y, 5, 0.4f, CA(80, 100, 160, 120));
    }
    /* Navios com mastro, cordas e lanterna. */
    for (int s = 0; s < 2; s++) {
        float sx = 60 + s * 100, bob = sinf(t * 0.9f + s) * 1.2f;
        DrawTriangle((Vector2){sx, 90 + bob}, (Vector2){sx + 8, 100 + bob}, (Vector2){sx + 50, 90 + bob}, C(20, 18, 30));
        rect(sx + 8, 90 + bob, 42, 10, C(20, 18, 30));
        rect(sx + 24, 50 + bob, 1.4f, 40, C(24, 20, 30));
        DrawTriangle((Vector2){sx + 25, 52 + bob}, (Vector2){sx + 25, 86 + bob}, (Vector2){sx + 46, 86 + bob}, C(58, 54, 70));
        line(sx + 24, 50 + bob, sx + 2, 90 + bob, 0.3f, C(50, 46, 60));
        line(sx + 24, 50 + bob, sx + 50, 90 + bob, 0.3f, C(50, 46, 60));
        glow(sx + 40, 88 + bob, 6, C(255, 170, 80));
    }
    /* Varal de lanternas de papel. */
    for (int i = 0; i < 12; i++) {
        float x = i * 28 + 6, y = 66 + sinf(i * 0.5f) * 6 + sinf(t * 1.1f + i) * 1.2f;
        paper_lantern(x, y, 2.4f, C(214, 60, 40), t, i);
    }
    /* Taikos com corda e couro. */
    for (int d = 0; d < 2; d++) {
        float dx = d ? 286 : 34, r = 20 + c->beat * 2.5f;
        hgrad(dx - 18, 134, 36, 16, C(60, 34, 22), C(96, 58, 36));
        DrawEllipse((int)dx, 120, r, r * 1.05f, C(120, 52, 30));
        DrawCircleGradient((Vector2){dx, 120}, r * 0.82f, C(236, 220, 186), C(200, 180, 140));
        for (int k = 0; k < 12; k++) {
            float ang = k * 0.52f;
            DrawCircleV((Vector2){dx + cosf(ang) * r * 0.92f, 120 + sinf(ang) * r * 0.96f}, 0.9f, C(60, 30, 20));
        }
        DrawCircleV((Vector2){dx, 120}, r * 0.26f, fade(C(190, 40, 30), 0.85f));
    }
    /* Píer de tábuas. */
    floor_shade(C(96, 64, 42), C(52, 34, 22));
    for (int x = 0; x < LOW_W; x += 14) line(x, GROUND_LOW - 6, x - 4, LOW_H, 0.5f, C(44, 28, 18));
    rect(0, GROUND_LOW - 6, LOW_W, 1.5f, C(140, 98, 66));
}

/* ------------------------------------------------------------------ */
/* 7. Salão de baile com espelhos                                      */
/* ------------------------------------------------------------------ */
static void salao(const ArenaCtx *c) {
    float t = c->t;
    vgrad(0, 0, LOW_W, GROUND_LOW, C(90, 46, 44), C(170, 118, 88));
    /* Cortinas de veludo nas pontas, com pregas. */
    for (int side = 0; side < 2; side++)
        for (int k = 0; k < 5; k++) {
            float x = side ? LOW_W - 22 + k * 4.4f : k * 4.4f;
            hgrad(x, 0, 2.2f, GROUND_LOW, C(90, 10, 20), C(160, 30, 40));
            hgrad(x + 2.2f, 0, 2.2f, GROUND_LOW, C(160, 30, 40), C(90, 10, 20));
        }
    /* Espelhos em arco com moldura dourada e reflexo. */
    for (int m = 0; m < 4; m++) {
        float x = 36 + m * 66, w = 38;
        vgrad(x - 3, 28, w + 6, 110, C(230, 190, 96), C(160, 118, 50));
        DrawCircleV((Vector2){x + w / 2, 40}, w / 2 + 3, C(226, 184, 90));
        vgrad(x, 40, w, 96, C(170, 180, 196), C(110, 116, 140));
        DrawCircleV((Vector2){x + w / 2, 40}, w / 2, C(170, 180, 196));
        float sh = fract(t * 0.25f + m * 0.2f);
        BeginBlendMode(BLEND_ADDITIVE);
        for (int k = 0; k < 5; k++) line(x + sh * (w + 30) - 20 + k, 40, x + sh * (w + 30) - 34 + k, 134, 0.6f, CA(255, 255, 255, 40));
        EndBlendMode();
        for (int k = 0; k < 3; k++) DrawCircleV((Vector2){x + w / 2 - 8 + k * 8, 22}, 1.2f, C(230, 190, 96));
    }
    /* Lustres de cristal. */
    for (int l = 0; l < 2; l++) {
        float lx = l ? 230 : 90, sw = sinf(t * 0.7f + l) * 1.2f;
        line(lx, 0, lx + sw, 10, 0.5f, C(200, 160, 70));
        DrawEllipse((int)(lx + sw), 15, 18, 3.5f, C(210, 170, 80));
        for (int k = 0; k < 9; k++) {
            float cx = lx + sw - 16 + k * 4, fl = 0.7f + 0.3f * sinf(t * 11 + k * 2);
            line(cx, 16, cx, 22 + (k % 3) * 2, 0.4f, C(240, 236, 255));
            rect(cx - 0.3f, 10, 0.6f, 3, C(250, 240, 220));
            glow(cx, 9, 5 * fl, C(255, 200, 120));
        }
        glow(lx + sw, 14, 44, C(255, 180, 100));
    }
    /* Mármore polido em xadrez, com reflexo das luzes. */
    for (int gx = 0; gx < 16; gx++)
        for (int gy = 0; gy < 4; gy++)
            vgrad(gx * 20, GROUND_LOW - 6 + gy * 9, 20, 9, ((gx + gy) & 1) ? C(62, 40, 42) : C(226, 212, 190), ((gx + gy) & 1) ? C(40, 26, 28) : C(196, 180, 160));
    vgrad(0, GROUND_LOW - 6, LOW_W, 14, CA(255, 220, 160, 40), CA(255, 220, 160, 0));
    rect(0, GROUND_LOW - 6, LOW_W, 1.2f, C(214, 176, 90));
}

/* ------------------------------------------------------------------ */
/* 8. Teto do trem-bala                                                */
/* ------------------------------------------------------------------ */
static void trem(const ArenaCtx *c) {
    float t = c->t;
    vgrad(0, 0, LOW_W, GROUND_LOW, C(4, 6, 20), C(34, 30, 64));
    stars(60, 21, 80, 2, t);
    /* Montanhas lentas, cidade média e postes rápidos. */
    for (int x = 0; x < LOW_W; x++) {
        float wx = x + t * 8;
        float h = 88 + sinf(wx * 0.013f) * 14 + sinf(wx * 0.041f) * 6;
        vgrad(x, h, 1.05f, 80, C(30, 30, 60), C(16, 16, 34));
    }
    vgrad(0, 100, LOW_W, 20, CA(255, 180, 120, 0), CA(255, 180, 120, 30));
    for (int b = 0; b < 18; b++) {
        float bx = fract(b / 18.0f - t * 0.25f) * 380 - 30, h = 14 + hash1(b) * 30;
        rect(bx, 120 - h, 14, h + 20, C(14, 16, 32));
        for (int k = 0; k < 8; k++) if (hash1(b * 7 + k) > 0.5f) rect(bx + 2 + (k % 3) * 4, 124 - h + (k / 3) * 6, 1.4f, 1.6f, C(255, 204, 120));
    }
    for (int p = 0; p < 3; p++) {
        float px = fract(p / 3.0f - t * 2.2f) * 420 - 50;
        hgrad(px, 18, 4, 124, C(8, 8, 14), C(24, 24, 34));
        rect(px - 10, 22, 26, 2, C(14, 14, 22));
    }
    for (int k = 0; k < 2; k++) line(0, 24 + k * 4, LOW_W, 26 + k * 4, 0.5f, C(50, 50, 70));
    float tun = fract(t / 14.0f);
    if (tun > 0.9f) {
        float a = sinf((tun - 0.9f) / 0.1f * PI_F);
        rect(0, 0, LOW_W, GROUND_LOW, fade(C(4, 4, 8), a));
        for (int k = 0; k < 4; k++) rect(fract(k / 4.0f - t * 3) * 400 - 40, 60, 20, 3, fade(C(255, 200, 120), a));
    }
    /* Teto do vagão: chapas com rebites, respiros e o pantógrafo. */
    vgrad(0, GROUND_LOW - 8, LOW_W, 22, C(196, 204, 216), C(120, 128, 142));
    rect(0, GROUND_LOW - 8, LOW_W, 1.4f, C(236, 240, 246));
    for (int x = 0; x < LOW_W; x += 24) {
        rect(x, GROUND_LOW - 6, 0.5f, 20, C(100, 108, 122));
        for (int k = 0; k < 3; k++) DrawCircleV((Vector2){x + 3, GROUND_LOW - 3 + k * 6}, 0.5f, C(150, 156, 170));
    }
    for (int v = 0; v < 3; v++) {
        float vx = 40 + v * 110;
        hgrad(vx, GROUND_LOW - 12, 20, 4, C(90, 96, 110), C(150, 156, 170));
    }
    line(268, GROUND_LOW - 8, 280, GROUND_LOW - 30, 1, C(60, 64, 76));
    line(292, GROUND_LOW - 8, 280, GROUND_LOW - 30, 1, C(60, 64, 76));
    line(270, GROUND_LOW - 30, 300, GROUND_LOW - 30, 1, C(60, 64, 76));
    vgrad(0, 164, LOW_W, 16, C(200, 40, 50), C(120, 20, 30));
    rect(0, 166, LOW_W, 1, C(250, 240, 240));
}

/* ------------------------------------------------------------------ */
/* 9. Cachoeira                                                        */
/* ------------------------------------------------------------------ */
static void cachoeira(const ArenaCtx *c) {
    float t = c->t;
    vgrad(0, 0, LOW_W, GROUND_LOW, C(140, 186, 200), C(80, 120, 126));
    /* Paredões de rocha com musgo e volume. */
    for (int y = 0; y < GROUND_LOW; y++) {
        float l = 90 + sinf(y * 0.08f) * 8 + sinf(y * 0.21f) * 4;
        float r = 230 + sinf(y * 0.07f + 2) * 8;
        hgrad(0, y, l, 1.05f, C(46, 62, 56), C(84, 104, 92));
        hgrad(r, y, LOW_W - r, 1.05f, C(84, 102, 92), C(40, 54, 50));
        if (y % 9 == 0) { rect(l - 10, y, 10, 1.4f, C(80, 130, 70)); rect(r, y + 3, 8, 1.4f, C(80, 130, 70)); }
    }
    for (int k = 0; k < 4; k++) {
        float tx = k < 2 ? 14 + k * 26 : 250 + (k - 2) * 30;
        DrawTriangle((Vector2){tx, 20}, (Vector2){tx - 12, 60}, (Vector2){tx + 12, 60}, C(40, 70, 50));
        DrawTriangle((Vector2){tx, 34}, (Vector2){tx - 15, 76}, (Vector2){tx + 15, 76}, C(34, 60, 44));
    }
    /* A queda d'água em degradê com fios descendo. */
    vgrad(96, 0, 130, 120, C(200, 232, 244), C(236, 250, 255));
    for (int i = 0; i < 60; i++) {
        float x = 98 + hash1(i) * 126, y = fract(t * (1.2f + hash1(i + 2)) + hash1(i + 4)) * 140 - 20;
        rect(x, y, 0.8f, 14 + hash1(i + 1) * 12, C(255, 255, 255));
        rect(x + 0.8f, y + 6, 0.5f, 18, C(150, 200, 226));
    }
    /* Poço com espuma. */
    vgrad(80, 116, 160, 34, C(90, 150, 176), C(50, 100, 124));
    for (int i = 0; i < 40; i++) {
        float x = 88 + hash1(i) * 144, r = 2 + fabsf(sinf(t * 3 + i)) * 4;
        DrawCircleGradient((Vector2){x, 118 + hash1(i + 3) * 5}, r, C(246, 252, 255), CA(246, 252, 255, 0));
    }
    BeginBlendMode(BLEND_ADDITIVE);
    Color rb[] = {C(255, 0, 0), C(255, 160, 0), C(255, 255, 0), C(0, 255, 0), C(0, 120, 255), C(140, 0, 255)};
    for (int k = 0; k < 6; k++) DrawRing((Vector2){160, 150}, 110 + k * 1.6f, 111.6f + k * 1.6f, 200, 340, 60, fade(rb[k], 0.06f));
    EndBlendMode();
    /* Laje de pedra com fendas e poças. */
    floor_shade(C(104, 110, 106), C(62, 66, 64));
    for (int i = 0; i < 14; i++) {
        float x = hash1(i) * LOW_W, y = GROUND_LOW - 2 + hash1(i + 1) * 24;
        line(x, y, x + 10 + hash1(i + 2) * 16, y + hash1(i + 3) * 4 - 2, 0.4f, C(50, 54, 52));
    }
    for (int i = 0; i < 5; i++) DrawEllipse((int)(hash1(i + 20) * LOW_W), (int)(GROUND_LOW + 6 + hash1(i + 21) * 20), 8, 1.2f, CA(190, 226, 240, 80));
    rect(0, GROUND_LOW - 6, LOW_W, 1.2f, C(150, 156, 150));
}

/* ------------------------------------------------------------------ */
/* 10. Bambuzal à meia-noite                                           */
/* ------------------------------------------------------------------ */
static void bambuzal(const ArenaCtx *c) {
    float t = c->t, lit = 1 - c->blackout;
    vgrad(0, 0, LOW_W, GROUND_LOW, mix(C(2, 2, 6), C(10, 16, 34), lit), mix(C(4, 4, 8), C(26, 38, 52), lit));
    /* Três camadas de bambu com névoa entre elas. */
    for (int layer = 0; layer < 3; layer++) {
        Color col = mix(C(6, 8, 10), mix(C(24, 44, 40), C(58, 100, 66), layer / 2.0f), lit);
        for (int i = 0; i < 16 - layer * 3; i++) {
            float x = hash1(i + layer * 20) * LOW_W;
            float sway = sinf(t * (0.6f + layer * 0.2f) + i) * (2 + layer);
            float w = 2.5f + layer * 1.2f;
            line(x, GROUND_LOW, x + sway, 0, w, col);
            for (int y = 10; y < GROUND_LOW; y += 24) {
                float k = (float)(GROUND_LOW - y) / GROUND_LOW;
                line(x + sway * k - w / 2 - 0.4f, y, x + sway * k + w / 2 + 0.4f, y, 0.7f, mix(col, C(0, 0, 0), 0.35f));
                if ((y / 24 + i) % 3 == 0) {
                    float lx = x + sway * k;
                    DrawTriangle((Vector2){lx, y}, (Vector2){lx + 10, y - 3}, (Vector2){lx + 9, y - 1}, col);
                    DrawTriangle((Vector2){lx, y}, (Vector2){lx - 9, y - 2}, (Vector2){lx - 8, y}, col);
                }
            }
        }
        vgrad(0, 60, LOW_W, 90, fade(mix(C(4, 4, 8), C(40, 60, 70), lit), 0), fade(mix(C(4, 4, 8), C(40, 60, 70), lit), 0.25f));
    }
    /* Faixas de luar entre os bambus. */
    if (lit > 0.1f) {
        BeginBlendMode(BLEND_ADDITIVE);
        for (int k = 0; k < 3; k++) {
            float x = 70 + k * 90;
            DrawTriangle((Vector2){x, 0}, (Vector2){x - 30, GROUND_LOW}, (Vector2){x - 10, GROUND_LOW}, fade(C(160, 190, 230), 0.05f * lit));
        }
        EndBlendMode();
    }
    /* Lanternas de pedra: apagam no apagão. */
    for (int l = 0; l < 2; l++) {
        float lx = l ? 286 : 34;
        hgrad(lx - 3, 128, 6, 22, C(56, 58, 64), C(90, 92, 98));
        hgrad(lx - 7, 122, 14, 6, C(64, 66, 72), C(100, 102, 108));
        DrawTriangle((Vector2){lx - 10, 118}, (Vector2){lx + 10, 118}, (Vector2){lx, 110}, C(80, 82, 88));
        rect(lx - 9, 118, 18, 3, C(72, 74, 80));
        rect(lx - 4, 123, 8, 5, mix(C(10, 10, 10), C(255, 190, 90), lit));
        if (lit > 0.05f) glow(lx, 126, 34 * lit * (0.9f + 0.1f * sinf(t * 9 + l)), C(255, 150, 60));
    }
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < 22; i++) {
        float x = hash1(i) * LOW_W + sinf(t * 0.5f + i) * 12, y = 60 + hash1(i + 1) * 80 + cosf(t * 0.7f + i) * 8;
        float b = fmaxf(0, sinf(t * 2 + i * 2.1f));
        DrawCircleGradient((Vector2){x, y}, 2.5f, fade(C(200, 255, 120), b * (0.3f + 0.7f * lit)), fade(C(200, 255, 120), 0));
    }
    EndBlendMode();
    floor_shade(mix(C(4, 4, 6), C(34, 42, 34), lit), mix(C(2, 2, 4), C(16, 20, 16), lit));
    for (int i = 0; i < 60; i++) {
        float x = hash1(i) * LOW_W, y = GROUND_LOW - 4 + hash1(i + 2) * 28;
        line(x, y, x + 3, y - 0.6f, 0.5f, mix(C(8, 8, 8), C(70, 90, 56), lit));
    }
}

/* ------------------------------------------------------------------ */
/* 11. Forja dentro da cratera                                         */
/* ------------------------------------------------------------------ */
static void forja(const ArenaCtx *c) {
    float t = c->t;
    vgrad(0, 0, LOW_W, GROUND_LOW, C(18, 6, 6), C(90, 26, 12));
    /* Paredes de rocha com a luz da lava subindo nelas. */
    for (int x = 0; x < LOW_W; x++) {
        float h = 20 + sinf(x * 0.05f) * 10 + sinf(x * 0.13f) * 6;
        vgrad(x, 0, 1.05f, h, C(24, 10, 8), C(50, 18, 12));
        float h2 = 70 + sinf(x * 0.03f + 1) * 12;
        vgrad(x, h2, 1.05f, 60, C(80, 30, 18), C(40, 14, 10));
    }
    /* Rio de lava com correnteza e bolhas. */
    vgrad(0, 110, LOW_W, 24, C(255, 200, 80), C(190, 40, 10));
    for (int k = 0; k < 40; k++) {
        float x = fract(hash1(k) + t * 0.03f) * 360 - 20, y = 112 + hash1(k + 1) * 20;
        rect(x, y, 6 + hash1(k + 2) * 10, 0.8f, CA(255, 240, 160, 150));
    }
    for (int i = 0; i < 8; i++) {
        float ph = fract(t * 0.5f + hash1(i)), x = hash1(i + 10) * LOW_W;
        if (ph < 0.3f) DrawCircleLines((int)x, 116, ph * 10, C(255, 236, 140));
    }
    glow(160, 122, 150, C(170, 50, 10));
    /* Forno de tijolos com a boca acesa, fole e bigorna. */
    for (int r = 0; r < 8; r++)
        for (int k = 0; k < 5; k++) rect(250 + k * 10 + (r % 2) * 5, 82 + r * 6, 9.4f, 5.4f, C(70 + (k * 7 + r * 5) % 20, 34, 26));
    DrawCircleGradient((Vector2){275, 108}, 12, C(255, 220, 120), C(255, 100, 20));
    glow(275, 107, 36 * (0.9f + 0.1f * sinf(t * 7)), C(255, 110, 30));
    hgrad(26, 116, 36, 9, C(40, 40, 48), C(84, 84, 96));
    DrawTriangle((Vector2){26, 116}, (Vector2){26, 125}, (Vector2){14, 118}, C(50, 50, 58));
    hgrad(36, 125, 16, 16, C(34, 34, 40), C(64, 64, 74));
    line(40, 112, 58, 104, 1.2f, C(110, 80, 60));
    rect(55, 101, 6, 4, C(70, 70, 80));
    /* Chão de basalto rachado com veios em brasa. */
    floor_shade(C(44, 30, 28), C(20, 14, 14));
    for (int i = 0; i < 12; i++) {
        float x = hash1(i) * LOW_W, g = 0.6f + 0.4f * sinf(t * 2 + i);
        line(x, GROUND_LOW - 4, x + 14, GROUND_LOW + 10, 0.8f, fade(C(255, 130, 40), g));
        line(x + 14, GROUND_LOW + 10, x + 6, LOW_H, 0.6f, fade(C(255, 90, 20), g * 0.7f));
    }
}

/* ------------------------------------------------------------------ */
/* 12. Jardim de Vidro                                                 */
/* ------------------------------------------------------------------ */
static void jardim(const ArenaCtx *c) {
    float t = c->t;
    vgrad(0, 0, LOW_W, GROUND_LOW, C(16, 16, 36), C(74, 84, 116));
    stars(90, 31, 100, 1.2f, t);
    BeginBlendMode(BLEND_ADDITIVE);
    for (int x = 0; x < LOW_W; x++) {
        float y = 30 + sinf(x * 0.03f + t * 0.4f) * 10 + sinf(x * 0.07f - t * 0.3f) * 5;
        Color col = ColorFromHSV(160 + sinf(x * 0.01f + t * 0.2f) * 50, 0.6f, 0.8f);
        vgrad(x, y, 1.05f, 34, fade(col, 0.16f), fade(col, 0));
    }
    EndBlendMode();
    /* Árvores de cristal com faces claras e escuras. */
    for (int i = 0; i < 8; i++) {
        float x = 8 + i * 42 + hash1(i) * 10, h = 50 + hash1(i + 3) * 44, b = GROUND_LOW - 6;
        DrawTriangle((Vector2){x, b - h}, (Vector2){x - 11, b}, (Vector2){x, b}, fade(C(150, 210, 240), 0.55f));
        DrawTriangle((Vector2){x, b - h}, (Vector2){x, b}, (Vector2){x + 11, b}, fade(C(210, 236, 255), 0.6f));
        DrawTriangle((Vector2){x + 8, b - h * 0.6f}, (Vector2){x + 2, b}, (Vector2){x + 16, b}, fade(C(200, 180, 255), 0.45f));
        line(x, b - h, x, b, 0.5f, CA(255, 255, 255, 150));
        glow(x, b - h, 6, C(180, 220, 255));
    }
    for (int i = 0; i < 20; i++) {
        float x = hash1(i) * LOW_W + sinf(t * 0.4f + i) * 10, y = 40 + hash1(i + 4) * 80 + sinf(t * 0.6f + i * 2) * 6;
        DrawPoly((Vector2){x, y}, 3, 1.5f + hash1(i + 1) * 2, t * 40 + i * 30, fade(C(220, 240, 255), 0.8f));
    }
    vgrad(146, 92, 28, 40, fade(C(200, 220, 255), 0.5f), fade(C(160, 190, 240), 0.2f));
    glow(160, 100, 24, C(160, 200, 255));
    /* Piso de vidro escuro com reflexo. */
    floor_shade(C(40, 46, 72), C(18, 20, 34));
    for (int x = 0; x < LOW_W; x += 32) line(x, GROUND_LOW - 6, x - 20, LOW_H, 0.4f, C(80, 96, 140));
    rect(0, GROUND_LOW - 6, LOW_W, 0.8f, C(170, 200, 240));
}

/* ------------------------------------------------------------------ */
/* 13. Cidadela da Liga (BIG BOSS)                                     */
/* ------------------------------------------------------------------ */
static void cidadela(const ArenaCtx *c) {
    float t = c->t;
    Color skyTop[] = {C(30, 16, 50), C(12, 12, 28), C(40, 4, 8)};
    Color skyBot[] = {C(170, 86, 124), C(62, 62, 92), C(170, 34, 22)};
    int s = c->seal < 0 ? 0 : (c->seal > 2 ? 2 : c->seal);
    vgrad(0, 0, LOW_W, GROUND_LOW, skyTop[s], skyBot[s]);
    if (s == 2) {
        glow(160, 40, 70, C(255, 60, 30));
        DrawCircleV((Vector2){160, 40}, 24, C(255, 130, 70));
        DrawCircleV((Vector2){160, 40}, 21, C(10, 2, 4));
    } else {
        glow(250, 34, 34, s ? C(140, 140, 180) : C(250, 210, 230));
        DrawCircleV((Vector2){250, 34}, 13, s ? C(170, 170, 200) : C(250, 226, 240));
    }
    /* Castelo ao fundo, com janelas acesas. */
    for (int k = 0; k < 3; k++) {
        float w = 60 - k * 16, y = 70 - k * 12;
        rect(160 - w / 2, y, w, 12, C(24, 16, 30));
        DrawTriangle((Vector2){160 - w / 2 - 8, y}, (Vector2){160 + w / 2 + 8, y}, (Vector2){160, y - 8}, C(40, 22, 36));
        for (int j = 0; j < 4 - k; j++) rect(160 - w / 2 + 6 + j * (w - 12) / (3.0f - k + 0.01f), y + 4, 2, 3, C(255, 180, 90));
    }
    /* Nuvens em camadas, mais rápidas a cada selo. */
    for (int layer = 0; layer < 3; layer++)
        for (int i = 0; i < 8; i++) {
            float x = fract(hash1(i + layer * 9) + t * 0.01f * (layer + 1) * (1 + s)) * 400 - 40;
            float y = 90 + layer * 14 + hash1(i + 2) * 10;
            Color cl = mix(skyBot[s], C(20, 16, 30), 0.3f + layer * 0.2f);
            DrawEllipse((int)x, (int)y, 34 + hash1(i) * 22, 6, cl);
            DrawEllipse((int)(x + 12), (int)(y - 3), 20, 4, mix(cl, C(255, 255, 255), 0.08f));
        }
    if (c->lightning > 0.01f) {
        rect(0, 0, LOW_W, GROUND_LOW, fade(C(230, 230, 255), c->lightning * 0.6f));
        float lx = 60 + hash1(floorf(t * 3)) * 200, y = 0;
        while (y < 100) {
            float nx = lx + (hash1(y + lx) - 0.5f) * 20;
            line(lx, y, nx, y + 12, 1.4f, fade(C(255, 255, 255), c->lightning));
            lx = nx;
            y += 12;
        }
    }
    /* Torii com a viga superior curva. */
    Color red = s == 2 ? C(150, 20, 20) : C(176, 40, 36);
    hgrad(92, 62, 8, 88, mix(red, C(0, 0, 0), 0.3f), red);
    hgrad(220, 62, 8, 88, red, mix(red, C(0, 0, 0), 0.3f));
    for (int x = 76; x < 244; x++) {
        float k = (x - 160) / 84.0f, dy = k * k * 5;
        rect(x, 50 - dy, 1.05f, 7, C(36, 18, 22));
    }
    rect(86, 66, 148, 5, red);
    rect(156, 57, 8, 9, red);
    for (int k = 0; k < 4; k++) {
        if (k == 1 || k == 2) continue;
        float lx = 40 + k * 80;
        hgrad(lx - 4, 124, 8, 26, C(60, 56, 70), C(96, 90, 106));
        hgrad(lx - 7, 120, 14, 4, C(70, 66, 80), C(104, 98, 114));
        rect(lx - 3, 127, 6, 6, C(255, 186, 96));
        glow(lx, 130, 20 * (0.85f + 0.15f * sinf(t * 7 + k)), C(255, 140, 60));
    }
    /* Piso de pedra que racha a cada selo. */
    floor_shade(C(60, 54, 72), C(30, 26, 38));
    for (int x = 0; x < LOW_W; x += 20) line(x, GROUND_LOW - 6, x - 8, LOW_H, 0.5f, C(34, 30, 44));
    rect(0, GROUND_LOW - 6, LOW_W, 1.2f, C(120, 100, 140));
    for (int k = 0; k < s * 4; k++) {
        float x = hash1(k + 40) * LOW_W;
        line(x, GROUND_LOW - 4, x + 10 - hash1(k) * 20, LOW_H, 0.8f, s == 2 ? C(255, 90, 30) : C(20, 16, 24));
    }
}

/* ------------------------------------------------------------------ */
/* Encosta da serra (jinshi): pôr do sol, serras em camadas, vento     */
/* ------------------------------------------------------------------ */
static void serra(const ArenaCtx *c) {
    float t = c->t;
    vgrad(0, 0, LOW_W, 70, C(58, 40, 72), C(190, 110, 110));
    vgrad(0, 70, LOW_W, 60, C(190, 110, 110), C(255, 176, 110));
    glow(172, 104, 80, C(255, 150, 70));
    DrawCircleGradient((Vector2){172, 104}, 22, C(255, 240, 196), C(255, 200, 130));
    for (int i = 0; i < 6; i++) {
        float x = fract(hash1(i) + t * 0.004f * (1 + i % 3)) * 380 - 30, y = 20 + hash1(i + 4) * 44;
        DrawEllipse((int)x, (int)y, 28 + hash1(i + 1) * 16, 3, CA(250, 180, 150, 120));
    }
    /* Três camadas de serra com névoa entre elas. */
    Color layers[3] = {C(168, 110, 124), C(126, 80, 100), C(84, 54, 70)};
    for (int l = 0; l < 3; l++) {
        for (int x = 0; x < LOW_W; x++) {
            float fx = x + l * 57;
            float h = 70 + l * 20 + sinf(fx * (0.018f + l * 0.006f)) * (18 - l * 4) + sinf(fx * 0.061f + l) * 5;
            vgrad(x, h, 1.05f, GROUND_LOW - h, layers[l], mix(layers[l], C(40, 24, 34), 0.3f));
        }
        vgrad(0, 76 + l * 20, LOW_W, 20, CA(255, 190, 150, 0), CA(255, 190, 150, 36));
    }
    /* Pagode distante e pinheiros torcidos pelo vento. */
    rect(58, 88, 8, 14, C(70, 40, 54));
    for (int k = 0; k < 3; k++) {
        float w = 16 - k * 4, y = 88 - k * 6;
        DrawTriangle((Vector2){62 - w / 2, y}, (Vector2){62 + w / 2, y}, (Vector2){62, y - 5}, C(70, 40, 54));
    }
    for (int k = 0; k < 2; k++) {
        float px = k ? 268 : 28, sw = sinf(t * 0.9f + k) * 2;
        line(px, GROUND_LOW - 4, px + 2 + sw, 96, 2.2f, C(40, 26, 30));
        for (int b = 0; b < 3; b++) DrawEllipse((int)(px + 6 + sw + b * 2), (int)(100 + b * 10), 12 - b * 2, 3, C(46, 50, 44));
    }
    /* Campo de capim com as pontas pegando o sol. */
    floor_shade(C(84, 58, 52), C(44, 30, 30));
    for (int i = 0; i < 200; i++) {
        float x = hash1(i) * LOW_W, y = GROUND_LOW + 1 + hash1(i + 5) * 28;
        float sw = sinf(t * 1.6f + x * 0.05f) * 2.4f;
        line(x, y, x + sw, y - 7, 0.35f, C(120, 84, 70));
        DrawCircleV((Vector2){x + sw, y - 7}, 0.5f, C(230, 170, 120));
    }
}

/* ------------------------------------------------------------------ */
/* Portão do tigre branco                                              */
/* ------------------------------------------------------------------ */
/* Tigre de pedra sentado num pedestal, virado para o centro. */
/* Bordo de outono em pixel: tronco e galhos escuros e a copa em tufos de folhas,
 * mais claros em cima (a luz da lua) e mais escuros embaixo, com vãos. */
static void maple(float x, float y, float r, int seed) {
    rect(x - 1, y, 3, 26, C(50, 34, 30));
    line(x, y + 4, x - r * 0.5f, y - 2, 1, C(50, 34, 30));
    line(x + 1, y + 2, x + r * 0.55f, y - 4, 1, C(50, 34, 30));
    static const Color leaf[4] = {{112, 36, 30, 255}, {150, 52, 36, 255}, {190, 80, 42, 255}, {222, 124, 56, 255}};
    for (int k = 0; k < 70; k++) {
        float a = hash1(seed * 97 + k) * 6.2832f, d = sqrtf(hash1(seed * 31 + k * 3)) * r;
        float lx = x + cosf(a) * d * 1.15f, ly = y - 3 + sinf(a) * d * 0.8f;
        int shade = (int)((1 - (ly - (y - 3 - r * 0.8f)) / (r * 1.6f)) * 3.2f + hash1(k + seed) * 0.8f);
        if (shade < 0) shade = 0;
        if (shade > 3) shade = 3;
        rect(floorf(lx), floorf(ly), 2 + (k % 3 == 0), 2, leaf[shade]);
    }
}

static void stone_tiger(float x, float base, float dir) {
    Color light = C(214, 210, 198), mid = C(170, 166, 156), dark = C(110, 106, 100), stripe = C(60, 58, 60);
    rect(x - 10, base - 10, 20, 10, dark);
    rect(x - 11, base - 12, 22, 3, mid);
    float bx = x - dir * 2;
    DrawEllipse((int)bx, (int)(base - 19), 8, 7, mid);                       /* corpo sentado */
    DrawEllipse((int)(bx - dir * 1.5f), (int)(base - 20), 5, 5, light);
    DrawCircleV((Vector2){x + dir * 5, base - 30}, 5, light);                /* cabeça */
    DrawTriangle((Vector2){x + dir * 2, base - 35}, (Vector2){x + dir * 4, base - 38}, (Vector2){x + dir * 6, base - 35}, mid);
    DrawTriangle((Vector2){x + dir * 6, base - 35}, (Vector2){x + dir * 8, base - 38}, (Vector2){x + dir * 9, base - 34}, mid);
    for (int k = 0; k < 3; k++) line(bx - 5 + k * 4, base - 24, bx - 3 + k * 4, base - 15, 0.7f, stripe);
    line(x + dir * 3, base - 32, x + dir * 5, base - 29, 0.6f, stripe);
    rect(x + dir * 1 - 1, base - 14, 2.5f, 3, light);                        /* patas */
    rect(x + dir * 5 - 1, base - 14, 2.5f, 3, light);
    line(bx - dir * 8, base - 14, bx - dir * 12, base - 26, 1.2f, mid);     /* cauda */
}

static void templo(const ArenaCtx *c) {
    float t = c->t;
    vgrad(0, 0, LOW_W, GROUND_LOW, C(18, 16, 34), C(86, 70, 92));
    stars(60, 53, 90, 1.0f, t);
    /* Lua cheia do oeste. */
    glow(250, 38, 40, C(255, 236, 200));
    DrawCircleV((Vector2){250, 38}, 13, C(250, 244, 226));
    DrawCircleV((Vector2){246, 35}, 3, C(232, 224, 204));
    DrawCircleV((Vector2){254, 42}, 2, C(236, 228, 208));
    /* Serra ao fundo e bordos de outono. */
    for (int i = 0; i < 6; i++) {
        float x = i * 64 - 10, h = 36 + hash1(i + 9) * 22;
        DrawTriangle((Vector2){x - 50, 118}, (Vector2){x + 70, 118}, (Vector2){x + 10, 118 - h}, C(40, 34, 56));
    }
    for (int i = 0; i < 7; i++) {
        float x = 10 + i * 48 + hash1(i + 2) * 12, y = 96 + hash1(i + 5) * 10;
        maple(x, y, 12 + hash1(i) * 4, i);
    }
    /* Muro baixo do templo. */
    rect(0, 118, LOW_W, 14, C(88, 80, 84));
    rect(0, 116, LOW_W, 3, C(120, 110, 112));
    for (int x = 0; x < LOW_W; x += 16) line(x, 119, x, 132, 0.4f, C(60, 54, 60));
    /* O portão de pedra clara (torii do oeste). */
    Color stone = C(222, 218, 206), shade = C(170, 164, 152);
    hgrad(122, 40, 7, 92, shade, stone);
    hgrad(191, 40, 7, 92, shade, stone);
    vgrad(110, 38, 100, 5, stone, shade);
    DrawTriangle((Vector2){106, 38}, (Vector2){112, 33}, (Vector2){112, 38}, stone);
    DrawTriangle((Vector2){208, 33}, (Vector2){214, 38}, (Vector2){208, 38}, stone);
    rect(112, 33, 96, 5, C(236, 232, 222));
    rect(116, 52, 88, 4, shade);
    vgrad(151, 44, 18, 14, C(40, 34, 40), C(28, 24, 30));                    /* placa com o tigre */
    line(155, 48, 165, 48, 0.8f, C(230, 220, 190));
    line(157, 51, 163, 55, 0.8f, C(230, 220, 190));
    line(163, 51, 157, 55, 0.8f, C(230, 220, 190));
    /* Tigres de pedra e lanternas de pedra. */
    stone_tiger(88, 140, 1);
    stone_tiger(232, 140, -1);
    for (int l = 0; l < 2; l++) {
        float lx = l ? 284 : 36;
        rect(lx - 2, 118, 4, 22, C(120, 114, 110));
        rect(lx - 6, 110, 12, 8, C(150, 144, 138));
        rect(lx - 4, 112, 8, 4, C(255, 196, 120));
        DrawTriangle((Vector2){lx - 8, 110}, (Vector2){lx + 8, 110}, (Vector2){lx, 104}, C(110, 104, 100));
        glow(lx, 114, 14 * (0.85f + 0.15f * sinf(t * 7 + l * 2)), C(255, 170, 80));
    }
    /* Pátio de lajes claras sob a lua. */
    floor_shade(C(150, 144, 150), C(70, 64, 74));
    for (int i = 0; i < 12; i++) line(i * 32 - 16 + 16, GROUND_LOW - 6, i * 32 - 60, LOW_H, 0.5f, C(96, 90, 100));
    line(0, 160, LOW_W, 160, 0.5f, C(96, 90, 100));
    line(0, 172, LOW_W, 172, 0.5f, C(96, 90, 100));
    for (int i = 0; i < 16; i++) {
        float x = hash1(i + 40) * LOW_W, y = GROUND_LOW + hash1(i + 41) * 28;
        DrawEllipse((int)x, (int)y, 1.5f, 0.8f, hash1(i) < 0.5f ? C(170, 64, 40) : C(206, 110, 50));
    }
}

void arena_draw_back(ArenaId id, const ArenaCtx *c) {
    switch (id) {
        case ARENA_DOJO: dojo(c); break;
        case ARENA_SERRA: serra(c); break;
        case ARENA_CELEIRO: celeiro(c); break;
        case ARENA_COBERTURA: cobertura(c); break;
        case ARENA_PORTO: porto(c); break;
        case ARENA_SALAO: salao(c); break;
        case ARENA_TREM: trem(c); break;
        case ARENA_CACHOEIRA: cachoeira(c); break;
        case ARENA_BAMBUZAL: bambuzal(c); break;
        case ARENA_FORJA: forja(c); break;
        case ARENA_JARDIM: jardim(c); break;
        case ARENA_CIDADELA: cidadela(c); break;
        case ARENA_TEMPLO: templo(c); break;
        default: ClearBackground(BLACK); break;
    }
    /* Apagão genérico (qualquer cenário pode escurecer). */
    if (c->blackout > 0 && id != ARENA_BAMBUZAL) rect(0, 0, LOW_W, LOW_H, fade(C(0, 0, 0), c->blackout * 0.9f));
}

/* ------------------------------------------------------------------ */
/* Frente: por cima dos lutadores                                      */
/* ------------------------------------------------------------------ */
void arena_draw_front(ArenaId id, const ArenaCtx *c) {
    float t = c->t;
    switch (id) {
        case ARENA_DOJO:
            for (int i = 0; i < 24; i++) {
                float x = hash1(i) * LOW_W + sinf(t * 0.3f + i) * 8, y = fract(hash1(i + 1) - t * 0.01f) * LOW_H;
                DrawPixel((int)x, (int)y, CA(255, 225, 170, 110));
            }
            break;
        case ARENA_SERRA:
            /* Pétalas e folhas atravessando com o vento. */
            for (int i = 0; i < 18; i++) {
                float x = fract(hash1(i) + t * 0.05f) * 360 - 20, y = fract(hash1(i + 3) + t * 0.03f) * 190 - 5;
                DrawEllipse((int)x, (int)y, 1.4f, 0.7f, CA(255, 200, 210, 200));
            }
            break;
        case ARENA_CELEIRO:
            BeginBlendMode(BLEND_ADDITIVE);
            for (int i = 0; i < 14; i++) {
                float x = hash1(i) * LOW_W + sinf(t * 0.5f + i) * 10, y = 95 + hash1(i + 2) * 65 + cosf(t * 0.8f + i) * 5;
                float b = fmaxf(0, sinf(t * 1.7f + i * 2.3f));
                DrawPixel((int)x, (int)y, fade(C(255, 240, 120), b));
            }
            EndBlendMode();
            break;
        case ARENA_COBERTURA:
            for (int i = 0; i < 50; i++) {
                float x = fract(hash1(i) + t * 0.05f) * (LOW_W + 20) - 10, y = fract(hash1(i + 3) + t * (1.6f + hash1(i) * 0.6f)) * (LOW_H + 10) - 10;
                DrawLine((int)x, (int)y, (int)(x - 1), (int)(y + 5), CA(170, 190, 230, 110));
            }
            break;
        case ARENA_PORTO:
        case ARENA_CACHOEIRA: {
            bool falls = id == ARENA_CACHOEIRA;
            /* névoa em fiapos horizontais (no lugar das bolhas de luz) */
            for (int i = 0; i < (falls ? 14 : 8); i++) {
                float x = fract(hash1(i) + t * 0.02f * (1 + hash1(i + 1))) * 400 - 40;
                float y = floorf((falls ? 118 : 136) + hash1(i + 2) * 36 + sinf(t * 0.5f + i) * 2);
                float w = 18 + hash1(i + 3) * 26;
                rect(floorf(x), y, w, 1, CA(220, 235, 245, falls ? 60 : 40));
                rect(floorf(x + w * 0.2f), y - 1, w * 0.5f, 1, CA(220, 235, 245, falls ? 36 : 24));
            }
            if (falls) for (int i = 0; i < 40; i++) {
                float x = hash1(i) * LOW_W, y = fract(hash1(i + 1) + t * 0.9f) * LOW_H;
                DrawPixel((int)x, (int)y, CA(240, 250, 255, 140));
            }
            break;
        }
        case ARENA_SALAO:
            BeginBlendMode(BLEND_ADDITIVE);
            for (int i = 0; i < 18; i++) {
                float x = hash1(i) * LOW_W, y = fract(hash1(i + 1) + t * 0.03f) * LOW_H;
                float b = fmaxf(0, sinf(t * 3 + i));
                DrawPixel((int)x, (int)y, fade(C(255, 230, 160), b * 0.8f));
            }
            EndBlendMode();
            break;
        case ARENA_TREM:
            for (int i = 0; i < 16; i++) {
                float y = hash1(i) * LOW_H, x = fract(hash1(i + 1) - t * (2.5f + hash1(i + 2) * 2)) * 450 - 50;
                DrawLine((int)x, (int)y, (int)(x + 30 + hash1(i) * 40), (int)y, CA(200, 210, 255, 60));
            }
            break;
        case ARENA_BAMBUZAL:
            for (int i = 0; i < 10; i++) {
                float x = fract(hash1(i) + t * 0.04f) * 350 - 15, y = fract(hash1(i + 1) + t * 0.06f) * 190 - 5;
                DrawLine((int)x, (int)y, (int)(x + 2 * cosf(t * 2 + i)), (int)(y + 1), fade(C(80, 140, 80), 0.7f * (1 - c->blackout * 0.8f)));
            }
            if (c->blackout > 0) {
                /* A visão fecha em volta dos lutadores. */
                for (int r = 0; r < 5; r++)
                    DrawRing((Vector2){160, 110}, 95 + r * 22 - c->blackout * 45, 120 + r * 22, 0, 360, 48, fade(C(0, 0, 0), c->blackout * 0.35f));
            }
            break;
        case ARENA_FORJA:
            BeginBlendMode(BLEND_ADDITIVE);
            for (int i = 0; i < 36; i++) {
                float x = hash1(i) * LOW_W + sinf(t + i) * 5, y = LOW_H - fract(hash1(i + 1) + t * (0.15f + hash1(i + 2) * 0.2f)) * 190;
                DrawPixel((int)x, (int)y, fade(C(255, 140, 40), 0.9f));
            }
            EndBlendMode();
            break;
        case ARENA_TEMPLO:
            /* Folhas de bordo caindo, girando, por cima dos lutadores. */
            for (int i = 0; i < 18; i++) {
                float x = fract(hash1(i) + t * 0.015f + sinf(t * 0.7f + i) * 0.01f) * LOW_W;
                float y = fract(hash1(i + 1) + t * (0.05f + hash1(i + 2) * 0.04f)) * LOW_H;
                DrawPoly((Vector2){x, y}, 4, 1.4f, t * 90 + i * 40, hash1(i + 5) < 0.5f ? CA(200, 80, 44, 200) : CA(230, 140, 60, 200));
            }
            break;
        case ARENA_JARDIM:
            for (int i = 0; i < 12; i++) {
                float x = hash1(i) * LOW_W + sinf(t * 0.3f + i) * 15, y = fract(hash1(i + 1) + t * 0.02f) * LOW_H;
                DrawPoly((Vector2){x, y}, 3, 2, t * 60 + i * 20, CA(210, 235, 255, 90));
            }
            break;
        case ARENA_CIDADELA:
            for (int i = 0; i < (c->seal >= 1 ? 60 : 0); i++) {
                float x = fract(hash1(i) - t * 0.08f) * 350 - 15, y = fract(hash1(i + 3) + t * 1.8f) * 200 - 10;
                DrawLine((int)x, (int)y, (int)(x - 2), (int)(y + 6), CA(180, 170, 220, 90));
            }
            if (c->seal >= 2) {
                BeginBlendMode(BLEND_ADDITIVE);
                for (int i = 0; i < 24; i++) {
                    float x = hash1(i) * LOW_W, y = LOW_H - fract(hash1(i + 1) + t * 0.2f) * 190;
                    DrawPixel((int)x, (int)y, fade(C(255, 90, 40), 0.9f));
                }
                EndBlendMode();
            }
            break;
        default: break;
    }
}

Color arena_light(ArenaId id, const ArenaCtx *c) {
    Color base;
    switch (id) {
        case ARENA_DOJO: base = C(255, 236, 215); break;
        case ARENA_SERRA: base = C(255, 214, 186); break;
        case ARENA_CELEIRO: base = C(255, 215, 175); break;
        case ARENA_COBERTURA: base = C(200, 210, 240); break;
        case ARENA_PORTO: base = C(205, 205, 235); break;
        case ARENA_SALAO: base = C(255, 230, 200); break;
        case ARENA_TREM: base = C(200, 205, 235); break;
        case ARENA_CACHOEIRA: base = C(225, 240, 245); break;
        case ARENA_BAMBUZAL: base = C(190, 200, 225); break;
        case ARENA_FORJA: base = C(255, 200, 160); break;
        case ARENA_JARDIM: base = C(215, 225, 255); break;
        case ARENA_TEMPLO: base = C(236, 230, 240); break;
        case ARENA_CIDADELA: base = c->seal == 2 ? C(255, 180, 160) : C(220, 200, 235); break;
        default: base = WHITE; break;
    }
    if (c->lightning > 0) base = mix(base, WHITE, c->lightning);
    return mix(base, C(20, 20, 30), c->blackout * 0.72f);
}

/* Cor da luz de contorno nos lutadores, o neon de cada cenário. */
Color arena_rim(ArenaId id) {
    static const Color rim[ARENA_COUNT] = {
        {255, 190, 120, 255}, /* dojo */      {255, 170, 120, 255}, /* serra */
        {255, 150, 80, 255},  /* celeiro */   {90, 200, 255, 255},  /* cobertura */
        {140, 170, 255, 255}, /* porto */     {255, 210, 140, 255}, /* salão */
        {120, 255, 210, 255}, /* trem */      {190, 240, 255, 255}, /* cachoeira */
        {170, 255, 140, 255}, /* bambuzal */  {255, 120, 40, 255},  /* forja */
        {160, 210, 255, 255}, /* jardim */    {255, 90, 120, 255},  /* cidadela */
        {255, 236, 200, 255}, /* templo */
    };
    return (id >= 0 && id < ARENA_COUNT) ? rim[id] : WHITE;
}

float arena_reflection(ArenaId id) {
    switch (id) {
        case ARENA_SALAO: return 0.28f;
        case ARENA_COBERTURA: return 0.2f;
        case ARENA_JARDIM: return 0.32f;
        case ARENA_TREM: return 0.12f;
        default: return 0;
    }
}

const char *arena_name(ArenaId id) {
    static const char *names[ARENA_COUNT] = {
        "Dojo", "Serra", "Celeiro", "Cobertura", "Porto", "Salão", "Trem", "Cachoeira", "Bambuzal", "Forja", "Jardim", "Cidadela",
        "Templo"};
    return (id >= 0 && id < ARENA_COUNT) ? names[id] : "";
}
