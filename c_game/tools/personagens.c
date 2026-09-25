/*
 * personagens.c - gera os lutadores do aparar a partir das pranchas do Samurai #3
 * (o corpo do Kojiro) e dos packs próprios (Raizo, Shizuku, Oboro e os de duas armas).
 *
 * Não é só troca de paleta:
 *   - tira o chapéu (só o Daichi fica com um) e desenha a cabeça: coque, rabo de
 *     cavalo, capuz, careca, cabelo em chamas...
 *   - troca a arma seguindo a reta da katana em cada quadro;
 *   - pinta o rastro do golpe com o elemento de cada um;
 *   - põe acessórios atrás do corpo: cachecol, casco, fitas, rabo de cavalo.
 *
 * Tudo é troca e acréscimo de pixel inteiro, sem escala nem rotação. O mesmo
 * comando sempre gera os mesmos pixels. Não abre janela: só usa as funções de
 * imagem e de arquivo da raylib.
 *
 * Uso (de dentro de c_game/):
 *     make personagens
 *     ./personagens                   todas as pranchas, todos os personagens
 *     ./personagens --so enjin yoru   só alguns
 *     ./personagens --folhas          também as folhas de conferência
 *     ./personagens --lista           quem é quem
 *
 * Entrada: assets/sprites/_original/ (tiras de quadros 106 x 84 viradas para a
 *          direita). Na primeira vez é copiada de assets/sprites/musashi/ (onde
 *          fica o pack original); o protagonista gerado sai em kojiro/.
 * Saída:   assets/sprites/<personagem>/<ANIM>.png e sprite.txt (com o alcance
 *          do golpe), mais ESPECIAL.png e DESARMADO.png (sem a arma, para o
 *          desarme no jogo), e assets/sprites/_folhas/ com as folhas.
 * Detalhes e o elenco em docs/PERSONAGENS.md.
 */
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

#include "raylib.h"

/* Contas de ponto flutuante sem FMA, para sair igual em qualquer máquina. */
#ifdef __clang__
#pragma STDC FP_CONTRACT OFF
#endif

/* CW x CH é o maior quadro aceito (os packs novos vão até 128 x 108). Cada
   prancha guarda o próprio tamanho; o que passa dele fica transparente e é
   cortado ao salvar. O Samurai #3 usa 106 x 84. */
#define CW 128
#define CH 112
#define SRC_W 106
#define SRC_H 84
static int cellw = SRC_W, cellh = SRC_H;  /* quadro da prancha que está sendo feita agora */
#define MAX_STRIPS 48
#define MAX_FRAMES 64
#define MAX_BLADES 24
#define MAX_REND 96    /* tiras que saem de um personagem (o Oboro tem as da postura de cada um) */
#define PATHLEN 1024

typedef struct { unsigned char r, g, b; } Rgb;
#define HEX(h) {(unsigned char)(((h) >> 16) & 255), (unsigned char)(((h) >> 8) & 255), (unsigned char)((h) & 255)}

static bool rgb_set(Rgb c) { return c.r || c.g || c.b; }

/* ------------------------------------------------------------------------ */
/* Paleta de origem: cada cor do Samurai #3 vira uma letra.                  */
/* ------------------------------------------------------------------------ */
static const struct { unsigned char r, g, b; char k; } SRC[] = {
    {255, 255, 255, 'W'},  /* luz da camisa, lâmina, rastro */
    {199, 207, 221, 'L'},  /* camisa, chapéu, borda da lâmina e do rastro */
    {146, 161, 185, 'M'},  /* camisa e chapéu, meio-tom */
    {101, 115, 146, 'D'},  /* camisa e chapéu, sombra */
    {180, 180, 180, 'g'},  /* lâmina borrada no movimento */
    {93, 93, 93, '9'},     /* hakama, do claro... */
    {61, 61, 61, '6'},
    {39, 39, 39, '3'},
    {27, 27, 27, '2'},
    {19, 19, 19, '1'},     /* ...ao escuro; também cabelo e pomo */
    {28, 18, 28, 'h'},     /* bainha (saya) */
    {12, 46, 68, 'b'},     /* cabo (tsuka) */
    {230, 156, 105, 'S'},  /* pele */
    {191, 111, 74, 's'},
    {138, 72, 54, 'k'},
    {87, 28, 39, 'r'},
};
/* Os quase-brancos da camisa; todos os outros quase-brancos são da lâmina. */
static const Rgb SHIRT_NEAR_WHITE[] = {{244, 252, 254}, {246, 251, 255}};

/* O chapéu é sempre o mesmo carimbo de 17 x 8, só muda de lugar. */
static const char *HAT[8] = {
    ".........MM......",
    ".......MMLMD.....",
    "....MMMMLLMDD....",
    "..MMMMMLLMMDDD...",
    "DMMMMMMMMMMDDDD..",
    ".DDMMMMMMMMDDDDD.",
    "...DDDMMMMMDDDDDD",
    "......DDDDDDDDD..",
};

/* Rótulos das partes. */
enum { NONE, HATL, HATFX, HAIR, FACE, SKIN, SHIRT, DARK, SAYA, HANDLE, BLADE, SMEAR, OTHER };

static bool in_set(char c, const char *set) { return c && strchr(set, c) != NULL; }

/* Número pseudoaleatório estável (0..1) a partir de textos e inteiros. */
typedef struct { uint32_t x; } Hash;
static void h_str(Hash *h, const char *s) {
    for (; *s; s++) h->x = (h->x ^ (unsigned char)*s) * 16777619u;
    h->x = (h->x ^ 0x9e) * 16777619u;
}
static void h_int(Hash *h, int v) {
    char b[24];
    snprintf(b, sizeof b, "%d", v);
    h_str(h, b);
}
static double h_end(Hash *h) { return (double)(h->x & 0xffffff) / (double)0x1000000; }
static double hsh4(const char *tag, const char *anim, int idx, int x, int y) {
    Hash h = {2166136261u};
    h_str(&h, tag); h_str(&h, anim); h_int(&h, idx); h_int(&h, x); h_int(&h, y);
    return h_end(&h);
}
static double hsh6(const char *tag, const char *anim, int idx, int x, int y, int a, int b) {
    Hash h = {2166136261u};
    h_str(&h, tag); h_str(&h, anim); h_int(&h, idx); h_int(&h, x); h_int(&h, y); h_int(&h, a); h_int(&h, b);
    return h_end(&h);
}

/* Arredonda como o Python (meio para o par). */
static int pyround(double v) { return (int)rint(v); }

/* ------------------------------------------------------------------------ */
/* Máscaras                                                                  */
/* ------------------------------------------------------------------------ */
typedef bool Mask[CH][CW];

/* out[y][x] = m[y - dy][x - dx], sem dar a volta nas bordas. */
static void mask_shift(Mask out, Mask m, int dy, int dx) {
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            int sy = y - dy, sx = x - dx;
            out[y][x] = sy >= 0 && sy < CH && sx >= 0 && sx < CW && m[sy][sx];
        }
}
static void mask_dilate(Mask out, Mask m, int r, bool cross) {
    Mask tmp;
    memset(out, 0, sizeof(Mask));
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++) {
            if (cross && abs(dy) + abs(dx) > r) continue;
            mask_shift(tmp, m, dy, dx);
            for (int y = 0; y < CH; y++)
                for (int x = 0; x < CW; x++) out[y][x] |= tmp[y][x];
        }
}

/* Componentes conexos, na ordem em que o primeiro pixel aparece (linha a linha). */
typedef struct {
    int n;
    int start[CW * CH], len[CW * CH];
    short px[CW * CH], py[CW * CH];
} Comps;

static void components(Comps *cs, Mask m, int conn) {
    static int lab[CH][CW];
    static const int nb[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    int nn = conn == 8 ? 8 : 4, used = 0;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) lab[y][x] = -1;
    cs->n = 0;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            if (!m[y][x] || lab[y][x] >= 0) continue;
            int k = cs->n++, head = used;
            cs->start[k] = used;
            lab[y][x] = k;
            cs->px[used] = (short)x; cs->py[used] = (short)y; used++;
            while (head < used) {
                int cy = cs->py[head], cx = cs->px[head];
                head++;
                for (int i = 0; i < nn; i++) {
                    int ny = cy + nb[i][0], nx = cx + nb[i][1];
                    if (ny >= 0 && ny < CH && nx >= 0 && nx < CW && m[ny][nx] && lab[ny][nx] < 0) {
                        lab[ny][nx] = k;
                        cs->px[used] = (short)nx; cs->py[used] = (short)ny; used++;
                    }
                }
            }
            cs->len[k] = used - cs->start[k];
        }
}

/* ------------------------------------------------------------------------ */
/* Segmentação                                                               */
/* ------------------------------------------------------------------------ */
typedef struct {
    int n;
    short *x, *y;
    double *dist;
    double u[2], hilt[2];
    double nearest, farthest;  /* distância do cabo até o começo visível e até a ponta */
    bool loose;                /* pack: nenhuma mão por perto (a ponta aparecendo no rastro) */
    bool hand;                 /* achou a mão no prolongamento da lâmina */
} Blade;

typedef struct {
    char c[CH][CW];
    signed char lab[CH][CW];
    int ox, oy;
    double score;
    bool has_hat;
    int nblades;
    Blade blades[MAX_BLADES];
    int nerase;
    int erase[8][4];
    int nunknown;
} Seg;

typedef struct { Color p[CH][CW]; } Frame;

typedef struct {
    char name[64];
    int nframes, cw, ch;
    Frame *frames;
    Seg *segs;
} Strip;

typedef struct {
    bool nohat, hat;
    int hx, hy;
    int nerase;
    int erase[8][4];
} Fix;

/* Personagem cujo pack está sendo lido agora (cores próprias) e se é um pack. */
static const void *g_pack_ch;
static bool g_pack;

static char pack_class(Color p);
static bool pack_no_shirt(void);

static char classify_px(Color p, bool *unknown) {
    if (p.a == 0) return '.';
    if (g_pack) {
        char k = pack_class(p);
        if (k) return k;
    }
    for (size_t i = 0; i < sizeof SRC / sizeof SRC[0]; i++)
        if (SRC[i].r == p.r && SRC[i].g == p.g && SRC[i].b == p.b) return SRC[i].k;
    unsigned char mn = p.r < p.g ? p.r : p.g;
    if (p.b < mn) mn = p.b;
    if (mn >= 236) {
        for (int i = 0; i < 2; i++)
            if (SHIRT_NEAR_WHITE[i].r == p.r && SHIRT_NEAR_WHITE[i].g == p.g && SHIRT_NEAR_WHITE[i].b == p.b) return 'v';
        return 'w';
    }
    *unknown = true;
    return '?';
}

/* Acha o carimbo do chapéu. Aceita braços e cabo passando na frente. */
static double find_hat(const char c[CH][CW], int *rox, int *roy) {
    double best = -1e18;
    int bx = 0, by = 0;
    for (int oy = -2; oy < CH - 6; oy++)
        for (int ox = -6; ox < CW - 10; ox++) {
            double s = 0;
            for (int dy = 0; dy < 8; dy++)
                for (int dx = 0; HAT[dy][dx]; dx++) {
                    char k = HAT[dy][dx];
                    if (k == '.') continue;
                    int x = ox + dx, y = oy + dy;
                    char v = (x >= 0 && x < CW && y >= 0 && y < CH) ? c[y][x] : '.';
                    if (v == k) s += 2;
                    else if (in_set(v, "MLD")) s += 1;
                    if (v == '.') s -= 2;
                    else if (in_set(v, "Wwvg")) s -= 1;
                }
            if (s > best) { best = s; bx = ox; by = oy; }
        }
    *rox = bx; *roy = by;
    return best;
}

static void find_blades(Seg *s);

static void segment(const Frame *f, const Fix *fix, Seg *s) {
    static Mask hat, hatfx, white, block, near_block, wsum_b, seed, grow, smear, ring, tmp, tmp2;
    static Mask shirtcol, thick, shirt, blade, handle, bmask;
    static int wsum[CH][CW];
    static Comps cs;
    memset(s, 0, sizeof *s);
    bool unk = false;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            bool u = false;
            s->c[y][x] = classify_px(f->p[y][x], &u);
            if (u) unk = true;
        }
    s->nunknown = unk;
    char (*c)[CW] = s->c;
    int ox, oy;
    int bx0 = CW, bx1 = -1, by0 = CH, by1 = -1;  /* pack: caixa das cores do corpo */
    double score;
    if (g_pack) {
        /* corpo próprio: não há chapéu; a "caixa do corpo" sai da área das cores do corpo */
        for (int y = 0; y < CH; y++)
            for (int x = 0; x < CW; x++)
                if (c[y][x] != '.' && !in_set(c[y][x], "WLwvg")) {
                    if (x < bx0) bx0 = x;
                    if (x > bx1) bx1 = x;
                    if (y < by0) by0 = y;
                    if (y > by1) by1 = y;
                }
        ox = bx1 >= 0 ? (bx0 + bx1) / 2 - 8 : 0;
        oy = bx1 >= 0 ? by0 : 0;
        score = 0;
    } else {
        score = find_hat((const char (*)[CW])c, &ox, &oy);
    }
    if (fix && fix->hat) { ox = fix->hx; oy = fix->hy; score = 999; }
    s->ox = ox; s->oy = oy; s->score = score;
    s->has_hat = score >= 60 && !(fix && fix->nohat);
    if (fix) {
        s->nerase = fix->nerase;
        memcpy(s->erase, fix->erase, sizeof s->erase);
    }

    memset(hat, 0, sizeof hat);
    if (s->has_hat) {
        bool clean = score >= 150;
        for (int dy = 0; dy < 8; dy++)
            for (int dx = 0; HAT[dy][dx]; dx++) {
                char k = HAT[dy][dx];
                if (k == '.') continue;
                int x = ox + dx, y = oy + dy;
                if (x >= 0 && x < CW && y >= 0 && y < CH && (c[y][x] == k || (clean && in_set(c[y][x], "MLD"))))
                    hat[y][x] = true;
            }
    }
    /* Rastro de movimento do chapéu (no DASH): só o que encosta nele pela esquerda. */
    memset(hatfx, 0, sizeof hatfx);
    if (s->has_hat) {
        for (int y = oy < 0 ? 0 : oy; y < (oy + 8 < CH ? oy + 8 : CH); y++) {
            int first = -1;
            for (int x = 0; x < CW; x++)
                if (hat[y][x]) { first = x; break; }
            if (first < 0) continue;
            int x = first - 1, gap = 0;
            while (x >= 0 && gap <= 1) {
                if (in_set(c[y][x], "MLDWv")) { hatfx[y][x] = true; gap = 0; }
                else gap++;
                x--;
            }
        }
    }

    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            white[y][x] = in_set(c[y][x], "WLwvg");
            /* no pack, as cores próprias do corpo (cabelo, roupa) também seguram o rastro */
            block[y][x] = in_set(c[y][x], g_pack ? "MDvSskbr?" : "MDvSskbr") && !hat[y][x] && !hatfx[y][x];
        }
    /* Rastro: mancha grande de branco e cinza claro, sem sombra de camisa nem
       pele por perto, e que sai para fora do corpo. */
    mask_dilate(near_block, block, 1, false);
    memset(wsum, 0, sizeof wsum);
    memset(wsum_b, 0, sizeof wsum_b);
    for (int dy = -2; dy <= 2; dy++)
        for (int dx = -2; dx <= 2; dx++) {
            mask_shift(tmp, white, dy, dx);
            mask_shift(tmp2, block, dy, dx);
            for (int y = 0; y < CH; y++)
                for (int x = 0; x < CW; x++) {
                    wsum[y][x] += tmp[y][x];
                    wsum_b[y][x] |= tmp2[y][x];
                }
        }
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            seed[y][x] = white[y][x] && !wsum_b[y][x] && wsum[y][x] >= 16 && !hat[y][x] && !hatfx[y][x];
            grow[y][x] = white[y][x] && !hat[y][x] && !hatfx[y][x] && c[y][x] != 'v' && !near_block[y][x];
        }
    components(&cs, grow, 4);
    memset(smear, 0, sizeof smear);
    for (int k = 0; k < cs.n; k++) {
        if (cs.len[k] < 20) continue;
        bool seeded = false;
        int outside = 0;
        for (int i = cs.start[k]; i < cs.start[k] + cs.len[k]; i++) {
            int px = cs.px[i], py = cs.py[i];
            if (seed[py][px]) seeded = true;
            bool in_body = g_pack ? bx0 - 1 <= px && px <= bx1 + 1 && by0 - 1 <= py && py <= by1 + 1
                                  : ox - 3 <= px && px <= ox + 20 && oy - 1 <= py && py <= oy + 34;
            if (!in_body) outside++;
        }
        /* no pack, o clarão branco do HURT fica dentro do corpo: não é rastro */
        if (seeded && outside >= 8 && (!g_pack || outside * 4 >= cs.len[k]))
            for (int i = cs.start[k]; i < cs.start[k] + cs.len[k]; i++) smear[cs.py[i]][cs.px[i]] = true;
    }
    /* A borda cinza do rastro que encosta na hakama também é rastro. */
    for (int it = 0; it < 2; it++) {
        mask_dilate(ring, smear, 1, true);
        for (int y = 0; y < CH; y++)
            for (int x = 0; x < CW; x++)
                tmp[y][x] = ring[y][x] && in_set(c[y][x], "LWwg") && !hat[y][x] && !near_block[y][x] && !smear[y][x];
        for (int y = 0; y < CH; y++)
            for (int x = 0; x < CW; x++) smear[y][x] |= tmp[y][x];
    }

    /* Camisa x lâmina: a camisa é grossa (blocos 2 x 2), a lâmina é um fio. */
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            shirtcol[y][x] = in_set(c[y][x], "WLMDvw") && !hat[y][x] && !smear[y][x] && !hatfx[y][x];
    memset(thick, 0, sizeof thick);
    if (g_pack) {
        /* Os packs maiores têm lâmina de 2 ou 3 px: a camisa é o que tem miolo de 3 x 3. */
        for (int y = 1; y < CH - 1; y++)
            for (int x = 1; x < CW - 1; x++) {
                bool full = true;
                for (int dy = -1; dy <= 1 && full; dy++)
                    for (int dx = -1; dx <= 1 && full; dx++) full = shirtcol[y + dy][x + dx];
                tmp[y][x] = full;
            }
        mask_dilate(thick, tmp, 1, false);
        mask_dilate(tmp2, thick, 1, false);
        for (int y = 0; y < CH; y++)
            for (int x = 0; x < CW; x++) {
                thick[y][x] = thick[y][x] && shirtcol[y][x];
                shirt[y][x] = thick[y][x] || (shirtcol[y][x] && tmp2[y][x] && in_set(c[y][x], "Dv"));
                blade[y][x] = in_set(c[y][x], "WLwvgM") && !hat[y][x] && !hatfx[y][x] && !smear[y][x] && !shirt[y][x];
            }
        /* Mancha grossa que fica quase toda fora da caixa do corpo é rastro (o rastro
           se apagando é só cinza, sem o miolo branco que o acha acima). */
        components(&cs, thick, 8);
        mask_dilate(tmp, blade, 1, false);
        bool sem_camisa = pack_no_shirt();
        for (int k = 0; k < cs.n && bx1 >= 0; k++) {
            int out = 0, n = cs.len[k];
            double mx = 0, my = 0, sxx = 0, sxy = 0, syy = 0;
            bool touches = false;
            for (int i = cs.start[k]; i < cs.start[k] + n; i++) {
                int px = cs.px[i], py = cs.py[i];
                out += !(bx0 <= px && px <= bx1 && by0 <= py && py <= by1);
                touches |= tmp[py][px];
                mx += px; my += py;
            }
            mx /= n; my /= n;
            for (int i = cs.start[k]; i < cs.start[k] + n; i++) {
                double dx = cs.px[i] - mx, dy = cs.py[i] - my;
                sxx += dx * dx; sxy += dx * dy; syy += dy * dy;
            }
            sxx /= n; sxy /= n; syy /= n;
            double tr = sxx + syy, det = sxx * syy - sxy * sxy, disc = sqrt(fmax(0.0, tr * tr / 4 - det));
            double l1 = tr / 2 + disc, l2 = tr / 2 - disc;
            /* comprida e fina (lâmina de 3 px, como a do Demon), ou um pedaço grosso colado
               numa lâmina, fino ou fora do corpo: é lâmina, não camisa */
            if ((l1 > 9 && l2 < 1.2) || (touches && n < 40 && (l2 < 1.5 || out * 2 >= n))) {
                for (int i = cs.start[k]; i < cs.start[k] + n; i++) {
                    int px = cs.px[i], py = cs.py[i];
                    shirt[py][px] = false;
                    blade[py][px] = in_set(c[py][px], "WLwvgM");
                }
                continue;
            }
            /* Mancha grossa que fica quase toda fora da caixa do corpo é rastro (o rastro
               se apagando é só cinza, sem o miolo branco que o acha acima). Sem camisa
               branca no pack, qualquer mancha grossa grande é rastro. */
            if (n >= 12 && (out * 2 >= n || (sem_camisa && n >= 24)))
                for (int i = cs.start[k]; i < cs.start[k] + n; i++) {
                    int px = cs.px[i], py = cs.py[i];
                    smear[py][px] = true;
                    shirt[py][px] = blade[py][px] = false;
                }
        }
        /* O rabo do rastro (cinza, grosso) fora da caixa do corpo também é rastro; sem
           camisa branca no pack, também o que passa na frente do corpo. */
        for (int it = 0; it < 64 && bx1 >= 0; it++) {
            bool changed = false;
            mask_dilate(ring, smear, 1, false);
            for (int y = 0; y < CH; y++)
                for (int x = 0; x < CW; x++)
                    if (ring[y][x] && !smear[y][x] && thick[y][x] && !blade[y][x] &&
                        (sem_camisa || !(bx0 <= x && x <= bx1 && by0 <= y && y <= by1))) {
                        smear[y][x] = true;
                        shirt[y][x] = blade[y][x] = false;
                        changed = true;
                    }
            if (!changed) break;
        }
    }
    for (int y = 0; y < CH - 1 && !g_pack; y++)
        for (int x = 0; x < CW - 1; x++)
            if (shirtcol[y][x] && shirtcol[y + 1][x] && shirtcol[y][x + 1] && shirtcol[y + 1][x + 1])
                thick[y][x] = thick[y + 1][x] = thick[y][x + 1] = thick[y + 1][x + 1] = true;
    for (int y = 0; y < CH && !g_pack; y++)
        for (int x = 0; x < CW; x++) {
            int n = (y > 0 && thick[y - 1][x]) + (y < CH - 1 && thick[y + 1][x]) +
                    (x > 0 && thick[y][x - 1]) + (x < CW - 1 && thick[y][x + 1]);
            shirt[y][x] = thick[y][x] || (shirtcol[y][x] && (n >= 2 || in_set(c[y][x], "MDv")));
            blade[y][x] = white[y][x] && !hat[y][x] && !hatfx[y][x] && !smear[y][x] && !shirt[y][x];
        }

    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) bmask[y][x] = c[y][x] == 'b';
    mask_dilate(tmp, bmask, 1, false);
    signed char (*L)[CW] = s->lab;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            int rx = x - ox, ry = y - oy;
            bool hair_zone = rx >= 0 && rx <= 8 && ry >= 5 && ry <= 9;
            bool face_zone = rx >= 6 && rx <= 13 && ry >= 7 && ry <= 11;
            char k = c[y][x];
            bool darkc = in_set(k, "12369"), skinc = in_set(k, "Ssk");
            handle[y][x] = k == 'b' || (k == '1' && tmp[y][x] && !hair_zone);
            int lb = k != '.' ? OTHER : NONE;
            if (darkc) lb = DARK;
            if (skinc) lb = SKIN;
            if (k == 'h') lb = SAYA;
            if (handle[y][x]) lb = HANDLE;
            if (shirt[y][x]) lb = SHIRT;
            if (blade[y][x]) lb = BLADE;
            if (smear[y][x]) lb = SMEAR;
            if (s->has_hat) {
                if (darkc && hair_zone) lb = HAIR;
                if (skinc && face_zone) lb = FACE;
            }
            if (hatfx[y][x]) lb = HATFX;
            if (hat[y][x]) lb = HATL;
            if (k == '.') lb = NONE;
            L[y][x] = (signed char)lb;
        }
    find_blades(s);
}

static void free_seg(Seg *s) {
    for (int i = 0; i < s->nblades; i++) {
        free(s->blades[i].x);
        free(s->blades[i].y);
        free(s->blades[i].dist);
    }
    s->nblades = 0;
}

/* Maior autovetor de uma matriz simétrica 2 x 2 [[a, b], [b, c]]. */
static void principal_axis(double a, double b, double c, double u[2]) {
    if (b == 0) {
        if (a >= c) { u[0] = 1; u[1] = 0; }
        else { u[0] = 0; u[1] = 1; }
        return;
    }
    double l = (a + c) / 2 + sqrt((a - c) * (a - c) / 4 + b * b);
    double v1x = l - c, v1y = b, v2x = b, v2y = l - a;
    double n1 = hypot(v1x, v1y), n2 = hypot(v2x, v2y);
    if (n1 >= n2) { u[0] = v1x / n1; u[1] = v1y / n1; }
    else { u[0] = v2x / n2; u[1] = v2y / n2; }
}

/* Cada fio de lâmina vira uma reta: empunhadura, direção e ponta. */
static void find_blades(Seg *s) {
    static Mask bm;
    static Comps cs;
    signed char (*L)[CW] = s->lab;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) bm[y][x] = L[y][x] == BLADE;
    components(&cs, bm, 8);
    s->nblades = 0;
    for (int k = 0; k < cs.n && s->nblades < MAX_BLADES; k++) {
        int n = cs.len[k], st = cs.start[k];
        if (n < 4) continue;
        double mx = 0, my = 0;
        for (int i = 0; i < n; i++) { mx += cs.px[st + i]; my += cs.py[st + i]; }
        mx /= n; my /= n;
        double sxx = 0, sxy = 0, syy = 0;
        for (int i = 0; i < n; i++) {
            double dx = cs.px[st + i] - mx, dy = cs.py[st + i] - my;
            sxx += dx * dx; sxy += dx * dy; syy += dy * dy;
        }
        double u[2];
        principal_axis(sxx / (n - 1), sxy / (n - 1), syy / (n - 1), u);
        double tmin = 1e18, tmax = -1e18;
        double *t = malloc(sizeof(double) * (size_t)n);
        for (int i = 0; i < n; i++) {
            t[i] = (cs.px[st + i] - mx) * u[0] + (cs.py[st + i] - my) * u[1];
            if (t[i] < tmin) tmin = t[i];
            if (t[i] > tmax) tmax = t[i];
        }
        if (tmax - tmin < (g_pack ? 6 : 4)) { free(t); continue; }  /* pack: sobras finas da camisa não são lâmina */
        double e0x = mx + u[0] * tmin, e0y = my + u[1] * tmin, e1x = mx + u[0] * tmax, e1y = my + u[1] * tmax;
        bool found = false;
        double best = 0, bqx = 0, bqy = 0;
        for (int pass = 0; pass < 2; pass++) {
            if (pass == 1 && found && best <= 14) break;
            int want = pass == 0 ? HANDLE : SKIN;
            for (int y = 0; y < CH; y++)
                for (int x = 0; x < CW; x++) {
                    if (L[y][x] != want) continue;
                    double dx = x - mx, dy = y - my;
                    double pp = fabs(dx * u[1] - dy * u[0]);
                    double along = dx * u[0] + dy * u[1];
                    double beyond = fmax(0.0, fmax(tmin - along, along - tmax));
                    double inside = (tmin < along && along < tmax) ? 1.0 : 0.0;
                    double sc = pp * 2 + beyond + inside * 30 + (pass == 1 ? 3 : 0);
                    if (!found || sc < best) { found = true; best = sc; bqx = x; bqy = y; }
                }
        }
        double th;
        bool loose = false;
        if (found && best <= 20) {
            th = (bqx - mx) * u[0] + (bqy - my) * u[1];
        } else {
            loose = g_pack;
            double bx = s->ox + 8, by = s->oy + 18;
            th = hypot(e0x - bx, e0y - by) < hypot(e1x - bx, e1y - by) ? tmin : tmax;
        }
        if (fabs(th - tmin) > fabs(th - tmax)) {
            u[0] = -u[0]; u[1] = -u[1];
            for (int i = 0; i < n; i++) t[i] = -t[i];
            double a = tmin;
            tmin = -tmax; tmax = -a; th = -th;
        }
        Blade *b = &s->blades[s->nblades++];
        b->n = n;
        b->x = malloc(sizeof(short) * (size_t)n);
        b->y = malloc(sizeof(short) * (size_t)n);
        b->dist = malloc(sizeof(double) * (size_t)n);
        for (int i = 0; i < n; i++) {
            b->x[i] = cs.px[st + i];
            b->y[i] = cs.py[st + i];
            b->dist[i] = t[i] - th;
        }
        b->u[0] = u[0]; b->u[1] = u[1];
        b->hilt[0] = mx + u[0] * th; b->hilt[1] = my + u[1] * th;
        b->nearest = tmin - th;
        b->farthest = tmax - th;
        b->loose = loose;
        b->hand = found && best <= 20;
        free(t);
    }
}

/* A lâmina sai da mão: começa perto dela, ou começa mais longe mas a mão está
   no prolongamento (o chapéu ou a cabeça escondiam o pedaço de baixo). */
static bool blade_at_hand(const Blade *b) {
    return !b->loose && (b->nearest <= 8 || (b->hand && b->nearest <= 16));
}

/* Comprimento da katana do pack, do cabo à ponta (a mediana das lâminas que
   aparecem inteiras). Toda arma nova usa este número, então tem o mesmo tamanho
   em todos os quadros, por mais que a katana apareça cortada em algum deles. */
static double KATANA = 16.6;

/* `q`: 0,5 é a mediana (Samurai #3). Nos packs, a lâmina meio escondida se
   repete em vários quadros (o DEFEND), então vale o quartil de cima. */
static void measure_katana(const Seg *const *segs, int n, double q) {
    double v[1024];
    int m = 0;
    for (int i = 0; i < n; i++)
        for (int b = 0; b < segs[i]->nblades && m < 1024; b++)
            if (segs[i]->blades[b].nearest <= 3 && !segs[i]->blades[b].loose && segs[i]->blades[b].farthest >= 8) v[m++] = segs[i]->blades[b].farthest;
    if (m < 3) return;
    for (int i = 0; i < m; i++)
        for (int j = i + 1; j < m; j++)
            if (v[j] < v[i]) { double t = v[i]; v[i] = v[j]; v[j] = t; }
    KATANA = v[(int)(m * q)];
}

static double blade_dist(const Blade *b, int x, int y, double def) {
    for (int i = 0; i < b->n; i++)
        if (b->x[i] == x && b->y[i] == y) return b->dist[i];
    return def;
}

/* ------------------------------------------------------------------------ */
/* Personagens                                                               */
/* ------------------------------------------------------------------------ */
typedef enum { W_KATANA, W_DUPLA, W_ODACHI, W_PESADA, W_FLORETE, W_ADAGA, W_CURTA, W_LANCA, W_CAJADO, W_GARRAS, W_FOICE } WeaponKind;
static const char *WEAPON_NAMES[] = {"katana", "dupla", "odachi", "pesada", "florete", "adaga", "curta", "lanca", "cajado", "garras", "foice"};

typedef enum {
    EL_NONE, EL_FOGO, EL_AGUA, EL_RAIO, EL_ROXO, EL_PENA, EL_TERRA, EL_VENTO, EL_OURO, EL_SOMBRA, EL_MUSGO, EL_POEIRA,
    EL_LUA
} Element;

typedef enum { AC_NONE, AC_CACHECOL, AC_CASCO, AC_TRAPO, AC_CAUDA, AC_LISTRAS, AC_CABELO_LONGO } Accessory;

/* Golpe especial: a coreografia (de que quadros do pack ele é montado) e o
   efeito grande do elemento no ponto do impacto. */
typedef enum { SP_NENHUM, SP_SALTO, SP_INVESTIDA, SP_ESTOCADA, SP_ASCENDENTE } SpecialMove;
typedef enum {
    FX_NADA, FX_CHOQUE, FX_PEDRAS, FX_AVALANCHE, FX_FOGO, FX_RAIO, FX_ONDA, FX_GOTA, FX_X, FX_GARRA, FX_VORTICE,
    FX_CASCO, FX_SOMBRA
} SpecialFx;

typedef struct {
    WeaponKind kind;
    double escala;      /* comprimento em relação à katana */
    double comprimento; /* adaga, espada curta e garras, em px */
    int largura;        /* espada pesada: 2 ou 3 */
    Rgb cor_largura;
    bool par;           /* uma segunda arma na outra mão */
    double par_comprimento; /* katana: a segunda é uma lâmina mais curta, deste tamanho em px */
    bool reverso;       /* adaga empunhada ao contrário: a lâmina sai da mão para trás */
    Rgb brilho;         /* halo em volta da lâmina (a katana da lua do Jinshi) */
    Rgb cor_par;
    Rgb guarda;
    int atras, ponta;   /* lança e cajado: px atrás da mão, px da ponta */
    Rgb haste[2];
    Rgb ponteira;
} Weapon;

typedef struct {
    const char *id, *titulo;
    Weapon arma;
    const char *cabeca;  /* estilo da cabeça (quem não tem chapéu) */
    int chapeu;          /* 0 sem, 1 palha, 2 raiden */
    Rgb chapeu_cor[3];
    const char *rosto;   /* debaixo do chapéu */
    Rgb camisa[4], hakama[5], pele[3], cabelo[3];
    Rgb saya;
    bool sem_saya;
    Rgb cabo, lamina[2], rastro[3], destaque[2], destaque2, obi, olho, mascara[2];
    Element elemento;
    Accessory acessorios[3];
    int largura, altura; /* colunas e linhas a mais (ou a menos) no corpo */
    SpecialMove especial;
    SpecialFx efeito;
    const char *pack;                       /* corpo próprio: pasta em _packs/ (sem isso, o Samurai #3) */
    struct { Rgb de, para; } troca[24];     /* troca exata de cor no corpo do pack */
    struct { Rgb cor; char classe; } leitura[8]; /* como o separador lê as cores próprias do pack */
    bool ecos;                              /* Oboro: cada ataque em cada uma das onze posturas, e o grito */
    bool pack_par;                          /* o pack já vem com uma arma em cada mão */
    bool pack_sem_camisa;                   /* o pack não tem roupa branca: todo branco grosso é rastro */
    bool pack_arma;                         /* a arma do pack fica como é (só ganha a cor e o brilho do elemento) */
    bool sem_arma;                          /* Hanzo: não luta mais; sem espada, sem golpes */
    Rgb bainha[2];                          /* pack do Hanzo: cores da espada embainhada, que sai */
    Rgb rastro_pack[3];                     /* pack cujo rastro usa as cores da camisa: longe do corpo, vira rastro */
} Char;

static bool pack_no_shirt(void) {
    const Char *ch = g_pack_ch;
    return ch && ch->pack_sem_camisa;
}

static char pack_class(Color p) {
    const Char *ch = g_pack_ch;
    if (!ch) return 0;
    for (int i = 0; i < 8 && rgb_set(ch->leitura[i].cor); i++)
        if (ch->leitura[i].cor.r == p.r && ch->leitura[i].cor.g == p.g && ch->leitura[i].cor.b == p.b) return ch->leitura[i].classe;
    return 0;
}

static const Char ORIG = {
    .camisa = {{255, 255, 255}, {199, 207, 221}, {146, 161, 185}, {101, 115, 146}},
    .hakama = {{93, 93, 93}, {61, 61, 61}, {39, 39, 39}, {27, 27, 27}, {19, 19, 19}},
    .pele = {{230, 156, 105}, {191, 111, 74}, {138, 72, 54}},
    .cabelo = {{20, 18, 26}, {46, 44, 60}, {92, 90, 120}},
    .saya = {28, 18, 28},
    .cabo = {12, 46, 68},
    .lamina = {{250, 252, 252}, {199, 207, 221}},
    .rastro = {{255, 255, 255}, {236, 240, 248}, {199, 207, 221}},
    .destaque = {{214, 52, 52}, {140, 28, 36}},
    .olho = {24, 16, 20},
    .mascara = {{30, 30, 36}, {50, 50, 60}},
};

static Char CHARS[] = {
    /* O protagonista: sem chapéu e sem máscara, coque solto no alto da cabeça, katana. */
    {.id = "kojiro", .titulo = "Kojiro", .arma = {.kind = W_KATANA}, .cabeca = "coque"},
    /* 1. Terra. Katana, devagar. Chapéu de palha, verde oliva e ocre, barba. */
    {.id = "daichi", .titulo = "Daichi",
     .arma = {.kind = W_KATANA},
     .chapeu = 1, .chapeu_cor = {HEX(0xe0bc72), HEX(0xb48c48), HEX(0x7c5c2c)}, .rosto = "barba",
     .camisa = {HEX(0xc4c48a), HEX(0x9a9a5e), HEX(0x72743e), HEX(0x50522a)},
     .hakama = {HEX(0x6a5238), HEX(0x503c28), HEX(0x3a2c1e), HEX(0x2a2016), HEX(0x1c150e)},
     .pele = {HEX(0xd49468), HEX(0xac6c46), HEX(0x78462e)},
     .cabelo = {HEX(0x2a1c10), HEX(0x4a3420), HEX(0x6c5034)},
     .destaque = {HEX(0xe0a030), HEX(0xa06c18)}, .obi = HEX(0xc08a2a),
     .saya = HEX(0x3a2c1e), .cabo = HEX(0x6a4a1c),
     .lamina = {HEX(0xd8d8cc), HEX(0x9c9a8a)},
     .rastro = {HEX(0xfbf0d0), HEX(0xe0b868), HEX(0xa47a3a)}, .elemento = EL_TERRA, .largura = 1,
     .especial = SP_SALTO, .efeito = FX_PEDRAS},
    /* 2. Tartaruga. Katana simples e o casco nas costas. Verde. */
    {.id = "genbu", .titulo = "Genbu", .arma = {.kind = W_KATANA}, .cabeca = "careca",
     .acessorios = {AC_CASCO},
     .camisa = {HEX(0xb6d0a0), HEX(0x88ac74), HEX(0x5e8452), HEX(0x3e5e38)},
     .hakama = {HEX(0x3e5a40), HEX(0x2e4632), HEX(0x223424), HEX(0x18261a), HEX(0x101a12)},
     .pele = {HEX(0xd8a078), HEX(0xb07852), HEX(0x7a4c36)},
     .cabelo = {HEX(0x8a8a86), HEX(0xb8b8b2), HEX(0xdcdcd6)},
     .destaque = {HEX(0x4c9a3c), HEX(0x22502a)}, .destaque2 = HEX(0x8ad06a), .obi = HEX(0x2e6a2a),
     .saya = HEX(0x22502a), .cabo = HEX(0x2e4632),
     .rastro = {HEX(0xeaffdc), HEX(0x8ee070), HEX(0x3e9a3a)}, .elemento = EL_MUSGO, .largura = 1, .altura = -1,
     .especial = SP_ASCENDENTE, .efeito = FX_CASCO},
    /* 3. Touro. Espadão: odachi mais comprida e bem mais larga. Marrom e amarelo. */
    {.id = "raizo", .titulo = "Raizo",
     .arma = {.kind = W_ODACHI, .escala = 1.45, .largura = 3, .cor_largura = HEX(0xb8b4a8)}, .cabeca = "touro",
     .camisa = {HEX(0xa8703e), HEX(0x82522a), HEX(0x5e381c), HEX(0x402412)},
     .hakama = {HEX(0x4a3a2c), HEX(0x382c22), HEX(0x2a2019), HEX(0x1f1712), HEX(0x150f0c)},
     .pele = {HEX(0xe8a878), HEX(0xc07a4e), HEX(0x84503a)},
     .cabelo = {HEX(0x1c140e), HEX(0x3a2a1c), HEX(0x5e4630)},
     .destaque = {HEX(0xffd23c), HEX(0xc08a14)}, .obi = HEX(0xffd23c),
     .saya = HEX(0x3a2412), .cabo = HEX(0x7a4a14),
     .rastro = {HEX(0xfff4c8), HEX(0xffd84a), HEX(0xc89a2a)}, .elemento = EL_OURO, .largura = 2,
     .especial = SP_SALTO, .efeito = FX_CHOQUE,
     /* corpo do samurai do espadão (chapéu de palha): o espadão e o chapéu ficam como vêm,
        a roupa preta vira marrom; o corpo largo da lâmina não é camisa nem rastro */
     .pack = "espadao", .pack_arma = true, .pack_sem_camisa = true,
     .leitura = {{HEX(0xf6ca9f), 'S'}, {HEX(0x657392), '?'}, {HEX(0x424c6e), '?'}},
     .troca = {{HEX(0x131313), HEX(0x24160e)}, {HEX(0x272727), HEX(0x4a3020)}, {HEX(0x3d3d3d), HEX(0x74502e)}}},
    /* 4. Água. Florete. Azul claro e ciano. */
    {.id = "shizuku", .titulo = "Shizuku", .arma = {.kind = W_FLORETE, .escala = 1.15}, .cabeca = "rabo",
     .camisa = {HEX(0xeef8ff), HEX(0xbfe2f6), HEX(0x86bde6), HEX(0x5a8cc4)},
     .hakama = {HEX(0x4a78b0), HEX(0x36609a), HEX(0x284a7c), HEX(0x1d3862), HEX(0x142848)},
     .pele = {HEX(0xf2c29c), HEX(0xd69a74), HEX(0xa86a4e)},
     .cabelo = {HEX(0x1a2a52), HEX(0x2e4a82), HEX(0x5a7cc0)},
     .destaque = {HEX(0x58f0ff), HEX(0x1aa6c8)}, .obi = HEX(0x58f0ff),
     .saya = HEX(0xdff4ff), .cabo = HEX(0x1aa6c8),
     .lamina = {HEX(0xf4fbff), HEX(0xa8d8f0)},
     .rastro = {HEX(0xf0fdff), HEX(0x9ff0ff), HEX(0x4ac0e8)}, .elemento = EL_AGUA, .largura = -1,
     .especial = SP_ESTOCADA, .efeito = FX_GOTA,
     /* corpo do Samurai #4: o cabelo roxo vira azul petróleo, a roupa vai para o azul da água */
     .pack = "samurai4",
     .leitura = {{HEX(0xf6ca9f), 'S'}, {HEX(0xf9e6cf), 'S'}},
     .troca = {{HEX(0x0e071b), HEX(0x08202e)}, {HEX(0x3b1443), HEX(0x15506c)}, {HEX(0x622461), HEX(0x2a8eac)},
               {HEX(0xffffff), HEX(0xeef8ff)}, {HEX(0xc7cfdd), HEX(0xbfe2f6)}, {HEX(0x92a1b9), HEX(0x86bde6)},
               {HEX(0x657392), HEX(0x5a8cc4)},
               {HEX(0x1a1932), HEX(0x14264a)}, {HEX(0x2a2f4e), HEX(0x1f3f70)}, {HEX(0x424c6e), HEX(0x3462a0)},
               {HEX(0x571c27), HEX(0x1a86b0)}, {HEX(0x891e2b), HEX(0x58e0ff)}, {HEX(0x5ac54f), HEX(0x7ae8ff)}}},
    /* 5. Tigre (Byakko). Garras nas duas mãos, cauda. Cabelo loiro listrado, roupa preta com detalhes vermelhos. */
    {.id = "garfiel", .titulo = "Garfiel", .arma = {.kind = W_GARRAS, .comprimento = 10, .par = true}, .cabeca = "tigre",
     .acessorios = {AC_CAUDA, AC_LISTRAS},
     .camisa = {HEX(0x54505c), HEX(0x3a3642), HEX(0x28252e), HEX(0x1a181e)},
     .hakama = {HEX(0x34303a), HEX(0x28252c), HEX(0x1e1c22), HEX(0x151318), HEX(0x0d0c10)},
     .pele = {HEX(0xeab088), HEX(0xc6845e), HEX(0x8c5640)},
     .cabelo = {HEX(0xe8b83c), HEX(0xb88428), HEX(0xfff0a0)},
     .mascara = {HEX(0x18161a), HEX(0x2c2a30)}, .olho = HEX(0xffb020),
     .destaque = {HEX(0xe0302c), HEX(0xa01820)}, .obi = HEX(0xd02828),
     .sem_saya = true, .cabo = HEX(0x2c2c32),
     .lamina = {HEX(0xfffaf0), HEX(0xc89040)},
     .rastro = {HEX(0xfff6e0), HEX(0xffc860), HEX(0xc08030)}, .elemento = EL_TERRA, .largura = 1,
     .especial = SP_INVESTIDA, .efeito = FX_GARRA,
     /* corpo do samurai de duas espadas (uma garra em cada mão): roupa preta listrada de vermelho, cabelo loiro */
     .pack = "samurai5", .pack_par = true, .pack_sem_camisa = true,
     .leitura = {{HEX(0xf6ca9f), 'S'}, {HEX(0x0c2e44), '?'}},
     .troca = {{HEX(0x1e6f50), HEX(0x3a3642)}, {HEX(0x134c4c), HEX(0x28252e)}, {HEX(0x0c2e44), HEX(0x1a181e)},
               {HEX(0x391f21), HEX(0xa01820)}, {HEX(0x5d2c28), HEX(0xd02828)},
               {HEX(0x272727), HEX(0xb88428)}, {HEX(0x3d3d3d), HEX(0xe8b83c)}, {HEX(0x5ac54f), HEX(0xffb020)}}},
    /* 6. Corvo. Katana na mão da frente e wakizashi (a curta) na outra. Preto e vermelho. */
    {.id = "karasu", .titulo = "Karasu",
     .arma = {.kind = W_KATANA, .escala = 0.95, .par = true, .par_comprimento = 13, .cor_par = HEX(0xece4e6),
              .guarda = HEX(0x8c1018)},
     .cabeca = "corvo",
     .acessorios = {AC_TRAPO},
     .camisa = {HEX(0x5a5058), HEX(0x3e363e), HEX(0x2a242a), HEX(0x1c181c)},
     .hakama = {HEX(0x3a2a2e), HEX(0x2c1e22), HEX(0x201518), HEX(0x170f11), HEX(0x0f0a0b)},
     .pele = {HEX(0xecc0a0), HEX(0xc89478), HEX(0x8c6250)},
     .cabelo = {HEX(0x100c10), HEX(0x241c26), HEX(0x3c3040)},
     .olho = HEX(0xff2a2a),
     .destaque = {HEX(0xe0202c), HEX(0x8c1018)}, .obi = HEX(0xc0182a),
     .sem_saya = true, .cabo = HEX(0x3a2a2e),
     .lamina = {HEX(0xf4eef0), HEX(0x7a1820)},
     .rastro = {HEX(0xffe0e0), HEX(0xff4a4a), HEX(0xa01020)}, .elemento = EL_PENA, .largura = -1, .altura = 1,
     .especial = SP_INVESTIDA, .efeito = FX_X,
     /* corpo do samurai de duas espadas: a espada na direita e a lâmina mais curta na esquerda */
     .pack = "samurai5", .pack_par = true, .pack_sem_camisa = true,
     .leitura = {{HEX(0xf6ca9f), 'S'}, {HEX(0x0c2e44), '?'}},
     .troca = {{HEX(0x1e6f50), HEX(0x5a5058)}, {HEX(0x134c4c), HEX(0x3e363e)}, {HEX(0x0c2e44), HEX(0x1c181c)},
               {HEX(0x391f21), HEX(0x8c1018)}, {HEX(0x5d2c28), HEX(0xc0182a)},
               {HEX(0x272727), HEX(0x100c10)}, {HEX(0x3d3d3d), HEX(0x241c26)}, {HEX(0x5ac54f), HEX(0xff2a2a)}}},
    /* 7. Vento. Duas foices (kama), uma em cada mão, e cortes de vento. Verde claro e limão, cachecol. */
    {.id = "hayate", .titulo = "Hayate",
     .arma = {.kind = W_FOICE, .par = true, .haste = {HEX(0x8a6a44), HEX(0x5a4228)}},
     .cabeca = "vento", .acessorios = {AC_CACHECOL}, .sem_saya = true,
     .camisa = {HEX(0xeefce0), HEX(0xc2eca8), HEX(0x8ccc78), HEX(0x5c9c54)},
     .hakama = {HEX(0x4a6448), HEX(0x384e38), HEX(0x283a2a), HEX(0x1c2a1e), HEX(0x131e15)},
     .pele = {HEX(0xeab088), HEX(0xc6845e), HEX(0x8c5640)},
     .cabelo = {HEX(0x16261a), HEX(0x2c4a30), HEX(0x4c7a4c)},
     .destaque = {HEX(0xc8ff3c), HEX(0x7cc81c)},
     .saya = HEX(0x2c4a30), .cabo = HEX(0x5c9c1c),
     .lamina = {HEX(0xf4fff0), HEX(0xbce8b0)},
     .rastro = {HEX(0xf6ffe8), HEX(0xd4ff7a), HEX(0x8ad04a)}, .elemento = EL_VENTO,
     .especial = SP_ASCENDENTE, .efeito = FX_VORTICE,
     /* corpo do samurai de duas espadas (uma foice em cada mão): verde claro e limão */
     .pack = "samurai5", .pack_par = true, .pack_sem_camisa = true,
     .leitura = {{HEX(0xf6ca9f), 'S'}, {HEX(0x0c2e44), '?'}},
     .troca = {{HEX(0x1e6f50), HEX(0x8ccc78)}, {HEX(0x134c4c), HEX(0x5c9c54)}, {HEX(0x0c2e44), HEX(0x2c4a30)},
               {HEX(0x391f21), HEX(0x7cc81c)}, {HEX(0x5d2c28), HEX(0xc8ff3c)},
               {HEX(0x272727), HEX(0x16261a)}, {HEX(0x3d3d3d), HEX(0x2c4a30)}, {HEX(0x5ac54f), HEX(0xc8ff3c)}}},
    /* 8. Chama. Espada de fogo. Vermelho e amarelo. */
    {.id = "enjin", .titulo = "Enjin", .arma = {.kind = W_KATANA}, .cabeca = "chamas",
     .camisa = {HEX(0xf0584a), HEX(0xc02a2e), HEX(0x861a24), HEX(0x58101c)},
     .hakama = {HEX(0x4a1a16), HEX(0x361210), HEX(0x280d0c), HEX(0x1c0909), HEX(0x130606)},
     .pele = {HEX(0xe6a078), HEX(0xc07650), HEX(0x864a34)},
     .cabelo = {HEX(0xb0200c), HEX(0xff6a1c), HEX(0xffd048)},
     .destaque = {HEX(0xffb020), HEX(0xd8501a)}, .obi = HEX(0xffb020),
     .saya = HEX(0x1c0909), .cabo = HEX(0xa82a1e),
     .lamina = {HEX(0xfff0b0), HEX(0xff9030)},
     .rastro = {HEX(0xfff4b8), HEX(0xffa030), HEX(0xe04420)}, .elemento = EL_FOGO, .largura = 1,
     .especial = SP_SALTO, .efeito = FX_FOGO},
    /* 9. Mar. Lança de água. Azul mar e turquesa. */
    {.id = "suiren", .titulo = "Suiren",
     .arma = {.kind = W_LANCA, .escala = 1.2, .atras = 14, .ponta = 5, .haste = {HEX(0x5a9cc0), HEX(0x24506e)}},
     .cabeca = "faixa",
     .camisa = {HEX(0x9ed8f0), HEX(0x4aa0d4), HEX(0x2a70b0), HEX(0x1c4c84)},
     .hakama = {HEX(0x1e3c64), HEX(0x172e50), HEX(0x11223c), HEX(0x0c182c), HEX(0x08101e)},
     .pele = {HEX(0xdca07a), HEX(0xb47654), HEX(0x7e4c38)},
     .cabelo = {HEX(0x0e1c34), HEX(0x1e3a64), HEX(0x3a64a0)},
     .destaque = {HEX(0x3cf0d8), HEX(0x14a8a0)}, .obi = HEX(0x3cf0d8),
     .sem_saya = true, .cabo = HEX(0x14a8a0),
     .lamina = {HEX(0xd8fffa), HEX(0x3cf0d8)},
     .rastro = {HEX(0xe0fffc), HEX(0x5cf0e0), HEX(0x1c9cc8)}, .elemento = EL_AGUA,
     .especial = SP_ESTOCADA, .efeito = FX_ONDA},
    /* 10. Tempestade. Duas espadas com raios. Preto com o chapéu do Raiden. */
    {.id = "arashi", .titulo = "Arashi", .arma = {.kind = W_DUPLA, .par = true, .cor_par = HEX(0x7cc0ff)},
     .chapeu = 2, .chapeu_cor = {HEX(0xf2eee0), HEX(0xcfc6a8), HEX(0x948a6e)}, .rosto = "olho_raio",
     .camisa = {HEX(0x4e5264), HEX(0x363a4a), HEX(0x262a36), HEX(0x1a1c26)},
     .hakama = {HEX(0x2e3240), HEX(0x222530), HEX(0x181a24), HEX(0x111219), HEX(0x0a0b10)},
     .pele = {HEX(0xdca880), HEX(0xb47e5c), HEX(0x7c5040)},
     .cabelo = {HEX(0x101218), HEX(0x20242e), HEX(0x343a4a)},
     .olho = HEX(0xb4f0ff),
     .destaque = {HEX(0x3ca8ff), HEX(0x1a5ad0)}, .obi = HEX(0x3ca8ff),
     .saya = HEX(0x111219), .cabo = HEX(0x1a5ad0),
     .lamina = {HEX(0xeef8ff), HEX(0x5cb4ff)},
     .rastro = {HEX(0xf4faff), HEX(0x7cc8ff), HEX(0x2a6cf0)}, .elemento = EL_RAIO,
     .especial = SP_INVESTIDA, .efeito = FX_RAIO,
     /* corpo do Samurai #5 (já com as duas espadas): o verde vira preto, cinto e botas em azul
        elétrico, cabelo prateado e olhos de raio */
     .pack = "samurai5", .pack_par = true, .pack_sem_camisa = true,
     .leitura = {{HEX(0xf6ca9f), 'S'}, {HEX(0x0c2e44), '?'}},
     .troca = {{HEX(0x1e6f50), HEX(0x3a4056)}, {HEX(0x134c4c), HEX(0x252a3a)}, {HEX(0x0c2e44), HEX(0x14161f)},
               {HEX(0x391f21), HEX(0x1a4cc0)}, {HEX(0x5d2c28), HEX(0x3ca8ff)},
               {HEX(0x272727), HEX(0x8e9ab4)}, {HEX(0x3d3d3d), HEX(0xd4def0)}, {HEX(0x5ac54f), HEX(0xb4f0ff)}}},
    /* 11. Noite. Uma adaga em cada mão, que brilham roxo. Ninja preto e roxo. */
    {.id = "yoru", .titulo = "Yoru",
     .arma = {.kind = W_ADAGA, .comprimento = 8, .par = true, .reverso = true, .cor_par = HEX(0xd8b0ff), .guarda = HEX(0x4a1c7a)},
     .cabeca = "capuz",
     .camisa = {HEX(0x4a4660), HEX(0x34324a), HEX(0x252438), HEX(0x1a1a28)},
     .hakama = {HEX(0x343044), HEX(0x26243a), HEX(0x1c1a2c), HEX(0x141322), HEX(0x0d0c18)},
     .pele = {HEX(0xe0a47c), HEX(0xb87a56), HEX(0x7e4c36)},
     .cabelo = {HEX(0x141320), HEX(0x26243a), HEX(0x3c3a56)},
     .mascara = {HEX(0x141320), HEX(0x2a2840)}, .olho = HEX(0xe8c8ff),
     .destaque = {HEX(0xb45cff), HEX(0x6e2cb4)}, .obi = HEX(0x8a3ce0),
     .sem_saya = true, .cabo = HEX(0x4a1c7a),
     .lamina = {HEX(0xf4e0ff), HEX(0xb45cff)},
     .rastro = {HEX(0xf6e6ff), HEX(0xc88cff), HEX(0x7a3cd8)}, .elemento = EL_ROXO, .largura = -1,
     .especial = SP_INVESTIDA, .efeito = FX_X,
     /* corpo do samurai de duas espadas (uma adaga em cada mão): ninja preto e roxo */
     .pack = "samurai5", .pack_par = true, .pack_sem_camisa = true,
     .leitura = {{HEX(0xf6ca9f), 'S'}, {HEX(0x0c2e44), '?'}},
     .troca = {{HEX(0x1e6f50), HEX(0x4a3e6a)}, {HEX(0x134c4c), HEX(0x2e2844)}, {HEX(0x0c2e44), HEX(0x1a1628)},
               {HEX(0x391f21), HEX(0x4a1c7a)}, {HEX(0x5d2c28), HEX(0x8a3ce0)},
               {HEX(0x272727), HEX(0x141320)}, {HEX(0x3d3d3d), HEX(0x26243a)}, {HEX(0x5ac54f), HEX(0xe8c8ff)}}},
    /* 12. Lua. Corpo do Samurai #4 (o mesmo da Shizuku): roupa roxa com verde, o
       cabelo roxo do pack mais comprido, caindo pelas costas, e a katana bem branca,
       com brilho. */
    {.id = "jinshi", .titulo = "Jinshi",
     .arma = {.kind = W_KATANA, .brilho = HEX(0xe8eeff)},
     .acessorios = {AC_CABELO_LONGO},
     .camisa = {HEX(0xc9a6e8), HEX(0x9d74c8), HEX(0x7450a0), HEX(0x503678)},
     .hakama = {HEX(0x2a5a3a), HEX(0x1a3e28), HEX(0x12301e), HEX(0x0e2418), HEX(0x08180f)},
     .pele = {HEX(0xf2c29c), HEX(0xd69a74), HEX(0xa86a4e)},
     .cabelo = {HEX(0x1c0c30), HEX(0x4e1e68), HEX(0x8040a0)},
     .destaque = {HEX(0x6ae0a0), HEX(0x2f8a5a)}, .obi = HEX(0x5ad08a),
     .saya = HEX(0xe8ecf4), .cabo = HEX(0x2f8a5a),
     .lamina = {HEX(0xffffff), HEX(0xf2f5ff)},
     .rastro = {HEX(0xffffff), HEX(0xe6ecff), HEX(0xb8c4ec)}, .elemento = EL_LUA, .largura = -1,
     .especial = SP_SALTO, .efeito = FX_CHOQUE,
     .rastro_pack = {HEX(0xffffff), HEX(0xc7cfdd), HEX(0x92a1b9)},
     .pack = "samurai4",
     .leitura = {{HEX(0xf6ca9f), 'S'}, {HEX(0xf9e6cf), 'S'}},
     .troca = {{HEX(0x0e071b), HEX(0x1c0c30)}, {HEX(0x3b1443), HEX(0x4e1e68)}, {HEX(0x622461), HEX(0x8040a0)},
               {HEX(0xffffff), HEX(0xc9a6e8)}, {HEX(0xc7cfdd), HEX(0x9d74c8)}, {HEX(0x92a1b9), HEX(0x7450a0)},
               {HEX(0x657392), HEX(0x503678)},
               {HEX(0x1a1932), HEX(0x0e2418)}, {HEX(0x2a2f4e), HEX(0x1a3e28)}, {HEX(0x424c6e), HEX(0x2a5a3a)},
               {HEX(0x571c27), HEX(0x2f8a5a)}, {HEX(0x891e2b), HEX(0x5ad08a)}, {HEX(0x5ac54f), HEX(0x1fc45a)}}},
    /* Oboro, o último da trilha. Katana de Hanzo. Roxo escuro e dourado. */
    {.id = "oboro", .titulo = "Oboro", .arma = {.kind = W_KATANA}, .cabeca = "rabo_longo",
     .camisa = {HEX(0x8a6ab0), HEX(0x5e4488), HEX(0x3e2c62), HEX(0x281c42)},
     .hakama = {HEX(0x2a2030), HEX(0x201826), HEX(0x18121c), HEX(0x110d14), HEX(0x0b080d)},
     .pele = {HEX(0xe6b894), HEX(0xc08c6c), HEX(0x845c48)},
     .cabelo = {HEX(0x0e0a12), HEX(0x221a2c), HEX(0x3e3250)},
     .destaque = {HEX(0xffcc40), HEX(0xb08018)}, .destaque2 = HEX(0xffcc40), .obi = HEX(0xffcc40),
     .saya = HEX(0x18121c), .cabo = HEX(0xb08018),
     .lamina = {HEX(0xfffbe8), HEX(0xe8c060)},
     .rastro = {HEX(0xfff6dc), HEX(0xb48cff), HEX(0x6a3cc0)}, .elemento = EL_SOMBRA, .altura = 1,
     .especial = SP_INVESTIDA, .efeito = FX_SOMBRA,
     /* corpo do Demon (oni), com as cores dele; ataca em cada postura dos onze e tem a cena do grito */
     .pack = "demon", .ecos = true, .pack_sem_camisa = true,
     .leitura = {{HEX(0xf6ca9f), 'S'}, {HEX(0x0c2e44), '?'}}},
    /* Hanzo: um velho que não luta mais. Sem espada; cabelo branco, sem barba, azul escuro. */
    {.id = "hanzo", .titulo = "Hanzo", .arma = {.kind = W_KATANA}, .cabeca = "mestre", .sem_arma = true, .sem_saya = true,
     .pack = "hanzo", .bainha = {HEX(0x571c27), HEX(0x391f21)},
     .camisa = {HEX(0x8ca0c8), HEX(0x5a70a0), HEX(0x3c4e7c), HEX(0x283658)},
     .hakama = {HEX(0x2a3450), HEX(0x20283e), HEX(0x181e30), HEX(0x121624), HEX(0x0c0f18)},
     .pele = {HEX(0xe0a67e), HEX(0xb87c5a), HEX(0x82543e)},
     .cabelo = {HEX(0xa8a8a8), HEX(0xd4d4d0), HEX(0xf4f4f0)},
     .destaque = {HEX(0xc42a2a), HEX(0x7a1414)}, .obi = HEX(0xc42a2a),
     .saya = HEX(0xa01c1c), .cabo = HEX(0x20283e), .altura = -1},
};
#define NCHARS ((int)(sizeof CHARS / sizeof CHARS[0]))

/* Completa com o sprite original o que o personagem não definiu. */
static void fill_defaults(Char *ch) {
#define DEF(f) do { bool any = false; for (size_t i = 0; i < sizeof ch->f / sizeof(Rgb); i++) any |= rgb_set(((Rgb *)&ch->f)[i]); \
                    if (!any) memcpy(&ch->f, &ORIG.f, sizeof ch->f); } while (0)
    DEF(camisa); DEF(hakama); DEF(pele); DEF(cabelo); DEF(saya); DEF(cabo);
    DEF(lamina); DEF(rastro); DEF(destaque); DEF(olho); DEF(mascara);
#undef DEF
    if (!rgb_set(ch->destaque2)) ch->destaque2 = ch->destaque[0];
}

/* ------------------------------------------------------------------------ */
/* Cabeças                                                                   */
/* ------------------------------------------------------------------------ */
/* Coordenadas relativas ao canto do chapéu (ox, oy). O rosto do sprite fica em
   x 7..11, y 8..10; a nuca em x 4; a gola começa em y 9.
   Cada linha é (y, x inicial, texto). Letras:
     H h i  cabelo escuro, meio, luz      F f k  pele clara, meio, escura
     e      olho                          A a    destaque claro, escuro        b      barba por fazer
     B      destaque 2 (ouro, anel)       K m    máscara escura, meio
     X      apaga                         .      não mexe
   'frente' pinta por cima do chapéu e do rosto; 'atras' só no vazio. */
typedef struct { int y, x; const char *t; } Row;
typedef struct { int y, x, n; } Tape;
typedef struct {
    const char *name;
    Row frente[20];
    Row atras[2][14];
    Tape fitas[3];
} Head;

static const Head HEADS[] = {
    /* Kojiro (o Musashi de Vagabond): cabelo rente, todo puxado para cima e amarrado
       com fita vermelha no alto da cabeça, de onde sai um tufo curto e desgrenhado que
       cai para trás, em fiapos (as pontas mexem com o vento); a nuca
       curta, sem cabelo descendo. Rosto liso, sem nariz nem olho, e a barba por fazer
       (b: a pele puxada para o cinza) no queixo e no maxilar. */
    {"coque", {{-2, 5, "hHHh"}, {-1, 4, "hHiHHh"}, {0, 3, "hH.HHHHh"}, {1, 3, "h.hHHHh"},
               {2, 7, "aAa"}, {3, 4, "h"}, {3, 7, "HHhhH"}, {4, 6, "HHhHHhH"},
               {5, 6, "HHHhHHH"}, {6, 5, "XHHHHfFF"}, {7, 5, "XHHHkfFF"}, {8, 4, "XXXHkbfbb"},
               {9, 4, "XXXXkkbbX"}, {10, 4, "XXXXkkXXX"}, {11, 4, "XXX"}},
     {{{-1, 3, "h"}},
      {{2, 3, "h"}}}},
    /* Hanzo: coque grande de cabelo branco, testa alta, sem barba. */
    {"mestre", {{1, 4, "HHH"}, {2, 3, "HhiiH"}, {3, 4, "HhhH"}, {4, 5, "aA"}, {5, 5, "HHhiH"},
                {6, 4, "HHHhiiF"}, {7, 4, "HHHhFFFF"}, {8, 4, "HHHfFkeF"}}},
    /* Raijin: cabelo curto com dois tufos de chifre e faixa amarela. */
    {"touro", {{1, 4, "H"}, {1, 10, "H"}, {2, 4, "Hh"}, {2, 9, "hH"}, {3, 4, "HhHHiH"}, {4, 3, "HHHHhiiH"},
               {5, 3, "HHHHHHHHH"}, {6, 3, "aAAAAAAAA"}, {7, 3, "HHHHHfFFF"}, {8, 4, "HHHfFFeF"}},
     {{{6, 1, "AA"}, {7, 0, "Aa"}, {8, -1, "Aa"}, {9, -1, "a"}},
      {{6, 1, "AA"}, {7, 1, "aA"}, {8, 0, "aA"}, {9, 0, "a"}}}},
    /* Shizuku: franja e rabo de cavalo alto. */
    {"rabo", {{3, 4, "A"}, {4, 4, "aHHhH"}, {5, 4, "HHHHhiH"}, {6, 4, "HHHHHHiH"}, {7, 4, "HHHHHHHH"},
              {8, 4, "HHHfFFeH"}, {9, 4, "H"}},
     {{{3, 2, "HH"}, {4, 1, "HH"}, {5, 0, "HHH"}, {6, 0, "HH"}, {7, -1, "HH"}, {8, -1, "HH"}, {9, -1, "H"}, {10, -2, "H"}},
      {{3, 2, "HH"}, {4, 1, "HH"}, {5, 1, "HHH"}, {6, 0, "HH"}, {7, 0, "HH"}, {8, -1, "HH"}, {9, -1, "H"}, {10, -1, "H"}}}},
    /* Kage: capuz ninja, só a fresta dos olhos, fitas roxas atrás. */
    {"capuz", {{3, 6, "KK"}, {4, 5, "KKmK"}, {5, 4, "KKKKmK"}, {6, 4, "KKKKKmmK"}, {7, 4, "KAAAAAAA"},
               {8, 4, "KKKKFeFF"}, {9, 8, "KKKK"}, {10, 8, "KKK"}},
     {{{0}}}, {{7, 3, 9}, {8, 3, 6}}},
    /* Hayate: cabelo espetado varrido pelo vento. */
    {"vento", {{3, 7, "H"}, {4, 5, "HHhH"}, {5, 2, "HH.HHHhiH"}, {6, 1, "HHHHHHHhiHH"}, {7, 3, "HH.HHHHHHF"},
               {8, 4, "HHHfFFeF"}}},
    /* Genbu: careca, bigode e barbicha grisalhos. */
    {"careca", {{5, 5, "fFFFf"}, {6, 4, "fFFiFFf"}, {7, 4, "fFFFFFFF"}, {8, 4, "kffkFFeF"}, {10, 8, "hhh"},
                {11, 9, "hh"}}},
    /* Enjin: cabelo em chamas, alto. */
    {"chamas", {{0, 8, "H"}, {1, 6, "H.Hh"}, {2, 5, "HhHhi"}, {3, 3, "H.HhhiH"}, {4, 2, "HHHhhiiH"},
                {5, 3, "HHHHhhiH"}, {6, 3, "HHHHHhhiH"}, {7, 4, "HHHHHHhF"}, {8, 4, "HHHfFFeF"}}},
    /* Suiren: cabelo curto e faixa turquesa com pontas soltas. */
    {"faixa", {{4, 5, "HHhH"}, {5, 4, "HHHhiiH"}, {6, 4, "AAAAAAAA"}, {7, 4, "HHHHHHHF"}, {8, 4, "HHHfFFeF"}},
     {{{6, 2, "aA"}, {7, 0, "aA"}, {8, -1, "a"}},
      {{6, 2, "aA"}, {7, 1, "aa"}, {8, 0, "a"}, {8, -2, "a"}}}},
    /* Garfiel: cabelo loiro espetado de tigre, listras pretas, olho âmbar. */
    {"tigre", {{2, 5, "i"}, {2, 9, "i"}, {3, 4, "iHi.iHi"}, {4, 4, "HHKHHHKH"}, {5, 3, "HHKHHHKHHi"},
               {6, 3, "HHHHHHHHHi"}, {7, 4, "HHHHHhHF"}, {8, 4, "HHHfFFeF"}}},
    /* Karasu: cabelo em penas para trás, olho vermelho. */
    {"corvo", {{4, 5, "HHHH"}, {5, 2, "HH.HHHhiH"}, {6, 0, "HHHHHHHHhiH"}, {7, 2, "HHHHHHHHHF"},
               {8, 3, "HHHHfFeF"}, {9, 2, "HH"}}},
    /* Jinshi: cabelo grisalho comprido caindo nas costas, barba. */
    {"eremita", {{4, 5, "HHhH"}, {5, 4, "HHHhiH"}, {6, 3, "HHHHHhiH"}, {7, 3, "HHHHHHHF"}, {8, 3, "HHHHfFeF"},
                 {9, 3, "H"}, {10, 8, "hhh"}, {11, 8, "hhh"}, {12, 9, "h"}},
     {{{9, 2, "HH"}, {10, 1, "Hh"}, {11, 1, "Hh"}, {12, 1, "Hh"}, {13, 1, "H"}, {14, 1, "H"}},
      {{9, 2, "HH"}, {10, 1, "Hh"}, {11, 0, "Hh"}, {12, 0, "Hh"}, {13, 0, "H"}, {14, 1, "H"}}}},
    /* Oboro: rabo de cavalo alto e comprido, anel de ouro, mecha no rosto. */
    {"rabo_longo", {{2, 4, "HH"}, {3, 3, "HBH"}, {4, 4, "HHHhH"}, {5, 4, "HHHHhiH"}, {6, 4, "HHHHHHiH"},
                    {7, 4, "HHHHHHHHH"}, {8, 4, "HHHfFFeH"}, {9, 11, "H"}},
     {{{2, 2, "HH"}, {3, 1, "Hh"}, {4, 0, "Hh"}, {5, -1, "Hh"}, {6, -1, "Hh"}, {7, -2, "Hh"}, {8, -2, "Hh"},
       {9, -3, "Hh"}, {10, -3, "H"}, {11, -4, "H"}},
      {{2, 2, "HH"}, {3, 1, "Hh"}, {4, 0, "Hh"}, {5, 0, "Hh"}, {6, -1, "Hh"}, {7, -1, "Hh"}, {8, -2, "Hh"},
       {9, -2, "Hh"}, {10, -3, "H"}, {11, -3, "H"}}}},
};

/* Rosto de quem fica com chapéu (só o que aparece debaixo da aba). */
static const Row FACE_BARBA[] = {{9, 8, "h"}, {10, 8, "HHH"}, {11, 9, "HH"}, {0, 0, NULL}};
static const Row FACE_OLHO_RAIO[] = {{8, 10, "e"}, {0, 0, NULL}};

static const Head *find_head(const char *name) {
    for (size_t i = 0; i < sizeof HEADS / sizeof HEADS[0]; i++)
        if (!strcmp(HEADS[i].name, name)) return &HEADS[i];
    return NULL;
}

/* ------------------------------------------------------------------------ */
/* Desenho                                                                   */
/* ------------------------------------------------------------------------ */
/* Camada de cada pixel: o que é corpo (a aura contorna), o que é arma ou rastro
   (o alcance mede) e o que é efeito solto. */
enum { T_NONE, T_BODY, T_WEAPON, T_FX };

typedef struct {
    Color a[CH][CW];
    unsigned char tag[CH][CW];
    bool empty[CH][CW];  /* vazio no quadro original: dá para desenhar atrás */
    bool wpx[CH][CW];    /* pixels da arma nova */
    int pen;             /* camada de quem está desenhando agora */
    const Seg *seg;
    const Frame *orig;   /* o quadro como veio do pack */
} Canvas;

/* Onde o quadro está dentro do golpe. */
enum { PH_NONE, PH_ANTICIPATION, PH_STRIKE, PH_CONTACT, PH_RECOVERY };
typedef struct {
    const char *anim;
    int idx, n, hold, contact, phase;
} Ctx;

static bool cv_ok(int x, int y) { return x >= 0 && x < CW && y >= 0 && y < CH; }
static void cv_put(Canvas *cv, int x, int y, Rgb c) {
    if (!cv_ok(x, y)) return;
    cv->a[y][x] = (Color){c.r, c.g, c.b, 255};
    cv->tag[y][x] = (unsigned char)cv->pen;
}
static void cv_clear(Canvas *cv, int x, int y) {
    if (!cv_ok(x, y)) return;
    cv->a[y][x] = (Color){0, 0, 0, 0};
    cv->tag[y][x] = T_NONE;
}
static bool cv_is_empty(const Canvas *cv, int x, int y) { return cv_ok(x, y) && cv->a[y][x].a == 0; }
/* Pinta só onde não havia nada (fica atrás do corpo). */
static bool cv_behind(Canvas *cv, int x, int y, Rgb c) {
    if (cv_ok(x, y) && cv->empty[y][x] && cv->a[y][x].a == 0) {
        cv_put(cv, x, y, c);
        return true;
    }
    return false;
}
static void set_rgb(Canvas *cv, int x, int y, Rgb c) {
    cv->a[y][x].r = c.r; cv->a[y][x].g = c.g; cv->a[y][x].b = c.b;
}
static int lab_at(const Canvas *cv, int x, int y) { return cv->seg->lab[y][x]; }

typedef struct { int n; int x[512], y[512]; } Pts;

/* Pontos inteiros de uma reta, um por passo no eixo maior. */
static void line_pts(Pts *p, double x0, double y0, double x1, double y1) {
    p->n = 0;
    /* arredonda o número de passos para cima: com menos passos que pixels a reta pula linhas */
    int n = (int)ceil(fmax(fabs(x1 - x0), fabs(y1 - y0)) - 1e-9);
    if (n == 0) {
        p->x[0] = pyround(x0); p->y[0] = pyround(y0); p->n = 1;
        return;
    }
    if (n > 510) n = 510;
    for (int i = 0; i <= n; i++) {
        double t = (double)i / n;
        int px = (int)floor(x0 + (x1 - x0) * t + 0.5), py = (int)floor(y0 + (y1 - y0) * t + 0.5);
        if (p->n == 0 || p->x[p->n - 1] != px || p->y[p->n - 1] != py) {
            p->x[p->n] = px; p->y[p->n] = py; p->n++;
        }
    }
}

/* Apaga um pixel; se ele estava no meio do corpo, fecha com a cor vizinha. */
static void erase_px(Canvas *cv, int x, int y) {
    static const int d[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    Color body[4];
    int nb = 0;
    for (int i = 0; i < 4; i++) {
        int nx = x + d[i][0], ny = y + d[i][1];
        if (cv_ok(nx, ny)) {
            int lb = lab_at(cv, nx, ny);
            if ((lb == SHIRT || lb == DARK || lb == SKIN) && cv->a[ny][nx].a) body[nb++] = cv->a[ny][nx];
        }
    }
    if (nb >= 3) {
        /* a cor que mais aparece; empate: a primeira a aparecer */
        int best = 0, bestc = 0;
        for (int i = 0; i < nb; i++) {
            int cnt = 0;
            for (int j = 0; j < nb; j++)
                cnt += body[i].r == body[j].r && body[i].g == body[j].g && body[i].b == body[j].b;
            if (cnt > bestc) { bestc = cnt; best = i; }
        }
        cv->a[y][x] = (Color){body[best].r, body[best].g, body[best].b, 255};
        cv->tag[y][x] = T_BODY;
    } else {
        cv->a[y][x] = (Color){0, 0, 0, 0};
        cv->tag[y][x] = T_NONE;
    }
}

static void recolor(Canvas *cv, const Char *ch, const char *anim) {
    const Seg *s = cv->seg;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            int lb = s->lab[y][x];
            char k = s->c[y][x];
            const Rgb *col = NULL;
            /* pack próprio: o corpo só muda pela troca exata lá embaixo; aqui, só a lâmina */
            if (ch->pack && lb != BLADE) continue;
            switch (lb) {
                case NONE: continue;
                case SHIRT:
                    if (in_set(k, "Wvw")) col = &ch->camisa[0];
                    else if (in_set(k, "Lg")) col = &ch->camisa[1];
                    else if (k == 'M') col = &ch->camisa[2];
                    else if (k == 'D') col = &ch->camisa[3];
                    break;
                case DARK: {
                    const char *p = strchr("96321", k);
                    if (p) col = &ch->hakama[p - "96321"];
                    break;
                }
                case SKIN: case FACE: {
                    const char *p = strchr("Ssk", k);
                    if (p) col = &ch->pele[p - "Ssk"];
                    break;
                }
                case HAIR: col = in_set(k, "12") ? &ch->cabelo[0] : &ch->cabelo[1]; break;
                case SAYA: col = &ch->saya; break;
                case HANDLE: col = k == 'b' ? &ch->cabo : &ch->hakama[4]; break;
                case BLADE: col = in_set(k, "Wwv") ? &ch->lamina[0] : &ch->lamina[1]; break;
                case HATL: case HATFX:
                    if (ch->chapeu) col = k == 'M' ? &ch->chapeu_cor[1] : k == 'D' ? &ch->chapeu_cor[2] : &ch->chapeu_cor[0];
                    break;
                default: break;
            }
            if (col) set_rgb(cv, x, y, *col);
        }
    /* Faixa (obi): a última linha da camisa em cima da hakama. */
    if (rgb_set(ch->obi) && !ch->pack)
        for (int y = 0; y < CH - 1; y++)
            for (int x = 0; x < CW; x++)
                if (s->lab[y][x] == SHIRT && s->lab[y + 1][x] == DARK) set_rgb(cv, x, y, ch->obi);
    /* Pack próprio: troca exata de cor no corpo (a cor original decide). Quando a
       camisa do pack é tão clara quanto a lâmina (rastro_pack), fora dos quadros em
       que a espada aparece o que o separador leu como lâmina é camisa. */
    bool espada = strstr(anim, "ATTACK") || strstr(anim, "ESPECIAL") || strstr(anim, "DEFEND") || strstr(anim, "THROW");
    if (ch->pack)
        for (int y = 0; y < CH; y++)
            for (int x = 0; x < CW; x++) {
                int lb = s->lab[y][x];
                if (lb == NONE || lb == SMEAR) continue;
                if (lb == BLADE && (espada || !rgb_set(ch->rastro_pack[0]))) continue;
                Color o = cv->orig->p[y][x];
                for (int i = 0; i < 24 && rgb_set(ch->troca[i].de); i++)
                    if (ch->troca[i].de.r == o.r && ch->troca[i].de.g == o.g && ch->troca[i].de.b == o.b) {
                        set_rgb(cv, x, y, ch->troca[i].para);
                        break;
                    }
            }
    /* O rastro de alguns packs usa as mesmas cores da camisa: o que estiver longe do
       resto do corpo (cabelo, hakama, pele, contorno) é rastro, e fica nas cores dele. */
    bool golpe = strstr(anim, "ATTACK") || strstr(anim, "ESPECIAL");
    if (ch->pack && rgb_set(ch->rastro_pack[0]) && golpe) {
        static Mask core, near;
        for (int y = 0; y < CH; y++)
            for (int x = 0; x < CW; x++) {
                Color o = cv->orig->p[y][x];
                bool trail = false;
                for (int i = 0; i < 3; i++)
                    trail |= rgb_set(ch->rastro_pack[i]) && o.r == ch->rastro_pack[i].r && o.g == ch->rastro_pack[i].g &&
                             o.b == ch->rastro_pack[i].b;
                core[y][x] = o.a && !trail && s->lab[y][x] != SMEAR && s->lab[y][x] != BLADE;
            }
        mask_dilate(near, core, 6, false);
        /* abaixo da faixa (o alto da hakama) camisa não há: o que for dessas cores é rastro */
        int belt = CH;
        for (int y = 0; y < CH && belt == CH; y++)
            for (int x = 0; x < CW; x++) {
                Color o = cv->orig->p[y][x];
                if (o.a && rgb_set(ch->hakama[0])) {
                    for (int i = 0; i < 24 && rgb_set(ch->troca[i].de); i++)
                        if (ch->troca[i].de.r == o.r && ch->troca[i].de.g == o.g && ch->troca[i].de.b == o.b &&
                            (ch->troca[i].para.r == ch->hakama[0].r && ch->troca[i].para.g == ch->hakama[0].g &&
                             ch->troca[i].para.b == ch->hakama[0].b)) { belt = y; break; }
                    if (belt < CH) break;
                }
            }
        for (int y = 0; y < CH; y++)
            for (int x = 0; x < CW; x++) {
                Color o = cv->orig->p[y][x];
                if (!o.a || (near[y][x] && y < belt + 3) || s->lab[y][x] == BLADE) continue;
                for (int i = 0; i < 3; i++)
                    if (rgb_set(ch->rastro_pack[i]) && o.r == ch->rastro_pack[i].r && o.g == ch->rastro_pack[i].g &&
                        o.b == ch->rastro_pack[i].b) {
                        set_rgb(cv, x, y, ch->rastro[i]);
                        break;
                    }
            }
    }
    /* Bainha: quem não usa espada comprida não carrega a saya. */
    if (ch->sem_saya)
        for (int y = 0; y < CH; y++)
            for (int x = 0; x < CW; x++)
                if (s->lab[y][x] == SAYA) erase_px(cv, x, y);
}

/* ----- rastro ------------------------------------------------------------- */
/* Rastro em três tons (miolo, borda, cauda) e partículas do elemento. */
static void paint_smear(Canvas *cv, const Char *ch, const char *anim, int idx) {
    static Mask sm, ring;
    const Seg *s = cv->seg;
    bool any = false;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) any |= (sm[y][x] = s->lab[y][x] == SMEAR && cv->tag[y][x] == T_WEAPON && cv->a[y][x].a);
    if (!any) return;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            if (!sm[y][x]) continue;
            bool inner = y > 0 && y < CH - 1 && x > 0 && x < CW - 1 && sm[y - 1][x] && sm[y + 1][x] && sm[y][x - 1] && sm[y][x + 1];
            set_rgb(cv, x, y, in_set(s->c[y][x], "Lg") ? ch->rastro[2] : inner ? ch->rastro[0] : ch->rastro[1]);
        }
    Element el = ch->elemento;
    if (el == EL_NONE) return;
    /* Partículas soltas perto da borda de fora do rastro. */
    mask_dilate(ring, sm, 2, false);
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            if (!ring[y][x] || s->lab[y][x] != NONE || cv->a[y][x].a) continue;
            double r = hsh4("p", anim, idx, x, y);
            if (el == EL_FOGO && r < 0.07) {
                cv_put(cv, x, y, r < 0.035 ? (Rgb){255, 120, 30} : (Rgb){255, 200, 70});
            } else if (el == EL_AGUA && r < 0.05) {
                cv_put(cv, x, y, (Rgb){150, 230, 255});
            } else if (el == EL_RAIO && r < 0.05) {
                /* faísca em zigue-zague saindo do rastro */
                cv_put(cv, x, y, (Rgb){240, 250, 255});
                int nx = x + (r < 0.025 ? 1 : -1), ny = y - 1;
                if (cv_ok(nx, ny) && s->lab[ny][nx] == NONE && cv_is_empty(cv, nx, ny)) cv_put(cv, nx, ny, (Rgb){90, 170, 255});
            } else if (el == EL_ROXO && r < 0.04) {
                cv_put(cv, x, y, (Rgb){190, 110, 255});
            } else if (el == EL_PENA && r < 0.035) {
                cv_put(cv, x, y, (Rgb){40, 30, 36});
                if (cv_ok(x - 1, y + 1) && cv_is_empty(cv, x - 1, y + 1)) cv_put(cv, x - 1, y + 1, (Rgb){200, 40, 50});
            } else if (el == EL_TERRA && r < 0.05 && y > s->oy + 26) {
                cv_put(cv, x, y, (Rgb){150, 120, 70});
            } else if (el == EL_VENTO && r < 0.03) {
                cv_put(cv, x, y, (Rgb){220, 255, 170});
            } else if (el == EL_OURO && r < 0.035) {
                cv_put(cv, x, y, (Rgb){255, 214, 90});
            } else if (el == EL_SOMBRA && r < 0.04) {
                cv_put(cv, x, y, r < 0.012 ? (Rgb){255, 214, 90} : (Rgb){70, 40, 110});
            } else if (el == EL_MUSGO && r < 0.03) {
                cv_put(cv, x, y, (Rgb){150, 230, 110});
            } else if (el == EL_POEIRA && r < 0.04) {
                cv_put(cv, x, y, r < 0.02 ? (Rgb){200, 194, 180} : (Rgb){130, 126, 118});
            }
        }
}

/* Distância de cada pixel do rastro até a borda dele (1 = na borda), em passos de 4 vizinhos. */
static void smear_depth(const Canvas *cv, int d[CH][CW]) {
    static short qx[CW * CH], qy[CW * CH];
    static const int nb[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    int head = 0, tail = 0;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) d[y][x] = cv->seg->lab[y][x] == SMEAR && cv->tag[y][x] == T_WEAPON ? -1 : 0;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            if (d[y][x] != -1) continue;
            for (int k = 0; k < 4; k++) {
                int nx = x + nb[k][0], ny = y + nb[k][1];
                if (!cv_ok(nx, ny) || d[ny][nx] == 0) {
                    d[y][x] = 1;
                    qx[tail] = (short)x; qy[tail] = (short)y; tail++;
                    break;
                }
            }
        }
    while (head < tail) {
        int x = qx[head], y = qy[head];
        head++;
        for (int k = 0; k < 4; k++) {
            int nx = x + nb[k][0], ny = y + nb[k][1];
            if (cv_ok(nx, ny) && d[ny][nx] == -1) {
                d[ny][nx] = d[y][x] + 1;
                qx[tail] = (short)nx; qy[tail] = (short)ny; tail++;
            }
        }
    }
}

/* Tira um pixel de rastro. Se ele passava na frente do corpo, o corpo aparece:
   copia a cor do pedaço de corpo mais perto, na vertical ou na horizontal. */
static void remove_smear_px(Canvas *cv, int x, int y) {
    static const int dir[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
    int dist[4];
    Color col[4];
    for (int k = 0; k < 4; k++) {
        dist[k] = 99;
        for (int s = 1; s <= 8; s++) {
            int nx = x + dir[k][0] * s, ny = y + dir[k][1] * s;
            if (!cv_ok(nx, ny)) break;
            if (cv->tag[ny][nx] == T_BODY && cv->a[ny][nx].a) { dist[k] = s; col[k] = cv->a[ny][nx]; break; }
        }
    }
    int best = -1;
    if (dist[0] < 99 && dist[1] < 99) best = dist[0] <= dist[1] ? 0 : 1;
    else if (dist[2] < 99 && dist[3] < 99) best = dist[2] <= dist[3] ? 2 : 3;
    if (best >= 0) {
        cv->a[y][x] = col[best];
        cv->tag[y][x] = T_BODY;
    } else {
        cv->a[y][x] = (Color){0, 0, 0, 0};
        cv->tag[y][x] = T_NONE;
    }
}

/* O formato do rastro conta a arma: adaga corta duplo, garra deixa três
   riscos, arma pesada abre um rastro grosso, duas espadas deixam um eco. */
static void smear_style(Canvas *cv, const Char *ch) {
    static int d[CH][CW];
    const Seg *s = cv->seg;
    WeaponKind k = ch->arma.kind;
    bool any = false;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) any |= s->lab[y][x] == SMEAR && cv->tag[y][x] == T_WEAPON;
    if (!any) return;
    if (k == W_ADAGA || k == W_GARRAS) {
        smear_depth(cv, d);
        for (int y = 0; y < CH; y++)
            for (int x = 0; x < CW; x++) {
                if (!d[y][x]) continue;
                bool keep = k == W_ADAGA ? d[y][x] == 1 : (d[y][x] == 1 || d[y][x] == 3);
                if (!keep) remove_smear_px(cv, x, y);
                else if (d[y][x] == 3) set_rgb(cv, x, y, ch->rastro[0]);
            }
    } else if (k == W_ODACHI || k == W_PESADA || k == W_CAJADO) {
        static Mask m, g;
        for (int y = 0; y < CH; y++)
            for (int x = 0; x < CW; x++) m[y][x] = s->lab[y][x] == SMEAR && cv->tag[y][x] == T_WEAPON;
        mask_dilate(g, m, 1, true);
        cv->pen = T_WEAPON;
        for (int y = 0; y < CH; y++)
            for (int x = 0; x < CW; x++)
                if (g[y][x] && !m[y][x] && cv->a[y][x].a == 0) cv_put(cv, x, y, ch->rastro[2]);
    } else if (!ch->pack_par && (k == W_DUPLA || k == W_FOICE || (k == W_KATANA && ch->arma.par))) {
        /* eco da segunda arma, atrás e um pouco abaixo */
        cv->pen = T_WEAPON;
        for (int y = CH - 1; y >= 0; y--)
            for (int x = 0; x < CW; x++) {
                if (s->lab[y][x] != SMEAR) continue;
                int ex = x - 3, ey = y + 3;
                if (cv_ok(ex, ey) && cv->a[ey][ex].a == 0) cv_put(cv, ex, ey, ch->rastro[2]);
            }
    }
}

/* ----- cabeça ------------------------------------------------------------- */
static bool head_paintable(const Canvas *cv, int x, int y) {
    if (!cv_ok(x, y)) return false;
    int lb = lab_at(cv, x, y);
    return lb == NONE || lb == HATL || lb == HATFX || lb == HAIR || lb == FACE;
}

typedef struct { Rgb c[128]; } Pal;

static void head_palette(const Char *ch, Pal *p) {
    memset(p, 0, sizeof *p);
    p->c['H'] = ch->cabelo[0]; p->c['h'] = ch->cabelo[1]; p->c['i'] = ch->cabelo[2];
    p->c['F'] = ch->pele[0]; p->c['f'] = ch->pele[1]; p->c['k'] = ch->pele[2];
    p->c['e'] = ch->olho;
    p->c['A'] = ch->destaque[0]; p->c['a'] = ch->destaque[1];
    p->c['B'] = ch->destaque2;
    p->c['K'] = ch->mascara[0]; p->c['m'] = ch->mascara[1];
    /* barba por fazer: a pele clara com um terço do cabelo por cima */
    Rgb pe = ch->pele[0], ca = ch->cabelo[1];
    p->c['b'] = (Rgb){(unsigned char)(pe.r * 0.65 + ca.r * 0.35), (unsigned char)(pe.g * 0.65 + ca.g * 0.35),
                      (unsigned char)(pe.b * 0.65 + ca.b * 0.35)};
}

static void paint_rows(Canvas *cv, const Row *rows, int nrows, const Pal *pal, bool behind) {
    const Seg *s = cv->seg;
    for (int r = 0; r < nrows && rows[r].t; r++)
        for (int i = 0; rows[r].t[i]; i++) {
            char k = rows[r].t[i];
            if (k == '.') continue;
            int x = s->ox + rows[r].x + i, y = s->oy + rows[r].y;
            if (behind) {
                if (k != 'X') cv_behind(cv, x, y, pal->c[(int)k]);
                continue;
            }
            if (!head_paintable(cv, x, y)) continue;
            if (k == 'X') cv_clear(cv, x, y);
            else cv_put(cv, x, y, pal->c[(int)k]);
        }
}

/* Tira ondulando para trás (esquerda), presa em (x0, y0). */
static void ribbon(Canvas *cv, int x0, int y0, int length, Rgb c0, Rgb c1, int idx, double droop, double amp,
                   int thick, double speed) {
    double ph = idx * speed;
    for (int i = 0; i < length; i++) {
        int x = x0 - i;
        double yy = y0 + i * droop + amp * sin(i * 0.55 - ph) * fmin(1.0, i / 4.0);
        int y = (int)floor(yy + 0.5);
        int nt = i < length - 2 ? thick : 1;
        for (int t = 0; t < nt; t++) cv_behind(cv, x, y + t, t == 0 ? c0 : c1);
    }
}

static void draw_head(Canvas *cv, const Char *ch, int idx) {
    const Head *hd = find_head(ch->cabeca);
    if (!hd) return;
    Pal pal;
    head_palette(ch, &pal);
    for (int i = 0; i < 3 && hd->fitas[i].n; i++)
        ribbon(cv, cv->seg->ox + hd->fitas[i].x, cv->seg->oy + hd->fitas[i].y, hd->fitas[i].n, ch->destaque[1],
               ch->destaque[1], idx + hd->fitas[i].y, 0.2, 1.1, 1, 1.3);
    int nv = (hd->atras[0][0].t != NULL) + (hd->atras[1][0].t != NULL);
    if (nv) paint_rows(cv, hd->atras[(idx / 2) % nv], 14, &pal, true);
    if (!strcmp(hd->name, "coque")) {
        /* coque: o cabelo que o chapéu escondia descendo pela nuca sai; fica a nuca curta */
        const Seg *s = cv->seg;
        for (int y = s->oy + 8; y <= s->oy + 12; y++)
            for (int x = s->ox - 2; x <= s->ox + 6; x++)
                if (cv_ok(x, y) && s->lab[y][x] == HAIR) cv_clear(cv, x, y);
    }
    paint_rows(cv, hd->frente, 20, &pal, false);
}

static void remove_hat(Canvas *cv) {
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (cv->seg->lab[y][x] == HATL || cv->seg->lab[y][x] == HATFX) cv_clear(cv, x, y);
}

static const Row RAIDEN_HAT[] = {
    {2, 8, "LLMD"},
    {3, 5, "LLLLLMMDDD"},
    {4, 2, "LLLLLLLLMMMMDDDD"},
    {5, -1, "LLLLLLLLLLLMMMMMDDDDD"},
    {6, -4, "iLLLLLLLLLLLLLLMMMMMMDDDDD"},
    {7, -3, "DDDDDDDDDDDDDDDDDDDDDDD"},
};

/* Chapéu do Raiden: largo e baixo, cobre os olhos, que brilham por baixo. */
static void raiden_hat(Canvas *cv, const Char *ch) {
    const Seg *s = cv->seg;
    remove_hat(cv);
    for (size_t r = 0; r < sizeof RAIDEN_HAT / sizeof RAIDEN_HAT[0]; r++)
        for (int i = 0; RAIDEN_HAT[r].t[i]; i++) {
            char k = RAIDEN_HAT[r].t[i];
            int x = s->ox + RAIDEN_HAT[r].x + i, y = s->oy + RAIDEN_HAT[r].y;
            if (!cv_ok(x, y)) continue;
            int lb = lab_at(cv, x, y);
            if (lb == NONE || lb == HATL || lb == HATFX || lb == HAIR)
                cv_put(cv, x, y, k == 'M' ? ch->chapeu_cor[1] : k == 'D' ? ch->chapeu_cor[2] : ch->chapeu_cor[0]);
        }
    /* cabelo que o chapéu original escondia e o novo não cobre */
    Pal pal;
    head_palette(ch, &pal);
    static const Row hair[] = {{8, 4, "HHH"}};
    paint_rows(cv, hair, 1, &pal, false);
}

/* ----- acessórios atrás do corpo -------------------------------------------- */
static const char *SHELL[] = {
    "...aaaa...",
    "..abbbba..",
    ".abAAAAba.",
    "abAAaaAAba",
    "aAAaAAaAAa",
    "aAAaAAaAAa",
    "aAAAaaAAAa",
    "aAaAAAAaAa",
    ".aAAAAAAa.",
    "..aaaaaa..",
};

static bool is_hair(const Canvas *cv, const Char *ch, int x, int y) {
    if (!cv_ok(x, y)) return false;
    Color c = cv->a[y][x];
    if (!c.a) return false;
    for (int h = 0; h < 3; h++)
        if (c.r == ch->cabelo[h].r && c.g == ch->cabelo[h].g && c.b == ch->cabelo[h].b) return true;
    return false;
}

static void accessories(Canvas *cv, const Char *ch, int idx) {
    const Seg *s = cv->seg;
    for (int a = 0; a < 3; a++) {
        switch (ch->acessorios[a]) {
            case AC_CACHECOL:
                ribbon(cv, s->ox + 4, s->oy + 10, 15, ch->destaque[0], ch->destaque[1], idx, 0.15, 1.4, 2, 1.3);
                ribbon(cv, s->ox + 3, s->oy + 11, 9, ch->destaque[1], ch->destaque[1], idx + 1, 0.3, 1.0, 1, 1.3);
                break;
            case AC_CASCO:
                for (int ry = 0; ry < 10; ry++)
                    for (int rx = 0; SHELL[ry][rx]; rx++) {
                        char k = SHELL[ry][rx];
                        if (k == '.') continue;
                        Rgb c = k == 'a' ? ch->destaque[1] : k == 'A' ? ch->destaque[0] : ch->destaque2;
                        cv_behind(cv, s->ox - 6 + rx, s->oy + 10 + ry, c);
                    }
                break;
            case AC_CAUDA:
                /* cauda de tigre saindo da cintura, da cor do cabelo, com a ponta preta */
                ribbon(cv, s->ox + 3, s->oy + 21, 11, ch->cabelo[0], ch->cabelo[1], idx, -0.25, 1.3, 2, 1.1);
                cv_behind(cv, s->ox + 3 - 10, s->oy + 21 - 3, ch->mascara[0]);
                cv_behind(cv, s->ox + 3 - 9, s->oy + 21 - 3, ch->mascara[0]);
                break;
            case AC_LISTRAS:
                /* listras de tigre na roupa, no vermelho de destaque, presas ao corpo (contadas a partir da cabeça) */
                for (int y = 0; y < CH; y++)
                    for (int x = 0; x < CW; x++) {
                        /* no corpo do Samurai #3, a camisa; num pack, a roupa (as duas primeiras cores de `troca`) */
                        Color o = cv->orig->p[y][x];
                        bool roupa = ch->pack ? s->lab[y][x] == OTHER && o.a &&
                                                    ((o.r == ch->troca[0].de.r && o.g == ch->troca[0].de.g && o.b == ch->troca[0].de.b) ||
                                                     (o.r == ch->troca[1].de.r && o.g == ch->troca[1].de.g && o.b == ch->troca[1].de.b))
                                              : s->lab[y][x] == SHIRT;
                        if (!roupa) continue;
                        int rx = x - s->ox + 64, ry = y - s->oy + 64;
                        if ((rx * 2 + ry) % 9 < 2 && (ry / 3) % 2 == 0) set_rgb(cv, x, y, ch->destaque[1]);
                    }
                break;
            case AC_CABELO_LONGO: {
                /* Cabelo solto e comprido no lugar do rabo de cavalo do pack. O cabelo da
                   cabeça é a mancha de cabelo que chega mais à frente (o rosto olha para a
                   direita); as outras manchas atrás dela, na altura da cabeça, são o rabo de
                   cavalo e saem. Depois o cabelo desce da nuca, reto, até a cintura. */
                static signed char comp[CH][CW];
                memset(comp, -1, sizeof comp);
                /* só o alto da figura: o cabo roxo da espada lá embaixo não conta */
                int ytop = -1;
                for (int y = 0; y < CH && ytop < 0; y++)
                    for (int x = 0; x < CW; x++)
                        if (is_hair(cv, ch, x, y)) { ytop = y; break; }
                if (ytop < 0) break;
                int ymax = ytop + 16 < CH ? ytop + 16 : CH;
                int n = 0, cx0[32], cx1[32], cy0[32], cy1[32];
                for (int y = 0; y < ymax; y++)
                    for (int x = 0; x < CW; x++) {
                        if (comp[y][x] >= 0 || !is_hair(cv, ch, x, y) || n >= 32) continue;
                        /* mancha de cabelo (8 vizinhos) */
                        static int st[CH * CW][2];
                        int sp = 0;
                        st[sp][0] = x; st[sp][1] = y; sp++;
                        comp[y][x] = (signed char)n;
                        cx0[n] = cx1[n] = x; cy0[n] = cy1[n] = y;
                        while (sp > 0) {
                            sp--;
                            int px = st[sp][0], py = st[sp][1];
                            if (px < cx0[n]) cx0[n] = px;
                            if (px > cx1[n]) cx1[n] = px;
                            if (py < cy0[n]) cy0[n] = py;
                            if (py > cy1[n]) cy1[n] = py;
                            for (int dy = -1; dy <= 1; dy++)
                                for (int dx = -1; dx <= 1; dx++) {
                                    int qx = px + dx, qy = py + dy;
                                    if (!cv_ok(qx, qy) || qy >= ymax || comp[qy][qx] >= 0 || !is_hair(cv, ch, qx, qy)) continue;
                                    comp[qy][qx] = (signed char)n;
                                    st[sp][0] = qx; st[sp][1] = qy; sp++;
                                }
                        }
                        n++;
                    }
                if (!n) break;
                int head = -1;
                for (int k = 0; k < n; k++)
                    if (cy1[k] - cy0[k] >= 3 && (head < 0 || cx1[k] > cx1[head])) head = k;
                if (head < 0) break;
                /* o rabo de cavalo (e a fita dele) sai; quando ele encosta na cabeça, o que
                   passa de uma cabeça de largura (10 px a partir da frente) também sai */
                for (int y = 0; y < ymax; y++)
                    for (int x = 0; x < CW; x++) {
                        int k = comp[y][x];
                        if (k < 0) continue;
                        bool tail = k != head ? cx1[k] <= cx0[head] + 3 : x < cx1[head] - 10;
                        if (!tail) continue;
                        cv_clear(cv, x, y);
                        cv->empty[y][x] = true;      /* o cabelo solto pode ocupar o lugar */
                    }
                for (int y = 0; y < ymax; y++)
                    for (int x = 0; x < CW; x++)
                        if (comp[y][x] == head && x < cx1[head] - 10) comp[y][x] = -1;
                /* a fita do rabo de cavalo e o que sobrou dele no alto da cabeça: dentro do
                   cabelo vira cabelo (senão parece uma risca); fora dele, sai */
                for (int y = cy0[head] - 4; y <= cy0[head] + 3; y++)          /* acima do rosto */
                    for (int x = cx1[head] - 12; x <= cx1[head]; x++) {
                        if (!cv_ok(x, y) || !cv->a[y][x].a || is_hair(cv, ch, x, y)) continue;
                        int opaque = 0;
                        for (int dy = -1; dy <= 1; dy++)
                            for (int dx = -1; dx <= 1; dx++) opaque += (dx || dy) && cv_ok(x + dx, y + dy) && cv->a[y + dy][x + dx].a;
                        /* a arma fica, a não ser um pixel solto acima da cabeça */
                        if (cv->tag[y][x] == T_WEAPON && (y >= cy0[head] || opaque > 1)) continue;
                        int nh = 0;
                        for (int dy = -1; dy <= 1; dy++)
                            for (int dx = -1; dx <= 1; dx++) nh += (dx || dy) && is_hair(cv, ch, x + dx, y + dy);
                        if (nh >= 3) cv_put(cv, x, y, ch->cabelo[(x + y) % 2 ? 0 : 1]);
                        else { cv_clear(cv, x, y); cv->empty[y][x] = true; }
                    }
                cx0[head] = cx1[head] - 10 > cx0[head] ? cx1[head] - 10 : cx0[head];
                for (int y = cy0[head]; y <= cy0[head] + 4 && y < CH; y++)
                    for (int x = cx0[head] - 3; x < cx0[head]; x++)
                        if (cv_ok(x, y) && cv->a[y][x].a && !is_hair(cv, ch, x, y) && s->lab[y][x] != BLADE &&
                            cv->tag[y][x] != T_WEAPON) {
                            cv_clear(cv, x, y);
                            cv->empty[y][x] = true;
                        }
                /* o cabelo solto: da nuca para baixo, colado nas costas, abrindo um pouco
                   para trás e com as pontas desencontradas */
                int top = cy0[head] + 2, len = 24;
                double sway = sin(idx * 0.8) * 1.0;
                for (int i = 0; i < len; i++) {
                    int y = top + i;
                    if (y >= CH) break;
                    double u = (double)i / len;
                    int back = cx0[head];
                    for (int x = cx0[head]; x <= cx1[head]; x++)
                        if (y <= cy1[head] && comp[y][x] == head) { back = x; break; }
                    int left = back - 3 - (int)floor(u * 3.5 + sway * u + 0.5), right = back + 3;
                    for (int x = left; x <= right; x++) {
                        int col = x - left;
                        /* cada fio termina numa altura: as pontas ficam desencontradas */
                        if (i > len - 5 && hsh4("fio", "cabelo", col, 0, 1) * 5 < i - (len - 5)) continue;
                        Rgb c = col == 0 ? ch->cabelo[0]
                              : (col % 3 == 1 ? ch->cabelo[2] : ch->cabelo[1]);
                        if (col == 1 && i % 5 == 2) c = ch->cabelo[1];
                        cv_behind(cv, x, y, c);
                    }
                }
                break;
            }
            case AC_TRAPO:
                ribbon(cv, s->ox + 4, s->oy + 10, 10, ch->destaque[0], ch->destaque[1], idx, 0.35, 1.2, 2, 1.3);
                for (int k = 0; k < 3; k++)
                    ribbon(cv, s->ox + 4 - 5 - k * 2, s->oy + 12 + k, 2, ch->destaque[1], ch->destaque[1], idx + k, 1.0, 0, 1, 1.3);
                break;
            default: break;
        }
    }
}

/* ----- armas --------------------------------------------------------------- */
/* A arma nova só ocupa o vazio ou o lugar da lâmina antiga. */
static bool weapon_ok(const Canvas *cv, int x, int y) {
    if (!cv_ok(x, y)) return false;
    int lb = lab_at(cv, x, y);
    return (lb == NONE && cv->a[y][x].a == 0) || lb == BLADE || (lb == SMEAR && cv->a[y][x].a == 0);
}
static bool empty_orig(const Canvas *cv, int x, int y) {
    return cv_ok(x, y) && (lab_at(cv, x, y) == NONE || lab_at(cv, x, y) == SMEAR) && cv->a[y][x].a == 0;
}

/* Normal que aponta para cima (y menor). */
static void perp(const double u[2], double n[2]) {
    n[0] = -u[1]; n[1] = u[0];
    if (!(n[1] < 0 || (n[1] == 0 && n[0] > 0))) { n[0] = -n[0]; n[1] = -n[1]; }
}

static void mark(Canvas *cv, int x, int y) { if (cv_ok(x, y)) cv->wpx[y][x] = true; }

/* Adaga invertida na mão da frente: fica deitada por cima do antebraço, então
   pode pintar em cima do corpo. */
static bool g_over_body;

static void stroke(Canvas *cv, const Blade *b, double t0, double t1, Rgb core, const Rgb *edge, double offx,
                   double offy, int every, bool only_empty);

/* Adaga ou espada curta desenhada do zero: cabo escuro atrás da mão, guarda
   atravessada, lâmina com fio claro e a ponta mais clara ainda. */
static void draw_short_blade(Canvas *cv, const Blade *b, double offx, double offy, double len, Rgb core, Rgb edge,
                             Rgb guard, Rgb grip, bool only_empty) {
    stroke(cv, b, -2, 0, grip, NULL, offx, offy, 2, only_empty);
    stroke(cv, b, 1, len, core, &edge, offx, offy, 1, only_empty);
    double n[2];
    perp(b->u, n);
    double gx = b->hilt[0] + offx + b->u[0] * 0.8, gy = b->hilt[1] + offy + b->u[1] * 0.8;
    for (int k = -1; k <= 1; k += 2) {
        int x = pyround(gx + n[0] * k), y = pyround(gy + n[1] * k);
        if (cv_ok(x, y) && (only_empty ? empty_orig(cv, x, y) : weapon_ok(cv, x, y) || cv->a[y][x].a == 0)) cv_put(cv, x, y, guard);
    }
}

/* Reta ao longo da lâmina, de t0 a t1 (distância a partir do cabo). */
static void stroke(Canvas *cv, const Blade *b, double t0, double t1, Rgb core, const Rgb *edge, double offx,
                   double offy, int every, bool only_empty) {
    static Pts p;
    double hx = b->hilt[0] + offx, hy = b->hilt[1] + offy, ux = b->u[0], uy = b->u[1];
    line_pts(&p, hx + ux * t0, hy + uy * t0, hx + ux * t1, hy + uy * t1);
    double n[2];
    perp(b->u, n);
    int sx = pyround(n[0]), sy = pyround(n[1]);
    if (sx == 0 && sy == 0) sy = -1;
    for (int i = 0; i < p.n; i++) {
        int x = p.x[i], y = p.y[i];
        if (only_empty ? !empty_orig(cv, x, y) : !(weapon_ok(cv, x, y) || (g_over_body && cv_ok(x, y) && cv->a[y][x].a))) continue;
        cv_put(cv, x, y, core);
        mark(cv, x, y);
        if (edge && i % every == 0) {
            int ex = x + sx, ey = y + sy;
            if (empty_orig(cv, ex, ey)) {
                cv_put(cv, ex, ey, *edge);
                mark(cv, ex, ey);
            }
        }
    }
}

/* A outra mão: no corpo do Samurai #3 as duas mãos ficam juntas no cabo, então a
   segunda arma sai do mesmo punho, um pouco mais para dentro e mais baixo, e
   abre em leque (empunhada ao contrário), desenhada atrás do corpo. */
static Blade other_hand(const Blade *b, double open) {
    Blade o = {0};
    double a = atan2(b->u[1], b->u[0]), a1 = a + open, a2 = a - open;
    double y1 = sin(a1), y2 = sin(a2);
    double na = (fabs(y1 - y2) < 1e-6 ? cos(a1) < cos(a2) : y1 > y2) ? a1 : a2;
    o.u[0] = cos(na);
    o.u[1] = sin(na);
    o.hilt[0] = b->hilt[0] - b->u[0] * 2;
    o.hilt[1] = b->hilt[1] - b->u[1] * 2 + 2;
    return o;
}

/* Três lâminas em leque saindo do punho. `behind`: só no vazio (a mão de trás). */
static void claws(Canvas *cv, const Blade *b, double size, Rgb core, Rgb edge, bool behind) {
    double ang = atan2(b->u[1], b->u[0]), n[2];
    perp(b->u, n);
    /* três lâminas saindo direto dos nós dos dedos, abertas em leque (sem barra) */
    static const double da[3] = {-0.22, 0.0, 0.22};
    static Pts p;
    for (int j = 0; j < 3; j++) {
        double a = ang + da[j], u2x = cos(a), u2y = sin(a);
        double p0x = b->hilt[0] + b->u[0] * 0.6 + n[0] * (j - 1) * 0.9;
        double p0y = b->hilt[1] + b->u[1] * 0.6 + n[1] * (j - 1) * 0.9;
        double ln = size - (j != 1 ? 1 : 0);
        line_pts(&p, p0x, p0y, p0x + u2x * ln, p0y + u2y * ln);
        for (int i = 0; i < p.n; i++) {
            int x = p.x[i], y = p.y[i];
            bool ok = behind ? empty_orig(cv, x, y) && cv->a[y][x].a == 0
                             : cv_ok(x, y) && (lab_at(cv, x, y) == NONE || lab_at(cv, x, y) == BLADE);
            if (ok) {
                /* a raiz fica na cor da borda (sai de dentro da mão); a ponta, clara */
                cv_put(cv, x, y, i == 0 ? edge : core);
                mark(cv, x, y);
            }
        }
    }
}

/* Foice (kama): cabo de madeira saindo do punho e, na ponta, a lâmina curva de
   lado, virada para a frente (para baixo quando o cabo está deitado). */
static void kama(Canvas *cv, const Blade *b, double len, const Weapon *w, Rgb core, Rgb edge, bool behind) {
    static Pts p;
    Rgb wood = rgb_set(w->haste[0]) ? w->haste[0] : (Rgb){138, 106, 68};
    double tx = b->hilt[0] + b->u[0] * len, ty = b->hilt[1] + b->u[1] * len;
    double n[2] = {-b->u[1], b->u[0]};
    if (n[0] < -1e-9 || (fabs(n[0]) < 1e-9 && n[1] < 0)) { n[0] = -n[0]; n[1] = -n[1]; }
    line_pts(&p, b->hilt[0] - b->u[0], b->hilt[1] - b->u[1], tx, ty);
    for (int i = 0; i < p.n; i++) {
        int x = p.x[i], y = p.y[i];
        bool ok = behind ? empty_orig(cv, x, y) && cv->a[y][x].a == 0
                         : cv_ok(x, y) && (lab_at(cv, x, y) == NONE || lab_at(cv, x, y) == BLADE || cv->a[y][x].a == 0);
        if (ok) { cv_put(cv, x, y, wood); mark(cv, x, y); }
    }
    /* anel de metal onde a lâmina encaixa no cabo */
    for (int k = -1; k <= 1; k++) {
        int x = pyround(tx - b->u[0] * 1.2 + n[0] * k * 0.8), y = pyround(ty - b->u[1] * 1.2 + n[1] * k * 0.8);
        bool ok = behind ? empty_orig(cv, x, y) && cv->a[y][x].a == 0 : cv_ok(x, y) && (lab_at(cv, x, y) == NONE || cv->a[y][x].a == 0);
        if (ok) cv_put(cv, x, y, (Rgb){150, 150, 160});
    }
    for (int i = 0; i <= 6; i++) {
        double bend = i * i * 0.09;  /* a ponta volta em direção à mão */
        for (int e = 0; e < (i < 3 ? 3 : 2); e++) {
            if (e && i > 4) break;
            double x = tx + n[0] * i - b->u[0] * (bend + e), y = ty + n[1] * i - b->u[1] * (bend + e);
            int xi = pyround(x), yi = pyround(y);
            bool ok = behind ? empty_orig(cv, xi, yi) && cv->a[yi][xi].a == 0
                             : cv_ok(xi, yi) && (lab_at(cv, xi, yi) == NONE || lab_at(cv, xi, yi) == BLADE || cv->a[yi][xi].a == 0);
            if (ok) { cv_put(cv, xi, yi, e ? edge : core); mark(cv, xi, yi); }
        }
    }
}

static void blade_fx(Canvas *cv, const Char *ch, const char *anim, int idx) {
    Element el = ch->elemento;
    if (rgb_set(ch->arma.brilho))
        for (int x = 0; x < CW; x++)
            for (int y = 0; y < CH; y++) {
                if (!cv->wpx[y][x]) continue;
                static const int d4[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
                for (int k = 0; k < 4; k++) {
                    int nx = x + d4[k][0], ny = y + d4[k][1];
                    double r = hsh6("lua", anim, idx, x, y, k, 0);
                    if (r < 0.45 && cv_is_empty(cv, nx, ny)) cv_put(cv, nx, ny, ch->arma.brilho);
                }
                if (hsh4("estrela", anim, idx, x, y) < 0.03 && cv_is_empty(cv, x + 1, y - 2)) cv_put(cv, x + 1, y - 2, (Rgb){255, 255, 255});
            }
    for (int x = 0; x < CW; x++)
        for (int y = 0; y < CH; y++) {
            if (!cv->wpx[y][x]) continue;
            double r = hsh4("b", anim, idx, x, y);
            if (el == EL_FOGO) {
                if (r < 0.5 && cv_is_empty(cv, x, y - 1)) {
                    cv_put(cv, x, y - 1, r < 0.3 ? (Rgb){255, 150, 40} : (Rgb){255, 90, 30});
                    if (r < 0.15 && cv_is_empty(cv, x, y - 2)) cv_put(cv, x, y - 2, (Rgb){255, 214, 100});
                }
            } else if (el == EL_RAIO) {
                if (r < 0.2) {
                    static const int d[4][2] = {{1, -1}, {-1, 1}, {1, 1}, {-1, -1}};
                    int k = (int)(r * 100) % 4, dx = d[k][0], dy = d[k][1];
                    if (cv_is_empty(cv, x + dx, y + dy)) {
                        cv_put(cv, x + dx, y + dy, (Rgb){120, 200, 255});
                        if (r < 0.1 && cv_is_empty(cv, x + 2 * dx, y)) cv_put(cv, x + 2 * dx, y, (Rgb){235, 248, 255});
                    }
                }
            } else if (el == EL_ROXO) {
                static const int d[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
                for (int i = 0; i < 4; i++) {
                    int dx = d[i][0], dy = d[i][1];
                    if (hsh6("g", anim, idx, x, y, dx, dy) < 0.08 && cv_is_empty(cv, x + dx, y + dy))
                        cv_put(cv, x + dx, y + dy, (Rgb){96, 40, 160});
                }
            } else if (el == EL_AGUA) {
                if (r < 0.12) {
                    static const int d[4][2] = {{0, -2}, {1, -1}, {-1, 1}, {0, 2}};
                    int k = (int)(r * 100) % 4, dx = d[k][0], dy = d[k][1];
                    if (cv_is_empty(cv, x + dx, y + dy)) cv_put(cv, x + dx, y + dy, (Rgb){150, 235, 255});
                }
            }
        }
}


/* ----- estocada ------------------------------------------------------------ */
/* Lança e florete não cortam em arco no golpe reto: estocam. A arma fica na
   horizontal na altura das mãos, recua na preparação e avança no contato. */
static bool thrust_anim(const Char *ch, const char *anim) {
    return (ch->arma.kind == W_LANCA || ch->arma.kind == W_FLORETE) && !ch->pack &&
           (!strcmp(anim, "ATTACK_1") || !strcmp(anim, "DASH_ATTACK"));
}

/* Onde estão as mãos: o cabo da lâmina que sai delas, o cabo (tsuka) ou a pele
   mais à frente na altura do peito. */
static bool hand_point(const Canvas *cv, double *hx, double *hy) {
    const Seg *s = cv->seg;
    for (int i = 0; i < s->nblades; i++)
        if (s->blades[i].nearest <= 3 && !s->blades[i].loose && s->blades[i].farthest >= 5) {
            *hx = s->blades[i].hilt[0];
            *hy = s->blades[i].hilt[1];
            return true;
        }
    double sx = 0, sy = 0;
    int n = 0;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (s->c[y][x] == 'b') { sx += x; sy += y; n++; }
    if (n) { *hx = sx / n; *hy = sy / n; return true; }
    int bx = -1, by = 0;
    for (int y = s->oy + 8; y <= s->oy + 20 && y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (y >= 0 && s->lab[y][x] == SKIN && x > bx) { bx = x; by = y; }
    if (bx < 0) return false;
    *hx = bx;
    *hy = by;
    return true;
}

static void thrust(Canvas *cv, const Char *ch, const Ctx *ctx) {
    const Seg *s = cv->seg;
    const Weapon *w = &ch->arma;
    bool lanca = w->kind == W_LANCA;
    /* some a katana e o arco do rastro */
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            if (s->lab[y][x] == BLADE) erase_px(cv, x, y);
            else if (s->lab[y][x] == SMEAR) remove_smear_px(cv, x, y);
        }
    double hx, hy;
    if (!hand_point(cv, &hx, &hy)) return;
    hy = floor(hy + 0.5);
    /* quanto a arma passa das mãos em cada fase do golpe */
    int reach_tbl[5][2] = {{22, 12}, {22, 12}, {16, 16}, {34, 12}, {26, 12}};  /* lança: frente, trás */
    int foil_tbl[5][2] = {{20, 2}, {20, 2}, {15, 2}, {30, 2}, {24, 2}};
    int ph = ctx->phase;
    int front = lanca ? reach_tbl[ph][0] : foil_tbl[ph][0], back = lanca ? reach_tbl[ph][1] : foil_tbl[ph][1];
    if (ph == PH_RECOVERY && ctx->idx > ctx->contact + 1) front -= 4;
    if (hx + front > cellw - 4) front = cellw - 4 - (int)hx;  /* a ponta não passa da borda do quadro */
    Blade b = {0};
    b.u[0] = 1; b.u[1] = 0;
    b.hilt[0] = hx; b.hilt[1] = hy;
    cv->pen = T_WEAPON;
    Rgb core = ch->lamina[0], edge = ch->lamina[1];
    if (lanca) {
        int head = w->ponta ? w->ponta : 5;
        stroke(cv, &b, -back, front - head, w->haste[0], NULL, 0, 0, 2, false);
        stroke(cv, &b, front - head, front, core, &edge, 0, 0, 1, false);
        for (int k = -1; k <= 1; k += 2) {
            int x = pyround(hx + front - head + 1.5), y = (int)hy + k;
            if (empty_orig(cv, x, y) || (cv_ok(x, y) && cv->a[y][x].a == 0)) { cv_put(cv, x, y, edge); mark(cv, x, y); }
        }
    } else {
        stroke(cv, &b, -back, front, core, NULL, 0, 0, 2, false);
        for (int k = -1; k <= 1; k++) {
            int x = pyround(hx + 1.5), y = (int)hy + k;
            if (cv_ok(x, y) && (cv->a[y][x].a == 0 || lab_at(cv, x, y) == BLADE || lab_at(cv, x, y) == HANDLE))
                cv_put(cv, x, y, k ? ch->destaque[0] : ch->destaque[1]);
        }
    }
    if (ph != PH_CONTACT) return;
    /* rastro reto da estocada, afinando nas pontas, e duas linhas de velocidade */
    int x0 = (int)hx + 4, x1 = (int)hx + front + 8;
    for (int x = x0; x <= x1; x++) {
        double t = (double)(x - x0) / (x1 - x0);
        int y = (int)hy;
        Rgb c = t > 0.8 ? ch->rastro[2] : ch->rastro[0];
        if (cv_ok(x, y) && cv->a[y][x].a == 0) cv_put(cv, x, y, c);
        if (t > 0.08 && t < 0.72)
            for (int k = -1; k <= 1; k += 2)
                if (cv_ok(x, y + k) && cv->a[y + k][x].a == 0) cv_put(cv, x, y + k, ch->rastro[1]);
        if (t > 0.2 && t < 0.5)
            for (int k = -2; k <= 2; k += 4)
                if (cv_ok(x, y + k) && cv->a[y + k][x].a == 0) cv_put(cv, x, y + k, ch->rastro[2]);
    }
    for (int k = -1; k <= 1; k += 2)
        for (int x = (int)hx - 6; x < (int)hx + 6; x++) {
            int y = (int)hy + k * 4;
            if (cv_ok(x, y) && cv->a[y][x].a == 0 && (x & 1)) cv_put(cv, x, y, ch->rastro[2]);
        }
}

static void weapons(Canvas *cv, const Char *ch, const Ctx *ctx) {
    const char *anim = ctx->anim;
    int idx = ctx->idx;
    const Seg *s = cv->seg;
    const Weapon *w = &ch->arma;
    Rgb core = ch->lamina[0], edge = ch->lamina[1];
    double escala = w->escala > 0 ? w->escala : 1.0;
    memset(cv->wpx, 0, sizeof cv->wpx);
    cv->pen = T_WEAPON;
    if (thrust_anim(ch, anim) && ctx->phase != PH_NONE) {
        thrust(cv, ch, ctx);
        if (ch->elemento != EL_NONE) {
            cv->pen = T_FX;
            blade_fx(cv, ch, anim, idx);
        }
        return;
    }
    /* Corpo de duas espadas com uma lâmina mais curta: a da mão de trás (a esquerda,
       com o cabo mais atrás) fica com o tamanho da segunda arma. */
    int front_blade = -1;
    for (int bi = 0; bi < s->nblades; bi++) {
        const Blade *b = &s->blades[bi];
        if (b->nearest > 8 || b->loose || b->farthest < 5) continue;
        if (front_blade < 0 || b->hilt[0] > s->blades[front_blade].hilt[0]) front_blade = bi;
    }
    int left_blade = -1;
    if (ch->pack && ch->pack_par && w->par_comprimento > 0) {
        int nh = 0;
        for (int bi = 0; bi < s->nblades; bi++) {
            const Blade *b = &s->blades[bi];
            if (b->nearest > 8 || b->loose || b->farthest < 5) continue;
            nh++;
            if (left_blade < 0 || b->hilt[0] < s->blades[left_blade].hilt[0]) left_blade = bi;
        }
        if (nh < 2) left_blade = -1;
    }
    for (int bi = 0; bi < s->nblades; bi++) {
        const Blade *b = &s->blades[bi];
        double full = b->farthest;
        if (bi == left_blade) {
            for (int i = 0; i < b->n; i++) {
                if (b->dist[i] > w->par_comprimento) erase_px(cv, b->x[i], b->y[i]);
                else mark(cv, b->x[i], b->y[i]);
            }
            continue;
        }
        if (ch->pack && ch->pack_arma) {
            for (int i = 0; i < b->n; i++) mark(cv, b->x[i], b->y[i]);
            continue;
        }
        if (full < 5) continue;
        bool skip_pair = false;
        /* só a lâmina que sai da mão cresce ou vira outra arma; pedaço solto (a
           ponta aparecendo no meio do rastro) fica como está */
        bool at_hand = blade_at_hand(b);
        switch (w->kind) {
            case W_KATANA: case W_DUPLA:
                if (escala < 1.0 && at_hand) {
                    /* wakizashi: a mesma espada, mais curta */
                    for (int i = 0; i < b->n; i++) {
                        if (b->dist[i] > KATANA * escala) erase_px(cv, b->x[i], b->y[i]);
                        else mark(cv, b->x[i], b->y[i]);
                    }
                    break;
                }
                for (int i = 0; i < b->n; i++) mark(cv, b->x[i], b->y[i]);
                if (escala > 1.0 && at_hand) stroke(cv, b, full, fmax(full, KATANA * escala), core, &edge, 0, 0, 2, false);
                break;
            case W_ODACHI:
            case W_PESADA: {
                for (int i = 0; i < b->n; i++) mark(cv, b->x[i], b->y[i]);
                if (!at_hand) break;
                stroke(cv, b, full - 1, fmax(full, KATANA * escala), core, &edge, 0, 0, 2, false);
                if (w->largura <= 0) break;
                /* lâmina larga: uma fileira a mais do lado do fio */
                double n[2];
                perp(b->u, n);
                Rgb cl = rgb_set(w->cor_largura) ? w->cor_largura : edge;
                static bool snap[CH][CW];
                memcpy(snap, cv->wpx, sizeof snap);
                for (int y = 0; y < CH; y++)
                    for (int x = 0; x < CW; x++) {
                        if (!snap[y][x]) continue;
                        for (int k = 1; k <= (w->largura > 2 ? 2 : 1); k++) {
                            int ex = pyround(x - n[0] * k), ey = pyround(y - n[1] * k);
                            if (blade_dist(b, x, y, 99) > 2 && empty_orig(cv, ex, ey)) cv_put(cv, ex, ey, cl);
                        }
                    }
                break;
            }
            case W_FLORETE: {
                /* florete de esgrima: some a lâmina curva do pack e entra uma lâmina reta
                   e fina, do cabo à ponta, com o copo (a campânula) na mão */
                for (int i = 0; i < b->n; i++) erase_px(cv, b->x[i], b->y[i]);
                if (!at_hand) break;
                double len = fmax(full, KATANA * escala);
                stroke(cv, b, 2, len - 2, core, NULL, 0, 0, 2, false);
                stroke(cv, b, len - 2, len, edge, NULL, 0, 0, 2, false);
                double n[2];
                perp(b->u, n);
                for (int k = -2; k <= 2; k++) {
                    double back = abs(k) == 2 ? 0.6 : 1.6;  /* copo em arco, as bordas voltadas para a mão */
                    int x = pyround(b->hilt[0] + b->u[0] * back + n[0] * k), y = pyround(b->hilt[1] + b->u[1] * back + n[1] * k);
                    if (cv_ok(x, y) && (weapon_ok(cv, x, y) || lab_at(cv, x, y) == HANDLE))
                        cv_put(cv, x, y, abs(k) == 2 ? ch->destaque[1] : ch->destaque[0]);
                }
                break;
            }
            case W_ADAGA: case W_CURTA: {
                /* a lâmina curta é desenhada inteira: cabo, guarda e lâmina */
                double keep = w->comprimento > 0 ? w->comprimento : 7;
                for (int i = 0; i < b->n; i++) erase_px(cv, b->x[i], b->y[i]);
                if (b->nearest > keep) { skip_pair = true; break; }
                Rgb grip = ch->cabo, guard = rgb_set(w->guarda) ? w->guarda : ch->destaque[1];
                Blade rb = *b;  /* empunhadura invertida: a lâmina aponta para o outro lado do punho */
                if (w->reverso) { rb.u[0] = -b->u[0]; rb.u[1] = -b->u[1]; }
                g_over_body = w->reverso && bi == front_blade;
                draw_short_blade(cv, &rb, 0, 0, keep, core, edge, guard, grip, false);
                g_over_body = false;
                if (w->par && !ch->pack_par) {
                    /* a outra adaga na mão esquerda, empunhada ao contrário */
                    Blade o = other_hand(b, 0.95);
                    Rgb c2 = rgb_set(w->cor_par) ? w->cor_par : edge;
                    draw_short_blade(cv, &o, 0, 0, keep - 1, c2, edge, guard, grip, true);
                }
                skip_pair = true;
                break;
            }
            case W_LANCA: case W_CAJADO: {
                double tip = KATANA * escala;
                for (int i = 0; i < b->n; i++) erase_px(cv, b->x[i], b->y[i]);
                if (!at_hand) { skip_pair = true; break; }
                int head = w->kind == W_LANCA ? (w->ponta ? w->ponta : 5) : 2;
                int back = w->atras ? w->atras : 14;
                stroke(cv, b, -back, tip - head, w->haste[0], &w->haste[1], 0, 0, 2, false);
                if (w->kind == W_LANCA) {
                    stroke(cv, b, tip - head, tip, core, &edge, 0, 0, 1, false);
                    /* ponta em folha: larga no terço de baixo e afinando até a ponta */
                    double n[2];
                    perp(b->u, n);
                    for (int j = 1; j <= 2; j++) {
                        double t = tip - head + 1 + j;
                        for (int k = -1; k <= 1; k += 2) {
                            int x = pyround(b->hilt[0] + b->u[0] * t + n[0] * k), y = pyround(b->hilt[1] + b->u[1] * t + n[1] * k);
                            if (empty_orig(cv, x, y)) {
                                cv_put(cv, x, y, j == 1 ? edge : core);
                                mark(cv, x, y);
                            }
                        }
                    }
                    /* anel de metal na base da ponta e uma fita curta que balança */
                    double cx = b->hilt[0] + b->u[0] * (tip - head), cy = b->hilt[1] + b->u[1] * (tip - head);
                    for (int k = -1; k <= 1; k++) {
                        int x = pyround(cx + n[0] * k), y = pyround(cy + n[1] * k);
                        if (cv_ok(x, y) && weapon_ok(cv, x, y)) cv_put(cv, x, y, ch->destaque[1]);
                    }
                    int sw = (ctx->idx / 2) % 2;
                    for (int j = 1; j <= 3; j++) {
                        int x = pyround(cx - b->u[0] * (1 + sw) - n[0] * 0.3 * j), y = pyround(cy - b->u[1] * (1 + sw) + j);
                        if (cv_ok(x, y) && cv->a[y][x].a == 0) cv_put(cv, x, y, j == 3 ? ch->destaque[1] : ch->destaque[0]);
                    }
                } else {
                    Rgb cap = rgb_set(w->ponteira) ? w->ponteira : core;
                    stroke(cv, b, tip - 2, tip, cap, NULL, 0, 0, 2, false);
                    stroke(cv, b, -back - 2, -back, cap, NULL, 0, 0, 2, false);
                }
                break;
            }
            case W_FOICE: {
                for (int i = 0; i < b->n; i++) erase_px(cv, b->x[i], b->y[i]);
                skip_pair = true;
                if (b->nearest > 3 || b->loose) break;
                /* foice pequena (kama): quase tudo é cabo, e a lâmina curva fica na ponta */
                double len = w->comprimento > 0 ? w->comprimento : KATANA * escala * 0.8;
                kama(cv, b, len, w, core, edge, false);
                /* a outra foice na mão esquerda */
                if (w->par && !ch->pack_par) {
                    Blade o = other_hand(b, 0.8);
                    kama(cv, &o, len - 1, w, rgb_set(w->cor_par) ? w->cor_par : core, edge, true);
                }
                break;
            }
            case W_GARRAS: {
                for (int i = 0; i < b->n; i++) erase_px(cv, b->x[i], b->y[i]);
                if (b->nearest > 3 || b->loose) { skip_pair = true; break; }
                double size = w->comprimento > 0 ? w->comprimento : 8;
                claws(cv, b, size, core, edge, false);
                /* garras também na mão esquerda */
                if (w->par && !ch->pack_par) {
                    Blade o = other_hand(b, 0.8);
                    claws(cv, &o, size - 1, rgb_set(w->cor_par) ? w->cor_par : core, edge, true);
                }
                skip_pair = true;
                break;
            }
        }
        /* segunda arma na outra mão: cópia paralela, atrás do corpo */
        if (!skip_pair && w->par && w->par_comprimento > 0 && !ch->pack_par && at_hand && b->nearest <= 3) {
            /* espada e lâmina mais curta: a segunda sai da outra mão, aberta para baixo */
            Blade o = other_hand(b, 0.6);
            Rgb guard = rgb_set(w->guarda) ? w->guarda : ch->destaque[1];
            draw_short_blade(cv, &o, 0, 0, w->par_comprimento, rgb_set(w->cor_par) ? w->cor_par : core, edge, guard,
                             ch->cabo, true);
        } else if (!skip_pair && w->par && !ch->pack_par && (w->kind == W_DUPLA || w->kind == W_ADAGA || w->kind == W_KATANA) && b->nearest <= 3 && !b->loose) {
            double n[2];
            perp(b->u, n);
            double keep = w->comprimento > 0 ? w->comprimento : KATANA * escala;
            stroke(cv, b, 1, keep, rgb_set(w->cor_par) ? w->cor_par : edge, NULL,
                   -n[0] * 3 - b->u[0] * 2, -n[1] * 3 - b->u[1] * 2, 2, true);
        }
    }
    if (ch->elemento != EL_NONE) {
        cv->pen = T_FX;
        blade_fx(cv, ch, anim, idx);
    }
}


/* ----- aura ---------------------------------------------------------------- */
/* Partícula que nasce, anda e some em `life` quadros. A posição de nascimento
   muda a cada ciclo; entre um quadro e o seguinte ela anda de verdade, então a
   animação corre lisa. Sempre atrás do corpo e da arma. */
static void particles(Canvas *cv, const Ctx *ctx, const char *tag, int n, int life, int x0, int x1, int y0, int y1,
                      double vx, double vy, double wob, Rgb c0, Rgb c1, Rgb c2, int size) {
    for (int k = 0; k < n; k++) {
        int phase = (int)(hsh4(tag, ctx->anim, k, 0, 1) * life);
        int t = (ctx->idx + phase) % life, gen = (ctx->idx + phase) / life;
        double px = x0 + hsh4(tag, ctx->anim, k, gen, 2) * (x1 - x0) + vx * t + wob * sin(t * 1.3 + k);
        double py = y0 + hsh4(tag, ctx->anim, k, gen, 3) * (y1 - y0) + vy * t;
        int x = (int)floor(px + 0.5), y = (int)floor(py + 0.5);
        Rgb c = t * 3 < life ? c0 : t * 3 < 2 * life ? c1 : c2;
        for (int j = 0; j < size; j++)
            if (cv_ok(x, y - j) && cv->a[y - j][x].a == 0) cv_put(cv, x, y - j, c);
    }
}

static bool near_body(const Canvas *cv, int x, int y) {
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (cv_ok(x + dx, y + dy) && cv->tag[y + dy][x + dx] == T_BODY) return true;
    return false;
}

/* Contorno que cintila em volta do corpo: `p` é a chance de cada pixel acender. */
static void glow(Canvas *cv, const Ctx *ctx, double p, int ymid, Rgb top, Rgb bottom, bool only_top) {
    static bool out[CH][CW];
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) out[y][x] = cv->a[y][x].a == 0 && near_body(cv, x, y);
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            if (!out[y][x] || (only_top && y > ymid)) continue;
            if (hsh4("c", ctx->anim, ctx->idx, x, y) < p * 0.4) cv_put(cv, x, y, y < ymid ? top : bottom);
        }
}

/* Aura do elemento de cada um. Mais forte na preparação e no contato. */
static void aura(Canvas *cv, const Char *ch, const Ctx *ctx) {
    Element el = ch->elemento;
    int bx0 = CW, bx1 = -1, by0 = CH, by1 = -1;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (cv->tag[y][x] == T_BODY && cv->a[y][x].a) {
                if (x < bx0) bx0 = x;
                if (x > bx1) bx1 = x;
                if (y < by0) by0 = y;
                if (y > by1) by1 = y;
            }
    if (bx1 < 0) return;
    double pw = ctx->phase == PH_CONTACT ? 2.0 : ctx->phase == PH_STRIKE ? 1.5 : 1.0;
    int ymid = (by0 + by1) / 2;
    cv->pen = T_FX;
    WeaponKind k = ch->arma.kind;
    /* pó no chão no contato das armas pesadas */
    if (ctx->phase == PH_CONTACT && (k == W_ODACHI || k == W_PESADA || k == W_CAJADO)) {
        int fx0 = CW, fx1 = -1;
        for (int x = 0; x < CW; x++)
            if (cv->tag[by1][x] == T_BODY) { if (x < fx0) fx0 = x; if (x > fx1) fx1 = x; }
        for (int i = 1; i <= 6; i++) {
            Rgb d = i < 3 ? (Rgb){176, 160, 136} : (Rgb){120, 108, 94};
            for (int side = -1; side <= 1; side += 2) {
                int x = side < 0 ? fx0 - i : fx1 + i, y = by1 - (i > 2 && i < 5 ? 1 : 0);
                if (cv_ok(x, y) && cv->a[y][x].a == 0 && hsh4("d", ctx->anim, ctx->idx, x, y) < 0.8) cv_put(cv, x, y, d);
            }
        }
    }
    switch (el) {
        case EL_FOGO: {
            /* labaredas saindo do alto da silhueta */
            for (int x = bx0; x <= bx1; x++) {
                int yt = -1;
                for (int y = 0; y < CH; y++)
                    if (cv->tag[y][x] == T_BODY && cv->a[y][x].a) { yt = y; break; }
                if (yt < 0 || yt > ymid) continue;
                double r = hsh4("f", ctx->anim, ctx->idx, x, 0);
                if (r > 0.3 * pw) continue;
                int h = 1 + (int)(hsh4("g", ctx->anim, ctx->idx, x, 0) * 3 * pw);
                for (int j = 1; j <= h; j++) {
                    int y = yt - j;
                    if (!cv_ok(x, y) || cv->a[y][x].a) break;
                    cv_put(cv, x, y, j == h ? (Rgb){255, 222, 110} : j == 1 ? (Rgb){214, 56, 30} : (Rgb){255, 138, 40});
                }
            }
            particles(cv, ctx, "brasa", (int)(4 * pw), 8, bx0, bx1, ymid, by1, 0.3, -2.0, 0.6,
                      (Rgb){255, 230, 120}, (Rgb){255, 140, 40}, (Rgb){190, 50, 30}, 1);
            glow(cv, ctx, 0.12 * pw, ymid, (Rgb){255, 120, 40}, (Rgb){170, 40, 30}, false);
            break;
        }
        case EL_AGUA:
            glow(cv, ctx, 0.14 * pw, ymid, (Rgb){150, 235, 255}, (Rgb){60, 170, 230}, false);
            particles(cv, ctx, "bolha", (int)(4 * pw), 10, bx0, bx1, ymid, by1, 0, -1.2, 0.8,
                      (Rgb){200, 250, 255}, (Rgb){120, 220, 250}, (Rgb){60, 160, 220}, 1);
            break;
        case EL_RAIO: {
            glow(cv, ctx, 0.1 * pw, ymid, (Rgb){170, 220, 255}, (Rgb){60, 130, 255}, false);
            /* raios curtos saindo da silhueta */
            int nb = (int)(2 * pw);
            for (int i = 0; i < nb; i++) {
                int x = bx0 + (int)(hsh4("r", ctx->anim, ctx->idx, i, 1) * (bx1 - bx0 + 1));
                int y = by0 + (int)(hsh4("r", ctx->anim, ctx->idx, i, 2) * (by1 - by0 + 1));
                int dir = x < (bx0 + bx1) / 2 ? -1 : 1;
                while (cv_ok(x, y) && cv->a[y][x].a && x > 0 && x < CW - 1) x += dir;
                for (int j = 0; j < 5; j++) {
                    int yy = y + ((j % 2) ? 1 : -1) * (j > 0), xx = x + dir * j;
                    if (cv_ok(xx, yy) && cv->a[yy][xx].a == 0)
                        cv_put(cv, xx, yy, j < 2 ? (Rgb){240, 250, 255} : (Rgb){90, 170, 255});
                }
            }
            break;
        }
        case EL_ROXO:
            glow(cv, ctx, 0.16 * pw, ymid, (Rgb){150, 70, 230}, (Rgb){90, 40, 150}, false);
            /* a fumaça sobe do chão, para não cobrir as adagas */
            particles(cv, ctx, "fumo", (int)(4 * pw), 10, bx0 - 2, bx1 + 2, by1 - 6, by1, -0.4, -1.2, 0.5,
                      (Rgb){150, 80, 230}, (Rgb){110, 50, 180}, (Rgb){70, 34, 120}, 2);
            break;
        case EL_PENA:
            glow(cv, ctx, 0.07 * pw, ymid, (Rgb){200, 30, 44}, (Rgb){120, 16, 28}, false);
            for (int i = 0; i < (int)(3 * pw); i++) {
                int life = 12;
                int phase = (int)(hsh4("pena", ctx->anim, i, 0, 1) * life);
                int t = (ctx->idx + phase) % life, gen = (ctx->idx + phase) / life;
                int x = bx0 - 4 + (int)(hsh4("pena", ctx->anim, i, gen, 2) * (bx1 - bx0 + 8)) - t / 2;
                int y = by0 + (int)(hsh4("pena", ctx->anim, i, gen, 3) * (ymid - by0)) + t;
                if (cv_ok(x, y) && cv->a[y][x].a == 0) cv_put(cv, x, y, (Rgb){36, 26, 32});
                if (cv_ok(x + 1, y - 1) && cv->a[y - 1][x + 1].a == 0) cv_put(cv, x + 1, y - 1, (Rgb){60, 44, 54});
                if (cv_ok(x - 1, y + 1) && cv->a[y + 1][x - 1].a == 0) cv_put(cv, x - 1, y + 1, (Rgb){200, 40, 50});
            }
            break;
        case EL_TERRA:
            particles(cv, ctx, "po", (int)(4 * pw), 6, bx0 - 2, bx1 + 2, by1 - 1, by1, 0, -0.4, 1.5,
                      (Rgb){190, 160, 110}, (Rgb){150, 120, 76}, (Rgb){110, 88, 60}, 1);
            glow(cv, ctx, 0.06 * pw, ymid, (Rgb){224, 176, 80}, (Rgb){150, 110, 60}, false);
            break;
        case EL_VENTO:
            for (int i = 0; i < (int)(3 * pw); i++) {
                int life = 8;
                int phase = (int)(hsh4("vento", ctx->anim, i, 0, 1) * life);
                int t = (ctx->idx + phase) % life, gen = (ctx->idx + phase) / life;
                int y = by0 + 2 + (int)(hsh4("vento", ctx->anim, i, gen, 2) * (by1 - by0 - 4));
                int len = 4 + (int)(hsh4("vento", ctx->anim, i, gen, 3) * 4);
                int x = bx1 + 8 - t * 4;
                for (int j = 0; j < len; j++)
                    if (cv_ok(x + j, y) && cv->a[y][x + j].a == 0)
                        cv_put(cv, x + j, y, j == 0 ? (Rgb){240, 255, 220} : (Rgb){190, 250, 110});
            }
            break;
        case EL_OURO:
            glow(cv, ctx, 0.08 * pw, ymid, (Rgb){255, 214, 90}, (Rgb){190, 140, 40}, true);
            particles(cv, ctx, "ouro", (int)(4 * pw), 10, bx0, bx1, ymid, by1, 0, -1.6, 0.7,
                      (Rgb){255, 244, 190}, (Rgb){255, 208, 80}, (Rgb){180, 130, 40}, 1);
            break;
        case EL_SOMBRA:
            glow(cv, ctx, 0.22 * pw, ymid, (Rgb){70, 40, 110}, (Rgb){44, 24, 70}, false);
            particles(cv, ctx, "sombra", (int)(6 * pw), 10, bx0, bx1, by0, by1, -0.3, -1.1, 0.6,
                      (Rgb){110, 70, 170}, (Rgb){70, 40, 110}, (Rgb){40, 22, 64}, 2);
            particles(cv, ctx, "fagulha", (int)(2 * pw), 8, bx0, bx1, ymid, by1, 0, -1.8, 0.4,
                      (Rgb){255, 236, 150}, (Rgb){255, 204, 64}, (Rgb){170, 120, 30}, 1);
            break;
        case EL_MUSGO:
            glow(cv, ctx, 0.05 * pw, ymid, (Rgb){150, 230, 110}, (Rgb){70, 140, 60}, false);
            particles(cv, ctx, "esporo", (int)(3 * pw), 12, bx0, bx1, ymid, by1, 0.2, -0.8, 1.0,
                      (Rgb){200, 255, 160}, (Rgb){140, 220, 100}, (Rgb){80, 150, 70}, 1);
            break;
        case EL_LUA:
            /* luar: um halo pálido e poeira de prata subindo devagar */
            glow(cv, ctx, 0.07 * pw, ymid, (Rgb){226, 222, 255}, (Rgb){150, 140, 200}, true);
            particles(cv, ctx, "prata", (int)(3 * pw), 8, bx0 - 2, bx1 + 2, ymid, by1, 0, -0.7, 0.8,
                      (Rgb){255, 255, 255}, (Rgb){214, 208, 255}, (Rgb){150, 140, 210}, 1);
            break;
        case EL_POEIRA:
            particles(cv, ctx, "pedra", (int)(3 * pw), 10, bx0 - 2, bx1 + 2, by0, ymid, -0.2, 1.4, 0.3,
                      (Rgb){220, 214, 200}, (Rgb){160, 154, 142}, (Rgb){110, 106, 98}, 1);
            glow(cv, ctx, 0.05 * pw, ymid, (Rgb){236, 230, 212}, (Rgb){150, 146, 136}, false);
            break;
        default: break;
    }
}

/* ----- corpo --------------------------------------------------------------- */
/* Largura e altura próprias: colunas repetidas (ou tiradas) no meio do tronco e
   linhas na cintura. Tudo em pixel inteiro; os pés continuam no chão. */
static void resize_body(Canvas *cv, int cx, int wy, int dw, int dh) {
    static Color a[CH][CW];
    static unsigned char t[CH][CW];
    if (dw && cx > 0 && cx < CW - 1) {
        memcpy(a, cv->a, sizeof a);
        memcpy(t, cv->tag, sizeof t);
        for (int y = 0; y < CH; y++)
            for (int x = cx + 1; x < CW; x++) {
                int sx = dw > 0 ? (x - dw > cx ? x - dw : cx) : x - dw;
                if (sx < CW) { cv->a[y][x] = a[y][sx]; cv->tag[y][x] = t[y][sx]; }
                else { cv->a[y][x] = (Color){0, 0, 0, 0}; cv->tag[y][x] = T_NONE; }
            }
    }
    if (dh && wy > 1 && wy < CH) {
        memcpy(a, cv->a, sizeof a);
        memcpy(t, cv->tag, sizeof t);
        for (int y = 0; y < wy; y++) {
            int sy = y + dh;  /* mais alto: a parte de cima sobe dh linhas */
            if (dh > 0 && sy >= wy) sy = wy - 1;
            if (sy >= 0 && sy < wy) { memcpy(cv->a[y], a[sy], sizeof a[0]); memcpy(cv->tag[y], t[sy], sizeof t[0]); }
            else { memset(cv->a[y], 0, sizeof a[0]); memset(cv->tag[y], 0, sizeof t[0]); }
        }
    }
}

/* Passo à frente no golpe (o quadro inteiro anda dx pixels). */
static void translate(Canvas *cv, int dx) {
    if (!dx) return;
    static Color a[CH][CW];
    static unsigned char t[CH][CW];
    memcpy(a, cv->a, sizeof a);
    memcpy(t, cv->tag, sizeof t);
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            int sx = x - dx;
            if (sx >= 0 && sx < CW) { cv->a[y][x] = a[y][sx]; cv->tag[y][x] = t[y][sx]; }
            else { cv->a[y][x] = (Color){0, 0, 0, 0}; cv->tag[y][x] = T_NONE; }
        }
}

/* Quanto o corpo avança no contato e logo depois, conforme a arma. */
static int lunge(const Char *ch, const Ctx *ctx) {
    int c = 0, r = 0;
    switch (ch->arma.kind) {
        case W_LANCA: c = 3; r = 1; break;
        case W_FLORETE: c = 2; r = 1; break;
        case W_ADAGA: case W_GARRAS: case W_FOICE: c = 2; r = 1; break;
        case W_ODACHI: case W_PESADA: c = 1; break;
        default: break;
    }
    if (ctx->phase == PH_CONTACT) return c;
    if (ctx->phase == PH_RECOVERY && ctx->idx == ctx->contact + 1) return r;
    return 0;
}


/* ----- golpe especial ------------------------------------------------------ */
/* Um golpe só, bem telegrafado: num jogo de parry cada golpe que aparece tem
   que bater com um contato do núcleo. Muda a preparação e o impacto. */
typedef struct { const char *anim; int frame, phase, dx; } Step;

static int special_steps(SpecialMove m, Step *st) {
    static const Step salto[] = {  /* ergue a arma, segura lá em cima e desce com tudo */
        {"ATTACK_3", 0, PH_ANTICIPATION, 0}, {"ATTACK_3", 0, PH_ANTICIPATION, -1}, {"ATTACK_3", 1, PH_ANTICIPATION, -1},
        {"ATTACK_3", 1, PH_STRIKE, -2}, {"ATTACK_3", 2, PH_CONTACT, 2}, {"ATTACK_3", 3, PH_RECOVERY, 2},
        {"ATTACK_3", 3, PH_RECOVERY, 1}, {"ATTACK_3", 4, PH_RECOVERY, 0}};
    static const Step investida[] = {  /* agacha, some num risco e corta do outro lado */
        {"DASH_ATTACK", 0, PH_ANTICIPATION, 0}, {"DASH_ATTACK", 1, PH_ANTICIPATION, 0}, {"DASH_ATTACK", 2, PH_ANTICIPATION, -1},
        {"DASH_ATTACK", 3, PH_STRIKE, -2}, {"DASH_ATTACK", 4, PH_CONTACT, 4}, {"DASH_ATTACK", 5, PH_RECOVERY, 3},
        {"DASH_ATTACK", 6, PH_RECOVERY, 2}, {"DASH_ATTACK", 7, PH_RECOVERY, 1}, {"DASH_ATTACK", 8, PH_RECOVERY, 0}};
    static const Step estocada[] = {  /* agacha e dispara uma estocada longa */
        {"DASH_ATTACK", 0, PH_ANTICIPATION, 0}, {"DASH_ATTACK", 2, PH_ANTICIPATION, -1}, {"DASH_ATTACK", 3, PH_STRIKE, -2},
        {"ATTACK_1", 2, PH_CONTACT, 6}, {"ATTACK_1", 3, PH_RECOVERY, 4}, {"ATTACK_1", 3, PH_RECOVERY, 2},
        {"ATTACK_1", 4, PH_RECOVERY, 0}};
    static const Step ascendente[] = {  /* abaixa a guarda e corta subindo */
        {"ATTACK_2", 0, PH_ANTICIPATION, 0}, {"ATTACK_2", 0, PH_ANTICIPATION, -1}, {"ATTACK_2", 1, PH_STRIKE, -1},
        {"ATTACK_2", 2, PH_CONTACT, 2}, {"ATTACK_2", 3, PH_RECOVERY, 2}, {"ATTACK_2", 3, PH_RECOVERY, 1},
        {"ATTACK_2", 4, PH_RECOVERY, 0}};
    const Step *src = NULL;
    int n = 0;
    switch (m) {
        case SP_SALTO: src = salto; n = (int)(sizeof salto / sizeof salto[0]); break;
        case SP_INVESTIDA: src = investida; n = (int)(sizeof investida / sizeof investida[0]); break;
        case SP_ESTOCADA: src = estocada; n = (int)(sizeof estocada / sizeof estocada[0]); break;
        case SP_ASCENDENTE: src = ascendente; n = (int)(sizeof ascendente / sizeof ascendente[0]); break;
        default: return 0;
    }
    memcpy(st, src, sizeof(Step) * (size_t)n);
    return n;
}

static void fx_px(Canvas *cv, int x, int y, Rgb c) {
    if (!cv_ok(x, y)) return;
    bool smear = cv->tag[y][x] == T_WEAPON && cv->seg->lab[y][x] == SMEAR;
    if (cv->a[y][x].a == 0 || smear) cv_put(cv, x, y, c);
}

/* Linha grossa no meio e fina nas pontas (corte em X, garras). */
static void fx_slash(Canvas *cv, double x0, double y0, double x1, double y1, Rgb core, Rgb edge) {
    static Pts p;
    line_pts(&p, x0, y0, x1, y1);
    for (int i = 0; i < p.n; i++) {
        double t = p.n > 1 ? (double)i / (p.n - 1) : 0.5;
        fx_px(cv, p.x[i], p.y[i], core);
        if (t > 0.2 && t < 0.8) { fx_px(cv, p.x[i] + 1, p.y[i], edge); fx_px(cv, p.x[i], p.y[i] + 1, edge); }
    }
}

/* O efeito do elemento no impacto. O alvo de verdade fica fora do quadro de
   106 px, então o efeito vai no caminho da lâmina: os de chão a 3/4 do alcance,
   os de ar um pouco antes da ponta. ax é a âncora, (rdx, rdy) o alcance do
   contato, gy a linha do chão e age os quadros desde o contato. */
static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

static void special_fx(Canvas *cv, const Char *ch, SpecialFx fx, int ax, int rdx, int ty, int gy, int age) {
    bool ground = fx == FX_CHOQUE || fx == FX_PEDRAS || fx == FX_AVALANCHE || fx == FX_FOGO || fx == FX_ONDA || fx == FX_RAIO;
    int tx = ground ? clampi(ax + (int)(rdx * 0.75), 0, cellw - 10) : clampi(ax + rdx - 6, 0, cellw - 12);
    cv->pen = T_FX;
    Rgb c0 = ch->rastro[0], c1 = ch->rastro[1], c2 = ch->rastro[2];
    switch (fx) {
        case FX_CHOQUE: case FX_PEDRAS: case FX_AVALANCHE: {
            /* onda de choque rasteira que abre a partir do impacto */
            if (age <= 3) {
                int r = 7 + age * 7;
                for (int dx = -r; dx <= r; dx++) {
                    double k = 1 - (double)(dx * dx) / (r * r);
                    int h = (int)floor(sqrt(k > 0 ? k : 0) * r * 0.35 + 0.5);
                    /* no fim a onda já baixou: só a poeira assentando no chão */
                    if (age == 3) {
                        if (dx % 3 == 0 && abs(dx) > r / 3) fx_px(cv, tx + dx, gy - (h > 1), (Rgb){176, 160, 136});
                        continue;
                    }
                    /* poeira clara na base e o brilho do elemento na crista */
                    fx_px(cv, tx + dx, gy - h, age <= 1 ? (Rgb){236, 226, 206} : (Rgb){176, 160, 136});
                    if (age < 3) fx_px(cv, tx + dx, gy - h - 1, age == 0 ? (Rgb){255, 255, 255} : c0);
                }
                for (int k = 0; k < 8 - age * 2; k++) {
                    int x = tx - r + (int)(hsh4("poeira", "fx", age, k, 0) * 2 * r), y = gy - (int)(hsh4("poeira", "fx", age, k, 1) * (4 + age * 2));
                    fx_px(cv, x, y, (Rgb){176, 160, 136});
                }
            }
            if (age == 0)
                for (int y = gy - 16; y <= gy; y++) fx_px(cv, tx, y, c0);
            if (fx == FX_PEDRAS && age <= 3) {
                /* rachaduras no chão e pedras voando */
                Rgb rock = {120, 96, 64}, dark = {70, 56, 40};
                for (int k = 0; k < 6; k++) {
                    double vx = (k - 2.5) * 1.4, vy = 5 + (k % 3) * 1.5, t = age + 1;
                    int x = tx + (int)floor(vx * t + 0.5), y = gy - (int)floor(vy * t - 1.2 * t * t + 0.5);
                    if (y <= gy) { fx_px(cv, x, y, rock); fx_px(cv, x + 1, y, dark); fx_px(cv, x, y - 1, rock); }
                }
                for (int side = -1; side <= 1; side += 2)
                    for (int i = 1; i <= 7 + age * 2; i++) fx_px(cv, tx + side * i, gy + ((i / 2) % 2 ? -1 : 0), dark);
            }
            if (fx == FX_AVALANCHE && age <= 3) {
                /* pedras caindo do alto em cima do alvo */
                Rgb rock = {176, 170, 158}, dark = {110, 106, 98};
                for (int k = 0; k < 6; k++) {
                    int x = tx - 10 + k * 4 + (k % 2) * 2, y = 2 + age * 14 + (k * 5) % 11;
                    if (y + 2 >= gy) continue;
                    for (int j = 0; j < 3; j++)
                        for (int i = 0; i < 3; i++)
                            if (!(i == 2 && j == 0)) fx_px(cv, x + i, y + j, j == 2 || i == 2 ? dark : rock);
                }
            }
            break;
        }
        case FX_FOGO: {
            /* coluna de fogo no alvo */
            static const int hh[] = {12, 30, 38, 22, 10};
            if (age > 4) break;
            int h = hh[age];
            for (int j = 0; j < h; j++) {
                int y = gy - j;
                double t = (double)j / h;
                int w = t < 0.8 ? 3 : 1;
                for (int dx = -w; dx <= w; dx++) {
                    double r = hsh4("pilar", "fx", age, dx, j);
                    if (abs(dx) == w && r < 0.35) continue;
                    Rgb c = abs(dx) <= 1 && t < 0.7 ? (Rgb){255, 244, 190} : abs(dx) <= 2 ? (Rgb){255, 150, 40} : (Rgb){214, 56, 30};
                    fx_px(cv, tx + dx + (int)((hsh4("pilar", "x", age, j, 0) - 0.5) * 2), y, c);
                }
            }
            for (int k = 0; k < 6; k++)
                fx_px(cv, tx - 6 + (int)(hsh4("brasa", "fx", age, k, 0) * 12), gy - h - (int)(hsh4("brasa", "fx", age, k, 1) * 8),
                      (Rgb){255, 200, 70});
            break;
        }
        case FX_RAIO: {
            /* raio caindo do céu no alvo */
            if (age > 2) break;
            int x = tx;
            for (int y = 2; y <= gy; y++) {
                if (y % 4 == 0) x += (hsh4("raio", "fx", 0, y, 0) < 0.5) ? -1 : 1;
                if (age == 0) {
                    fx_px(cv, x, y, (Rgb){250, 252, 255});
                    fx_px(cv, x + 1, y, (Rgb){90, 170, 255});
                    fx_px(cv, x - 1, y, (Rgb){60, 120, 255});
                } else if (age == 1 && (y / 3) % 2) {
                    fx_px(cv, x, y, (Rgb){120, 190, 255});
                }
                if (age == 0 && y % 13 == 0) fx_slash(cv, x, y, x + 5, y + 4, (Rgb){170, 220, 255}, (Rgb){60, 120, 255});
            }
            for (int dx = -(4 + age * 4); dx <= 4 + age * 4; dx += 2) fx_px(cv, tx + dx, gy - (age == 0 ? 1 : 0), (Rgb){170, 220, 255});
            break;
        }
        case FX_ONDA: {
            /* onda de água que rola para a frente a partir da lança */
            if (age > 3) break;
            int x0 = tx - 14 + age * 5, len = 22, top = 16 - age * 3;
            for (int i = 0; i < len; i++) {
                double t = (double)i / (len - 1);
                int h = (int)floor(sin(t * 3.14159) * top + 0.5);
                for (int j = 0; j <= h; j++)
                    fx_px(cv, x0 + i, gy - j, j == h ? (Rgb){236, 255, 255} : j > h - 3 ? c0 : j > h / 2 ? c1 : c2);
            }
            for (int k = 0; k < 6; k++)
                fx_px(cv, x0 + len - 2 + (int)(hsh4("gota", "fx", age, k, 0) * 6), gy - top - 2 - (int)(hsh4("gota", "fx", age, k, 1) * 6), c0);
            break;
        }
        case FX_GOTA: {
            /* coroa de água no ponto da estocada */
            if (age > 3) break;
            int r = 3 + age * 3;
            for (int a = 0; a < 16; a++) {
                if ((a + age) % 3 == 0) continue;
                double ang = a * 3.14159 / 8;
                fx_px(cv, tx + (int)floor(cos(ang) * r + 0.5), ty + (int)floor(sin(ang) * r * 0.8 + 0.5), a % 2 ? c0 : c1);
            }
            for (int k = 0; k < 5; k++) {
                double ang = -1.2 - k * 0.2;
                fx_px(cv, tx + (int)(cos(ang) * (r + 3)), ty + (int)(sin(ang) * (r + 3)) + age, (Rgb){200, 250, 255});
            }
            break;
        }
        case FX_X: case FX_SOMBRA: {
            /* corte em X no alvo */
            if (age > 2) break;
            int L = 7 + age * 2;
            Rgb core = fx == FX_SOMBRA ? (Rgb){40, 22, 64} : c0, edge = fx == FX_SOMBRA ? (Rgb){110, 70, 170} : c1;
            if (age == 2) { core = edge; edge = c2; }
            fx_slash(cv, tx - L, ty - L, tx + L, ty + L, core, edge);
            fx_slash(cv, tx - L, ty + L, tx + L, ty - L, core, edge);
            if (fx == FX_SOMBRA)
                for (int k = 0; k < 6; k++)
                    fx_px(cv, tx - 8 + (int)(hsh4("ouro", "fx", age, k, 0) * 16), ty - 8 + (int)(hsh4("ouro", "fx", age, k, 1) * 16),
                          (Rgb){255, 214, 90});
            break;
        }
        case FX_GARRA: {
            /* três riscos de garra no alvo e penas soltas */
            if (age > 2) break;
            for (int k = -1; k <= 1; k++)
                fx_slash(cv, tx + 6 + k * 3, ty - 8, tx - 6 + k * 3, ty + 8, age == 0 ? (Rgb){255, 240, 240} : c1, c2);
            for (int k = 0; k < 5; k++) {
                int x = tx - 10 + (int)(hsh4("pena", "fx", age, k, 0) * 20), y = ty - 10 + (int)(hsh4("pena", "fx", age, k, 1) * 20) + age * 2;
                fx_px(cv, x, y, (Rgb){36, 26, 32});
                fx_px(cv, x + 1, y - 1, (Rgb){200, 40, 50});
            }
            break;
        }
        case FX_VORTICE: {
            /* redemoinho de vento em volta do alvo */
            if (age > 3) break;
            for (int ring = 0; ring < 2; ring++) {
                int r = 5 + ring * 4 + age;
                for (int a = 0; a < 24; a++) {
                    double ang = a * 3.14159 / 12 + age * 0.9 + ring;
                    if ((a + ring * 5) % 8 < 3) continue;
                    fx_px(cv, tx + (int)floor(cos(ang) * r + 0.5), ty + (int)floor(sin(ang) * r * 0.6 + 0.5), ring ? c1 : c0);
                }
            }
            break;
        }
        case FX_CASCO: {
            /* escudo de casco que se abre na frente e racha */
            if (age > 3) break;
            static const char *hexa[] = {
                "....aaaaa....", "..aabbbbbaa..", ".abAAaAAaAAba", "abAAaAAAaAAba", "aAAaAAAAAaAAa",
                "aAaAAAAAAAaAa", "aAAaAAAAAaAAa", "abAAaAAAaAAba", ".abAAaAAaAAba", "..aabbbbbaa..", "....aaaaa....",
            };
            for (int ry = 0; ry < 11; ry++)
                for (int rx = 0; rx < 13; rx++) {
                    char k = hexa[ry][rx];
                    if (k == '.' || (age >= 2 && hsh4("casco", "fx", age, rx, ry) < 0.3 * age)) continue;
                    fx_px(cv, tx - 6 + rx, ty - 5 + ry, k == 'a' ? ch->destaque[1] : k == 'A' ? ch->destaque[0] : ch->destaque2);
                }
            break;
        }
        default: break;
    }
}

/* ----- grito do Oboro -------------------------------------------------------- */
/* Linhas do grito saindo da cabeça e anéis de fúria abrindo em volta do corpo. */
static void scream_fx(Canvas *cv, int age) {
    int bx0 = CW, bx1 = -1, by0 = CH, by1 = -1;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (cv->tag[y][x] == T_BODY && cv->a[y][x].a) {
                if (x < bx0) bx0 = x;
                if (x > bx1) bx1 = x;
                if (y < by0) by0 = y;
                if (y > by1) by1 = y;
            }
    if (bx1 < 0) return;
    cv->pen = T_FX;
    /* cabeça: o meio das primeiras linhas do corpo */
    int hx0 = CW, hx1 = -1;
    for (int y = by0; y < by0 + 8 && y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (cv->tag[y][x] == T_BODY && cv->a[y][x].a) { if (x < hx0) hx0 = x; if (x > hx1) hx1 = x; }
    int hx = (hx0 + hx1) / 2 + 2, hy = by0 + 7;
    for (int i = 0; i < 9; i++) {
        double a = -1.35 + i * 0.34 + ((age & 1) ? 0.12 : 0);
        double r0 = 7 + age % 4, len = 3 + (i + age) % 3;
        for (int j = 0; j < (int)len; j++) {
            int x = hx + (int)floor(cos(a) * (r0 + j) + 0.5), y = hy + (int)floor(sin(a) * (r0 + j) * 0.8 + 0.5);
            if (cv_ok(x, y) && cv->a[y][x].a == 0) cv_put(cv, x, y, j == 0 ? (Rgb){255, 255, 255} : (Rgb){255, 120, 120});
        }
    }
    int cx = (bx0 + bx1) / 2, cy = (by0 + by1) / 2, R = 6 + age * 5;
    for (int k = 0; k < 64; k++) {
        if ((k + age) % 4 == 0) continue;
        double a = k * 3.14159 / 32;
        int x = cx + (int)floor(cos(a) * R + 0.5), y = cy + (int)floor(sin(a) * R * 0.75 + 0.5);
        if (cv_ok(x, y) && cv->a[y][x].a == 0) cv_put(cv, x, y, k % 3 ? (Rgb){196, 36, 48} : (Rgb){255, 190, 190});
    }
}

static void render(const Frame *f, const Seg *seg, const Char *ch, const Ctx *ctx, Canvas *cv);

/* Parado, a fúria sobe, ele treme e grita. Monta a partir do IDLE e do IDLE_FURIA. */
static int grito(const Strip *idle, const Strip *fury, const Char *ch, Canvas *cv, Frame *out) {
    static const struct { int furia, frame, shake, age; } seq[] = {
        {0, 0, 0, -1}, {0, 1, 0, -1}, {0, 2, 1, -1}, {1, 0, -1, 0}, {1, 1, 2, 1}, {1, 2, -2, 2}, {1, 3, 2, 3},
        {1, 4, -2, 4}, {1, 5, 2, 5}, {1, 0, -2, 6}, {1, 1, 1, 7}, {1, 2, -1, 8}, {1, 3, 0, -1}, {1, 4, 0, -1},
    };
    int n = (int)(sizeof seq / sizeof seq[0]);
    for (int k = 0; k < n; k++) {
        const Strip *st = seq[k].furia ? fury : idle;
        int fi = seq[k].frame < st->nframes ? seq[k].frame : st->nframes - 1;
        Ctx c = {st->name, fi, st->nframes, -1, -1, seq[k].age >= 0 ? PH_CONTACT : PH_NONE};
        render(&st->frames[fi], &st->segs[fi], ch, &c, cv);
        translate(cv, seq[k].shake);
        if (seq[k].age >= 0) scream_fx(cv, seq[k].age);
        memcpy(out[k].p, cv->a, sizeof cv->a);
    }
    return n;
}

/* ----- corte de vento (Hayate) ---------------------------------------------- */
/* No contato e nos dois quadros seguintes, uma meia-lua de vento sai da ponta da
   arma e voa para a frente, abrindo. É efeito: não entra no alcance do parry. */
static void wind_slash(Canvas *cv, const Char *ch, const Ctx *ctx) {
    if (ctx->contact < 0 || ctx->idx < ctx->contact || ctx->idx > ctx->contact + 2) return;
    int age = ctx->idx - ctx->contact, xm = -1;
    for (int x = CW - 1; x >= 0 && xm < 0; x--)
        for (int y = 0; y < CH; y++)
            if (cv->tag[y][x] == T_WEAPON && cv->a[y][x].a) { xm = x; break; }
    if (xm < 0) return;
    double sy = 0;
    int n = 0;
    for (int x = xm - 3; x <= xm; x++)
        for (int y = 0; y < CH; y++)
            if (cv_ok(x, y) && cv->tag[y][x] == T_WEAPON && cv->a[y][x].a) { sy += y; n++; }
    int cx = xm + 2 + age * 8, cy = (int)floor(sy / n + 0.5), r = 5 + age * 2;
    cv->pen = T_FX;
    for (int k = -12; k <= 12; k++) {
        double t = k * 0.11;
        for (int e = 0; e < (age < 2 ? 2 : 1); e++) {
            double rr = r - e;
            int x = cx + (int)floor(cos(t) * rr * 0.55 + 0.5), y = cy + (int)floor(sin(t) * rr + 0.5);
            if (x >= cellw || !cv_ok(x, y) || cv->a[y][x].a) continue;
            cv_put(cv, x, y, e ? ch->rastro[1] : abs(k) > 9 ? ch->rastro[2] : ch->rastro[0]);
        }
    }
    for (int k = -1; k <= 1; k += 2) {
        int y = cy + k * (r / 2);
        for (int x = cx - 6 - age * 2; x < cx - 2; x++)
            if (((x + age) & 1) && cv_ok(x, y) && cv->a[y][x].a == 0) cv_put(cv, x, y, ch->rastro[2]);
    }
}

/* Hanzo não luta mais: some a espada, o rastro e o cabo; as mãos ficam vazias. */
static void no_weapon(Canvas *cv, const Char *ch) {
    const Seg *s = cv->seg;
    static const int d[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            int lb = s->lab[y][x];
            if (lb == BLADE || lb == SMEAR) {
                erase_px(cv, x, y);
            } else if (lb == HANDLE) {
                bool hand = false;
                for (int i = 0; i < 4 && !hand; i++)
                    hand = cv_ok(x + d[i][0], y + d[i][1]) && s->lab[y + d[i][1]][x + d[i][0]] == SKIN;
                if (hand) set_rgb(cv, x, y, ch->pele[1]);
                else erase_px(cv, x, y);
            }
        }
    /* no espadão (arma do próprio pack) a face larga da lâmina ficou como "outro":
       some o que for "outro" e encostar na lâmina */
    static short stack[CW * CH][2];
    static bool seen[CH][CW];
    memset(seen, 0, sizeof seen);
    int sp = 0;
    if (ch->pack && ch->pack_arma)
        for (int y = 0; y < CH; y++)
            for (int x = 0; x < CW; x++)
                if (s->lab[y][x] == BLADE || s->lab[y][x] == SMEAR) { seen[y][x] = true; stack[sp][0] = (short)x; stack[sp++][1] = (short)y; }
    while (sp > 0) {
        int x = stack[--sp][0], y = stack[sp][1];
        for (int i = 0; i < 4; i++) {
            int nx = x + d[i][0], ny = y + d[i][1];
            if (!cv_ok(nx, ny) || seen[ny][nx] || s->lab[ny][nx] != OTHER) continue;
            seen[ny][nx] = true;
            erase_px(cv, nx, ny);
            stack[sp][0] = (short)nx;
            stack[sp++][1] = (short)ny;
        }
    }
    /* e os restos soltos da lâmina (até 4 pixels longe do corpo) */
    memset(seen, 0, sizeof seen);
    static short comp[CW * CH][2];
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            if (seen[y][x] || !cv->a[y][x].a) continue;
            int n = 0;
            sp = 0;
            seen[y][x] = true;
            stack[sp][0] = (short)x;
            stack[sp++][1] = (short)y;
            while (sp > 0) {
                int cx = stack[--sp][0], cy = stack[sp][1];
                comp[n][0] = (short)cx;
                comp[n++][1] = (short)cy;
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = cx + dx, ny = cy + dy;
                        if (!cv_ok(nx, ny) || seen[ny][nx] || !cv->a[ny][nx].a) continue;
                        seen[ny][nx] = true;
                        stack[sp][0] = (short)nx;
                        stack[sp++][1] = (short)ny;
                    }
            }
            if (n <= 4)
                for (int i = 0; i < n; i++) erase_px(cv, comp[i][0], comp[i][1]);
        }
}

/* O Hanzo do próprio pack fica como vem (o cabelo branco confundiria o separador
 * de lâminas); só a espada embainhada sai: cada mancha das cores da bainha com
 * 6 px ou mais de largura (as botas e os punhos são menores). */
static void drop_sheath(Canvas *cv, const Char *ch) {
    static bool seen[CH][CW];
    static short stack[CW * CH][2], comp[CW * CH][2];
    memset(seen, 0, sizeof seen);
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            Color c = cv->a[y][x];
            bool is = false;
            for (int k = 0; k < 2 && !is; k++)
                is = rgb_set(ch->bainha[k]) && c.a && c.r == ch->bainha[k].r && c.g == ch->bainha[k].g && c.b == ch->bainha[k].b;
            if (seen[y][x] || !is) continue;
            int n = 0, sp = 0, x0 = x, x1 = x;
            seen[y][x] = true;
            stack[sp][0] = (short)x;
            stack[sp++][1] = (short)y;
            while (sp > 0) {
                int cx = stack[--sp][0], cy = stack[sp][1];
                comp[n][0] = (short)cx;
                comp[n++][1] = (short)cy;
                if (cx < x0) x0 = cx;
                if (cx > x1) x1 = cx;
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = cx + dx, ny = cy + dy;
                        if (!cv_ok(nx, ny) || seen[ny][nx] || !cv->a[ny][nx].a) continue;
                        Color d = cv->a[ny][nx];
                        bool same = false;
                        for (int k = 0; k < 2 && !same; k++)
                            same = d.r == ch->bainha[k].r && d.g == ch->bainha[k].g && d.b == ch->bainha[k].b;
                        if (!same) continue;
                        seen[ny][nx] = true;
                        stack[sp][0] = (short)nx;
                        stack[sp++][1] = (short)ny;
                    }
            }
            if (x1 - x0 >= 6)
                for (int i = 0; i < n; i++) erase_px(cv, comp[i][0], comp[i][1]);
        }
}

/* Tempo de cada quadro dos golpes: leves rápidos, pesados lentos. */
static int frame_ms(const Char *ch) {
    switch (ch->arma.kind) {
        case W_ADAGA: case W_GARRAS: case W_FLORETE: case W_FOICE: return 60;
        case W_ODACHI: case W_PESADA: case W_CAJADO: return 100;
        default: return 80;
    }
}

static void render(const Frame *f, const Seg *seg, const Char *ch, const Ctx *ctx, Canvas *cv) {
    const char *anim = ctx->anim;
    int idx = ctx->idx;
    memcpy(cv->a, f->p, sizeof cv->a);
    cv->seg = seg;
    cv->orig = f;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            int lb = seg->lab[y][x];
            cv->empty[y][x] = lb == NONE;
            cv->tag[y][x] = lb == NONE ? T_NONE : (lb == SMEAR || lb == BLADE) ? T_WEAPON : T_BODY;
        }
    cv->pen = T_BODY;
    if (ch->pack && rgb_set(ch->bainha[0])) {
        drop_sheath(cv, ch);
        return;
    }
    recolor(cv, ch, anim);
    accessories(cv, ch, idx);
    if (seg->has_hat) {
        if (ch->chapeu) {
            if (ch->chapeu == 2) raiden_hat(cv, ch);
            if (ch->rosto) {
                Pal pal;
                head_palette(ch, &pal);
                paint_rows(cv, !strcmp(ch->rosto, "barba") ? FACE_BARBA : FACE_OLHO_RAIO, 8, &pal, false);
            }
        } else {
            remove_hat(cv);
            draw_head(cv, ch, idx);
        }
    }
    if (ch->sem_arma) {
        no_weapon(cv, ch);
    } else {
        weapons(cv, ch, ctx);
        cv->pen = T_FX;
        paint_smear(cv, ch, anim, idx);
        smear_style(cv, ch);
        aura(cv, ch, ctx);
        if (ch->elemento == EL_VENTO) wind_slash(cv, ch, ctx);
    }
    for (int i = 0; i < seg->nerase; i++)
        for (int y = seg->erase[i][1] < 0 ? 0 : seg->erase[i][1]; y <= seg->erase[i][3] && y < CH; y++)
            for (int x = seg->erase[i][0] < 0 ? 0 : seg->erase[i][0]; x <= seg->erase[i][2] && x < CW; x++) {
                cv->a[y][x] = (Color){0, 0, 0, 0};
                cv->tag[y][x] = T_NONE;
            }
    /* o corpo do Samurai #3 ganha largura, altura e passo; os packs próprios já vêm animados */
    if (!ch->pack) {
        resize_body(cv, seg->ox + 8, seg->oy + 19, ch->largura, ch->altura);
        translate(cv, lunge(ch, ctx));
    }
}

/* ------------------------------------------------------------------------ */
/* Arquivos                                                                  */
/* ------------------------------------------------------------------------ */
typedef struct {
    char name[64];
    int order;
    int ntok;
    char key[12][16];
    int val[12];
    bool has_val[12];
} AnimInfo;

typedef struct {
    int cw, chh;
    int n;
    AnimInfo a[MAX_STRIPS];
} Manifest;

static void make_dir(const char *p) {
#ifdef _WIN32
    _mkdir(p);
#else
    mkdir(p, 0755);
#endif
}

static void path_join(char *out, const char *a, const char *b) { snprintf(out, PATHLEN, "%.600s/%.400s", a, b); }

static bool is_integer(const char *s) {
    if (*s == '-') s++;
    if (!*s) return false;
    for (; *s; s++)
        if (*s < '0' || *s > '9') return false;
    return true;
}

/* Quebra uma linha em palavras, ignorando o que vem depois de '#'. */
static int split_words(char *line, char **w, int max) {
    char *hash = strchr(line, '#');
    if (hash) *hash = 0;
    int n = 0;
    for (char *t = strtok(line, " \t\r\n"); t && n < max; t = strtok(NULL, " \t\r\n")) w[n++] = t;
    return n;
}

static void read_manifest(const char *path, Manifest *m) {
    memset(m, 0, sizeof *m);
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof line, f)) {
        char *w[32];
        int n = split_words(line, w, 32);
        if (n >= 3 && !strcmp(w[0], "cell")) { m->cw = atoi(w[1]); m->chh = atoi(w[2]); }
        else if (n >= 2 && !strcmp(w[0], "anim") && m->n < MAX_STRIPS) {
            AnimInfo *a = &m->a[m->n];
            snprintf(a->name, sizeof a->name, "%s", w[1]);
            a->order = m->n++;
            for (int k = 2; k < n && a->ntok < 12;) {
                snprintf(a->key[a->ntok], 16, "%s", w[k]);
                if (k + 1 < n && is_integer(w[k + 1])) {
                    a->val[a->ntok] = atoi(w[k + 1]);
                    a->has_val[a->ntok] = true;
                    k += 2;
                } else {
                    k += 1;
                }
                a->ntok++;
            }
        }
    }
    fclose(f);
}

static AnimInfo *find_anim(Manifest *m, const char *name) {
    for (int i = 0; i < m->n; i++)
        if (!strcmp(m->a[i].name, name)) return &m->a[i];
    return NULL;
}
static bool anim_get(const AnimInfo *a, const char *key, int *v) {
    if (!a) return false;
    for (int i = 0; i < a->ntok; i++)
        if (!strcmp(a->key[i], key) && a->has_val[i]) { *v = a->val[i]; return true; }
    return false;
}

typedef struct { char anim[64]; int frame; Fix fix; } FixEntry;
static FixEntry FIXES[256];
static int NFIXES;

/* ajustes.txt, uma linha por correção:
     ANIM quadro chapeu ox oy          o chapéu está com o canto em (ox, oy)
     ANIM quadro semchapeu             não há chapéu na cabeça neste quadro
     ANIM quadro apaga x0 y0 x1 y1     apaga um retângulo (ex.: chapéu caindo) */
static void read_fixes(const char *path) {
    NFIXES = 0;
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof line, f)) {
        char *w[16];
        int n = split_words(line, w, 16);
        if (n < 3) continue;
        FixEntry *e = NULL;
        for (int i = 0; i < NFIXES; i++)
            if (!strcmp(FIXES[i].anim, w[0]) && FIXES[i].frame == atoi(w[1])) e = &FIXES[i];
        if (!e) {
            if (NFIXES >= 256) continue;
            e = &FIXES[NFIXES++];
            memset(e, 0, sizeof *e);
            snprintf(e->anim, sizeof e->anim, "%s", w[0]);
            e->frame = atoi(w[1]);
        }
        if (!strcmp(w[2], "chapeu") && n >= 5) { e->fix.hat = true; e->fix.hx = atoi(w[3]); e->fix.hy = atoi(w[4]); }
        else if (!strcmp(w[2], "semchapeu")) e->fix.nohat = true;
        else if (!strcmp(w[2], "apaga") && n >= 7 && e->fix.nerase < 8) {
            for (int k = 0; k < 4; k++) e->fix.erase[e->fix.nerase][k] = atoi(w[3 + k]);
            e->fix.nerase++;
        }
    }
    fclose(f);
}
static const Fix *find_fix(const char *anim, int frame) {
    for (int i = 0; i < NFIXES; i++)
        if (!strcmp(FIXES[i].anim, anim) && FIXES[i].frame == frame) return &FIXES[i].fix;
    return NULL;
}

static int cmp_str(const void *a, const void *b) { return strcmp(*(const char *const *)a, *(const char *const *)b); }

static int load_strips(const char *dir, Strip *out, int cw, int ch) {
    FilePathList fl = LoadDirectoryFiles(dir);
    const char **names = malloc(sizeof(char *) * (fl.count + 1));
    int nn = 0;
    for (unsigned i = 0; i < fl.count; i++)
        if (IsFileExtension(fl.paths[i], ".png") || IsFileExtension(fl.paths[i], ".PNG")) names[nn++] = fl.paths[i];
    qsort(names, (size_t)nn, sizeof(char *), cmp_str);
    int n = 0;
    for (int i = 0; i < nn && n < MAX_STRIPS; i++) {
        Image im = LoadImage(names[i]);
        if (!im.data) continue;
        ImageFormat(&im, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        if (im.height != ch || im.width % cw || cw > CW || ch > CH) {
            printf("  pulando %s: %d x %d não é tira de %d x %d\n", GetFileName(names[i]), im.width, im.height, cw, ch);
            UnloadImage(im);
            continue;
        }
        Strip *s = &out[n++];
        memset(s, 0, sizeof *s);
        snprintf(s->name, sizeof s->name, "%s", GetFileNameWithoutExt(names[i]));
        s->cw = cw;
        s->ch = ch;
        s->nframes = im.width / cw;
        if (s->nframes > MAX_FRAMES) s->nframes = MAX_FRAMES;
        s->frames = calloc((size_t)s->nframes, sizeof(Frame));
        s->segs = calloc((size_t)s->nframes, sizeof(Seg));
        Color *px = im.data;
        for (int k = 0; k < s->nframes; k++)
            for (int y = 0; y < ch; y++)
                for (int x = 0; x < cw; x++) {
                    Color c = px[y * im.width + k * cw + x];
                    if (c.a < 128) c = (Color){0, 0, 0, 0};  /* restos quase transparentes */
                    s->frames[k].p[y][x] = c;
                }
        UnloadImage(im);
    }
    free(names);
    UnloadDirectoryFiles(fl);
    return n;
}

static void save_strip(const char *path, Frame *frames, int n, int cw, int ch) {
    Image im = GenImageColor(cw * n, ch, (Color){0, 0, 0, 0});
    Color *px = im.data;
    for (int k = 0; k < n; k++)
        for (int y = 0; y < ch; y++)
            for (int x = 0; x < cw; x++) px[y * im.width + k * cw + x] = frames[k].p[y][x];
    ExportImage(im, path);
    UnloadImage(im);
}

static void copy_dir(const char *from, const char *to) {
    make_dir(to);
    FilePathList fl = LoadDirectoryFiles(from);
    for (unsigned i = 0; i < fl.count; i++) {
        if (!IsPathFile(fl.paths[i])) continue;
        int size = 0;
        unsigned char *data = LoadFileData(fl.paths[i], &size);
        char dst[PATHLEN];
        path_join(dst, to, GetFileName(fl.paths[i]));
        if (data) SaveFileData(dst, data, size);
        UnloadFileData(data);
    }
    UnloadDirectoryFiles(fl);
}

static const char *MARK = ".gerado";
static const char *MARK_TEXT = "Pasta escrita por tools/personagens.c. Rodar de novo sobrescreve os PNGs daqui.\n";

static bool has_png(const char *dir) {
    if (!DirectoryExists(dir)) return false;
    FilePathList fl = LoadDirectoryFiles(dir);
    bool any = false;
    for (unsigned i = 0; i < fl.count; i++) any |= IsFileExtension(fl.paths[i], ".png");
    UnloadDirectoryFiles(fl);
    return any;
}

/* Nunca sobrescreve pranchas que não foram geradas por este programa. */
static void out_dir(char *d, const char *root, const char *name, const char *src) {
    char mark[PATHLEN], rs[4096], rd[4096];
    path_join(d, root, name);
    bool same = false;
#ifndef _WIN32
    if (realpath(d, rd) && realpath(src, rs)) same = !strcmp(rd, rs);
#endif
    path_join(mark, d, MARK);
    if (same) {
        strncat(d, "_gerado", PATHLEN - strlen(d) - 1);
    } else if (has_png(d) && !FileExists(mark)) {
        printf("  %s já tem pranchas de outro pack; o gerado vai para %s_gerado\n", d, d);
        strncat(d, "_gerado", PATHLEN - strlen(d) - 1);
    }
    make_dir(d);
    path_join(mark, d, MARK);
    /* pasta do programa: tira as tiras da rodada anterior (uma animação que deixou
       de existir, como os golpes do Hanzo, não fica para trás) */
    if (FileExists(mark)) {
        FilePathList fl = LoadDirectoryFiles(d);
        for (unsigned i = 0; i < fl.count; i++)
            if (IsPathFile(fl.paths[i]) && (IsFileExtension(fl.paths[i], ".png") || IsFileExtension(fl.paths[i], ".PNG")))
                remove(fl.paths[i]);
        UnloadDirectoryFiles(fl);
    }
    SaveFileText(mark, (char *)MARK_TEXT);
}

/* ------------------------------------------------------------------------ */
/* Alcance                                                                   */
/* ------------------------------------------------------------------------ */
/* Ponto mais à frente da arma ou do rastro, relativo à âncora dos pés. */
static bool reach(const Canvas *cv, int ax, int ay, int *dx, int *dy) {
    int xm = -1;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (cv->tag[y][x] == T_WEAPON && cv->a[y][x].a && x > xm) xm = x;
    bool any_tag = xm >= 0;
    if (!any_tag)
        for (int y = 0; y < CH; y++)
            for (int x = 0; x < CW; x++)
                if (cv->a[y][x].a && x > xm) xm = x;
    if (xm < 0) return false;
    double sy = 0;
    int n = 0;
    for (int y = 0; y < CH; y++)
        if ((!any_tag || cv->tag[y][xm] == T_WEAPON) && cv->a[y][xm].a) { sy += y; n++; }
    *dx = xm - ax;
    *dy = pyround(sy / n) - ay;
    return true;
}

/* Âncora dos pés do personagem pronto: última linha do corpo e o meio dela. */
static void body_anchor(const Canvas *cv, int *ax, int *ay) {
    for (int y = CH - 1; y >= 0; y--) {
        int x0 = -1, x1 = -1;
        for (int x = 0; x < CW; x++)
            if (cv->tag[y][x] == T_BODY && cv->a[y][x].a) { if (x0 < 0) x0 = x; x1 = x; }
        if (x0 >= 0) {
            *ax = pyround((x0 + x1) / 2.0);
            *ay = y;
            return;
        }
    }
    *ax = CW / 2;
    *ay = CH - 1;
}

static Ctx make_ctx(const char *anim, int idx, int n, const AnimInfo *info, int contact) {
    Ctx c = {anim, idx, n, -1, contact, PH_NONE};
    if (contact < 0) return c;
    int hold;
    c.hold = anim_get(info, "hold", &hold) ? hold : (contact > 0 ? contact - 1 : 0);
    if (idx < c.hold) c.phase = PH_ANTICIPATION;
    else if (idx < contact) c.phase = PH_STRIKE;
    else if (idx == contact) c.phase = PH_CONTACT;
    else c.phase = PH_RECOVERY;
    return c;
}

static int contact_frame(const char *name, const AnimInfo *info, const Strip *st) {
    int v;
    if (anim_get(info, "contact", &v)) return v;
    if (info || !strstr(name, "ATTACK")) return -1;
    /* golpe fora do manifesto: o quadro com mais rastro */
    int best = -1, besta = 0;
    for (int k = 0; k < st->nframes; k++) {
        int a = 0;
        for (int y = 0; y < CH; y++)
            for (int x = 0; x < CW; x++) a += st->segs[k].lab[y][x] == SMEAR;
        if (a > besta) { besta = a; best = k; }
    }
    return besta > 40 ? best : -1;
}

/* ------------------------------------------------------------------------ */
/* Folhas de conferência                                                      */
/* ------------------------------------------------------------------------ */
/* Letras de 3 x 5, só o que as folhas usam. */
static const char *FONT[] = {
    "A.#.#.#####.##.#", "B##.#.###.#.###.", "C.###..#..#...##", "D##.#.##.##.###.", "E####..##.#..###", "F####..##.#..#..",
    "G.###..#.##.#.##", "H#.##.#####.##.#", "I###.#..#..#.###", "J..#..#..##.#.#.", "K#.##.###.#.##.#", "L#..#..#..#..###",
    "M#.########.##.#", "N##.#.##.##.##.#", "O.#.#.##.##.#.#.", "P##.#.###.#..#..", "Q.#.#.##.###..##", "R##.#.###.#.##.#",
    "S.###...#...###.", "T###.#..#..#..#.", "U#.##.##.##.####", "V#.##.##.##.#.#.", "W#.##.########.#", "X#.##.#.#.#.##.#",
    "Y#.##.#.#..#..#.", "Z###..#.#.#..###", "0####.##.##.####", "1.#.##..#..#.###", "2##...#.#.#..###", "3##...#.#...###.",
    "4#.##.####..#..#", "5####..##...###.", "6.###..####.####", "7###..#.#..#..#.", "8####.#####.####", "9####.####..###.",
    "_............###", "-......###......", "..............#.", ":....#.....#....",
};

static void draw_text(Image *im, int x, int y, const char *s, Color c, int scale) {
    for (; *s; s++) {
        unsigned char ch = (unsigned char)*s;
        if (ch >= 0xc0) { s++; ch = 'A'; } /* acento: vira letra simples */
        if (ch >= 'a' && ch <= 'z') ch = (unsigned char)(ch - 32);
        const char *g = NULL;
        for (size_t i = 0; i < sizeof FONT / sizeof FONT[0]; i++)
            if ((unsigned char)FONT[i][0] == ch) g = FONT[i] + 1;
        if (g)
            for (int k = 0; k < 15; k++)
                if (g[k] == '#') ImageDrawRectangle(im, x + (k % 3) * scale, y + (k / 3) * scale, scale, scale, c);
        x += 4 * scale;
    }
}

static Image frame_image(const Frame *f, Color bg, int x0, int y0, int x1, int y1, int zoom) {
    Image im = GenImageColor((x1 - x0) * zoom, (y1 - y0) * zoom, bg);
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            Color c = f->p[y][x];
            if (!c.a) continue;
            float a = c.a / 255.0f;
            Color o = {(unsigned char)(c.r * a + bg.r * (1 - a)), (unsigned char)(c.g * a + bg.g * (1 - a)),
                       (unsigned char)(c.b * a + bg.b * (1 - a)), 255};
            ImageDrawRectangle(&im, (x - x0) * zoom, (y - y0) * zoom, zoom, zoom, o);
        }
    return im;
}

static void trim_box(Frame *const *fs, int n, int *x0, int *y0, int *x1, int *y1) {
    int ax0 = CW, ay0 = CH, ax1 = -1, ay1 = -1;
    for (int k = 0; k < n; k++)
        for (int y = 0; y < cellh; y++)
            for (int x = 0; x < cellw; x++)
                if (fs[k]->p[y][x].a) {
                    if (x < ax0) ax0 = x;
                    if (x > ax1) ax1 = x;
                    if (y < ay0) ay0 = y;
                    if (y > ay1) ay1 = y;
                }
    if (ax1 < 0) { *x0 = 0; *y0 = 0; *x1 = cellw; *y1 = cellh; return; }
    *x0 = ax0 - 2 < 0 ? 0 : ax0 - 2;
    *y0 = ay0 - 2 < 0 ? 0 : ay0 - 2;
    *x1 = ax1 + 3 > cellw ? cellw : ax1 + 3;
    *y1 = ay1 + 3 > cellh ? cellh : ay1 + 3;
}

static const Color BG = {40, 38, 52, 255};
static const Color INK = {235, 235, 245, 255};
static const Color INK2 = {150, 150, 170, 255};

typedef struct { const char *name; int n; Frame *frames; } Rendered;

/* Todas as pranchas do personagem, quadro a quadro, com o número embaixo. */
static void sheet_character(const char *title, const Rendered *r, int nr, const char *path) {
    const int zoom = 3;
    int W = 0, H = 16, boxes[MAX_REND][4];
    if (nr > MAX_REND) nr = MAX_REND;
    for (int i = 0; i < nr; i++) {
        Frame *fs[MAX_FRAMES];
        for (int k = 0; k < r[i].n; k++) fs[k] = &r[i].frames[k];
        trim_box(fs, r[i].n, &boxes[i][0], &boxes[i][1], &boxes[i][2], &boxes[i][3]);
        int w = 110 + r[i].n * ((boxes[i][2] - boxes[i][0]) * zoom + 4);
        if (w > W) W = w;
        H += (boxes[i][3] - boxes[i][1]) * zoom + 22;
    }
    Image im = GenImageColor(W, H, BG);
    draw_text(&im, 6, 4, title, INK, 2);
    int y = 20;
    for (int i = 0; i < nr; i++) {
        int w = (boxes[i][2] - boxes[i][0]) * zoom, h = (boxes[i][3] - boxes[i][1]) * zoom;
        draw_text(&im, 4, y + h / 2, r[i].name, INK, 1);
        for (int k = 0; k < r[i].n; k++) {
            Image t = frame_image(&r[i].frames[k], BG, boxes[i][0], boxes[i][1], boxes[i][2], boxes[i][3], zoom);
            ImageDraw(&im, t, (Rectangle){0, 0, (float)t.width, (float)t.height},
                      (Rectangle){(float)(110 + k * (w + 4)), (float)y, (float)t.width, (float)t.height}, WHITE);
            UnloadImage(t);
            char num[16];
            snprintf(num, sizeof num, "%d", k);
            draw_text(&im, 112 + k * (w + 4), y + h + 3, num, INK2, 1);
        }
        y += h + 22;
    }
    ExportImage(im, path);
    UnloadImage(im);
}

/* Corpos de packs diferentes têm quadros de tamanhos diferentes: para a folha do
   elenco, cada pose vai para o meio do quadro com os pés na mesma linha. */
static void feet_align(const Frame *in, Frame *out) {
    int x0 = CW, x1 = -1, y1 = -1;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (in->p[y][x].a) {
                if (x < x0) x0 = x;
                if (x > x1) x1 = x;
                if (y > y1) y1 = y;
            }
    memset(out, 0, sizeof *out);
    if (x1 < 0) return;
    int dx = CW / 2 - (x0 + x1) / 2, dy = CH - 3 - y1;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            int sx = x - dx, sy = y - dy;
            if (sx >= 0 && sx < CW && sy >= 0 && sy < CH) out->p[y][x] = in->p[sy][sx];
        }
}

/* Todos lado a lado: tamanho do jogo em cima, ampliado embaixo. */
static void sheet_lineup(Frame *const *poses, const char *const *titles, int n, const char *path, bool gray) {
    const int zoom = 4;
    int x0, y0, x1, y1;
    static Frame al[64];
    static Frame *alp[64];
    if (n > 64) n = 64;
    for (int i = 0; i < n; i++) { feet_align(poses[i], &al[i]); alp[i] = &al[i]; }
    poses = alp;
    int ow = cellw, oh = cellh;
    cellw = CW; cellh = CH;
    trim_box(poses, n, &x0, &y0, &x1, &y1);
    cellw = ow; cellh = oh;
    int w = x1 - x0, h = y1 - y0, colw = (w * zoom > 72 ? w * zoom : 72) + 8;
    Image im = GenImageColor(n * colw + 8, h + h * zoom + 40, BG);
    for (int i = 0; i < n; i++) {
        Frame f = *poses[i];
        if (gray)
            for (int y = 0; y < CH; y++)
                for (int x = 0; x < CW; x++) {
                    Color c = f.p[y][x];
                    unsigned char g = (unsigned char)(c.r * 0.299 + c.g * 0.587 + c.b * 0.114);
                    f.p[y][x] = (Color){g, g, g, c.a};
                }
        int cx = 8 + i * colw;
        Image small = frame_image(&f, BG, x0, y0, x1, y1, 1), big = frame_image(&f, BG, x0, y0, x1, y1, zoom);
        ImageDraw(&im, small, (Rectangle){0, 0, (float)w, (float)h},
                  (Rectangle){(float)(cx + (colw - 8 - w) / 2), 6, (float)w, (float)h}, WHITE);
        ImageDraw(&im, big, (Rectangle){0, 0, (float)big.width, (float)big.height},
                  (Rectangle){(float)(cx + (colw - 8 - w * zoom) / 2), (float)(h + 14), (float)big.width, (float)big.height}, WHITE);
        UnloadImage(small);
        UnloadImage(big);
        draw_text(&im, cx + 2, h + h * zoom + 22, titles[i], INK, 2);
    }
    ExportImage(im, path);
    UnloadImage(im);
}

/* Um personagem por linha, o quadro de contato de cada golpe lado a lado. */
static void sheet_strikes(Frame *const (*rows)[MAX_STRIPS], const char *const (*names)[MAX_STRIPS], const int *nper,
                          const char *const *titles, int n, const char *path) {
    const int zoom = 2, w = CW * zoom, h = CH * zoom;
    int ncol = 0;
    for (int i = 0; i < n; i++)
        if (nper[i] > ncol) ncol = nper[i];
    if (!ncol) return;
    Image im = GenImageColor(80 + ncol * w, 16 + n * h, BG);
    for (int j = 0; j < nper[0]; j++) draw_text(&im, 80 + j * w + 4, 4, names[0][j], INK2, 1);
    for (int i = 0; i < n; i++) {
        draw_text(&im, 4, 16 + i * h + h / 2, titles[i], INK, 1);
        for (int j = 0; j < nper[i]; j++) {
            Image t = frame_image(rows[i][j], BG, 0, 0, CW, CH, zoom);
            ImageDraw(&im, t, (Rectangle){0, 0, (float)w, (float)h}, (Rectangle){(float)(80 + j * w), (float)(16 + i * h), (float)w, (float)h}, WHITE);
            UnloadImage(t);
        }
    }
    ExportImage(im, path);
    UnloadImage(im);
}

static const Color LABEL_COLORS[] = {
    [HATL] = {120, 140, 255, 255}, [HATFX] = {80, 90, 200, 255}, [HAIR] = {255, 60, 200, 255},
    [FACE] = {255, 200, 150, 255}, [SKIN] = {220, 150, 110, 255}, [SHIRT] = {255, 110, 110, 255},
    [DARK] = {80, 80, 80, 255}, [SAYA] = {170, 0, 170, 255}, [HANDLE] = {0, 120, 255, 255},
    [BLADE] = {0, 255, 255, 255}, [SMEAR] = {255, 255, 0, 255}, [OTHER] = {0, 255, 0, 255},
};

/* Cada parte que o programa achou, em cor chapada. Serve para conferir as pranchas novas. */
static void sheet_detection(const Strip *st, int n, const char *path) {
    const int zoom = 2;
    int W = 0;
    for (int i = 0; i < n; i++)
        if (st[i].nframes * CW * zoom > W) W = st[i].nframes * CW * zoom;
    Image im = GenImageColor(W, n * (CH * zoom + 4), (Color){16, 16, 22, 255});
    for (int i = 0; i < n; i++) {
        int y0 = i * (CH * zoom + 4);
        for (int k = 0; k < st[i].nframes; k++) {
            const Seg *s = &st[i].segs[k];
            int x0 = k * CW * zoom;
            ImageDrawRectangle(&im, x0, y0, CW * zoom, CH * zoom, (Color){24, 24, 32, 255});
            for (int y = 0; y < CH; y++)
                for (int x = 0; x < CW; x++)
                    if (s->lab[y][x] != NONE) ImageDrawRectangle(&im, x0 + x * zoom, y0 + y * zoom, zoom, zoom, LABEL_COLORS[s->lab[y][x]]);
            if (s->has_hat && cv_ok(s->ox, s->oy)) ImageDrawRectangle(&im, x0 + s->ox * zoom, y0 + s->oy * zoom, zoom, zoom, WHITE);
            char tag[32];
            snprintf(tag, sizeof tag, "%d%s", k, s->has_hat ? "" : " sem chapeu");
            draw_text(&im, x0 + 4, y0 + CH * zoom - 10, tag, INK2, 1);
        }
        draw_text(&im, 4, y0 + 4, st[i].name, INK, 1);
    }
    ExportImage(im, path);
    UnloadImage(im);
}

/* Quadro de contato de cada golpe com a âncora (vermelho) e o alcance (ciano). */
static void sheet_reach(const char *title, const Rendered *r, int nr, const int *contact, const int (*reachv)[2],
                        const bool *has_reach, int ax, int ay, const char *path) {
    const int zoom = 3;
    int nt = 0;
    for (int i = 0; i < nr; i++) nt += has_reach[i];
    if (!nt) return;
    Image im = GenImageColor(nt * (CW * zoom + 4), CH * zoom + 20, BG);
    draw_text(&im, 4, 4, title, INK, 2);
    int x = 0;
    for (int i = 0; i < nr; i++) {
        if (!has_reach[i]) continue;
        Image t = frame_image(&r[i].frames[contact[i]], BG, 0, 0, CW, CH, zoom);
        int rx = (ax + reachv[i][0]) * zoom + zoom / 2, ry = (ay + reachv[i][1]) * zoom + zoom / 2;
        ImageDrawLine(&t, ax * zoom + zoom / 2, 0, ax * zoom + zoom / 2, CH * zoom, (Color){255, 60, 60, 255});
        ImageDrawLine(&t, rx, 0, rx, CH * zoom, (Color){60, 230, 255, 255});
        ImageDrawCircleLines(&t, rx, ry, 4, WHITE);
        char tag[96];
        snprintf(tag, sizeof tag, "%s Q%d ALCANCE %d", r[i].name, contact[i], reachv[i][0]);
        draw_text(&t, 4, 4, tag, INK, 1);
        ImageDraw(&im, t, (Rectangle){0, 0, (float)t.width, (float)t.height}, (Rectangle){(float)x, 20, (float)t.width, (float)t.height}, WHITE);
        UnloadImage(t);
        x += CW * zoom + 4;
    }
    ExportImage(im, path);
    UnloadImage(im);
}

/* ------------------------------------------------------------------------ */
/* Programa                                                                  */
/* ------------------------------------------------------------------------ */
/* Quem não luta (Hanzo) não ganha as pranchas de golpe nem de guarda. */
static bool skip_strip(const Char *ch, const char *name) {
    return ch->sem_arma && (strstr(name, "ATTACK") || strstr(name, "DEFEND") || strstr(name, "THROW") || strstr(name, "DASH"));
}

static void write_manifest(const char *path, Manifest *m, const Strip *st, int ns, const int *contact,
                           const int (*reachv)[2], const bool *has_reach, int ax, int ay, const int *guard, int ms,
                           int cw, int ch, const Char *who) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "# Gerado por tools/personagens.c a partir do sprite.txt das pranchas de origem.\n");
    fprintf(f, "# ancora: ponto dos pés (IDLE quadro 0). alcance dx dy: ponta da arma ou do\n");
    fprintf(f, "# rastro no quadro de contato, a partir da âncora (dy negativo = acima dos pés).\n");
    fprintf(f, "# ms: duração de cada quadro do golpe (sem ms, 80). Leves são rápidos, pesados lentos.\n");
    fprintf(f, "cell %d %d\nancora %d %d\n", cw, ch, ax, ay);
    if (guard) fprintf(f, "guarda %d %d\n", guard[0], guard[1]);
    /* ordem: a do manifesto, depois o resto pelo nome */
    int order[MAX_STRIPS];
    for (int i = 0; i < ns; i++) order[i] = i;
    for (int i = 0; i < ns; i++)
        for (int j = i + 1; j < ns; j++) {
            AnimInfo *a = find_anim(m, st[order[i]].name), *b = find_anim(m, st[order[j]].name);
            int oa = a ? a->order : 999, ob = b ? b->order : 999;
            if (ob < oa || (ob == oa && strcmp(st[order[j]].name, st[order[i]].name) < 0)) {
                int t = order[i];
                order[i] = order[j];
                order[j] = t;
            }
        }
    for (int oi = 0; oi < ns; oi++) {
        int i = order[oi];
        if (skip_strip(who, st[i].name)) continue;
        AnimInfo *a = find_anim(m, st[i].name);
        char line[512];
        int p = snprintf(line, sizeof line, "anim %-13s", st[i].name);
        bool has_contact = false;
        if (a)
            for (int k = 0; k < a->ntok; k++) {
                if (!strcmp(a->key[k], "contact")) has_contact = true;
                if (a->has_val[k]) p += snprintf(line + p, sizeof line - (size_t)p, "  %s %d", a->key[k], a->val[k]);
                else p += snprintf(line + p, sizeof line - (size_t)p, "  %s", a->key[k]);
            }
        if (has_reach[i]) {
            if (!has_contact && contact[i] >= 0) p += snprintf(line + p, sizeof line - (size_t)p, "  contact %d", contact[i]);
            p += snprintf(line + p, sizeof line - (size_t)p, "  alcance %d %d", reachv[i][0], reachv[i][1]);
            if (ms != 80 && contact[i] >= 0 && strcmp(st[i].name, "DEFEND"))
                p += snprintf(line + p, sizeof line - (size_t)p, "  ms %d", ms);
        }
        while (p > 0 && line[p - 1] == ' ') line[--p] = 0;
        fprintf(f, "%s\n", line);
    }
    fclose(f);
}

/* Pack próprio: o especial sai de um golpe do próprio pack, com a preparação
   segurada, o passo para trás antes do bote e o avanço no contato. */
static int pack_special_steps(const Strip *strips, int ns, Manifest *man, SpecialMove m, Step *st) {
    if (m == SP_NENHUM) return 0;
    const char *anim = m == SP_SALTO ? "ATTACK_3" : m == SP_ASCENDENTE ? "ATTACK_2" : "ATTACK_1";
    const Strip *s = NULL;
    for (int i = 0; i < ns; i++)
        if (!strcmp(strips[i].name, anim)) s = &strips[i];
    if (!s) return 0;
    AnimInfo *info = find_anim(man, anim);
    int contact = contact_frame(anim, info, s), hold;
    if (contact < 1) return 0;
    if (!anim_get(info, "hold", &hold) || hold >= contact) hold = contact - 1;
    bool dash = m == SP_INVESTIDA || m == SP_ESTOCADA;
    int n = 0;
    for (int k = 0; k <= hold && n < 4; k++) st[n++] = (Step){s->name, k, PH_ANTICIPATION, 0};
    st[n++] = (Step){s->name, hold, PH_ANTICIPATION, -1};
    if (hold + 1 >= contact) st[n++] = (Step){s->name, hold, PH_STRIKE, -2};
    for (int k = hold + 1; k < contact && n < 8; k++) st[n++] = (Step){s->name, k, PH_STRIKE, -2};
    st[n++] = (Step){s->name, contact, PH_CONTACT, dash ? 6 : 2};
    int dx = dash ? 4 : 2;
    for (int k = contact + 1; k < s->nframes && n < 15; k++) {
        st[n++] = (Step){s->name, k, PH_RECOVERY, dx};
        dx = dx > 1 ? dx - 2 : 0;
    }
    if (dx > 0 && n < 16) st[n++] = (Step){s->name, s->nframes - 1, PH_RECOVERY, 0};
    return n;
}

/* ------------------------------------------------------------------------ */
/* Fontes das pranchas: o Samurai #3 (corpo do Kojiro) e os packs próprios   */
/* ------------------------------------------------------------------------ */
#define MAX_PACKS 16   /* um por personagem com pack próprio (as cores dele entram na separação) */

typedef struct {
    char dir[PATHLEN], who[32];
    Strip strips[MAX_STRIPS];
    int ns, ref, cw, ch;
    double katana;
    Manifest man;
} Source;

static const Strip *source_strip(const Source *sc, const char *name) {
    for (int i = 0; i < sc->ns; i++)
        if (!strcmp(sc->strips[i].name, name)) return &sc->strips[i];
    return NULL;
}

static Source *SOURCES[MAX_PACKS + 1];
static int NSOURCES;

/* Lê as tiras de uma pasta e separa as partes de cada quadro. `pack_ch` é o
   personagem dono do pack (as cores próprias dele), ou NULL para o Samurai #3. */
static bool load_source(Source *s, const char *dir, int cw, int ch, const Char *pack_ch) {
    memset(s, 0, sizeof *s);
    snprintf(s->dir, sizeof s->dir, "%s", dir);
    snprintf(s->who, sizeof s->who, "%s", pack_ch ? pack_ch->id : "");
    char mpath[PATHLEN];
    path_join(mpath, dir, "sprite.txt");
    read_manifest(mpath, &s->man);
    if (pack_ch) {
        if (!s->man.cw || !s->man.chh) {
            printf("  %s: falta 'cell largura altura' em %s\n", pack_ch->id, mpath);
            return false;
        }
        cw = s->man.cw;
        ch = s->man.chh;
    } else if (s->man.cw && (s->man.cw != cw || s->man.chh != ch)) {
        printf("o gerador foi feito para o quadro %d x %d do Samurai #3; o manifesto diz %d x %d\n", cw, ch, s->man.cw, s->man.chh);
        return false;
    }
    if (cw > CW || ch > CH) {
        printf("  quadro %d x %d maior que o máximo %d x %d\n", cw, ch, CW, CH);
        return false;
    }
    s->cw = cw;
    s->ch = ch;
    s->ns = load_strips(dir, s->strips, cw, ch);
    if (!s->ns) { printf("nenhuma prancha em %s\n", dir); return false; }
    SOURCES[NSOURCES++] = s;
    char fpath[PATHLEN];
    path_join(fpath, dir, "ajustes.txt");
    read_fixes(fpath);

    g_pack = pack_ch != NULL;
    g_pack_ch = pack_ch;
    cellw = cw;
    cellh = ch;
    printf("lendo %d pranchas de %s\n", s->ns, dir);
    for (int i = 0; i < s->ns; i++) {
        bool unk = false;
        char nohat[256] = "";
        for (int k = 0; k < s->strips[i].nframes; k++) {
            segment(&s->strips[i].frames[k], find_fix(s->strips[i].name, k), &s->strips[i].segs[k]);
            unk |= s->strips[i].segs[k].nunknown > 0 && !g_pack;  /* pack: cor própria é esperada */
            if (!g_pack && !s->strips[i].segs[k].has_hat) {
                char b[16];
                snprintf(b, sizeof b, "%s%d", nohat[0] ? ", " : "", k);
                strncat(nohat, b, sizeof nohat - strlen(nohat) - 1);
            }
        }
        if (getenv("DEBUG_BLADES"))  /* lista as lâminas achadas em cada quadro */
            for (int k = 0; k < s->strips[i].nframes; k++)
                for (int b = 0; b < s->strips[i].segs[k].nblades; b++) {
                    const Blade *bl = &s->strips[i].segs[k].blades[b];
                    printf("    %s %d: n %d perto %.1f longe %.1f cabo %.0f,%.0f u %.2f,%.2f\n", s->strips[i].name, k, bl->n,
                           bl->nearest, bl->farthest, bl->hilt[0], bl->hilt[1], bl->u[0], bl->u[1]);
                }
        printf("  %s: %d quadros", s->strips[i].name, s->strips[i].nframes);
        if (unk || nohat[0]) {
            printf(" (");
            if (unk) printf("há cores fora da paleta, ficam como estão%s", nohat[0] ? "; " : "");
            if (nohat[0]) printf("chapéu não achado nos quadros %s", nohat);
            printf(")");
        }
        printf("\n");
    }
    g_pack = false;
    g_pack_ch = NULL;

    static const Seg *all[MAX_STRIPS * MAX_FRAMES];
    int na = 0;
    for (int i = 0; i < s->ns; i++)
        for (int k = 0; k < s->strips[i].nframes; k++) all[na++] = &s->strips[i].segs[k];
    KATANA = 16.6;
    measure_katana(all, na, pack_ch ? 0.75 : 0.5);
    s->katana = KATANA;
    printf("  katana do pack: %.1f px do cabo à ponta\n", KATANA);
    s->ref = 0;
    for (int i = 0; i < s->ns; i++)
        if (!strcmp(s->strips[i].name, "IDLE")) s->ref = i;
    return true;
}

/* O pack de um personagem, lido uma vez só. NULL se a pasta não tem tiras. */
static Source *get_pack(const char *dir, const Char *ch) {
    for (int i = 0; i < NSOURCES; i++)
        if (!strcmp(SOURCES[i]->dir, dir) && !strcmp(SOURCES[i]->who, ch->id)) return SOURCES[i];
    if (NSOURCES >= MAX_PACKS + 1 || !DirectoryExists(dir) || !has_png(dir)) return NULL;
    Source *s = calloc(1, sizeof *s);
    if (!load_source(s, dir, 0, 0, ch)) {
        for (int i = 0; i < NSOURCES; i++)
            if (SOURCES[i] == s) SOURCES[i] = SOURCES[--NSOURCES];
        for (int i = 0; i < s->ns; i++) {
            for (int k = 0; k < s->strips[i].nframes; k++) free_seg(&s->strips[i].segs[k]);
            free(s->strips[i].frames);
            free(s->strips[i].segs);
        }
        free(s);
        return NULL;
    }
    return s;
}

static void free_sources(void) {
    for (int j = 0; j < NSOURCES; j++) {
        Source *s = SOURCES[j];
        for (int i = 0; i < s->ns; i++) {
            for (int k = 0; k < s->strips[i].nframes; k++) free_seg(&s->strips[i].segs[k]);
            free(s->strips[i].frames);
            free(s->strips[i].segs);
        }
        if (j > 0) free(s);  /* a primeira é a do Samurai #3, estática */
    }
    NSOURCES = 0;
}

static void usage(void) {
    printf("uso: personagens [--entrada pasta] [--saida pasta] [--so nome ...] [--folhas] [--lista]\n"
           "  --entrada  pranchas originais (padrão: <saida>/_original, copiada de <saida>/musashi na primeira vez)\n"
           "  --saida    onde sai uma pasta por personagem (padrão: assets/sprites)\n"
           "  --so       só estes personagens\n"
           "  --folhas   também as folhas de conferência em <saida>/_folhas\n"
           "  --lista    mostra os personagens e sai\n");
}

int main(int argc, char **argv) {
    SetTraceLogLevel(LOG_WARNING);
    const char *entrada = NULL, *saida = "assets/sprites";
    const char *only[64];
    int nonly = 0;
    bool folhas = false, lista = false;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--entrada") && i + 1 < argc) entrada = argv[++i];
        else if (!strcmp(argv[i], "--saida") && i + 1 < argc) saida = argv[++i];
        else if (!strcmp(argv[i], "--folhas")) folhas = true;
        else if (!strcmp(argv[i], "--lista")) lista = true;
        else if (!strcmp(argv[i], "--so")) {
            while (i + 1 < argc && argv[i + 1][0] != '-' && nonly < 64) only[nonly++] = argv[++i];
        } else { usage(); return 1; }
    }
    for (int i = 0; i < NCHARS; i++) fill_defaults(&CHARS[i]);
    if (lista) {
        for (int i = 0; i < NCHARS; i++)
            printf("%-9s %-8s %s%s%s\n", CHARS[i].id, CHARS[i].sem_arma ? "nenhuma" : WEAPON_NAMES[CHARS[i].arma.kind],
                   CHARS[i].pack ? "corpo do pack " : CHARS[i].chapeu == 1 ? "chapéu palha" : CHARS[i].cabeca,
                   CHARS[i].pack ? CHARS[i].pack : "", CHARS[i].arma.par ? " (uma em cada mão)" : "");
        return 0;
    }
    char src[PATHLEN];
    if (entrada) snprintf(src, sizeof src, "%s", entrada);
    else {
        path_join(src, saida, "_original");
        char legacy[PATHLEN];
        path_join(legacy, saida, "musashi");
        if (!DirectoryExists(src) && DirectoryExists(legacy)) {
            copy_dir(legacy, src);
            char mark[PATHLEN];
            path_join(mark, legacy, MARK);
            SaveFileText(mark, (char *)MARK_TEXT);
            printf("guardei as pranchas originais em %s; o protagonista gerado sai em kojiro/\n", src);
            printf("  (daqui em diante, edite o manifesto em _original/sprite.txt)\n");
        }
    }
    int sel[NCHARS], nsel = 0;
    if (nonly) {
        for (int k = 0; k < nonly; k++) {
            int found = -1;
            for (int i = 0; i < NCHARS; i++)
                if (!strcmp(CHARS[i].id, only[k])) found = i;
            if (found < 0) { printf("personagem desconhecido: %s (use --lista)\n", only[k]); return 1; }
            sel[nsel++] = found;
        }
    } else {
        for (int i = 0; i < NCHARS; i++) sel[nsel++] = i;
    }

    /* De onde vêm as pranchas de cada personagem: o Samurai #3 (pack A) ou um pack próprio. */
    static Source base;
    if (!load_source(&base, src, SRC_W, SRC_H, NULL)) return 1;
    char packs_dir[PATHLEN];
    path_join(packs_dir, saida, "_packs");

    char fol[PATHLEN];
    path_join(fol, saida, "_folhas");
    if (folhas) {
        make_dir(fol);
        char p[PATHLEN];
        path_join(p, fol, "deteccao.png");
        cellw = base.cw; cellh = base.ch;
        sheet_detection(base.strips, base.ns, p);
    }

    static Canvas cv;
    static Frame *poses[NCHARS];
    static const char *titles[NCHARS];
    static Frame *strike_rows[NCHARS][MAX_STRIPS];
    static const char *strike_names[NCHARS][MAX_STRIPS];
    static int strike_n[NCHARS], guard_mi[2];
    static Rendered rend[NCHARS][MAX_REND];
    static int nrend[NCHARS];
    bool guard_ok = false;
    static int base_reach[MAX_STRIPS][2];
    static bool base_has[MAX_STRIPS];
    for (int si = 0; si < nsel; si++) {
        const Char *ch = &CHARS[sel[si]];
        Source *sc = &base;
        if (ch->pack) {
            char pd[PATHLEN];
            path_join(pd, packs_dir, ch->pack);
            sc = get_pack(pd, ch);
            if (!sc) {
                printf("  %s: falta o pack em %s (usando o corpo do Samurai #3)\n", ch->id, pd);
                Char *mut = &CHARS[sel[si]];
                mut->pack = NULL;
                mut->pack_par = false;
                sc = &base;
            }
        }
        cellw = sc->cw;
        cellh = sc->ch;
        KATANA = sc->katana;
        if (folhas && sc != &base) {
            char q[PATHLEN], fq[80];
            snprintf(fq, sizeof fq, "deteccao_%.40s.png", ch->id);
            path_join(q, fol, fq);
            sheet_detection(sc->strips, sc->ns, q);
        }
        char d[PATHLEN];
        out_dir(d, saida, ch->id, src);
        int guard[2], ax, ay, contact[MAX_REND], reachv[MAX_REND][2];
        bool has_guard = false, has_reach[MAX_REND];
        /* âncora dos pés deste personagem (o corpo pode ser mais largo ou mais alto) */
        {
            const Strip *rs = &sc->strips[sc->ref];
            AnimInfo *info = find_anim(&sc->man, rs->name);
            Ctx c0 = make_ctx(rs->name, 0, rs->nframes, info, contact_frame(rs->name, info, rs));
            render(&rs->frames[0], &rs->segs[0], ch, &c0, &cv);
            body_anchor(&cv, &ax, &ay);
        }
        int nr = 0;
        char p[PATHLEN], fn[PATHLEN];
        for (int i = 0; i < sc->ns; i++) {
            Strip *st = &sc->strips[i];
            Rendered *r = &rend[si][nr];
            r->name = st->name;
            r->n = st->nframes;
            r->frames = calloc((size_t)st->nframes, sizeof(Frame));
            AnimInfo *info = find_anim(&sc->man, st->name);
            int k = contact_frame(st->name, info, st);
            contact[nr] = k;
            has_reach[nr] = false;
            for (int j = 0; j < st->nframes; j++) {
                Ctx cx = make_ctx(st->name, j, st->nframes, info, k);
                render(&st->frames[j], &st->segs[j], ch, &cx, &cv);
                memcpy(r->frames[j].p, cv.a, sizeof cv.a);
                if (j == k) has_reach[nr] = reach(&cv, ax, ay, &reachv[nr][0], &reachv[nr][1]);
            }
            /* o rastro vermelho da fúria tem as cores da máscara: o alcance é o do golpe normal,
               que tem o mesmo desenho e o mesmo tempo */
            const char *fu = strstr(st->name, "_FURIA");
            if (fu && !fu[6])
                for (int q = 0; q < nr; q++)
                    if (!strncmp(rend[si][q].name, st->name, (size_t)(fu - st->name)) &&
                        !rend[si][q].name[fu - st->name] && has_reach[q]) {
                        has_reach[nr] = true;
                        reachv[nr][0] = reachv[q][0];
                        reachv[nr][1] = reachv[q][1];
                    }
            if (!strcmp(st->name, "DEFEND") && has_reach[nr]) {
                has_guard = true;
                guard[0] = reachv[nr][0];
                guard[1] = reachv[nr][1];
            }
            if (skip_strip(ch, st->name)) {
                has_reach[nr] = false;
                nr++;
                continue;
            }
            snprintf(fn, sizeof fn, "%.63s.png", st->name);
            path_join(p, d, fn);
            save_strip(p, r->frames, r->n, sc->cw, sc->ch);
            nr++;
        }
        path_join(p, d, "sprite.txt");
        write_manifest(p, &sc->man, sc->strips, sc->ns, contact, (const int (*)[2])reachv, has_reach, ax, ay,
                       has_guard ? guard : NULL, frame_ms(ch), sc->cw, sc->ch, ch);
        if (!strcmp(ch->id, "kojiro")) {
            for (int i = 0; i < sc->ns && i < MAX_STRIPS; i++) {
                base_has[i] = has_reach[i];
                base_reach[i][0] = reachv[i][0];
                base_reach[i][1] = reachv[i][1];
            }
            if (has_guard) { guard_ok = true; guard_mi[0] = guard[0]; guard_mi[1] = guard[1]; }
        }
        FILE *mf = fopen(p, "a");

        /* golpe especial: montado dos quadros do pack, um contato só */
        Step steps[16];
        int nst = ch->sem_arma ? 0 : ch->pack ? pack_special_steps(sc->strips, sc->ns, &sc->man, ch->especial, steps)
                           : special_steps(ch->especial, steps), stripi[16];
        for (int k = 0; k < nst; k++) {
            stripi[k] = -1;
            for (int i = 0; i < sc->ns; i++)
                if (!strcmp(sc->strips[i].name, steps[k].anim) && steps[k].frame < sc->strips[i].nframes) stripi[k] = i;
            if (stripi[k] < 0) nst = 0;
        }
        if (nst && nr < MAX_REND) {
            static bool sil[3][CH][CW];
            Rendered *r = &rend[si][nr];
            r->name = "ESPECIAL";
            r->n = nst;
            r->frames = calloc((size_t)nst, sizeof(Frame));
            int hold = -1, ci = -1, ty = 0, rdx = 0, rdy = 0;
            bool hr = false;
            for (int k = 0; k < nst; k++) {
                Strip *st = &sc->strips[stripi[k]];
                AnimInfo *info = find_anim(&sc->man, st->name);
                Ctx cx = make_ctx(st->name, steps[k].frame, st->nframes, info, contact_frame(st->name, info, st));
                cx.phase = steps[k].phase;
                render(&st->frames[steps[k].frame], &st->segs[steps[k].frame], ch, &cx, &cv);
                translate(&cv, steps[k].dx);
                /* imagens do corpo ficando para trás na investida e na estocada */
                bool dash = ch->especial == SP_INVESTIDA || ch->especial == SP_ESTOCADA;
                if (dash && (cx.phase == PH_CONTACT || (ci >= 0 && k == ci + 1))) {
                    cv.pen = T_FX;
                    /* fantasma: o contorno, e a mais próxima meio preenchida em xadrez */
                    for (int j = 2; j >= 1; j--) {
                        if (k - j < 0) continue;
                        Rgb c = j == 1 ? ch->rastro[1] : ch->rastro[2];
                        bool (*g)[CW] = sil[(k - j) % 3];
                        for (int y = 1; y < CH - 1; y++)
                            for (int x = 1; x < CW - 1; x++) {
                                if (!g[y][x] || cv.a[y][x].a) continue;
                                bool edge = !(g[y][x - 1] && g[y][x + 1] && g[y - 1][x] && g[y + 1][x]);
                                if (edge || (j == 1 && ((x + y) & 1) == 0)) cv_put(&cv, x, y, c);
                            }
                    }
                }
                if (cx.phase == PH_STRIKE) hold = k;
                if (cx.phase == PH_CONTACT) {
                    ci = k;
                    hr = reach(&cv, ax, ay, &rdx, &rdy);
                    ty = ay + rdy;
                }
                for (int y = 0; y < CH; y++)
                    for (int x = 0; x < CW; x++) sil[k % 3][y][x] = cv.tag[y][x] == T_BODY && cv.a[y][x].a;
                if (ci >= 0 && hr) special_fx(&cv, ch, ch->efeito, ax, rdx, ty, ay, k - ci);
                memcpy(r->frames[k].p, cv.a, sizeof cv.a);
            }
            path_join(p, d, "ESPECIAL.png");
            save_strip(p, r->frames, nst, sc->cw, sc->ch);
            if (mf) {
                fprintf(mf, "anim %-13s  hold %d  contact %d", "ESPECIAL", hold, ci);
                if (hr) fprintf(mf, "  alcance %d %d", rdx, rdy);
                if (frame_ms(ch) != 80) fprintf(mf, "  ms %d", frame_ms(ch));
                fprintf(mf, "\n");
            }
            nr++;
        }

        /* Sem arma: DESARMADO é quem perdeu a arma no duelo (no Samurai #3, agachado no
           começo do DASH_ATTACK, com a lâmina longe do corpo; nos packs, de joelhos pela
           DEATH; sem nenhum dos dois, curvado pelo HURT). Quem não luta e não tem IDLE
           ganha o PARADO. */
        if (nr < MAX_REND) {
            const Strip *ss = NULL;
            int f0 = 0, f1 = -1;
            const char *out = ch->sem_arma ? "PARADO" : "DESARMADO";
            if (ch->sem_arma) {
                if (!source_strip(sc, "IDLE") && (ss = source_strip(sc, "DASH"))) f0 = f1 = ss->nframes - 1;
            } else if ((ss = source_strip(sc, "DASH_ATTACK"))) {
                f0 = f1 = ss->nframes > 1 ? 1 : 0;
            } else if ((ss = source_strip(sc, "DEATH"))) {
                f1 = ss->nframes * 2 / 5;          /* o quadro em que chega aos joelhos */
            } else if ((ss = source_strip(sc, "HURT"))) {
                f0 = ss->nframes > 1 ? 1 : 0;       /* o primeiro é o clarão do golpe */
                f1 = ss->nframes - 1;
            }
            if (ss && f1 >= f0) {
                Char tmp = *ch;
                tmp.sem_arma = true;
                Rendered *r = &rend[si][nr];
                r->name = out;
                r->n = f1 - f0 + 1;
                r->frames = calloc((size_t)r->n, sizeof(Frame));
                const AnimInfo *info = find_anim(&sc->man, ss->name);
                for (int j = f0; j <= f1; j++) {
                    Ctx cx = make_ctx(ss->name, j, ss->nframes, info, -1);
                    render(&ss->frames[j], &ss->segs[j], &tmp, &cx, &cv);
                    memcpy(r->frames[j - f0].p, cv.a, sizeof cv.a);
                }
                snprintf(fn, sizeof fn, "%s.png", out);
                path_join(p, d, fn);
                save_strip(p, r->frames, r->n, sc->cw, sc->ch);
                if (mf) {
                    if (r->n > 1) fprintf(mf, "anim %-13s  stop %d\n", out, r->n - 1);
                    else fprintf(mf, "anim %s\n", out);
                }
                nr++;
            }
        }

        /* Oboro: cada ataque em cada uma das onze posturas, e a cena do grito */
        if (ch->ecos) {
            static const char *atk[] = {"ATTACK_1", "ATTACK_2", "ATTACK_3"};
            for (int a = 0; a < NCHARS; a++) {
                const Char *ap = &CHARS[a];
                if (!strcmp(ap->id, "kojiro") || ap->sem_arma || !strcmp(ap->id, ch->id)) continue;
                /* a espada é a dele; só a aura, o rastro e o brilho do elemento são do aprendiz */
                Char tmp = *ch;
                tmp.elemento = ap->elemento;
                memcpy(tmp.rastro, ap->rastro, sizeof tmp.rastro);
                tmp.destaque[0] = ap->destaque[0];
                tmp.destaque[1] = ap->destaque[1];
                tmp.ecos = false;
                for (int q = 0; q < 3; q++) {
                    int i = -1;
                    for (int t = 0; t < sc->ns; t++)
                        if (!strcmp(sc->strips[t].name, atk[q])) i = t;
                    if (i < 0 || nr >= MAX_REND) continue;
                    Strip *st = &sc->strips[i];
                    AnimInfo *info = find_anim(&sc->man, st->name);
                    int k = contact_frame(st->name, info, st), rx = 0, ry = 0;
                    bool hr = false;
                    Rendered *r = &rend[si][nr];
                    static char names[MAX_REND][64];
                    snprintf(names[nr], 64, "%s_ECO_%s", atk[q], ap->id);
                    for (char *u = names[nr]; *u; u++)
                        if (*u >= 'a' && *u <= 'z') *u = (char)(*u - 32);
                    r->name = names[nr];
                    r->n = st->nframes;
                    r->frames = calloc((size_t)st->nframes, sizeof(Frame));
                    for (int j = 0; j < st->nframes; j++) {
                        Ctx cx = make_ctx(st->name, j, st->nframes, info, k);
                        render(&st->frames[j], &st->segs[j], &tmp, &cx, &cv);
                        memcpy(r->frames[j].p, cv.a, sizeof cv.a);
                        if (j == k) hr = reach(&cv, ax, ay, &rx, &ry);
                    }
                    snprintf(fn, sizeof fn, "%.63s.png", r->name);
                    path_join(p, d, fn);
                    save_strip(p, r->frames, r->n, sc->cw, sc->ch);
                    if (mf) {
                        int hold;
                        fprintf(mf, "anim %-24s", r->name);
                        if (anim_get(info, "hold", &hold)) fprintf(mf, "  hold %d", hold);
                        if (k >= 0) fprintf(mf, "  contact %d", k);
                        if (hr) fprintf(mf, "  alcance %d %d", rx, ry);
                        if (frame_ms(&tmp) != 80) fprintf(mf, "  ms %d", frame_ms(&tmp));
                        fprintf(mf, "\n");
                    }
                    nr++;
                }
            }
            int ii = -1, fi = -1;
            for (int t = 0; t < sc->ns; t++) {
                if (!strcmp(sc->strips[t].name, "IDLE")) ii = t;
                if (!strcmp(sc->strips[t].name, "IDLE_FURIA")) fi = t;
            }
            if (ii >= 0 && fi >= 0 && nr < MAX_REND) {
                Rendered *r = &rend[si][nr];
                r->name = "GRITO";
                r->frames = calloc(32, sizeof(Frame));
                r->n = grito(&sc->strips[ii], &sc->strips[fi], ch, &cv, r->frames);
                path_join(p, d, "GRITO.png");
                save_strip(p, r->frames, r->n, sc->cw, sc->ch);
                if (mf) fprintf(mf, "anim GRITO                     ms 90\n");
                nr++;
            }
        }
        if (mf) fclose(mf);
        nrend[si] = nr;

        const Strip *rs = &sc->strips[sc->ref];
        Frame *pose = &rend[si][sc->ref].frames[0];
        for (int j = 0; j < rs->nframes; j++) {
            bool any = false;
            for (int y = 0; y < CH && !any; y++)
                for (int x = 0; x < CW && !any; x++) any = rend[si][sc->ref].frames[j].p[y][x].a;
            if (any) { pose = &rend[si][sc->ref].frames[j]; break; }
        }
        poses[si] = pose;
        titles[si] = ch->titulo;
        strike_n[si] = 0;
        for (int i = 0; i < sc->ns && strike_n[si] < MAX_STRIPS; i++)
            if (has_reach[i] && contact[i] >= 0 && contact[i] < rend[si][i].n && strcmp(rend[si][i].name, "DEFEND")) {
                strike_rows[si][strike_n[si]] = &rend[si][i].frames[contact[i]];
                strike_names[si][strike_n[si]++] = rend[si][i].name;
            }
        if (folhas) {
            char q[PATHLEN];
            snprintf(fn, sizeof fn, "%s.png", ch->id);
            path_join(q, fol, fn);
            sheet_character(ch->titulo, rend[si], nrend[si], q);
            snprintf(fn, sizeof fn, "alcance_%s.png", ch->id);
            path_join(q, fol, fn);
            sheet_reach(ch->titulo, rend[si], sc->ns, contact, (const int (*)[2])reachv, has_reach, ax, ay, q);
        }
        printf("  %s: %s%s\n", ch->id, d, ch->pack ? " (pack próprio)" : "");
    }
    if (folhas) {
        char p[PATHLEN];
        path_join(p, fol, "elenco.png");
        sheet_lineup(poses, titles, nsel, p, false);
        path_join(p, fol, "elenco_pb.png");
        sheet_lineup(poses, titles, nsel, p, true);
        path_join(p, fol, "golpes.png");
        sheet_strikes((Frame *const (*)[MAX_STRIPS])strike_rows, (const char *const (*)[MAX_STRIPS])strike_names, strike_n,
                      titles, nsel, p);
        printf("folhas em %s\n", fol);
    }
    printf("distância entre as âncoras dos dois no quadro de contato = alcance do golpe + guarda do Kojiro:\n");
    for (int i = 0; i < base.ns; i++) {
        if (!base_has[i] || !strcmp(base.strips[i].name, "DEFEND")) continue;
        if (guard_ok)
            printf("  %-14s %d px + %d = %d px\n", base.strips[i].name, base_reach[i][0], guard_mi[0], base_reach[i][0] + guard_mi[0]);
        else
            printf("  %-14s %d px + guarda (falta a prancha DEFEND)\n", base.strips[i].name, base_reach[i][0]);
    }
    /* pastas geradas com os nomes antigos ficam paradas: avisa (não apaga nada) */
    static const char *old_names[] = {"raijin", "kage"};
    for (size_t i = 0; i < sizeof old_names / sizeof old_names[0]; i++) {
        char od[PATHLEN], om[PATHLEN];
        path_join(od, saida, old_names[i]);
        path_join(om, od, MARK);
        if (DirectoryExists(od) && FileExists(om))
            printf("%s é de antes da troca de nomes (gerada pelo programa); pode apagar\n", od);
    }
    for (int si = 0; si < nsel; si++)
        for (int i = 0; i < nrend[si]; i++) free(rend[si][i].frames);
    free_sources();
    return 0;
}
