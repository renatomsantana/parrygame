/*
 * lore.c - ilustrações da abertura, do final e do mapa da trilha,
 * em pixel art procedural (320 x 180).
 */
#include "lore.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "arenas.h"
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
/* O personagem das pranchas, parado e calmo (EMBAINHADO, IDLE, PARADO, o fim da
 * corrida com a lâmina baixa ou a guarda), com o contorno escuro do duelo. Sem as
 * pranchas, devolve false e a ilustração usa a silhueta. */
static bool sprite_person_pose(const char *id, const char *pose, float x, float feet, bool faceLeft, float t, Color tint,
                               Color rim) {
    const SprSet *s = spr_get(id);
    if (!s) return false;
    /* a pose pedida (o Hanzo sentado); fora da luta, a katana de Kojiro fica na bainha */
    const SprAnim *a = pose ? spr_anim(s, pose) : NULL;
    if (!a) a = spr_anim(s, "EMBAINHADO");
    if (!a) a = spr_anim(s, "IDLE");
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

static bool sprite_person_lit(const char *id, float x, float feet, bool faceLeft, float t, Color tint, Color rim) {
    return sprite_person_pose(id, NULL, x, feet, faceLeft, t, tint, rim);
}

static bool sprite_person(const char *id, float x, float feet, bool faceLeft, float t) {
    return sprite_person_lit(id, x, feet, faceLeft, t, WHITE, CA(0, 0, 0, 0));
}

/* Um pinheiro escuro: camadas de triângulo, a de baixo mais larga. */
static void pine(float x, float base, float h, Color c) {
    rect(x - 1, base - h * 0.25f, 2, h * 0.25f, C(30, 22, 24));
    for (int k = 0; k < 4; k++) {
        float top = base - h + k * h * 0.18f, w = 3 + k * h * 0.09f;
        DrawTriangle((Vector2){x - w, top + h * 0.3f}, (Vector2){x + w, top + h * 0.3f}, (Vector2){x, top}, c);
    }
}

/* Prova da rodada 7 (CABANA_PROVA=7): a mesma cabana pobre, acabada, simplificada no estilo
 * Katana Zero: poucas formas grandes e legíveis, a silhueta escura contra a noite, o luar frio só
 * nas bordas de cima e a luz quente de dentro escapando pelas frestas, pela porta e pelas falhas
 * da palha. Sem base de pedra. Ficam a palha gasta com falhas, as tábuas desiguais, o shoji
 * rasgado, a lenha, o barril e o makiwara, cada um numa forma só. */
static void cabin_poor7(float t) {
    float fl = 0.85f + 0.15f * sinf(t * 7) * sinf(t * 3.1f);
    Color wall = C(40, 29, 36), wall2 = C(48, 35, 42), joint = C(24, 17, 24), moon = C(96, 98, 132);
    Color warm = CA(255, 150, 70, (unsigned char)(210 * fl)), warmHi = CA(255, 204, 130, (unsigned char)(230 * fl));
    /* as paredes: tábuas largas de dois tons, as juntas escuras, o topo um pouco desencontrado */
    static const int pw[] = {7, 5, 8, 4, 7, 6, 9, 5, 7, 8}, pt[] = {0, 2, 1, 0, 3, 1, 0, 2, 1, 0};
    int x = 26;
    for (int i = 0; i < 10 && x < 94; i++) {
        int w = pw[i] > 94 - x ? 94 - x : pw[i], top = 102 + pt[i];
        rect(x, top, w, 138 - top, i % 2 ? wall : wall2);
        rect(x, top, 1, 138 - top, joint);
        if (i == 6) rect(x + 2, 128, 3, 10, C(30, 21, 28));                     /* um pedaço de tábua que falta embaixo */
        x += w;
    }
    /* três frestas: a luz de dentro em riscos compridos, uma delas mais aberta */
    static const float cr[3][3] = {{33, 108, 22}, {64, 106, 26}, {86, 110, 18}};
    for (int k = 0; k < 3; k++) {
        rect(cr[k][0], cr[k][1], 1, cr[k][2], warm);
        rect(cr[k][0], cr[k][1] + cr[k][2] * 0.35f, 1, cr[k][2] * 0.3f, warmHi);
    }
    rect(65, 112, 1, 9, warm);
    /* a porta de shoji acesa: o papel quente, a grade grossa, um painel rasgado (buraco escuro
       com a ponta de papel caída) e um remendo de papel mais claro */
    int dx = 42, dy = 111, dw = 16, dh = 27;
    rect(dx - 2, dy - 2, dw + 4, dh + 2, C(22, 15, 20));
    rect(dx, dy, dw, dh, CA(240, 168, 92, (unsigned char)(215 + 40 * fl)));
    rect(dx, dy, dw, 3, CA(255, 206, 140, (unsigned char)(220 * fl)));
    rect(dx + 9, dy + 12, 6, 6, C(214, 190, 150));                            /* o remendo */
    rect(dx + 2, dy + 5, 5, 6, C(26, 16, 20));                                /* o rasgo */
    rect(dx + 3, dy + 11, 2, 2, CA(240, 168, 92, 230));                       /* a ponta pendurada */
    rect(dx + 7, dy, 2, dh, C(34, 22, 24));                                   /* a grade */
    rect(dx, dy + 9, dw, 2, C(34, 22, 24));
    rect(dx, dy + 18, dw, 2, C(34, 22, 24));
    /* a janela de ripas: três fendas de luz */
    rect(72, 113, 9, 8, C(20, 14, 20));
    for (int k = 0; k < 3; k++) rect(73 + k * 3, 114, 1, 6, warm);
    /* o teto de palha: um trapézio grande e escuro, a cumeeira cedendo no meio, a palha em
       fios que descem a água do telhado (riscos verticais de três tons próximos), o luar só na
       borda de cima, o corte grosso do beiral com a sombra embaixo e a franja rala em tufos */
    for (int xx = 14; xx < 106; xx++) {
        float u = (xx - 14) / 92.0f;
        float ridge = 84 + 1.8f * sinf(u * 3.1416f);
        float top = xx < 36 ? ridge + (36 - xx) * 0.82f : xx > 84 ? ridge + (xx - 84) * 0.9f : ridge;
        float hr = hash1(xx * 0.91f);
        top += hr > 0.86f ? -1 : hr < 0.12f ? 1 : 0;                           /* a palha rala: a borda de cima irregular */
        float eave = 103 + (xx > 72 && xx < 82 ? 1.5f : 0) + (hash1(xx * 0.37f) > 0.7f ? 1 : 0) + (xx > 94 ? (xx - 94) * 0.25f : 0);
        for (int yy = (int)top; yy < (int)eave; yy++) {
            float h = hash1(xx * 3.7f + (yy / 5) * 1.3f);
            Color c = h > 0.72f ? C(74, 58, 50) : h > 0.3f ? C(64, 49, 45) : C(55, 41, 41);
            DrawPixel(xx, yy, c);
        }
        DrawPixel(xx, (int)top, moon);                                          /* o luar na borda */
        rect(xx, (int)eave, 1, 2, C(80, 62, 50));                               /* o corte do beiral */
        DrawPixel(xx, (int)eave + 2, C(30, 22, 26));                            /* a sombra embaixo */
        if (hash1(xx * 1.9f) > 0.6f) DrawPixel(xx, (int)eave + 3, C(66, 50, 44));   /* a franja rala */
    }
    /* duas falhas na palha: buracos tortos, a luz de dentro só na beira de baixo */
    static const int hole[2][6] = {{27, 93, 6, 3, 2, 1}, {73, 89, 5, 2, 1, 2}};
    for (int k = 0; k < 2; k++) {
        const int *h = hole[k];
        rect(h[0], h[1], h[2], h[3], C(20, 12, 16));
        rect(h[0] + h[4], h[1] - 1, h[2] - 3, 1, C(20, 12, 16));
        rect(h[0] + h[5], h[1] + h[3], h[2] - 2, 1, C(20, 12, 16));
        rect(h[0] + h[5], h[1] + h[3], h[2] - 2, 1, fade(C(255, 150, 70), 0.6f * fl));
    }
    static const int patch[6][2] = {{1, 6}, {0, 9}, {0, 10}, {1, 9}, {0, 8}, {2, 5}};   /* o remendo de palha nova, torto */
    for (int k = 0; k < 6; k++) rect(56 + patch[k][0], 88 + k, patch[k][1], 1, k == 0 ? C(120, 104, 78) : k % 2 ? C(92, 76, 56) : C(84, 68, 52));
    rect(58, 91, 1, 4, C(60, 46, 40));                                          /* a amarra */
    for (int k = 0; k < 4; k++) rect(97 + k * 2, 104 + (k % 2), 1, 3 + k % 2, C(58, 44, 42));   /* palha pendurada no beiral caído */
    /* a fumaça do respiro */
    for (int i = 0; i < 5; i++) {
        float k = fract(t * 0.12f + i / 5.0f);
        DrawCircle((int)(92 + sinf(k * 6 + i) * 3 + k * 10), (int)(84 - k * 46), 2 + k * 3, CA(110, 102, 126, (unsigned char)(70 * (1 - k))));
    }
    /* a luz da porta no chão */
    glow(50, 134, 12 * fl, C(255, 140, 60));
    DrawTriangle((Vector2){42, 138}, (Vector2){30, 144}, (Vector2){70, 144}, CA(255, 150, 70, (unsigned char)(50 * fl)));
    DrawTriangle((Vector2){42, 138}, (Vector2){70, 144}, (Vector2){58, 138}, CA(255, 150, 70, (unsigned char)(50 * fl)));
    /* a lenha: uma pilha em degraus, a ponta das toras em círculos, o luar em cima */
    for (int r = 0; r < 3; r++)
        for (int k = 0; k < 4 - r; k++) {
            float lx = 7 + k * 5 + r * 2.5f, ly = 135 - r * 4;
            DrawCircle((int)lx, (int)ly, 2, C(34, 24, 26));
            DrawPixel((int)lx, (int)ly, C(92, 70, 56));
            DrawPixel((int)lx - 1, (int)ly - 2, moon);
        }
    /* o barril: um bloco escuro, dois aros e o luar na tampa */
    rect(98, 126, 11, 12, C(36, 26, 30));
    rect(97, 128, 13, 8, C(36, 26, 30));                                        /* o bojo */
    rect(97, 129, 13, 1, C(58, 50, 56));
    rect(97, 134, 13, 1, C(58, 50, 56));
    rect(98, 126, 11, 1, moon);
    rect(97, 128, 1, 8, C(26, 19, 24));
    /* o makiwara: o poste escuro meio torto e a palha amarrada no alto, com a borda do luar */
    for (int k = 0; k < 28; k++) rect(121 + (k > 16 ? 0 : (16 - k) / 10), 138 - k, 3, 1, C(40, 30, 32));
    rect(119, 110, 6, 9, C(70, 58, 48));
    rect(119, 110, 6, 1, moon);
    rect(119, 113, 6, 1, C(40, 30, 32));
    rect(119, 116, 6, 1, C(40, 30, 32));
}

/* Prova da rodada 6 (CABANA_PROVA): a cabana pobre de Hanzo, na serra, meio acabada.
 * Teto de palha gasto, com falhas, remendos e meio torto; paredes de tábuas desiguais
 * com frestas por onde passa a luz fraca e quente de dentro; a porta de shoji rasgada e
 * remendada; o canto direito ainda só na armação, com tábuas soltas encostadas; base de
 * pedra, a lenha empilhada, o barril de água e o makiwara na frente. Tudo com ruído
 * fixo (hash), sem sorteio a cada quadro; só a luz tremula. */
static void cabin_poor(float t) {
    float fl = 0.8f + 0.2f * sinf(t * 7) * sinf(t * 3.1f);
    Color warm = CA(255, 150, 70, (unsigned char)(150 * fl)), warm2 = CA(255, 190, 110, (unsigned char)(200 * fl));
    /* o interior escuro atrás de tudo (aparece nas frestas e no canto sem tábua) */
    rect(27, 100, 66, 32, C(30, 20, 22));
    /* paredes: tábuas verticais de larguras e tons diferentes, topo desencontrado */
    static const Color plank[4] = {{74, 50, 38, 255}, {62, 42, 32, 255}, {86, 60, 44, 255}, {56, 40, 34, 255}};
    int x = 26;
    for (int i = 0; x < 80; i++) {
        int w = 3 + (int)(hash1(i * 1.7f) * 3), top = 102 + (int)(hash1(i * 2.3f + 1) * 3);
        Color c = plank[(int)(hash1(i * 3.1f) * 4) % 4];
        rect(x, top, w, 131 - top, c);
        rect(x, top, 1, 131 - top, fade(C(24, 16, 16), 0.5f));                  /* a junta escura */
        if (hash1(i + 9.3f) > 0.45f) rect(x + w - 1, top + 2, 1, 131 - top - 4, C(110, 84, 62));   /* luar na borda */
        for (int k = 0; k < 2; k++) {                                              /* nós e rachas da madeira */
            float ky = top + 4 + hash1(i * 5.0f + k) * (131 - top - 8);
            rect(x + 1 + (int)(hash1(i + k * 7.0f) * (w - 1)), ky, 1, 2 + k, C(44, 30, 26));
        }
        /* fresta: entre algumas tábuas fica 1 px aberto, a luz de dentro passa */
        if (hash1(i * 4.7f + 2) > 0.55f) {
            float y0 = top + 3 + hash1(i * 6.1f) * 6, y1 = 131 - hash1(i * 7.3f) * 8;
            rect(x + w, y0, 1, y1 - y0, warm);
            rect(x + w, y0 + (y1 - y0) * 0.4f, 1, (y1 - y0) * 0.2f, warm2);
            x += 1;
        }
        x += w;
    }
    /* o canto meio acabado: armação (esteios, a travessa e a mão-francesa), duas tábuas
       pregadas até a metade e a luz de dentro no vão */
    rect(80, 104, 13, 27, C(26, 18, 20));
    glow(86, 118, 10 * fl, C(255, 140, 60));
    rect(80, 102, 2, 29, C(112, 84, 60));
    rect(91, 104, 2, 27, C(104, 78, 56));
    rect(80, 116, 13, 1, C(104, 78, 56));
    for (int k = 0; k < 11; k++) rect(81 + k, 129 - k * 1.2f, 1, 2, C(98, 72, 52));   /* a mão-francesa */
    rect(83, 119, 3, 12, C(70, 48, 36));
    rect(86, 122, 3, 9, C(80, 56, 40));
    rect(83, 119, 1, 1, C(150, 150, 150));   /* os pregos */
    rect(87, 123, 1, 1, C(150, 150, 150));
    /* tábuas soltas encostadas do lado de fora, esperando */
    for (int k = 0; k < 3; k++)
        for (int j = 0; j < 22 - k * 4; j++) rect(95 + k * 2 + j * 0.25f, 131 - j, 2, 1, plank[(k + 2) % 4]);
    /* viga de cima e soleira */
    rect(25, 101, 69, 2, C(52, 36, 30));
    rect(25, 130, 69, 2, C(46, 32, 28));
    /* a porta de shoji: a grade de madeira, o papel aceso por dentro, um painel rasgado
       e outro remendado com papel de outra cor */
    int dx = 42, dy = 110, dw = 15, dh = 20;
    rect(dx - 1, dy - 1, dw + 2, dh + 2, C(44, 30, 24));
    rect(dx, dy, dw, dh, CA(236, 176, 104, (unsigned char)(200 + 40 * fl)));
    rect(dx + 9, dy + 5, 5, 5, C(208, 186, 146));                                /* o remendo */
    rect(dx + 9, dy + 5, 5, 1, C(186, 160, 120));
    rect(dx + 2, dy + 11, 4, 4, C(40, 24, 22));                                  /* o rasgo */
    rect(dx + 3, dy + 10, 2, 1, C(40, 24, 22));
    rect(dx + 1, dy + 13, 1, 1, C(40, 24, 22));
    rect(dx + 5, dy + 15, 1, 1, CA(236, 176, 104, 220));                         /* a ponta de papel pendurada */
    for (int k = 1; k < 3; k++) rect(dx + k * 5, dy, 1, dh, C(58, 40, 30));      /* a grade */
    for (int k = 1; k < 4; k++) rect(dx, dy + k * 5, dw, 1, C(58, 40, 30));
    /* janelinha de ripas: a luz entre elas */
    rect(66, 111, 9, 7, C(40, 26, 24));
    for (int k = 0; k < 4; k++) rect(67 + k * 2, 112, 1, 5, warm);
    rect(66, 118, 9, 1, C(78, 56, 40));
    /* base de pedras desiguais, com o luar no alto de cada uma */
    for (int i = 0; i < 18; i++) {
        float sx = 22 + i * 4.3f + hash1(i * 1.3f) * 2, sw = 4 + hash1(i * 2.9f) * 3, sh = 3 + hash1(i * 3.7f) * 4;
        Color c = hash1(i * 5.1f) > 0.5f ? C(78, 74, 82) : C(96, 92, 100);
        rect(sx, 138 - sh, sw, sh, C(36, 32, 40));
        rect(sx + 0.5f, 138 - sh + 0.5f, sw - 1, sh - 1, c);
        rect(sx + 1, 138 - sh + 0.5f, sw - 2, 1, C(128, 124, 136));
        if (hash1(i * 8.1f) > 0.6f) rect(sx + 1, 138 - sh, 2, 1, C(56, 72, 52));   /* musgo */
    }
    /* o teto de palha visto de lado: trapézio grosso, a cumeeira meio torta e afundada no
       meio, os beirais desencontrados, a palha em fios de três tons, falhas escuras com o
       caibro aparecendo, um remendo de palha nova e a franja solta nos beirais */
    for (int xx = 12; xx < 108; xx++) {
        float u = (xx - 12) / 96.0f;
        float ridge = 86 - u * 4 + 1.6f * sinf(u * 3.1416f), eave = 106 - u * 4 + (xx > 70 && xx < 78 ? 2 : 0);   /* torto: o lado direito mais alto, a cumeeira cedendo no meio e um beiral caído */
        float top = xx < 34 ? ridge + (34 - xx) * 0.75f : xx > 88 ? ridge + (xx - 88) * 0.8f : ridge;
        for (int yy = (int)top; yy < (int)eave; yy++) {
            float h = hash1(xx * 7.1f + (yy / 3) * 1.9f);
            Color c = h > 0.66f ? C(128, 104, 62) : h > 0.3f ? C(104, 84, 52) : C(84, 66, 42);
            if (yy - (int)top < 2) c = C(66, 52, 38);                                 /* a cumeeira */
            DrawPixel(xx, yy, c);
        }
        int fr = (int)(hash1(xx * 3.3f) * 3);                                          /* a franja */
        for (int k = 0; k < fr; k++) DrawPixel(xx, (int)eave + k, C(98, 78, 48));
        if (hash1(xx * 1.1f) > 0.8f) DrawPixel(xx, (int)top + 3, C(150, 136, 110));    /* luar na palha */
    }
    for (int k = 0; k < 8; k++) rect(38 + k * 6, 84 + (k > 3 ? 0 : 1), 1, 3, C(120, 96, 60));   /* amarras da cumeeira */
    /* falhas: buracos escuros, um com o caibro e o céu passando */
    static const float holes[5][4] = {{21, 95, 7, 4}, {57, 89, 5, 3}, {86, 95, 4, 4}, {44, 99, 3, 2}, {33, 90, 2, 2}};
    for (int k = 0; k < 5; k++) {
        float hx = holes[k][0], hy = holes[k][1], hw = holes[k][2], hh = holes[k][3];
        rect(hx, hy, hw, hh, C(24, 16, 18));
        rect(hx + 1, hy - 1, hw - 2, 1, C(24, 16, 18));                       /* a borda rota */
        rect(hx - 1, hy + 1, 1, hh - 2 > 0 ? hh - 2 : 1, C(24, 16, 18));
        rect(hx + 1, hy + hh - 1, hw - 2, 1, fade(C(255, 150, 70), 0.55f * fl));   /* a luz de dentro */
    }
    DrawLine(20, 96, 28, 99, C(96, 70, 48));   /* o caibro aparecendo nas falhas */
    DrawLine(86, 95, 90, 99, C(96, 70, 48));
    /* o remendo: um feixe de palha nova, mais clara, amarrado torto por cima */
    for (int k = 0; k < 11; k++) rect(64 + k, 89 + (k % 3 == 0) + k / 5, 1, 8 - (k % 2), k % 2 ? C(170, 150, 90) : C(150, 132, 78));
    rect(64, 93, 11, 1, C(90, 70, 44));
    rect(64, 94, 1, 1, C(90, 70, 44));
    /* fumaça saindo do respiro na ponta da cumeeira */
    for (int i = 0; i < 6; i++) {
        float k = fract(t * 0.12f + i / 6.0f);
        DrawCircle((int)(90 + sinf(k * 6 + i) * 3 + k * 10), (int)(84 - k * 50), 2 + k * 3, CA(120, 110, 130, (unsigned char)(80 * (1 - k))));
    }
    /* a luz fraca da porta no chão */
    glow(49, 131, 8 * fl, C(255, 140, 60));
    DrawTriangle((Vector2){42, 138}, (Vector2){34, 142}, (Vector2){64, 142}, CA(255, 150, 70, (unsigned char)(40 * fl)));
    DrawTriangle((Vector2){42, 138}, (Vector2){64, 142}, (Vector2){57, 138}, CA(255, 150, 70, (unsigned char)(40 * fl)));
    /* a lenha empilhada contra a parede da esquerda: toras com a casca e os anéis */
    for (int r = 0; r < 3; r++)
        for (int k = 0; k < 5 - r; k++) {
            int lx = 6 + k * 4 + r * 2 + (int)(hash1(r * 3.0f + k) * 2), ly = 135 - r * 3;
            Color bark = C(58, 40, 30), wood = hash1(r * 5.0f + k) > 0.5f ? C(168, 128, 84) : C(146, 108, 70);
            rect(lx - 1, ly - 1, 3, 1, bark);                   /* a ponta da tora, redonda */
            rect(lx - 2, ly, 5, 1, bark);
            rect(lx - 1, ly, 3, 1, wood);
            rect(lx - 1, ly + 1, 3, 1, bark);
            if (hash1(k * 9.0f + r) > 0.4f) DrawPixel(lx, ly, C(120, 86, 56));   /* o anel */
        }
    rect(4, 137, 22, 1, C(30, 24, 26));
    /* o barril de água com os aros de bambu e a concha */
    rect(99, 126, 10, 12, C(86, 60, 42));
    rect(98, 128, 12, 8, C(92, 64, 44));
    for (int k = 0; k < 3; k++) rect(99 + k * 3, 126, 1, 12, C(70, 48, 34));
    rect(98, 128, 12, 1, C(70, 86, 58));
    rect(98, 134, 12, 1, C(70, 86, 58));
    rect(99, 125, 10, 1, C(40, 30, 28));
    DrawLine(106, 125, 111, 118, C(150, 120, 80));
    rect(110, 117, 2, 2, C(150, 120, 80));
    /* o makiwara na frente: o poste de madeira meio inclinado, a palha amarrada no alto */
    for (int k = 0; k < 30; k++) rect(120 + (k > 18 ? 0 : (18 - k) / 12), 138 - k, 3, 1, k % 7 == 3 ? C(96, 70, 46) : C(118, 88, 58));
    for (int k = 0; k < 9; k++) {
        rect(119 + (k < 5), 110 + k, 5, 1, k % 2 ? C(176, 150, 96) : C(150, 124, 78));
        if (k % 3 == 1) rect(119, 110 + k, 5, 1, C(96, 74, 48));   /* a corda */
    }
    rect(118, 137, 7, 2, C(70, 66, 74));
}

/* A cabana de Hanzo na serra, à noite: ele de um lado da fogueira, Kojiro do outro. */
void lore_draw_cabin(float t) {
    sky(C(14, 12, 30), C(66, 48, 76));
    stars(50, t);
    glow(262, 30, 28, C(190, 190, 230));
    DrawCircle(262, 30, 9, C(236, 232, 214));
    DrawCircle(259, 28, 2, C(214, 208, 190));
    DrawCircle(265, 33, 1, C(214, 208, 190));
    mountain(70, 60, 180, C(44, 36, 66));
    mountain(250, 72, 180, C(34, 28, 54));
    /* o pinheiral atrás: uma fileira longe só na silhueta, e os de perto em camadas de
       agulha, com o luar na beira de cima de cada camada */
    for (int i = 0; i < 9; i++) pine(196 + i * 15 + (i % 2) * 4, 136, 22 + (i * 5 % 9), C(30, 30, 48));
    for (int i = 0; i < 3; i++) pine(108 + i * 14, 136, 20 + i * 3, C(30, 30, 48));
    static const Color pineTone[4] = {{12, 14, 22, 255}, {22, 28, 38, 255}, {40, 52, 64, 255}, {86, 98, 122, 255}};
    arena_pine(226, 140, 44, pineTone, 1);
    arena_pine(252, 140, 34, pineTone, 2);
    arena_pine(290, 140, 50, pineTone, 3);
    arena_pine(314, 140, 38, pineTone, 4);
    arena_pine(120, 140, 30, pineTone, 5);
    rect(0, 138, 320, 42, C(40, 32, 38));
    rect(0, 138, 320, 2, C(62, 50, 54));
    /* a cabana: tábuas, teto de palha, a porta acesa e uma janela */
    float fl = 0.9f + 0.1f * sinf(t * 7) * sinf(t * 3.1f);
    if (getenv("CABANA_PROVA") && !strcmp(getenv("CABANA_PROVA"), "7")) cabin_poor7(t);
    else if (getenv("CABANA_PROVA")) cabin_poor(t);
    else {
    rect(26, 104, 68, 36, C(74, 50, 38));
    for (int k = 0; k < 6; k++) rect(26, 104 + k * 6, 68, 1, C(58, 38, 30));
    DrawTriangle((Vector2){14, 106}, (Vector2){106, 106}, (Vector2){60, 80}, C(96, 76, 50));
    rect(14, 104, 92, 3, C(70, 54, 36));
    for (int k = 0; k < 9; k++) rect(20 + k * 10, 106, 1, 2, C(70, 54, 36));
    glow(54, 128, 24 * fl, C(255, 150, 70));
    rect(48, 118, 12, 22, C(255, 170, 90));
    rect(74, 112, 8, 7, C(255, 190, 110));
    rect(77, 112, 1, 7, C(74, 50, 38));
    rect(74, 115, 8, 1, C(74, 50, 38));
    /* fumaça subindo do teto */
    for (int i = 0; i < 6; i++) {
        float k = fract(t * 0.12f + i / 6.0f);
        DrawCircle((int)(80 + sinf(k * 6 + i) * 3 + k * 10), (int)(84 - k * 50), 2 + k * 3, CA(120, 110, 130, (unsigned char)(90 * (1 - k))));
    }
    }
    /* a fogueira entre os dois: a luz no chão, a roda de pedras, a lenha cruzada com a
       brasa, três línguas de fogo tremendo, a fumaça subindo e as faíscas */
    float fx = 173, fy = 140;
    glow(fx, fy - 6, 40 * fl, C(255, 130, 50));
    DrawEllipse((int)fx, (int)fy + 1, 26 * fl, 3, CA(255, 150, 70, 60));
    for (int k = 0; k < 9; k++) {
        float a = k / 9.0f * 6.2832f, px = fx + cosf(a) * 9, py = fy + sinf(a) * 1.6f;
        DrawEllipse((int)px, (int)py, 2, 1.3f, k % 2 ? C(96, 88, 92) : C(70, 64, 70));
        rect(floorf(px) - 1, floorf(py) - 1, 2, 1, sinf(a) > 0 ? C(150, 110, 90) : C(120, 118, 124));
    }
    for (int k = -1; k <= 1; k += 2) {   /* a lenha cruzada */
        for (int j = 0; j < 12; j++) {
            float u = j / 11.0f, lx = fx - 6 * k + 12 * k * u, ly = fy - 1 - 3 * u;
            rect(floorf(lx), floorf(ly), 2, 2, j < 2 || j > 9 ? C(92, 60, 40) : C(64, 40, 28));
            if (j > 3 && j < 9 && (j + k) % 3 == 0) rect(floorf(lx), floorf(ly) + 1, 1, 1, C(255, 120, 40));   /* a brasa */
        }
    }
    arena_flame(fx - 4, fy - 2, 5, 9, t, 1);
    arena_flame(fx + 4, fy - 2, 5, 8, t, 2);
    arena_flame(fx, fy - 2, 9, 17, t, 3);
    for (int i = 0; i < 5; i++) {   /* a fumaça, torta pelo vento da serra */
        float k = fract(t * 0.15f + i / 5.0f);
        DrawCircle((int)(fx + sinf(k * 5 + i) * 3 + k * 16), (int)(fy - 18 - k * 44), 1.5f + k * 4, CA(90, 84, 100, (unsigned char)(80 * (1 - k))));
    }
    for (int i = 0; i < 10; i++) {
        float k = fract(t * 0.6f + hash1(i));
        DrawPixel((int)(fx + sinf(i * 3 + t * 2) * (3 + k * 6)), (int)(fy - 8 - k * 30), CA(255, 200, 120, (unsigned char)(255 * (1 - k))));
    }
    /* Hanzo sentado em seiza, as mãos nas coxas, olhando o fogo */
    if (!sprite_person_pose("hanzo", "SENTADO", 142, 142, false, t, WHITE, CA(0, 0, 0, 0)))
        person(140, 142, 40, C(200, 200, 204), false);
    if (!sprite_person("kojiro", 206, 142, true, t + 0.7f)) person(206, 142, 36, C(220, 100, 40), true);
}

void lore_trail_point(int index, float *x, float *y) {
    /* Ziguezague subindo a serra até a cidadela. */
    float k = index / (float)(ROSTER_SIZE - 1);
    *x = 40 + (index % 2 ? 1 : -1) * 18 + k * 200 + sinf(index * 1.3f) * 10;
    *y = 108 - k * 86;          /* a primeira parada fica acima do painel da trilha */
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
