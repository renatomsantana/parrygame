/*
 * sprites.c - lê as pranchas geradas e desenha os lutadores em pixel art.
 */
#include "sprites.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SETS 20

static SprSet sets[MAX_SETS];
static bool tried[MAX_SETS];
static int nsets;
static Shader flat;
static bool flatOk;
static Shader slashColour;
static int slashColourMode;
static bool slashColourOk;

#define MAX_FX 24
static SprFx fxs[MAX_FX];     /* efeitos em folha, carregados na primeira vez */
static bool fxTried[MAX_FX];
static int nfx;
static Texture2D keysTex[2];
static bool uiTried, uiOk;

/* Silhueta: a cor da tinta com o alfa da prancha. */
static const char *FLAT_FS =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor; uniform sampler2D texture0; out vec4 finalColor;\n"
    "void main() { finalColor = vec4(fragColor.rgb, texture(texture0, fragTexCoord).a * fragColor.a); }\n";

void spr_init(void) {
    flat = LoadShaderFromMemory(NULL, FLAT_FS);
    flatOk = flat.id != 0;
    /* Preserva os brancos do pack; converte somente a paleta quente dos efeitos. */
    static const char *slashFS =
        "#version 330\n"
        "in vec2 fragTexCoord; in vec4 fragColor; uniform sampler2D texture0;\n"
        "uniform int lightning; out vec4 finalColor;\n"
        "void main() { vec4 c=texture(texture0,fragTexCoord); float w=max(c.r,c.g);\n"
        "vec3 rgb=lightning==1 ? vec3(c.b,.65*w+.35*c.b,w) : vec3(w,.48*w+.52*c.b,.06*w+.94*c.b);\n"
        "finalColor=vec4(rgb,c.a)*fragColor; }\n";
    slashColour = LoadShaderFromMemory(NULL, slashFS);
    slashColourOk = slashColour.id != 0;
    slashColourMode = GetShaderLocation(slashColour, "lightning");
}

static void solta_laminas(void);

void spr_shutdown(void) {
    solta_laminas();
    for (int i = 0; i < nsets; i++)
        for (int k = 0; k < sets[i].count; k++) {
            UnloadTexture(sets[i].anims[k].tex);
            if (sets[i].anims[k].cleanTex.id) UnloadTexture(sets[i].anims[k].cleanTex);
            if (sets[i].anims[k].weaponTex.id) UnloadTexture(sets[i].anims[k].weaponTex);
            if (sets[i].anims[k].steelTex.id) UnloadTexture(sets[i].anims[k].steelTex);
        }
    nsets = 0;
    for (int i = 0; i < nfx; i++)
        if (fxs[i].tex.id) UnloadTexture(fxs[i].tex);
    nfx = 0;
    if (uiOk) { UnloadTexture(keysTex[0]); UnloadTexture(keysTex[1]); }
    uiOk = false;
    if (flatOk) UnloadShader(flat);
    if (slashColourOk) UnloadShader(slashColour);
    flatOk = slashColourOk = false;
}

/* Altura do corpo no primeiro quadro: da âncora dos pés até o pixel mais alto. */
static int body_height(const Image *im, int cw, int ax, int ay) {
    const Color *px = (const Color *)im->data;
    for (int y = 0; y < im->height && y < ay; y++)
        for (int x = ax - cw / 3; x < ax + cw / 3; x++)
            if (x >= 0 && x < cw && x < im->width && px[y * im->width + x].a > 0) return ay - y;
    return 40;
}

/* O meio do corpo, da cintura para baixo (as pernas e a roupa): o que anda quando o lutador avança, sem o balanço da arma. */
static void body_centers(SprAnim *a, const Image *im, const SprSet *s) {
    const Color *px = (const Color *)im->data;
    const int top = s->ay - 28 > 0 ? s->ay - 28 : 0;
    for (int f = 0; f < a->frames && f < SPR_MAX_FRAMES; f++) {
        long sum = 0, n = 0;
        for (int y = top; y < im->height && y < s->ch; y++)
            for (int x = f * s->cw; x < (f + 1) * s->cw && x < im->width; x++)
                if (px[y * im->width + x].a > 128) { sum += x - f * s->cw; n++; }
        if (n > 0) { a->body[f] = (float)sum / (float)n - (float)s->ax; a->hasBody[f] = true; }
    }
}

float spr_salto_do_corpo(const SprAnim *a, int de, int para) {
    if (!a || de < 0 || para < 0 || de >= SPR_MAX_FRAMES || para >= SPR_MAX_FRAMES || !a->hasBody[de] || !a->hasBody[para]) return 0;
    return a->body[para] - a->body[de];
}

static void parse_anim(SprSet *s, char *line, const char *dir) {
    if (s->count >= SPR_MAX_ANIMS) return;
    SprAnim a = {0};
    a.hold = a.contact = a.stop = -1;
    char *tok = strtok(line, " \t\r\n");   /* "anim" */
    tok = strtok(NULL, " \t\r\n");
    if (!tok) return;
    snprintf(a.name, sizeof a.name, "%s", tok);
    int ms = 0;
    tok = strtok(NULL, " \t\r\n");
    while (tok) {
        /* `tempos 120 150 40`: a duração de cada quadro, em ms (o preparo devagar, o golpe rápido) */
        if (!strcmp(tok, "tempos")) {
            while ((tok = strtok(NULL, " \t\r\n")) && tok[0] >= '0' && tok[0] <= '9')
                if (a.ntimes < 16) a.times[a.ntimes++] = atoi(tok) / 1000.0f;
            continue;
        }
        if (!strcmp(tok, "loop")) a.loop = true;
        else if (!strcmp(tok, "hold")) { tok = strtok(NULL, " \t\r\n"); if (tok) a.hold = atoi(tok); }
        else if (!strcmp(tok, "contact")) { tok = strtok(NULL, " \t\r\n"); if (tok) a.contact = atoi(tok); }
        else if (!strcmp(tok, "stop")) { tok = strtok(NULL, " \t\r\n"); if (tok) a.stop = atoi(tok); }
        else if (!strcmp(tok, "ms")) { tok = strtok(NULL, " \t\r\n"); if (tok) ms = atoi(tok); }
        else if (!strcmp(tok, "alcance")) {
            char *x = strtok(NULL, " \t\r\n"), *y = x ? strtok(NULL, " \t\r\n") : NULL;
            if (x && y) { a.reachX = atoi(x); a.reachY = atoi(y); a.hasReach = true; }
        }
        tok = strtok(NULL, " \t\r\n");
    }
    a.frameTime = ms > 0 ? ms / 1000.0f : (a.loop ? 0.11f : 0.08f);
    char path[512];
    snprintf(path, sizeof path, "%s/%s.png", dir, a.name);
    if (!FileExists(path)) return;
    Image im = LoadImage(path);
    if (!im.data) return;
    ImageFormat(&im, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    a.frames = s->cw > 0 ? im.width / s->cw : 0;
    if (a.frames <= 0) { UnloadImage(im); return; }
    body_centers(&a, &im, s);
    bool stance = !strcmp(a.name, "IDLE") || !strcmp(a.name, "PARADO") || (!s->height && !strcmp(a.name, "ATTACK_1"));
    if (stance) s->height = body_height(&im, s->cw, s->ax, s->ay);
    a.tex = LoadTextureFromImage(im);
    UnloadImage(im);
    SetTextureFilter(a.tex, TEXTURE_FILTER_POINT);
    Texture2D *layers[2] = {&a.weaponTex, &a.steelTex};
    const char *names[2] = {"weapon", "steel"};
    for(int k=0;k<2;k++) {
        snprintf(path,sizeof path,"%s/_%s_%s.png",dir,names[k],a.name);
        if (!FileExists(path)) continue;
        Texture2D t=LoadTexture(path);
        if(t.id && t.width==a.tex.width && t.height==a.tex.height) {
            *layers[k]=t; SetTextureFilter(t,TEXTURE_FILTER_POINT);
        } else if(t.id) UnloadTexture(t);
    }
    snprintf(path, sizeof path, "%s/_clean_%s.png", dir, a.name);
    if (FileExists(path)) {
        Texture2D clean = LoadTexture(path);
        if (clean.id && clean.width == a.tex.width && clean.height == a.tex.height) {
            a.cleanTex = clean;
            SetTextureFilter(clean, TEXTURE_FILTER_POINT);
            snprintf(path, sizeof path, "%s/_clean_%s.txt", dir, a.name);
            FILE *points = fopen(path, "r");
            if (points) {
                char line[128], kind[8]; int frame; float x, y;
                while (fgets(line, sizeof line, points)) {
                    if (sscanf(line, "%7s %d %f %f", kind, &frame, &x, &y) != 4 || frame < 0 || frame >= a.frames || frame >= SPR_MAX_FRAMES) continue;
                    if (!strcmp(kind, "arma")) { a.weapon[frame] = (Vector2){x, y}; a.hasWeapon[frame] = true; }
                    if (!strcmp(kind, "arma2")) { a.offhand[frame] = (Vector2){x, y}; a.hasOffhand[frame] = true; }
                }
                fclose(points);
            }
        } else if (clean.id) UnloadTexture(clean);
    }
    if (a.hold >= a.frames) a.hold = a.frames - 1;
    if (a.contact >= a.frames) a.contact = a.frames - 1;
    if (a.stop >= a.frames) a.stop = a.frames - 1;
    s->anims[s->count++] = a;
}

static bool load_set(SprSet *s, const char *id) {
    memset(s, 0, sizeof *s);
    snprintf(s->id, sizeof s->id, "%s", id);
    char dir[256], path[300], line[512];
    snprintf(dir, sizeof dir, "assets/sprites/%s", id);
    snprintf(path, sizeof path, "%s/sprite.txt", dir);
    FILE *f = fopen(path, "r");
    if (!f) return false;
    while (fgets(line, sizeof line, f)) {
        if (!strncmp(line, "cell ", 5)) sscanf(line + 5, "%d %d", &s->cw, &s->ch);
        else if (!strncmp(line, "ancora ", 7)) sscanf(line + 7, "%d %d", &s->ax, &s->ay);
        else if (!strncmp(line, "guarda ", 7)) s->hasGuard = sscanf(line + 7, "%d %d", &s->guardX, &s->guardY) == 2;
        else if (!strncmp(line, "anim ", 5)) parse_anim(s, line, dir);
        else if (!strncmp(line, "arma2 ", 6)) {
            char name[40]; int frame; float x, y;
            if (sscanf(line + 6, "%39s %d %f %f", name, &frame, &x, &y) != 4 || frame < 0 || frame >= SPR_MAX_FRAMES) continue;
            for (int i = 0; i < s->count; i++) if (!strcmp(s->anims[i].name, name) && frame < s->anims[i].frames) {
                s->anims[i].offhand[frame] = (Vector2){x, y};
                s->anims[i].hasOffhand[frame] = true;
                break;
            }
        }
        else if (!strncmp(line, "arma ", 5)) {
            char name[40]; int frame; float x, y;
            if (sscanf(line + 5, "%39s %d %f %f", name, &frame, &x, &y) != 4 || frame < 0 || frame >= SPR_MAX_FRAMES) continue;
            for (int i = 0; i < s->count; i++) if (!strcmp(s->anims[i].name, name) && frame < s->anims[i].frames) {
                s->anims[i].weapon[frame] = (Vector2){x, y};
                s->anims[i].hasWeapon[frame] = true;
                break;
            }
        }
    }
    fclose(f);
    if (!s->ax && !s->ay) { s->ax = s->cw / 2; s->ay = s->ch - 8; }
    if (!s->height) s->height = 40;
    return s->count > 0;
}

const SprSet *spr_get(const char *id) {
    if (!id || !id[0]) return NULL;
    for (int i = 0; i < nsets; i++)
        if (!strcmp(sets[i].id, id)) return tried[i] && sets[i].count > 0 ? &sets[i] : NULL;
    if (nsets >= MAX_SETS) return NULL;
    int i = nsets++;
    tried[i] = true;
    if (!load_set(&sets[i], id)) {
        for (int k = 0; k < sets[i].count; k++) {
            UnloadTexture(sets[i].anims[k].tex);
            if (sets[i].anims[k].cleanTex.id) UnloadTexture(sets[i].anims[k].cleanTex);
            if (sets[i].anims[k].weaponTex.id) UnloadTexture(sets[i].anims[k].weaponTex);
            if (sets[i].anims[k].steelTex.id) UnloadTexture(sets[i].anims[k].steelTex);
        }
        sets[i].count = 0;
        snprintf(sets[i].id, sizeof sets[i].id, "%s", id);
        return NULL;
    }
    return &sets[i];
}

const SprAnim *spr_anim(const SprSet *s, const char *name) {
    if (!s || !name) return NULL;
    for (int i = 0; i < s->count; i++)
        if (!strcmp(s->anims[i].name, name)) return &s->anims[i];
    return NULL;
}

/* ------------------------------------------------------------------ */
/* A lâmina sozinha (as adagas do yoru, as únicas que aparecem no apagão) */
/* ------------------------------------------------------------------ */

#define LAMINA_LUZ_VIOLETA 75     /* o violeta da lâmina: de 75 ou mais, mas só perto de um ponto de lâmina */
#define LAMINA_ACIMA_MAX 28       /* abaixo da altura dos olhos: nada acima disto, a partir dos pés */
#define LAMINA_RAIO 10            /* px em volta de um ponto de lâmina */
#define LAMINA_MAX 32

bool spr_pixel_de_lamina(Color c, int acimaDosPes, bool pertoDoPonto) {
    if (c.a < 200 || acimaDosPes > LAMINA_ACIMA_MAX || c.b < c.r) return false;
    float luz = 0.30f * c.r + 0.59f * c.g + 0.11f * c.b;
    return pertoDoPonto && luz >= LAMINA_LUZ_VIOLETA;
}

static struct { const SprAnim *src; SprAnim lamina; bool ok; } lam[LAMINA_MAX];
static int nlam;

static Texture2D tira_da_lamina(const SprSet *s, const SprAnim *a) {
    Texture2D vazia = {0};
    char path[512];
    snprintf(path, sizeof path, "assets/sprites/%s/%s.png", s->id, a->name);
    if (!FileExists(path)) return vazia;
    Image im = LoadImage(path);
    if (!im.data) return vazia;
    ImageFormat(&im, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *px = (Color *)im.data;
    int w = im.width, h = im.height;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            const int q = x / s->cw;                       /* o quadro, e os pontos de lâmina dele (relativos aos pés) */
            bool perto = false;
            for (int k = 0; k < 2 && q < SPR_MAX_FRAMES; k++) {
                const bool tem = k ? a->hasOffhand[q] : a->hasWeapon[q];
                if (!tem) continue;
                const Vector2 ponto = k ? a->offhand[q] : a->weapon[q];
                const float dx = (float)(x % s->cw) - (float)(s->ax + ponto.x), dy = (float)y - (float)(s->ay + ponto.y);
                perto |= dx * dx + dy * dy <= (float)(LAMINA_RAIO * LAMINA_RAIO);
            }
            if (!spr_pixel_de_lamina(px[y * w + x], s->ay - y, perto)) px[y * w + x] = (Color){0, 0, 0, 0};   /* o que fica é o pixel do próprio aço, na cor dele */
        }
    Texture2D t = LoadTextureFromImage(im);
    UnloadImage(im);
    SetTextureFilter(t, TEXTURE_FILTER_POINT);
    return t;
}

const SprAnim *spr_lamina(const SprSet *s, const SprAnim *a) {
    if (!s || !a) return NULL;
    for (int i = 0; i < nlam; i++)
        if (lam[i].src == a) return lam[i].ok ? &lam[i].lamina : NULL;
    if (nlam >= LAMINA_MAX) return NULL;
    int i = nlam++;
    lam[i].src = a;
    lam[i].lamina = *a;
    if(a->steelTex.id) {
        Image steel=LoadImageFromTexture(a->steelTex);
        lam[i].lamina.tex=LoadTextureFromImage(steel);
        UnloadImage(steel);
        SetTextureFilter(lam[i].lamina.tex,TEXTURE_FILTER_POINT);
    } else lam[i].lamina.tex = tira_da_lamina(s, a);
    lam[i].ok = lam[i].lamina.tex.id != 0;
    return lam[i].ok ? &lam[i].lamina : NULL;
}

static void solta_laminas(void) {
    for (int i = 0; i < nlam; i++)
        if (lam[i].lamina.tex.id) UnloadTexture(lam[i].lamina.tex);
    nlam = 0;
}

static void blit(const SprAnim *a, Rectangle src, Rectangle dst, bool faceLeft, Color c) {
    if (faceLeft) src.width = -src.width;
    DrawTexturePro(a->tex, src, dst, (Vector2){0, 0}, 0, c);
}

void spr_draw(const SprSet *s, const SprAnim *a, int frame, Vector2 feet, SprDraw o) {
    if (!s || !a || a->frames <= 0) return;
    SprAnim clean;
    if (o.withoutTrail && a->cleanTex.id) { clean = *a; clean.tex = a->cleanTex; a = &clean; }
    if (frame < 0) frame = 0;
    if (frame >= a->frames) frame = a->frames - 1;
    float x = floorf(feet.x + 0.5f) - (o.faceLeft ? s->cw - 1 - s->ax : s->ax);
    float y = floorf(feet.y + 0.5f) - s->ay;
    float sx = (float)(frame * s->cw);
    if (o.flat && flatOk) BeginShaderMode(flat);
    if (o.breath) {
        /* o tronco desce e a cintura para baixo fica onde está */
        int waist = s->ay - (int)(s->height * 0.42f);
        if (waist < 1) waist = 1;
        blit(a, (Rectangle){sx, 0, (float)s->cw, (float)waist}, (Rectangle){x, y + o.breath, (float)s->cw, (float)waist}, o.faceLeft, o.color);
        blit(a, (Rectangle){sx, (float)waist, (float)s->cw, (float)(s->ch - waist)},
             (Rectangle){x, y + waist, (float)s->cw, (float)(s->ch - waist)}, o.faceLeft, o.color);
    } else {
        blit(a, (Rectangle){sx, 0, (float)s->cw, (float)s->ch}, (Rectangle){x, y, (float)s->cw, (float)s->ch}, o.faceLeft, o.color);
    }
    if (o.flat && flatOk) EndShaderMode();
}

/* ------------------------------------------------------------------ */
/* Efeitos em folha e ícones da interface                              */
/* ------------------------------------------------------------------ */

const SprFx *spr_fx(const char *name) {
    for (int i = 0; i < nfx; i++)
        if (!strcmp(fxs[i].name, name)) return fxTried[i] && fxs[i].frames > 0 ? &fxs[i] : NULL;
    if (nfx >= MAX_FX || !IsWindowReady()) return NULL;   /* sem janela não há textura: e a falha não fica guardada para a vez em que houver */
    SprFx *f = &fxs[nfx];
    fxTried[nfx++] = true;
    memset(f, 0, sizeof *f);
    snprintf(f->name, sizeof f->name, "%s", name);
    char path[256];
    snprintf(path, sizeof path, "assets/sprites/_fx/%s.png", name);
    if (!FileExists(path)) return NULL;
    f->tex = LoadTexture(path);
    if (!f->tex.id) return NULL;
    SetTextureFilter(f->tex, TEXTURE_FILTER_POINT);
    f->cell = !strcmp(name, "slash") ? 96 : 64;
    if (f->tex.width % f->cell || f->tex.height % f->cell ||
        (!strcmp(name, "slash") && (f->tex.width != 864 || f->tex.height != 1152))) {
        TraceLog(LOG_WARNING, "Folha de efeito inválida: %s", path);
        UnloadTexture(f->tex);
        f->tex = (Texture2D){0};
        return NULL;
    }
    f->frames = f->tex.width / f->cell;
    f->rows = f->tex.height / f->cell;
    if (!strcmp(name, "slash")) {
        Image image = LoadImage(path);
        Color *pixels = LoadImageColors(image);
        static const int peak[12] = {1,2,1,2,2,2,0,0,1,1,1,0};
        if (pixels) {
            for (int row = 0; row < 12; row++) {
                double weight = 0, xsum = 0, ysum = 0;
                for (int y = 0; y < 96; y++) for (int x = 0; x < 96; x++) {
                    unsigned a = pixels[(row * 96 + y) * image.width + peak[row] * 96 + x].a;
                    weight += a; xsum += (x + .5) * a; ysum += (y + .5) * a;
                }
                f->pivot[row] = weight ? (Vector2){xsum / weight, ysum / weight} : (Vector2){48,48};
            }
            UnloadImageColors(pixels);
        }
        UnloadImage(image);
    }
    return f->frames > 0 ? f : NULL;
}

int spr_fx_cache_count(void) { return nfx; }

int spr_fx_row_frames(const SprFx *f, int row) {
    if (!f || row < 0 || row >= f->rows) return 0;
    static const int slashFrames[] = {8, 8, 5, 9, 9, 5, 7, 7, 4, 8, 6, 4};
    return !strcmp(f->name, "slash") && row < 12 ? slashFrames[row] : f->frames;
}

void spr_fx_draw(const SprFx *f, int row, int frame, Vector2 center, bool flip, Color tint) {
    spr_fx_draw_scaled(f, row, frame, center, flip, tint, 1);
}

void spr_fx_draw_scaled(const SprFx *f, int row, int frame, Vector2 center, bool flip, Color tint, float scale) {
    spr_fx_draw_rotated(f, row, frame, center, flip, tint, scale, 0, false);
}

void spr_fx_draw_rotated(const SprFx *f, int row, int frame, Vector2 center, bool flip,
                         Color tint, float scale, float rotation, bool flatTint) {
    if (!f) return;
    if (row < 0) row = 0;
    if (row >= f->rows) row = f->rows - 1;
    if (frame < 0 || frame >= spr_fx_row_frames(f, row)) return;
    float c = (float)f->cell, d = floorf(c * scale + 0.5f);
    Rectangle src = {frame * c, row * c, flip ? -c : c, c};
    Rectangle dst = {floorf(center.x + 0.5f), floorf(center.y + 0.5f), d, d};
    if (flatTint && flatOk) BeginShaderMode(flat);
    DrawTexturePro(f->tex, src, dst, (Vector2){floorf(d / 2), floorf(d / 2)}, rotation, tint);
    if (flatTint && flatOk) EndShaderMode();
}

/* Pivô pintado no quadro principal, fixo durante a animação: a cauda não
 * salta ao centro de uma célula transparente nem é recentrada a cada quadro. */
void spr_fx_draw_weapon(const SprFx *f, int row, int frame, Vector2 at, bool flip, Color tint, float scale) {
    if (!f || row < 0 || row >= 12 || frame < 0 || frame >= spr_fx_row_frames(f, row)) return;
    float c = (float)f->cell, d = floorf(c * scale + .5f);
    Vector2 p = f->pivot[row];
    Rectangle src = {frame * c, row * c, flip ? -c : c, c};
    Rectangle dst = {floorf(at.x + .5f), floorf(at.y + .5f), d, d};
    if (slashColourOk) {
        int lightning = row == 1 || row == 7 || row == 10;
        BeginShaderMode(slashColour);
        SetShaderValue(slashColour, slashColourMode, &lightning, SHADER_UNIFORM_INT);
    }
    DrawTexturePro(f->tex, src, dst, (Vector2){(flip ? c - p.x : p.x) * d / c, p.y * d / c}, 0, tint);
    if (slashColourOk) EndShaderMode();
}

static void ui_load(void) {
    if (uiTried) return;
    uiTried = true;
    const char *k0 = "assets/sprites/_ui/buttons-spritesheet.png", *k1 = "assets/sprites/_ui/buttons-pressed-spritesheet.png";
    if (FileExists(k0) && FileExists(k1)) {
        keysTex[0] = LoadTexture(k0);
        keysTex[1] = LoadTexture(k1);
        uiOk = keysTex[0].id && keysTex[1].id;
    }
}

void spr_ui_preload(void) { ui_load(); }

/* Onde cada tecla está na folha: 16 x 16, e as largas com 24 (a barra, 32). */
static bool key_rect(const char *k, Rectangle *r) {
    static const struct { const char *name; int x, y, w; } WIDE[] = {
        {"ESC", 0, 96, 16}, {"ENTER", 16, 96, 16}, {"TAB", 32, 96, 24}, {"SHIFT", 56, 96, 24},
        {"DEL", 80, 96, 24}, {"CAPS", 104, 96, 24}, {"SPACE", 128, 96, 32},
    };
    for (size_t i = 0; i < sizeof WIDE / sizeof WIDE[0]; i++)
        if (!strcmp(k, WIDE[i].name)) { *r = (Rectangle){(float)WIDE[i].x, (float)WIDE[i].y, (float)WIDE[i].w, 16}; return true; }
    if (!k[0] || k[1]) return false;
    char c = k[0];
    if (c >= 'a' && c <= 'z') c = (char)(c - 32);
    int idx;
    if (c >= 'A' && c <= 'Z') idx = c - 'A';
    else if (c >= '1' && c <= '9') idx = 26 + (c - '1');
    else if (c == '0') idx = 35;
    else return false;
    *r = (Rectangle){(float)(idx % 12) * 16, 48 + (float)(idx / 12) * 16, 16, 16};
    return true;
}

float spr_key(const char *key, float x, float y, float unit, bool pressed, Color tint) {
    ui_load();
    Rectangle r;
    if (!uiOk || !key_rect(key, &r)) return 0;
    Rectangle dst = {floorf(x / unit + 0.5f) * unit, floorf(y / unit + 0.5f) * unit, r.width * unit, r.height * unit};
    DrawTexturePro(keysTex[pressed ? 1 : 0], r, dst, (Vector2){0, 0}, 0, tint);
    return dst.width;
}

/* Com `tempos`, a duração natural do trecho e o quadro no instante t (o trecho esticado
 * ou encolhido por igual até `dur`). */
static bool timed(const SprAnim *a, int to) { return a->ntimes > to; }
static float timed_len(const SprAnim *a, int from, int to) {
    float s = 0;
    for (int k = from; k <= to; k++) s += a->times[k];
    return s;
}
static int timed_frame(const SprAnim *a, int from, int to, float t, float dur) {
    float nat = timed_len(a, from, to), scale = nat > 0 && dur > 0 ? dur / nat : 1, acc = 0;
    for (int k = from; k <= to; k++) {
        acc += a->times[k] * scale;
        if (t < acc) return k;
    }
    return to;
}

void spr_play(SprPlayer *p, const SprAnim *a, int from, int to, float dur) {
    p->anim = a;
    p->loop = false;
    p->limit = 0;
    p->after = NULL;
    p->t = 0;
    if (!a) return;
    if (from < 0) from = 0;
    if (from >= a->frames) from = a->frames - 1;
    if (to >= a->frames) to = a->frames - 1;
    if (to < from) to = from;
    p->from = from;
    p->to = to;
    p->dur = dur > 0 ? dur : timed(a, to) ? timed_len(a, from, to) : (to - from + 1) * a->frameTime;
    p->frame = from;
}

void spr_loop(SprPlayer *p, const SprAnim *a) {
    spr_play(p, a, 0, a ? a->frames - 1 : 0, 0);
    p->loop = true;
}

void spr_cycle(SprPlayer *p, const SprAnim *a, float time) {
    spr_loop(p, a);
    p->limit = time > 0 ? time : 0.001f;
}

void spr_update(SprPlayer *p, float dt) {
    if (!p->anim) return;
    p->t += dt;
    int n = p->to - p->from + 1;
    if (p->loop) {
        float per = p->dur / n;
        if (timed(p->anim, p->to)) p->frame = timed_frame(p->anim, p->from, p->to, fmodf(p->t, p->dur), p->dur);
        else p->frame = p->from + (int)(fmodf(p->t, p->dur) / per) % n;
        return;
    }
    if (p->t >= p->dur && p->after) {
        const SprAnim *next = p->after;
        spr_loop(p, next);
        return;
    }
    if (timed(p->anim, p->to)) {
        p->frame = timed_frame(p->anim, p->from, p->to, p->t, p->dur);
        return;
    }
    int k = p->dur > 0 ? (int)(p->t / p->dur * n) : n - 1;
    p->frame = p->from + (k < n - 1 ? k : n - 1);
}

bool spr_done(const SprPlayer *p) {
    if (!p->anim) return true;
    return p->loop ? p->limit > 0 && p->t >= p->limit : p->t >= p->dur;
}

bool spr_weapon_point(const SprPlayer *p, Vector2 feet, bool faceLeft, int breath, Vector2 *point) {
    if (!p || !p->anim || p->frame < 0 || p->frame >= SPR_MAX_FRAMES || !p->anim->hasWeapon[p->frame]) return false;
    Vector2 local = p->anim->weapon[p->frame];
    *point = (Vector2){floorf(feet.x + 0.5f) + (faceLeft ? -local.x : local.x), floorf(feet.y + 0.5f) + local.y + breath};
    return true;
}

bool spr_offhand_point(const SprPlayer *p, Vector2 feet, bool faceLeft, int breath, Vector2 *point) {
    if (!p || !p->anim || p->frame < 0 || p->frame >= SPR_MAX_FRAMES) return false;
    int frame = p->frame;
    /* A lâmina pode estar coberta pelo corpo em uma pose. O ponto mais próximo
     * mantém o efeito associado à mesma mão até ela reaparecer. */
    if (!p->anim->hasOffhand[frame]) {
        int best = SPR_MAX_FRAMES;
        for (int i = 0; i < p->anim->frames && i < SPR_MAX_FRAMES; i++)
            if (p->anim->hasOffhand[i] && abs(i - p->frame) < best) { frame = i; best = abs(i - p->frame); }
        if (best == SPR_MAX_FRAMES) return false;
    }
    Vector2 local = p->anim->offhand[frame];
    *point = (Vector2){floorf(feet.x + 0.5f) + (faceLeft ? -local.x : local.x), floorf(feet.y + 0.5f) + local.y + breath};
    return true;
}
