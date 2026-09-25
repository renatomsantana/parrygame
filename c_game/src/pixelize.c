/*
 * pixelize.c - paleta curta por cena e dithering ordenado para os fundos.
 */
#include "pixelize.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

#define PAL_MAX 32            /* cores por cena */
#define KEYS 64

static RenderTexture2D bg;
static Shader quant;
static int locPal, locN;
static bool ready;
static float pals[KEYS][PAL_MAX * 3];
static int palN[KEYS];
static int current = -1;

/* Cada pixel vai para a cor da paleta mais perto. Só na faixa do meio entre dois
 * tons vizinhos (um degradê) o padrão de Bayer 4 x 4 mistura os dois em xadrez;
 * perto de uma cor, ou entre cores muito diferentes, fica chapado. Assim o
 * degradê vira faixas com xadrez de pixel nas bordas, como se faz à mão. */
static const char *QUANT_FS =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor; uniform sampler2D texture0;\n"
    "uniform vec3 pal[32]; uniform int n; out vec4 finalColor;\n"
    "const float B[16] = float[16](0.,8.,2.,10.,12.,4.,14.,6.,3.,11.,1.,9.,15.,7.,13.,5.);\n"
    "const vec3 W = vec3(0.55, 0.77, 0.33);\n"
    "void main() {\n"
    "    vec3 c = texture(texture0, fragTexCoord).rgb;\n"
    "    float d1 = 1e9, d2 = 1e9; vec3 c1 = c, c2 = c;\n"
    "    for (int i = 0; i < 32; i++) {\n"
    "        if (i >= n) break;\n"
    "        vec3 d = (c - pal[i]) * W; float e = dot(d, d);\n"
    "        if (e < d1) { d2 = d1; c2 = c1; d1 = e; c1 = pal[i]; } else if (e < d2) { d2 = e; c2 = pal[i]; }\n"
    "    }\n"
    "    float a = sqrt(d1), b = sqrt(d2);\n"
    "    float m = a / max(a + b, 1e-5);\n"
    "    float gap = length((c1 - c2) * W);\n"
    "    m = gap > 0.22 ? 0.0 : clamp((m - 0.3) / 0.2, 0.0, 1.0) * 0.5;\n"
    "    ivec2 p = ivec2(gl_FragCoord.xy) & 3;\n"
    "    float th = (B[p.y * 4 + p.x] + 0.5) / 16.0;\n"
    "    finalColor = vec4(m > th ? c2 : c1, 1.0);\n"
    "}\n";

void pix_init(int w, int h) {
    bg = LoadRenderTexture(w, h);
    SetTextureFilter(bg.texture, TEXTURE_FILTER_POINT);
    quant = LoadShaderFromMemory(NULL, QUANT_FS);
    locPal = GetShaderLocation(quant, "pal");
    locN = GetShaderLocation(quant, "n");
    ready = bg.id != 0 && quant.id != 0 && locPal >= 0;
}

void pix_shutdown(void) {
    if (bg.id) UnloadRenderTexture(bg);
    if (quant.id) UnloadShader(quant);
    ready = false;
}

void pix_capture_begin(void) {
    BeginTextureMode(bg);
    ClearBackground(BLACK);
}

/* ------------------------------------------------------------------ */
/* Paleta: corte mediano nas cores da cena e um refino de k-médias     */
/* ------------------------------------------------------------------ */

typedef struct { int lo, hi; } Box;   /* intervalo em `px` */
static unsigned char (*px)[3];
static int axis;

static int cmp_axis(const void *a, const void *b) {
    return (int)((const unsigned char *)a)[axis] - (int)((const unsigned char *)b)[axis];
}

static void box_range(const Box *b, int *ch, int *range) {
    int mn[3] = {255, 255, 255}, mx[3] = {0, 0, 0};
    for (int i = b->lo; i < b->hi; i++)
        for (int k = 0; k < 3; k++) {
            if (px[i][k] < mn[k]) mn[k] = px[i][k];
            if (px[i][k] > mx[k]) mx[k] = px[i][k];
        }
    static const float w[3] = {0.55f, 0.77f, 0.33f};
    *ch = 0;
    float best = -1;
    for (int k = 0; k < 3; k++)
        if ((mx[k] - mn[k]) * w[k] > best) { best = (mx[k] - mn[k]) * w[k]; *ch = k; }
    *range = (int)best;
}

static void make_palette(Image *im, float *out, int *n) {
    ImageFormat(im, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    int total = im->width * im->height;
    px = malloc(sizeof *px * (size_t)total);
    const unsigned char *d = im->data;
    for (int i = 0; i < total; i++) memcpy(px[i], d + i * 4, 3);
    Box boxes[PAL_MAX];
    int nb = 1;
    boxes[0] = (Box){0, total};
    while (nb < PAL_MAX) {
        int bi = -1, bestScore = 0, bch = 0;
        for (int i = 0; i < nb; i++) {
            if (boxes[i].hi - boxes[i].lo < 2) continue;
            int ch, range;
            box_range(&boxes[i], &ch, &range);
            /* divide a caixa mais espalhada, pesando um pouco o tamanho dela */
            int score = (int)(range * sqrtf((float)(boxes[i].hi - boxes[i].lo)));
            if (score > bestScore) { bestScore = score; bi = i; bch = ch; }
        }
        if (bi < 0 || bestScore <= 0) break;
        axis = bch;
        qsort(px + boxes[bi].lo, (size_t)(boxes[bi].hi - boxes[bi].lo), sizeof *px, cmp_axis);
        int mid = (boxes[bi].lo + boxes[bi].hi) / 2;
        boxes[nb++] = (Box){mid, boxes[bi].hi};
        boxes[bi].hi = mid;
    }
    float c[PAL_MAX][3];
    for (int i = 0; i < nb; i++) {
        double s[3] = {0, 0, 0};
        for (int j = boxes[i].lo; j < boxes[i].hi; j++)
            for (int k = 0; k < 3; k++) s[k] += px[j][k];
        int cnt = boxes[i].hi - boxes[i].lo;
        for (int k = 0; k < 3; k++) c[i][k] = (float)(s[k] / (cnt > 0 ? cnt : 1));
    }
    /* três passadas de k-médias para as cores assentarem */
    static const float w[3] = {0.55f, 0.77f, 0.33f};
    for (int it = 0; it < 3; it++) {
        double acc[PAL_MAX][3] = {{0}};
        int cnt[PAL_MAX] = {0};
        for (int j = 0; j < total; j++) {
            int best = 0;
            float be = 1e30f;
            for (int i = 0; i < nb; i++) {
                float e = 0;
                for (int k = 0; k < 3; k++) { float dd = (px[j][k] - c[i][k]) * w[k]; e += dd * dd; }
                if (e < be) { be = e; best = i; }
            }
            for (int k = 0; k < 3; k++) acc[best][k] += px[j][k];
            cnt[best]++;
        }
        for (int i = 0; i < nb; i++)
            if (cnt[i]) for (int k = 0; k < 3; k++) c[i][k] = (float)(acc[i][k] / cnt[i]);
    }
    for (int i = 0; i < nb; i++)
        for (int k = 0; k < 3; k++) out[i * 3 + k] = c[i][k] / 255.0f;
    *n = nb;
    free(px);
    px = NULL;
}

void pix_capture_end(int key) {
    EndTextureMode();
    if (!ready || key < 0 || key >= KEYS) { current = -1; return; }
    if (!palN[key]) {
        Image im = LoadImageFromTexture(bg.texture);
        make_palette(&im, pals[key], &palN[key]);
        UnloadImage(im);
    }
    current = key;
}

void pix_draw(void) {
    Rectangle src = {0, 0, (float)bg.texture.width, -(float)bg.texture.height};
    Rectangle dst = {0, 0, (float)bg.texture.width, (float)bg.texture.height};
    if (!ready || current < 0) {
        DrawTexturePro(bg.texture, src, dst, (Vector2){0, 0}, 0, WHITE);
        return;
    }
    BeginShaderMode(quant);
    SetShaderValueV(quant, locPal, pals[current], SHADER_UNIFORM_VEC3, palN[current]);
    SetShaderValue(quant, locN, &palN[current], SHADER_UNIFORM_INT);
    DrawTexturePro(bg.texture, src, dst, (Vector2){0, 0}, 0, WHITE);
    EndShaderMode();
}
