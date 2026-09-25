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

#define MAX_FX 24
static SprFx fxs[MAX_FX];     /* efeitos em folha, carregados na primeira vez */
static bool fxTried[MAX_FX];
static int nfx;
static Texture2D keysTex[2], mouseTex;
static bool uiTried, uiOk, mouseOk;

/* Silhueta: a cor da tinta com o alfa da prancha. */
static const char *FLAT_FS =
    "#version 330\n"
    "in vec2 fragTexCoord; in vec4 fragColor; uniform sampler2D texture0; out vec4 finalColor;\n"
    "void main() { finalColor = vec4(fragColor.rgb, texture(texture0, fragTexCoord).a * fragColor.a); }\n";

void spr_init(void) {
    flat = LoadShaderFromMemory(NULL, FLAT_FS);
    flatOk = flat.id != 0;
}

void spr_shutdown(void) {
    for (int i = 0; i < nsets; i++)
        for (int k = 0; k < sets[i].count; k++) UnloadTexture(sets[i].anims[k].tex);
    nsets = 0;
    for (int i = 0; i < nfx; i++)
        if (fxs[i].tex.id) UnloadTexture(fxs[i].tex);
    nfx = 0;
    if (uiOk) { UnloadTexture(keysTex[0]); UnloadTexture(keysTex[1]); }
    if (mouseOk) UnloadTexture(mouseTex);
    uiOk = mouseOk = false;
    if (flatOk) UnloadShader(flat);
    flatOk = false;
}

/* Altura do corpo no primeiro quadro: da âncora dos pés até o pixel mais alto. */
static int body_height(const Image *im, int cw, int ax, int ay) {
    const Color *px = (const Color *)im->data;
    for (int y = 0; y < im->height && y < ay; y++)
        for (int x = ax - cw / 3; x < ax + cw / 3; x++)
            if (x >= 0 && x < cw && x < im->width && px[y * im->width + x].a > 0) return ay - y;
    return 40;
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
    while ((tok = strtok(NULL, " \t\r\n"))) {
        if (!strcmp(tok, "loop")) a.loop = true;
        else if (!strcmp(tok, "hold")) { tok = strtok(NULL, " \t\r\n"); if (tok) a.hold = atoi(tok); }
        else if (!strcmp(tok, "contact")) { tok = strtok(NULL, " \t\r\n"); if (tok) a.contact = atoi(tok); }
        else if (!strcmp(tok, "stop")) { tok = strtok(NULL, " \t\r\n"); if (tok) a.stop = atoi(tok); }
        else if (!strcmp(tok, "ms")) { tok = strtok(NULL, " \t\r\n"); if (tok) ms = atoi(tok); }
        else if (!strcmp(tok, "alcance")) {
            char *x = strtok(NULL, " \t\r\n"), *y = x ? strtok(NULL, " \t\r\n") : NULL;
            if (x && y) { a.reachX = atoi(x); a.reachY = atoi(y); a.hasReach = true; }
        }
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
    bool stance = !strcmp(a.name, "IDLE") || !strcmp(a.name, "PARADO") || (!s->height && !strcmp(a.name, "ATTACK_1"));
    if (stance) s->height = body_height(&im, s->cw, s->ax, s->ay);
    a.tex = LoadTextureFromImage(im);
    UnloadImage(im);
    SetTextureFilter(a.tex, TEXTURE_FILTER_POINT);
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
        for (int k = 0; k < sets[i].count; k++) UnloadTexture(sets[i].anims[k].tex);
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

static void blit(const SprAnim *a, Rectangle src, Rectangle dst, bool faceLeft, Color c) {
    if (faceLeft) src.width = -src.width;
    DrawTexturePro(a->tex, src, dst, (Vector2){0, 0}, 0, c);
}

void spr_draw(const SprSet *s, const SprAnim *a, int frame, Vector2 feet, SprDraw o) {
    if (!s || !a || a->frames <= 0) return;
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
    if (nfx >= MAX_FX) return NULL;
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
    f->cell = 64;
    f->frames = f->tex.width / f->cell;
    f->rows = f->tex.height / f->cell;
    return f->frames > 0 ? f : NULL;
}

void spr_fx_draw(const SprFx *f, int row, int frame, Vector2 center, bool flip, Color tint) {
    if (!f || frame < 0 || frame >= f->frames) return;
    if (row < 0) row = 0;
    if (row >= f->rows) row = f->rows - 1;
    float c = (float)f->cell;
    Rectangle src = {frame * c, row * c, flip ? -c : c, c};
    Rectangle dst = {floorf(center.x + 0.5f) - c / 2, floorf(center.y + 0.5f) - c / 2, c, c};
    DrawTexturePro(f->tex, src, dst, (Vector2){0, 0}, 0, tint);
}

static void ui_load(void) {
    if (uiTried) return;
    uiTried = true;
    const char *k0 = "assets/sprites/_ui/buttons-spritesheet.png", *k1 = "assets/sprites/_ui/buttons-pressed-spritesheet.png";
    const char *m = "assets/sprites/_ui/mouse-spritesheet.png";
    if (FileExists(k0) && FileExists(k1)) {
        keysTex[0] = LoadTexture(k0);
        keysTex[1] = LoadTexture(k1);
        uiOk = keysTex[0].id && keysTex[1].id;
    }
    if (FileExists(m)) {
        mouseTex = LoadTexture(m);
        mouseOk = mouseTex.id != 0;
    }
}

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

float spr_mouse(int button, float x, float y, float unit, Color tint) {
    ui_load();
    if (!mouseOk || button < 0 || button > 3) return 0;
    Rectangle src = {button * 32.0f, 0, 32, 32};
    Rectangle dst = {floorf(x / unit + 0.5f) * unit, floorf(y / unit + 0.5f) * unit, 32 * unit, 32 * unit};
    DrawTexturePro(mouseTex, src, dst, (Vector2){0, 0}, 0, tint);
    return dst.width;
}

void spr_play(SprPlayer *p, const SprAnim *a, int from, int to, float dur) {
    p->anim = a;
    p->loop = false;
    p->after = NULL;
    p->t = 0;
    if (!a) return;
    if (from < 0) from = 0;
    if (from >= a->frames) from = a->frames - 1;
    if (to >= a->frames) to = a->frames - 1;
    if (to < from) to = from;
    p->from = from;
    p->to = to;
    p->dur = dur > 0 ? dur : (to - from + 1) * a->frameTime;
    p->frame = from;
}

void spr_loop(SprPlayer *p, const SprAnim *a) {
    spr_play(p, a, 0, a ? a->frames - 1 : 0, 0);
    p->loop = true;
}

void spr_update(SprPlayer *p, float dt) {
    if (!p->anim) return;
    p->t += dt;
    int n = p->to - p->from + 1;
    if (p->loop) {
        float per = p->dur / n;
        p->frame = p->from + (int)(fmodf(p->t, p->dur) / per) % n;
        return;
    }
    if (p->t >= p->dur && p->after) {
        const SprAnim *next = p->after;
        spr_loop(p, next);
        return;
    }
    int k = p->dur > 0 ? (int)(p->t / p->dur * n) : n - 1;
    p->frame = p->from + (k < n - 1 ? k : n - 1);
}

bool spr_done(const SprPlayer *p) { return !p->anim || (!p->loop && p->t >= p->dur); }
