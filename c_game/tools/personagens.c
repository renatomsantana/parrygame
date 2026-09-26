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
    EL_LUA, EL_GELO
} Element;

typedef enum { AC_NONE, AC_CACHECOL, AC_CASCO, AC_TRAPO, AC_CAUDA, AC_LISTRAS, AC_CABELO_LONGO, AC_COQUE_GELO } Accessory;

/* Golpe especial: a coreografia (de que quadros do pack ele é montado) e o
   efeito grande do elemento no ponto do impacto. */
typedef enum { SP_NENHUM, SP_SALTO, SP_INVESTIDA, SP_ESTOCADA, SP_ASCENDENTE } SpecialMove;
typedef enum {
    FX_NADA, FX_CHOQUE, FX_PEDRAS, FX_AVALANCHE, FX_FOGO, FX_RAIO, FX_ONDA, FX_GOTA, FX_X, FX_GARRA, FX_VORTICE,
    FX_CASCO, FX_SOMBRA, FX_CRISTAL
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

typedef struct Row_ { int y, x; const char *t; } Row;

/* Golpe desenhado pelo programa (arma e rastro próprios, no lugar do corte de
   katana do pack). */
typedef enum { GP_NADA, GP_LANCA, GP_FLORETE, GP_GARRAS, GP_KAMA, GP_DUAS, GP_ADAGAS, GP_RAIO } GolpeProprio;
/* Uma prancha remontada com os quadros de outra do mesmo pack (o mesmo número de
   quadros, hold e contato da original): a estocada da lança sai do arremesso,
   com o braço esticado. Cada quadro diz de que prancha e de que quadro vem. */
typedef struct { const char *de; signed char q; } RemontaQ;
typedef struct { const char *anim; RemontaQ f[12]; } Remonta;

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
    bool saque;                             /* Kojiro: EMBAINHADO e DESEMBAINHAR (antes de cada luta) */
    bool sentado;                           /* Hanzo: SENTADO, em seiza (a fogueira da cabana) */
    Rgb bainha[2];                          /* pack do Hanzo: cores da espada embainhada, que sai */
    Rgb rastro_pack[3];                     /* pack cujo rastro usa as cores da camisa: longe do corpo, vira rastro */
    bool sem_mascara;                       /* Oboro nas duas primeiras formas: rosto no lugar da máscara de oni */
    bool mascara_oni;                       /* Hanzo no fim: a máscara de oni no rosto */
    /* Samurai #5 (duas espadas): o pack vem com um pano cobrindo a boca e o nariz. */
    bool sem_pano;                          /* tira o pano: rosto de verdade, pele em três tons */
    bool kasa;                              /* chapéu de palha (o mesmo do Daichi) por cima */
    bool topete;                            /* cabelo espetado para cima e para a frente */
    bool mechas;                            /* mechas coloridas (destaque) no cabelo */
    const char *parado;                     /* o parado sai desta prancha do pack (quadro 0), respirando */
    GolpeProprio golpe;                     /* arma e rastro dos golpes desenhados aqui */
    const Row *cab_frente, *cab_costas; /* Samurai #5: cabelo novo (de frente e de costas), em volta do olho */
    int cab_balanco;                        /* 1: a juba balança com o vento; 2: o rabo de cavalo esvoaça */
    bool sem_rastro_pack;                   /* tira o corte de katana do pack (cores de rastro_pack longe do corpo) */
    bool rastro_pack_solto;                 /* o rastro do pack tem cor própria: sai inteiro (menos as lâminas) e o
                                               buraco que ele deixa no corpo é tapado com a cor em volta */
    Remonta remonta[3];
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

/* O corpo do Oboro, igual com e sem a máscara. */
#define OBORO                                                                                             \
    .arma = {.kind = W_KATANA}, .cabeca = "rabo_longo",                                                   \
    .camisa = {HEX(0x8a6ab0), HEX(0x5e4488), HEX(0x3e2c62), HEX(0x281c42)},                               \
    .hakama = {HEX(0x2a2030), HEX(0x201826), HEX(0x18121c), HEX(0x110d14), HEX(0x0b080d)},                \
    .pele = {HEX(0xe6b894), HEX(0xc08c6c), HEX(0x845c48)},                                                \
    .cabelo = {HEX(0x0e0a12), HEX(0x221a2c), HEX(0x3e3250)},                                              \
    .destaque = {HEX(0xffcc40), HEX(0xb08018)}, .destaque2 = HEX(0xffcc40), .obi = HEX(0xffcc40),         \
    .saya = HEX(0x18121c), .cabo = HEX(0xb08018),                                                         \
    .lamina = {HEX(0xfffbe8), HEX(0xe8c060)},                                                             \
    .rastro = {HEX(0xfff6dc), HEX(0xb48cff), HEX(0x6a3cc0)}, .elemento = EL_SOMBRA, .altura = 1,          \
    .especial = SP_INVESTIDA, .efeito = FX_SOMBRA,                                                        \
    .pack = "demon", .ecos = true, .pack_sem_camisa = true,                                               \
    .leitura = {{HEX(0xf6ca9f), 'S'}, {HEX(0x0c2e44), '?'}}

/* O corpo do Hanzo, com e sem a máscara: o pack dele, todo em branco (o manto azul e o
   cinto preto do pack viram branco e cinza claro). */
#define HANZO                                                                                             \
    .arma = {.kind = W_KATANA}, .cabeca = "mestre", .sem_arma = true, .sem_saya = true,                   \
    .pack = "hanzo", .bainha = {HEX(0x571c27), HEX(0x391f21)},                                            \
    .camisa = {HEX(0xf4f4f0), HEX(0xd4d4d0), HEX(0xa8a8a4), HEX(0x7a787e)},                               \
    .hakama = {HEX(0x7a787e), HEX(0x56545a), HEX(0x3a383e), HEX(0x2e2c30), HEX(0x262428)},                \
    .pele = {HEX(0xe0a67e), HEX(0xb87c5a), HEX(0x82543e)},                                                \
    .cabelo = {HEX(0xa8a8a4), HEX(0xd8d8d4), HEX(0xf4f4f0)},                                              \
    .destaque = {HEX(0xc42a2a), HEX(0x7a1414)}, .obi = HEX(0xc42a2a),                                     \
    .saya = HEX(0xa01c1c), .cabo = HEX(0x20283e), .altura = -1,                                           \
    .troca = {{HEX(0x0e071b), HEX(0xc4c4c0)}, {HEX(0x1a1932), HEX(0xa6a6a2)}, {HEX(0x2a2f4e), HEX(0xd2d2ce)}, \
              {HEX(0x424c6e), HEX(0xecece8)}, {HEX(0x391f21), HEX(0x4a4644)}, {HEX(0x5d2c28), HEX(0x6e6a66)}, \
              {HEX(0xc7cfdd), HEX(0xd8d8d4)}, {HEX(0x92a1b9), HEX(0xa8a8a4)}}

/* Golpes do Samurai #5 remontados com os quadros limpos do próprio pack (o corte
   de espada dele passa na frente das pernas no contato e, sem ele, o corpo
   ficaria sem elas): o braço da frente esticado (ATTACK_2:2) no golpe reto e no
   alto, agachado com as lâminas baixas dos dois lados (ATTACK_3:4 a 6) no
   baixo (o ATTACK_3:3 tem o corte do pack por cima da lâmina da frente). Mesmo número de
   quadros, hold e contato de antes. */
#define S5_REMONTA                                                                                               \
    .remonta = {{"ATTACK_1", {{"ATTACK_2", 1}, {"ATTACK_2", 2}, {"ATTACK_2", 2}, {"ATTACK_3", 6}, {"IDLE", 0}}},   \
                {"ATTACK_2", {{"ATTACK_2", 0}, {"ATTACK_2", 1}, {"ATTACK_2", 1}, {"ATTACK_3", 4}, {"ATTACK_3", 5},       \
                              {"ATTACK_3", 6}}},                                                                     \
                {"ATTACK_3", {{"ATTACK_3", 0}, {"ATTACK_3", 1}, {"ATTACK_2", 2}, {"ATTACK_3", 4}, {"ATTACK_3", 5},       \
                              {"ATTACK_3", 6}, {"IDLE", 0}}}},                                                       \
    .sem_rastro_pack = true, .rastro_pack_solto = true, .rastro_pack = {HEX(0xf8f8f8), HEX(0x92a1b9), HEX(0xc7cfdd)}

/* Moldes de cabelo do Samurai #5 (ver s5_hair). */
static const Row CAB_KARASU_FRENTE[] = {
    {-9, -9, "H"}, {-8, -8, "HhHH"}, {-7, -7, "HhHHhhh"}, {-6, -6, "hhhiiHHH"}, {-5, -11, "HHHHHHhhHHHHH"},
    {-4, -9, "hhhhHHHHHHHH"}, {-3, -7, "HHHHHHHHHHH"}, {-2, -9, "HHHHHHHHHHHHHH"}, {-1, -11, "HHhhhhHHHHH...HH"},
    {0, -6, "HHHHH"}, {1, -8, "HhhhhHHH"}, {2, -9, "HHH..HHHH"}, {0, 0, NULL},
};
static const Row CAB_KARASU_COSTAS[] = {
    {-9, -9, "H"}, {-8, -8, "HhHH"}, {-7, -7, "HhHHhhh"}, {-6, -6, "hhhiiHHH"}, {-5, -11, "HHHHHHhhHHHHH"},
    {-4, -9, "hhhhHHHHHHHH"}, {-3, -7, "HHHHHHHHHH"}, {-2, -9, "HHHHHHHHHHHH"}, {-1, -11, "HHhhhhHHHHHHHH"},
    {0, -6, "HHHHHHHH"}, {1, -8, "HhhhhHHHHH"}, {2, -9, "HHH..HHHH"}, {0, 0, NULL},
};
static const Row CAB_ARASHI_FRENTE[] = {
    {-12, -9, "h"}, {-11, -8, "hihh"}, {-10, -7, "iiihh"}, {-9, -15, "hhhhhhhhhhihhh"}, {-8, -12, "iiihhhhhhiii"},
    {-7, -10, "hihhhhhiihh"}, {-6, -16, "hhhhhhhhhhhhiihhhh"}, {-5, -19, "hhhhiihhhhhhhhhhhhhhhh"},
    {-4, -16, "hhhiiiiHHHHHihhhhhh"}, {-3, -13, "HHHHHHHHHhhhhhhhh"}, {-2, -18, "HHHHHHHHHHHHHHhhhhhhhhh"},
    {-1, -21, "HHHHhhhhhhhhhhHHHhhhh....h"}, {0, -15, "HHHHHHHHHHHhhh.....H"}, {1, -12, "HHHHHHHHhhhh"},
    {2, -14, "HHHHHHHhHHHhhh"}, {3, -17, "HHhhhhhhhHHHHH"}, {4, -19, "HHHhH.HH.HHHHHhhH"}, {5, -12, "HHHhhHhHH"},
    {6, -14, "HHhHh.H"}, {7, -15, "HH"}, {0, 0, NULL},
};
static const Row CAB_ARASHI_COSTAS[] = {
    {-12, -9, "h"}, {-11, -8, "hihh"}, {-10, -7, "iiihh"}, {-9, -15, "hhhhhhhhhhihhh"}, {-8, -12, "iiihhhhhhiii"},
    {-7, -10, "hihhhhhiihh"}, {-6, -16, "hhhhhhhhhhhhiihhhh"}, {-5, -19, "hhhhiihhhhhhhhhhhhhhhh"},
    {-4, -16, "hhhiiiiHHHHHihhhhhh"}, {-3, -13, "HHHHHHHHHhhhhhhh"}, {-2, -18, "HHHHHHHHHHHHHHhhhhhhh"},
    {-1, -21, "HHHHhhhhhhhhhhHHHhhhhhhh"}, {0, -15, "HHHHHHHHHHHhhhhhhh"}, {1, -12, "HHHHHHHHhhhhhh"},
    {2, -14, "HHHHHHHhHHHhhhh"}, {3, -17, "HHhhhhhhhHHHHH"}, {4, -19, "HHHhH.HH.HHHHHhhH"}, {5, -12, "HHHhhHhHH"},
    {6, -14, "HHhHh.H"}, {7, -15, "HH"}, {0, 0, NULL},
};
static const Row CAB_HAYATE_FRENTE[] = {
    {-4, -4, "HHH"}, {-3, -6, "HHHHHHH"}, {-2, -8, "HAAHHHHHHHHH"}, {-1, -22, "HHHHH.......HHHAHHHHHH...H"},
    {0, -20, "hhhHHHH.HHhhhhhHHHH"}, {1, -18, "HhhHHHhhHHH.HHHHHH"}, {2, -16, "HHh.........HHH"}, {0, 0, NULL},
};
static const Row CAB_HAYATE_COSTAS[] = {
    {-4, -4, "HHH"}, {-3, -6, "HHHHHHH"}, {-2, -8, "HAAHHHHHHH"}, {-1, -22, "HHHHH.......HHHAHHHHHHHH"},
    {0, -20, "hhhHHHH.HHhhhhhHHHHHHH"}, {1, -18, "HhhHHHhhHHH.HHHHHHH"}, {2, -16, "HHh.........HHH"}, {0, 0, NULL},
};
static const Row CAB_GARFIEL_FRENTE[] = {
    {-17, 3, "H"}, {-16, 2, "HH"}, {-15, -2, "H...H.....H"}, {-14, -2, "H..Hi....H"}, {-13, -3, "iH..iH...H"},
    {-12, -3, "iH.iih..iH"}, {-11, -3, "iHhHHhHiHh"}, {-10, -6, "H.HiHiHH.iHH.....H"},
    {-9, -6, "H.iHHiHHHHHH..iHH"}, {-8, -6, "iHiHhHHhiHH.HHH"}, {-7, -6, "iHHHiHiiHHHiHH"},
    {-6, -6, "iHHiHHHHHiHHH"}, {-5, -6, "iHHiHHHHHHHH"}, {-4, -6, "iHHHHHHHHH"}, {-3, -7, "hhhHHHHHHHH"},
    {-2, -7, "hhhHHHH"}, {-1, -6, "hhHHHH"}, {0, -6, "hhHHH"}, {1, -5, "hHHHH"}, {0, 0, NULL},
};
static const Row CAB_GARFIEL_COSTAS[] = {
    {-17, 3, "H"}, {-16, 2, "HH"}, {-15, -2, "H...H.....H"}, {-14, -2, "H..Hi....H"}, {-13, -3, "iH..iH...H"},
    {-12, -3, "iH.iih..iH"}, {-11, -3, "iHhHHhHiHh"}, {-10, -6, "H.HiHiHH.iHH.....H"},
    {-9, -6, "H.iHHiHHHHHH..iHH"}, {-8, -6, "iHiHhHHhiHH.HHH"}, {-7, -6, "iHHHiHiiHHHiHH"},
    {-6, -6, "iHHiHHHHHiHHH"}, {-5, -6, "iHHiHHHHHHHH"}, {-4, -6, "iHHHHHHHHH"}, {-3, -7, "hhhHHHHHHHH"},
    {-2, -7, "hhhHHHHHHH"}, {-1, -6, "hhHHHHHH"}, {0, -6, "hhHHHHHH"}, {1, -5, "hHHHHH"}, {0, 0, NULL},
};
static const Row CAB_YORU_FRENTE[] = {
    {-7, -4, "HHHh"}, {-6, -5, "hAAHhH"}, {-5, -6, "HhHaHhHH"}, {-4, -7, "HHhHaHhHHH"}, {-3, -7, "HHhHaHhHHHH"},
    {-2, -8, "HHHhHaHhHHaHH"}, {-1, -8, "HHHhHaHHHHHAH"}, {0, -8, "HHHhHaHHHH.HH"}, {1, -8, "HAHhHaHH"},
    {2, -8, "HAHhHaHH"}, {3, -8, "HAHhHaH"}, {4, -8, "HAHhHHH"}, {5, -8, "HAHhH"}, {6, -7, "HHH"}, {0, 0, NULL},
};
static const Row CAB_YORU_COSTAS[] = {
    {-7, -4, "HHHh"}, {-6, -5, "hAAHhH"}, {-5, -6, "HhHaHhHH"}, {-4, -7, "HHhHaHhHHH"}, {-3, -7, "HHhHaHhHHH"},
    {-2, -8, "HHHhHaHhHHH"}, {-1, -8, "HHHhHaHHHHH"}, {0, -8, "HHHhHaHHHH"}, {1, -8, "HAHhHaHHH"},
    {2, -8, "HAHhHaHH"}, {3, -8, "HAHhHaH"}, {4, -8, "HAHhHHH"}, {5, -8, "HAHhH"}, {6, -7, "HHH"}, {0, 0, NULL},
};


static Char CHARS[] = {
    /* O protagonista: sem chapéu e sem máscara, coque solto no alto da cabeça, katana. */
    {.id = "kojiro", .titulo = "Kojiro", .arma = {.kind = W_KATANA}, .cabeca = "coque", .saque = true},
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
    /* 4. Gelo. Florete com geada. Branco, prata e azul-gelo; cabelo prateado preso num
       coque baixo com um grampo de cristal (o rabo de cavalo azul ficou com a Suiren). */
    {.id = "shizuku", .titulo = "Shizuku", .arma = {.kind = W_FLORETE, .escala = 1.15}, .acessorios = {AC_COQUE_GELO},
     .camisa = {HEX(0xf6fcff), HEX(0xd6ecf8), HEX(0xa4c8e2), HEX(0x7094bc)},
     .hakama = {HEX(0xb8d8ee), HEX(0x8ab4d6), HEX(0x5e88b4), HEX(0x3e6490), HEX(0x2a466c)},
     .pele = {HEX(0xf6d2b8), HEX(0xdcaa8c), HEX(0xae7a62)},
     .cabelo = {HEX(0x3a4660), HEX(0x8a9ebc), HEX(0xdce8f6)},
     .destaque = {HEX(0x9ef0ff), HEX(0x3aa6d8)}, .obi = HEX(0x3aa6d8),
     .saya = HEX(0xe8f4ff), .cabo = HEX(0x3aa6d8),
     .lamina = {HEX(0xffffff), HEX(0xbfe8ff)},
     .rastro = {HEX(0xffffff), HEX(0xc8f0ff), HEX(0x7ec8f0)}, .elemento = EL_GELO, .largura = -1,
     .especial = SP_ESTOCADA, .efeito = FX_CRISTAL,
     /* o florete fica na mão em todos os quadros (em guarda, para baixo); os golpes
        são estocadas retas com rastro fino: a do meio e a alta saem do arremesso do
        pack (braço esticado), a baixa do fim do corte baixo, sem o rastro de katana */
     .golpe = GP_FLORETE, .sem_rastro_pack = true, .rastro_pack = {HEX(0xffffff), HEX(0xc7cfdd), HEX(0x92a1b9)},
     .remonta = {{"ATTACK_1", {{"THROW", 2}, {"THROW", 3}, {"THROW", 4}, {"THROW", 5}, {"THROW", 6}}},
                 {"ATTACK_3", {{"THROW", 2}, {"THROW", 3}, {"THROW", 4}, {"THROW", 5}, {"THROW", 6}}},
                 {"ATTACK_2", {{"ATTACK_2", 0}, {"ATTACK_2", 1}, {"ATTACK_3", 2}, {"ATTACK_3", 3}, {"ATTACK_3", 4},
                               {"IDLE", 0}}}},
     /* corpo do Samurai #4: o cabelo roxo vira prata, a roupa vai para o branco e o
        azul do gelo, os olhos verdes ficam azul-claros */
     .pack = "samurai4",
     .leitura = {{HEX(0xf6ca9f), 'S'}, {HEX(0xf9e6cf), 'S'}},
     .troca = {{HEX(0x0e071b), HEX(0x3a4660)}, {HEX(0x3b1443), HEX(0x8a9ebc)}, {HEX(0x622461), HEX(0xdce8f6)},
               {HEX(0xffffff), HEX(0xf6fcff)}, {HEX(0xc7cfdd), HEX(0xd6ecf8)}, {HEX(0x92a1b9), HEX(0xa4c8e2)},
               {HEX(0x657392), HEX(0x7094bc)},
               {HEX(0x1a1932), HEX(0x3e6490)}, {HEX(0x2a2f4e), HEX(0x6a94c0)}, {HEX(0x424c6e), HEX(0xa4c8e6)},
               {HEX(0x571c27), HEX(0x3aa6d8)}, {HEX(0x891e2b), HEX(0x9ef0ff)}, {HEX(0x5ac54f), HEX(0x7ae0ff)},
               {HEX(0xffc825), HEX(0xe8f6ff)}, {HEX(0xffa214), HEX(0x8ccaf0)}}},
    /* 5. Tigre (Byakko). Garras nas duas mãos, cauda. Cabelo loiro listrado, roupa preta com detalhes vermelhos. */
    {.id = "garfiel", .titulo = "Garfiel", .arma = {.kind = W_GARRAS, .comprimento = 13, .par = true, .brilho = HEX(0xffe8a0)}, .cabeca = "tigre",
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
     .sem_pano = true, .parado = "DEFEND", .pack = "samurai5",
     /* golpes de garra: sem o corte de espada do pack, três riscos de arranhão */
     .golpe = GP_GARRAS, S5_REMONTA,
     .cab_frente = CAB_GARFIEL_FRENTE, .cab_costas = CAB_GARFIEL_COSTAS, .pack_par = true, .pack_sem_camisa = true,
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
     /* golpes: sem o corte de espada do pack, dois arcos cruzados de tamanhos diferentes */
     .golpe = GP_DUAS, S5_REMONTA,
     .sem_pano = true, .pack = "samurai5", .cab_frente = CAB_KARASU_FRENTE, .cab_costas = CAB_KARASU_COSTAS, .pack_par = true, .pack_sem_camisa = true,
     .leitura = {{HEX(0xf6ca9f), 'S'}, {HEX(0x0c2e44), '?'}},
     .troca = {{HEX(0x1e6f50), HEX(0x5a5058)}, {HEX(0x134c4c), HEX(0x3e363e)}, {HEX(0x0c2e44), HEX(0x1c181c)},
               {HEX(0x391f21), HEX(0x8c1018)}, {HEX(0x5d2c28), HEX(0xc0182a)},
               {HEX(0x272727), HEX(0x100c10)}, {HEX(0x3d3d3d), HEX(0x241c26)}, {HEX(0x5ac54f), HEX(0xff2a2a)}}},
    /* 7. Vento. Duas foices (kama), uma em cada mão, e cortes de vento. Verde claro e limão, cachecol. */
    {.id = "hayate", .titulo = "Hayate",
     .arma = {.kind = W_FOICE, .comprimento = 7, .par = true, .haste = {HEX(0x8a6a44), HEX(0x5a4228)}},
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
     .sem_pano = true, .kasa = true, .pack = "samurai5",
     /* golpes de foice: sem o corte de espada do pack, dois arcos curtos e finos */
     .golpe = GP_KAMA, S5_REMONTA,
     .cab_frente = CAB_HAYATE_FRENTE, .cab_costas = CAB_HAYATE_COSTAS, .cab_balanco = 2, .pack_par = true, .pack_sem_camisa = true,
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
     .camisa = {HEX(0xeef8ff), HEX(0xbfe2f6), HEX(0x86bde6), HEX(0x5a8cc4)},
     .hakama = {HEX(0x4a78b0), HEX(0x36609a), HEX(0x284a7c), HEX(0x1d3862), HEX(0x142848)},
     .pele = {HEX(0xf2c29c), HEX(0xd69a74), HEX(0xa86a4e)},
     .cabelo = {HEX(0x1a2a52), HEX(0x2e4a82), HEX(0x5a7cc0)},
     .destaque = {HEX(0x3cf0d8), HEX(0x14a8a0)}, .obi = HEX(0x3cf0d8),
     .sem_saya = true, .cabo = HEX(0x14a8a0),
     .lamina = {HEX(0xd8fffa), HEX(0x3cf0d8)},
     .rastro = {HEX(0xe0fffc), HEX(0x5cf0e0), HEX(0x1c9cc8)}, .elemento = EL_AGUA, .largura = -1,
     .especial = SP_ESTOCADA, .efeito = FX_ONDA,
     /* golpes de lança: a estocada (e a alta) sai do arremesso do pack, com o braço
        esticado; a varrida baixa, do corte baixo (ATTACK_3) sem o rastro de katana */
     .golpe = GP_LANCA, .sem_rastro_pack = true, .rastro_pack = {HEX(0xffffff), HEX(0xc7cfdd), HEX(0x92a1b9)},
     .remonta = {{"ATTACK_1", {{"THROW", 2}, {"THROW", 3}, {"THROW", 4}, {"THROW", 5}, {"THROW", 6}}},
                 {"ATTACK_3", {{"THROW", 2}, {"THROW", 3}, {"THROW", 4}, {"THROW", 5}, {"THROW", 6}}},
                 {"ATTACK_2", {{"ATTACK_2", 0}, {"ATTACK_2", 1}, {"ATTACK_3", 2}, {"ATTACK_3", 3}, {"ATTACK_3", 4},
                               {"IDLE", 0}}}},
     /* corpo do Samurai #4 (o visual que era da Shizuku): rabo de cavalo azul petróleo,
        quimono claro e hakama azul do mar; a espada do pack vira a lança */
     .pack = "samurai4",
     .leitura = {{HEX(0xf6ca9f), 'S'}, {HEX(0xf9e6cf), 'S'}},
     .troca = {{HEX(0x0e071b), HEX(0x08202e)}, {HEX(0x3b1443), HEX(0x15506c)}, {HEX(0x622461), HEX(0x2a8eac)},
               {HEX(0xffffff), HEX(0xeef8ff)}, {HEX(0xc7cfdd), HEX(0xbfe2f6)}, {HEX(0x92a1b9), HEX(0x86bde6)},
               {HEX(0x657392), HEX(0x5a8cc4)},
               {HEX(0x1a1932), HEX(0x14264a)}, {HEX(0x2a2f4e), HEX(0x1f3f70)}, {HEX(0x424c6e), HEX(0x3462a0)},
               {HEX(0x571c27), HEX(0x14a8a0)}, {HEX(0x891e2b), HEX(0x3cf0d8)}, {HEX(0x5ac54f), HEX(0x7ae8ff)}}},
    /* 10. Tempestade. Duas espadas com raios. Preto com o chapéu do Raiden. */
    {.id = "arashi", .titulo = "Arashi", .arma = {.kind = W_DUPLA, .par = true, .cor_par = HEX(0x7cc0ff)},
     .chapeu = 2, .chapeu_cor = {HEX(0xf2eee0), HEX(0xcfc6a8), HEX(0x948a6e)}, .rosto = "olho_raio",
     .camisa = {HEX(0x4e5264), HEX(0x363a4a), HEX(0x262a36), HEX(0x1a1c26)},
     .hakama = {HEX(0x2e3240), HEX(0x222530), HEX(0x181a24), HEX(0x111219), HEX(0x0a0b10)},
     .pele = {HEX(0xdca880), HEX(0xb47e5c), HEX(0x7c5040)},
     .cabelo = {HEX(0x6a7690), HEX(0x9eaac4), HEX(0xe4ecf8)},
     .olho = HEX(0xb4f0ff),
     .destaque = {HEX(0x3ca8ff), HEX(0x1a5ad0)}, .obi = HEX(0x3ca8ff),
     .saya = HEX(0x111219), .cabo = HEX(0x1a5ad0),
     .lamina = {HEX(0xeef8ff), HEX(0x5cb4ff)},
     .rastro = {HEX(0xf4faff), HEX(0x7cc8ff), HEX(0x2a6cf0)}, .elemento = EL_RAIO,
     .especial = SP_INVESTIDA, .efeito = FX_RAIO,
     /* corpo do Samurai #5 (já com as duas espadas): o verde vira preto, cinto e botas em azul
        elétrico, cabelo prateado e olhos de raio */
     /* golpes: sem a meia-lua do pack, um raio em zigue-zague no caminho de cada espada */
     .golpe = GP_RAIO, S5_REMONTA,
     .sem_pano = true, .pack = "samurai5", .cab_frente = CAB_ARASHI_FRENTE, .cab_costas = CAB_ARASHI_COSTAS, .cab_balanco = 1, .pack_par = true, .pack_sem_camisa = true,
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
     /* golpes de adaga: sem o corte de espada do pack, cortes pequenos e secos */
     .golpe = GP_ADAGAS, S5_REMONTA,
     .pack = "samurai5", .cab_frente = CAB_YORU_FRENTE, .cab_costas = CAB_YORU_COSTAS, .pack_par = true, .pack_sem_camisa = true,
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
    /* Oboro, o último da trilha. Katana de Hanzo. Corpo do Demon (oni), com as cores
       dele; ataca em cada postura dos onze e tem a cena do grito. Nas duas primeiras
       formas luta de rosto descoberto; a máscara de oni só vem na terceira. */
    {.id = "oboro", .titulo = "Oboro", OBORO, .sem_mascara = true},
    {.id = "oboro_mascara", .titulo = "Oboro (máscara)", OBORO},
    /* Hanzo: um velho aposentado que não luta mais. Sem espada; cabelo e barba brancos,
       o manto cinza. No fim, de máscara de oni. */
    {.id = "hanzo", .titulo = "Hanzo", HANZO, .sentado = true},
    {.id = "hanzo_mascara", .titulo = "Hanzo (máscara)", HANZO, .mascara_oni = true},
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
    /* Genbu: careca, com o rosto do Kojiro (sombra da sobrancelha sobre o olho, a
       pele em três tons com a luz na frente), sobrancelha grossa e grisalha de velho
       sábio, a orelha na lateral e bigode e barbicha brancos caindo do queixo. */
    {"careca", {{3, 6, "fFFf"}, {4, 5, "fFFFFf"}, {5, 4, "kfFFFFFf"}, {6, 4, "kffFFfhhF"},
                {7, 4, "kkfkffeFF"}, {8, 4, "XkkfkffFFf"}, {9, 7, "XkfhhhF"}, {10, 8, "khiihh"},
                {11, 9, "hih"}, {12, 10, "h"}}},
    /* Enjin: cabelo em chamas, alto, preso atrás da testa; o rosto do Kojiro (olho
       debaixo da sombra da sobrancelha, pele em três tons) com a sobrancelha descendo
       para a frente, de quem está sempre bravo, e o queixo quadrado. */
    {"chamas", {{0, 8, "H"}, {1, 6, "H.Hh"}, {2, 5, "HhHhi"}, {3, 3, "H.HhhiH"}, {4, 2, "HHHhhiiH"},
                {5, 3, "HHHHhhiHh"}, {6, 3, "HHHHHhkfFF"}, {7, 4, "HHHHkfkkF"}, {8, 4, "XHHHkfeFF"},
                {9, 5, "XXkkffFf"}, {10, 7, "XkkfF"}}},
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
            } else if (el == EL_GELO && r < 0.05) {
                /* lasca de gelo: um ponto branco e, às vezes, a cruz de um cristal */
                cv_put(cv, x, y, (Rgb){236, 250, 255});
                if (r < 0.015)
                    for (int k = 0; k < 4; k++) {
                        static const int d[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
                        int nx = x + d[k][0], ny = y + d[k][1];
                        if (cv_ok(nx, ny) && s->lab[ny][nx] == NONE && cv_is_empty(cv, nx, ny)) cv_put(cv, nx, ny, (Rgb){130, 210, 245});
                    }
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

/* ----- cabeça do Samurai #5 (duas espadas) --------------------------------- */
/* O olho do pack: o branco (ffffff) com a íris verde (5ac54f) logo à frente, no alto
 * da figura. Devolve a posição do branco. */
static bool s5_eye_open(const Canvas *cv, int *ex, int *ey, bool closed_ok);
static bool s5_eye(const Canvas *cv, int *ex, int *ey) { return s5_eye_open(cv, ex, ey, true); }
static bool s5_eye_open(const Canvas *cv, int *ex, int *ey, bool closed_ok) {
    int top = -1;   /* o alto do corpo (o rastro do golpe pode passar por cima da cabeça) */
    for (int y = 0; y < CH && top < 0; y++)
        for (int x = 0; x < CW; x++)
            if (cv->orig->p[y][x].a && lab_at(cv, x, y) != SMEAR && lab_at(cv, x, y) != BLADE) { top = y; break; }
    if (top < 0) return false;
    for (int y = top; y < top + 14 && y < CH; y++)
        for (int x = 0; x + 1 < CW; x++) {
            Color o = cv->orig->p[y][x], n = cv->orig->p[y][x + 1];
            if (o.a && o.r == 255 && o.g == 255 && o.b == 255 && n.a && n.r == 0x5a && n.g == 0xc5 && n.b == 0x4f) {
                *ex = x; *ey = y;
                return true;
            }
        }
    if (!closed_ok) return false;
    /* olho fechado (a dor, a queda): a testa (a pele mais alta, na frente) fica 2 linhas acima */
    for (int y = top; y < top + 12 && y < CH; y++)
        for (int x = CW - 1; x >= 0; x--) {
            Color o = cv->orig->p[y][x];
            if (o.a && o.r == 0xe6 && o.g == 0x9c && o.b == 0x69) {
                *ex = x - 1; *ey = y + 2;
                return true;
            }
        }
    return false;
}

static bool orig_rgb(const Canvas *cv, int x, int y, uint32_t v) {
    if (!cv_ok(x, y)) return false;
    Color o = cv->orig->p[y][x];
    return o.a && o.r == (v >> 16 & 0xff) && o.g == (v >> 8 & 0xff) && o.b == (v & 0xff);
}

/* Sem o pano: o que era pano (os dois verdes do pack, embaixo do olho) vira rosto,
 * com a luz vindo de cima à esquerda: a bochecha clara na frente, o meio, a sombra no
 * queixo e atrás, e a boca num traço escuro. */
static void s5_face(Canvas *cv, const Char *ch) {
    int ex, ey;
    if (!s5_eye(cv, &ex, &ey)) return;
    Rgb mouth = {(unsigned char)(ch->pele[2].r * 0.7), (unsigned char)(ch->pele[2].g * 0.7), (unsigned char)(ch->pele[2].b * 0.7)};
    for (int r = 1; r <= 4; r++)
        for (int c = -1; c <= 3; c++) {
            int x = ex + c, y = ey + r;
            if (!orig_rgb(cv, x, y, 0x1e6f50) && !orig_rgb(cv, x, y, 0x134c4c) && !orig_rgb(cv, x, y, 0x0c2e44)) continue;
            Rgb col = ch->pele[1];
            if (r == 1 || (r == 2 && c >= 1)) col = ch->pele[0];
            if (c <= 0 || r == 4) col = ch->pele[2];
            if (r == 3 && c == 1) col = mouth;
            set_rgb(cv, x, y, col);
        }
}

/* O chapéu de palha do Daichi, pixel a pixel (claro, meio e sombra), com a aba na
 * altura da testa: os olhos aparecem logo embaixo. */
static void s5_kasa(Canvas *cv, const char *anim) {
    static const char *K[] = {
        "..........MM.......",
        ".......MMMLMD......",
        "....MMMMLLLMDD.....",
        "..MMMMMLLLMMDDD....",
        "DMMMMMMMMMMMDDDD...",
        ".DDMMMMMMMMMDDDDD..",
        "...DDDMMMMMMDDDDDD.",
    };
    int ex, ey;
    /* de olho fechado vale a testa só apanhando e caindo (no golpe a pele mais alta
       pode ser a mão erguida) */
    if (!s5_eye_open(cv, &ex, &ey, strstr(anim, "HURT") || strstr(anim, "DEATH"))) {
        /* de costas, girando no golpe: o chapéu fica centrado no alto da cabeça, o
           cabelo e o pano */
        static const uint32_t head[] = {0x272727, 0x3d3d3d, 0x391f21, 0x1e6f50, 0x134c4c};
        int top = -1, sx = 0, n = 0;
        for (int y = 0; y < CH; y++) {
            for (int x = 0; x < CW; x++) {
                if (lab_at(cv, x, y) == SMEAR || lab_at(cv, x, y) == BLADE) continue;
                bool h = false;
                for (int k = 0; k < 5 && !h; k++) h = orig_rgb(cv, x, y, head[k]);
                if (!h) continue;
                if (top < 0) top = y;
                sx += x; n++;
            }
            if (top >= 0 && y >= top + 4) break;
        }
        if (!n) return;
        ex = sx / n + 2;
        ey = top + 7;
    }
    int ox = ex - 11, oy = ey - 8;
    /* o coque do pack some debaixo do chapéu */
    for (int y = oy - 4; y < oy + 7; y++)
        for (int x = ox - 2; x < ox + 21; x++)
            if (orig_rgb(cv, x, y, 0x272727) || orig_rgb(cv, x, y, 0x3d3d3d) || orig_rgb(cv, x, y, 0x391f21)) {
                if (y < oy + 2) cv_clear(cv, x, y);
            }
    for (int r = 0; r < 7; r++)
        for (int c = 0; K[r][c]; c++) {
            char k = K[r][c];
            if (k == '.') continue;
            static const Rgb L = HEX(0xe0bc72), M = HEX(0xb48c48), D = HEX(0x7c5c2c);
            Rgb col = k == 'L' ? L : k == 'M' ? M : D;
            cv_put(cv, ox + c, oy + r, col);
        }
}

/* Topete espetado: pontas de cabelo subindo para a frente, por cima da cabeça. */
static void s5_topete(Canvas *cv, const Char *ch) {
    static const Row T[] = {
        {-10, 3, "i..i"}, {-9, 0, "hi.hHi.i"}, {-8, -2, "hHihHHHiHi"}, {-7, -4, "hHHHHHHHHHHi"},
        {-6, -5, "hHHHHHHHHHHi"}, {-5, -5, "hHHHHHHHHHH"}, {-4, -4, "hHHHHHHH"}, {0, 0, NULL},
    };
    int ex, ey;
    if (!s5_eye(cv, &ex, &ey)) return;
    for (int r = 0; T[r].t; r++)
        for (int i = 0; T[r].t[i]; i++) {
            char k = T[r].t[i];
            int x = ex + T[r].x + i, y = ey + T[r].y;
            if (k == '.' || !cv_ok(x, y)) continue;
            if (cv->a[y][x].a && !is_hair(cv, ch, x, y)) continue;   /* o rosto e a arma ficam na frente */
            cv_put(cv, x, y, k == 'H' ? ch->cabelo[0] : k == 'h' ? ch->cabelo[1] : ch->cabelo[2]);
        }
}

/* Mechas: faixas finas da cor de destaque no cabelo, a cada três colunas. */
static void s5_mechas(Canvas *cv, const Char *ch) {
    int top = -1;
    for (int y = 0; y < CH && top < 0; y++)
        for (int x = 0; x < CW; x++)
            if (cv->orig->p[y][x].a) { top = y; break; }
    if (top < 0) return;
    for (int y = top; y < top + 14 && y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (orig_rgb(cv, x, y, 0x3d3d3d) && (x % 3 == 1))
                set_rgb(cv, x, y, (y % 2) ? ch->destaque[0] : ch->destaque[1]);
}


/* ----- cabelo novo no corpo do Samurai #5 ---------------------------------- */
/* Os cinco que lutam com duas armas usavam o mesmo cabelo do pack (preso atrás).
 * Cada um ganha um molde próprio em volta do olho (coluna 0 = o branco do olho,
 * linha 0 = a linha dele): o do Karasu em mechas pontudas para trás, a juba longa
 * do Arashi, o rabo de cavalo do Hayate saindo por baixo do chapéu, o topete alto
 * do Garfiel e o liso até o ombro, com a franja e as mechas roxas, do Yoru. Sem o
 * olho aberto (de costas, no giro do golpe; de olhos fechados, apanhando e caindo),
 * a cabeça é a mancha de cabelo do pack: o olho fica 4 px antes da frente dela e 6
 * abaixo do alto. Letras: H h i cabelo (escuro, meio, luz), A a destaque, L contorno. */
static bool s5_hair_px(const Canvas *cv, int x, int y) {
    return orig_rgb(cv, x, y, 0x272727) || orig_rgb(cv, x, y, 0x3d3d3d) || orig_rgb(cv, x, y, 0x391f21);
}

static void s5_hair(Canvas *cv, const Char *ch, int idx, const char *anim) {
    static bool clus[CH][CW];
    static short st[CW * CH][2];
    memset(clus, 0, sizeof clus);
    /* a mancha de cabelo do pack: a que tem o pixel de cabelo mais alto */
    int tx = -1, ty = -1;
    for (int y = 0; y < CH && ty < 0; y++)
        for (int x = 0; x < CW; x++)
            if (s5_hair_px(cv, x, y) && lab_at(cv, x, y) != SMEAR && lab_at(cv, x, y) != BLADE) { tx = x; ty = y; break; }
    if (ty < 0) return;
    int sp = 0, n = 0, x0 = CW, x1 = -1, y0 = CH, y1 = -1;
    st[sp][0] = (short)tx; st[sp][1] = (short)ty; sp++;
    clus[ty][tx] = true;
    while (sp) {
        sp--;
        int cx = st[sp][0], cy = st[sp][1];
        n++;
        if (cx < x0) x0 = cx;
        if (cx > x1) x1 = cx;
        if (cy < y0) y0 = cy;
        if (cy > y1) y1 = cy;
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                int nx = cx + dx, ny = cy + dy;
                if (!cv_ok(nx, ny) || clus[ny][nx] || !s5_hair_px(cv, nx, ny)) continue;
                clus[ny][nx] = true;
                st[sp][0] = (short)nx; st[sp][1] = (short)ny; sp++;
            }
    }
    int ex, ey;
    bool open = s5_eye_open(cv, &ex, &ey, false), back = false, lying = false;
    int ground = CH - 1;
    if (!open) {
        if (n < 30) return;
        ex = x1 - 4;
        ey = y0 + 6;
        back = !strstr(anim, "HURT") && !strstr(anim, "DEATH");
        if (strstr(anim, "DEATH")) {
            int bx0 = CW, bx1 = -1, by0 = CH, by1 = -1;
            for (int y = 0; y < CH; y++)
                for (int x = 0; x < CW; x++)
                    if (cv->orig->p[y][x].a) {
                        if (x < bx0) bx0 = x;
                        if (x > bx1) bx1 = x;
                        if (y < by0) by0 = y;
                        if (y > by1) by1 = y;
                    }
            lying = by1 - by0 < (bx1 - bx0) * 8 / 10;
            if (lying) { ex = x1 - 5; ey = (y0 + y1) / 2; ground = by1; }
        }
    }
    /* o cabelo do pack sai (e os fiapos soltos dele em volta da cabeça) */
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (clus[y][x]) { cv_clear(cv, x, y); cv->empty[y][x] = true; }
    for (int y = y0 - 12; y <= y1 + 14; y++)
        for (int x = x0 - 16; x <= x1 + 16; x++) {
            if (!cv_ok(x, y) || !s5_hair_px(cv, x, y) || clus[y][x]) continue;
            bool touch = false;
            for (int dy = -1; dy <= 1 && !touch; dy++)
                for (int dx = -1; dx <= 1 && !touch; dx++)
                    touch = cv_ok(x + dx, y + dy) && cv->orig->p[y + dy][x + dx].a && !s5_hair_px(cv, x + dx, y + dy);
            if (!touch) { cv_clear(cv, x, y); cv->empty[y][x] = true; }
        }
    const Row *t = back ? ch->cab_costas : ch->cab_frente;
    for (int r = 0; t[r].t; r++)
        for (int i = 0; t[r].t[i]; i++) {
            char k = t[r].t[i];
            if (k == '.') continue;
            int c = t[r].x + i, rr = t[r].y;
            /* a juba e o rabo balançam, mais na ponta */
            if (ch->cab_balanco == 1 && c < -6) c += pyround(sin(idx * 0.9 + rr * 0.5) * (c < -12 ? 1.2 : 0.6));
            if (ch->cab_balanco == 2 && c < -8) rr += pyround(sin(idx * 1.3 - c * 0.45) * (c < -14 ? 1.4 : 0.7));
            int x = ex + c, y = ey + rr;
            /* caído de costas: o alto da cabeça para a direita; o cabelo de trás da nuca
               não sobe no ar, fica espalhado no chão, passando da cabeça */
            if (lying) {
                x = ex - rr;
                y = ey + c * 6 / 10;
                if (c < -4) {
                    x = ex - rr + pyround((-4 - c) * 0.6);
                    y = ey - 2 + pyround((-4 - c) * 0.3);
                }
                if (y > ground) y = ground;
            }
            if (!cv_ok(x, y)) continue;
            /* a arma e o rastro passam na frente; abaixo do queixo o cabelo fica atrás do corpo */
            if (cv->tag[y][x] == T_WEAPON || lab_at(cv, x, y) == BLADE || lab_at(cv, x, y) == SMEAR) continue;
            if (cv->a[y][x].a && !clus[y][x] && (rr > 2 || orig_rgb(cv, x, y, 0xe69c69))) continue;
            Rgb col = k == 'H' ? ch->cabelo[0] : k == 'h' ? ch->cabelo[1] : k == 'i' ? ch->cabelo[2]
                    : k == 'A' ? ch->destaque[0] : k == 'a' ? ch->destaque[1] : (Rgb){19, 19, 19};
            cv_put(cv, x, y, col);
            cv->tag[y][x] = T_BODY;
        }
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
            {
                /* cauda de tigre saindo da cintura, da cor do cabelo, com a ponta preta.
                   Caído, a cintura não fica abaixo da cabeça: a cauda sai de trás da faixa */
                int tx = s->ox + 3, ty = s->oy + 21;
                bool near = false;
                for (int dy = -2; dy <= 2 && !near; dy++)
                    for (int dx = -2; dx <= 2 && !near; dx++) near = cv_ok(tx + dx, ty + dy) && cv->orig->p[ty + dy][tx + dx].a;
                if (!near && ch->pack) {
                    int bx = CW;
                    for (int y = 0; y < CH; y++)
                        for (int x = 0; x < bx; x++) {
                            Color o = cv->orig->p[y][x];
                            for (int i = 0; i < 24 && rgb_set(ch->troca[i].de); i++)
                                if (o.a && ch->troca[i].de.r == o.r && ch->troca[i].de.g == o.g && ch->troca[i].de.b == o.b &&
                                    ch->troca[i].para.r == ch->obi.r && ch->troca[i].para.g == ch->obi.g && ch->troca[i].para.b == ch->obi.b) {
                                    bx = x; tx = x; ty = y;
                                    break;
                                }
                        }
                }
                ribbon(cv, tx, ty, 11, ch->cabelo[0], ch->cabelo[1], idx, -0.25, 1.3, 2, 1.1);
                cv_behind(cv, tx - 10, ty - 3, ch->mascara[0]);
                cv_behind(cv, tx - 9, ty - 3, ch->mascara[0]);
                break;
            }
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
            case AC_CABELO_LONGO: case AC_COQUE_GELO: {
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
                /* correndo, o rabo de cavalo sobe por cima da cabeça e encosta nela: o que
                   passa do alto da cabeça (7 linhas acima do olho) também sai */
                int eye_y = -1;
                for (int y = ytop; y < ymax && eye_y < 0; y++)
                    for (int x = 0; x < CW; x++) {
                        Color o = cv->orig->p[y][x];
                        if (o.a && o.r == 0x5a && o.g == 0xc5 && o.b == 0x4f) { eye_y = y; break; }
                    }
                if (eye_y < 0)   /* olhos fechados (caindo): a testa, 2 linhas acima deles */
                    for (int y = ytop; y < ymax && eye_y < 0; y++)
                        for (int x = cx1[head] - 8; x <= cx1[head] + 2; x++) {
                            if (!cv_ok(x, y)) continue;
                            Color o = cv->orig->p[y][x];
                            if (o.a && ((o.r == 0xf6 && o.g == 0xca && o.b == 0x9f) || (o.r == 0xf9 && o.g == 0xe6 && o.b == 0xcf))) {
                                eye_y = y + 2;
                                break;
                            }
                        }
                if (eye_y >= 0 && cy0[head] < eye_y - 7) cy0[head] = eye_y - 7;   /* o cabelo solto nasce na cabeça */
                if (eye_y >= 0)
                    for (int y = 0; y < eye_y - 7; y++)
                        for (int x = 0; x < CW; x++)
                            if (comp[y][x] >= 0 || (cv->a[y][x].a && is_hair(cv, ch, x, y))) {
                                cv_clear(cv, x, y);
                                cv->empty[y][x] = true;
                                comp[y][x] = -1;
                            }
                for (int y = 0; y < ymax; y++)
                    for (int x = 0; x < CW; x++)
                        if (comp[y][x] == head && x < cx1[head] - 10) comp[y][x] = -1;
                /* a fita do rabo de cavalo e o que sobrou dele no alto da cabeça: dentro do
                   cabelo vira cabelo (senão parece uma risca); fora dele, sai */
                for (int y = cy0[head] - 4; y <= cy0[head] + 3; y++)          /* acima do rosto */
                    for (int x = cx1[head] - 12; x <= cx1[head]; x++) {
                        if (!cv_ok(x, y) || !cv->a[y][x].a || is_hair(cv, ch, x, y)) continue;
                        /* só o que era cabelo ou fita no pack (o rastro claro do golpe passa por ali) */
                        if (!orig_rgb(cv, x, y, 0x0e071b) && !orig_rgb(cv, x, y, 0x3b1443) && !orig_rgb(cv, x, y, 0x622461) &&
                            !orig_rgb(cv, x, y, 0x891e2b) && !orig_rgb(cv, x, y, 0x571c27))
                            continue;
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
                if (ch->acessorios[a] == AC_COQUE_GELO) {
                    /* coque baixo na nuca, com a luz de cima à esquerda, e um grampo
                       (kanzashi) de cristal de gelo espetado para trás e para cima */
                    static const char *B[] = {".hiih.", "hiiihH", "hhhhHH", ".hHHH.", "..HH.."};
                    int bx = cx0[head] - 4, by = cy0[head] + 1;
                    for (int r = 0; r < 5; r++)
                        for (int c = 0; B[r][c]; c++) {
                            char k = B[r][c];
                            if (k == '.') continue;
                            cv_behind(cv, bx + c, by + r, k == 'H' ? ch->cabelo[0] : k == 'h' ? ch->cabelo[1] : ch->cabelo[2]);
                        }
                    for (int i = 1; i <= 3; i++) cv_behind(cv, bx + 1 - i, by - i, ch->destaque[1]);
                    int kx = bx - 3, ky = by - 4;
                    cv_behind(cv, kx, ky, (Rgb){250, 254, 255});
                    static const int d[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
                    for (int k = 0; k < 4; k++) cv_behind(cv, kx + d[k][0], ky + d[k][1], ch->destaque[0]);
                    cv_behind(cv, kx, ky - 2, ch->destaque[0]);
                    cv_behind(cv, kx - 2, ky, ch->destaque[0]);
                    break;
                }
                /* o cabelo solto: da nuca para baixo, colado nas costas, abrindo um pouco
                   para trás e com as pontas desencontradas */
                int top = cy0[head] + 2, len = 24;
                double sway = sin(idx * 0.8) * 1.0;
                /* caído, o cabelo não passa do chão: chega nele e segue deitado para trás
                   da cabeça, ao longo do chão */
                int ground = 0;
                for (int y = 0; y < CH; y++)
                    for (int x = 0; x < CW; x++)
                        if (cv->orig->p[y][x].a && s->lab[y][x] != SMEAR) ground = y;
                for (int i = 0; i < len; i++) {
                    int y = top + i, drift = 0;
                    if (y > ground) { drift = y - ground; y = ground - (drift % 2); }
                    if (y >= CH) break;
                    double u = (double)i / len;
                    int back = cx0[head];
                    for (int x = cx0[head]; x <= cx1[head]; x++)
                        if (y <= cy1[head] && comp[y][x] == head) { back = x; break; }
                    int left = back - 3 - (int)floor(u * 3.5 + sway * u + 0.5) + drift, right = back + 3 + drift;
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
static void florete(Canvas *cv, const Char *ch, const Blade *b, double len);
static void rest_weapon_at(Canvas *cv, const Char *ch, double hx, double hy, int idx);

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
    static const double da[3] = {-0.32, 0.0, 0.32};
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
/* a mão e o cabo de cada foice desenhada no quadro (para os arcos dos golpes) */
static struct { double hx, hy, ux, uy, len; bool back; } g_kama[4];
static int g_nkama;

/* Onde a foice pode pintar: no vazio, na lâmina do pack apagada e na roupa (a foice
   está na mão, na frente da roupa), nunca na pele (o punho e o rosto). A de trás
   (`behind`) não cobre a da frente. */
static bool kama_ok(const Canvas *cv, int x, int y, bool behind) {
    if (!cv_ok(x, y) || (behind && cv->wpx[y][x])) return false;
    int lb = lab_at(cv, x, y);
    if (lb == NONE || lb == BLADE || cv->a[y][x].a == 0) return true;
    Color o = cv->orig->p[y][x];
    uint32_t v = (uint32_t)o.r << 16 | (uint32_t)o.g << 8 | o.b;
    if (v == 0xe69c69 || v == 0xf6ca9f || v == 0xffffff || v == 0x5ac54f) return false;
    return cv->tag[y][x] == T_BODY && (lb == OTHER || lb == SHIRT || lb == DARK);
}

static void kama(Canvas *cv, const Blade *b0, double len, const Weapon *w, Rgb core, Rgb edge, bool behind, bool rest) {
    static Pts p;
    Rgb wood = rgb_set(w->haste[0]) ? w->haste[0] : (Rgb){138, 106, 68};
    /* parado: o cabo sobe da mão para a frente, e a lâmina fica em cima, para baixo */
    Blade bb = *b0;
    if (rest && bb.u[1] > 0) bb.u[1] = -bb.u[1];
    const Blade *b = &bb;
    double tx = b->hilt[0] + b->u[0] * len, ty = b->hilt[1] + b->u[1] * len;
    if (g_nkama < 4) {
        g_kama[g_nkama].hx = b->hilt[0];
        g_kama[g_nkama].hy = b->hilt[1];
        g_kama[g_nkama].ux = b->u[0];
        g_kama[g_nkama].uy = b->u[1];
        g_kama[g_nkama].len = len;
        g_kama[g_nkama].back = behind;
        g_nkama++;
    }
    /* a lâmina sai para baixo (a empunhadura natural de kama); com o cabo em pé, para a frente */
    double n[2] = {-b->u[1], b->u[0]};
    if (n[1] < -1e-9 || (fabs(n[1]) < 1e-9 && n[0] < 0)) { n[0] = -n[0]; n[1] = -n[1]; }
    /* caído, a lâmina não entra no chão (a linha mais baixa da figura): vira para cima */
    int ground = -1;
    for (int y = CH - 1; y >= 0 && ground < 0; y--)
        for (int x = 0; x < CW; x++)
            if (cv->orig->p[y][x].a) { ground = y; break; }
    if (rest && ty + n[1] * 5 > ground + 1) { n[0] = -n[0]; n[1] = -n[1]; }
    line_pts(&p, b->hilt[0] - b->u[0], b->hilt[1] - b->u[1], tx, ty);
    for (int i = 0; i < p.n; i++) {
        int x = p.x[i], y = p.y[i];
        if (kama_ok(cv, x, y, behind)) { cv_put(cv, x, y, wood); mark(cv, x, y); }
    }
    /* anel de metal onde a lâmina encaixa no cabo */
    for (int k = -1; k <= 1; k++) {
        int x = pyround(tx - b->u[0] * 1.2 + n[0] * k * 0.8), y = pyround(ty - b->u[1] * 1.2 + n[1] * k * 0.8);
        if (kama_ok(cv, x, y, behind)) cv_put(cv, x, y, (Rgb){150, 150, 160});
    }
    /* a lâmina: meia-lua fina que volta em direção à mão, com o fio claro por fora */
    int bl = len < 8 ? 5 : 6;
    for (int i = 0; i <= bl; i++) {
        double bend = i * i * (bl < 6 ? 0.12 : 0.09);
        for (int e = 0; e < (i < bl - 1 ? 2 : 1); e++) {
            double x = tx + n[0] * i - b->u[0] * (bend + e), y = ty + n[1] * i - b->u[1] * (bend + e);
            int xi = pyround(x), yi = pyround(y);
            if (kama_ok(cv, xi, yi, behind)) { cv_put(cv, xi, yi, e ? edge : core); mark(cv, xi, yi); }
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
            } else if (el == EL_GELO) {
                /* geada: espinhos de gelo curtos crescendo do fio, para cima e para os lados */
                if (r < 0.14) {
                    static const int d[4][2] = {{0, -1}, {1, -1}, {-1, -1}, {0, 1}};
                    int k = (int)(r * 100) % 4, dx = d[k][0], dy = d[k][1];
                    if (cv_is_empty(cv, x + dx, y + dy)) {
                        cv_put(cv, x + dx, y + dy, (Rgb){190, 236, 255});
                        if (r < 0.05 && cv_is_empty(cv, x + 2 * dx, y + 2 * dy)) cv_put(cv, x + 2 * dx, y + 2 * dy, (Rgb){250, 254, 255});
                    }
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

/* Lança (ou cajado) ao longo de b: a haste de -atras até a ponta, e na lança a
   ponta em folha, o anel e a fita curta que balança. */
static void lance(Canvas *cv, const Char *ch, const Blade *b, double tip, int idx) {
    const Weapon *w = &ch->arma;
    Rgb core = ch->lamina[0], edge = ch->lamina[1];
    int head = w->kind == W_LANCA ? (w->ponta ? w->ponta : 5) : 2;
    int back = w->atras ? w->atras : 14;
    stroke(cv, b, -back, tip - head, w->haste[0], &w->haste[1], 0, 0, 2, false);
    if (w->kind != W_LANCA) {
        Rgb cap = rgb_set(w->ponteira) ? w->ponteira : core;
        stroke(cv, b, tip - 2, tip, cap, NULL, 0, 0, 2, false);
        stroke(cv, b, -back - 2, -back, cap, NULL, 0, 0, 2, false);
        return;
    }
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
    int sw = (idx / 2) % 2;
    for (int j = 1; j <= 3; j++) {
        int x = pyround(cx - b->u[0] * (1 + sw) - n[0] * 0.3 * j), y = pyround(cy - b->u[1] * (1 + sw) + j);
        if (cv_ok(x, y) && cv->a[y][x].a == 0) cv_put(cv, x, y, j == 3 ? ch->destaque[1] : ch->destaque[0]);
    }
}

/* Lança num pack de espada: a katana embainhada (o cabo escuro e a tsuba dourada na
   cintura, ou na mão nos golpes) sai, porque a arma dela é a lança. */
static bool orig_hex(const Canvas *cv, int x, int y, uint32_t v) {
    Color o = cv->orig->p[y][x];
    return o.a && ((uint32_t)o.r << 16 | (uint32_t)o.g << 8 | o.b) == v;
}
static void drop_pack_hilt(Canvas *cv) {
    static bool gold[CH][CW];
    memset(gold, 0, sizeof gold);
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) gold[y][x] = orig_hex(cv, x, y, 0xffc825) || orig_hex(cv, x, y, 0xffa214);
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            if (!orig_hex(cv, x, y, 0x0e071b)) continue;
            bool near = false;
            for (int dy = -5; dy <= 5 && !near; dy++)
                for (int dx = -5; dx <= 5 && !near; dx++) near = cv_ok(x + dx, y + dy) && gold[y + dy][x + dx];
            if (near) erase_px(cv, x, y);
        }
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (gold[y][x]) erase_px(cv, x, y);
    /* a bainha preta (131313) que sai para trás do corpo correndo e pulando: cada mancha
       dela com 5 px ou mais, a maior parte na borda da figura, sai; o que ficava por
       dentro do corpo pega a cor da roupa em volta */
    static bool seen[CH][CW];
    static short st[CW * CH][2], comp[CW * CH][2];
    memset(seen, 0, sizeof seen);
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            if (seen[y][x] || !orig_hex(cv, x, y, 0x131313)) continue;
            int sp = 0, n = 0, out = 0;
            st[sp][0] = (short)x; st[sp][1] = (short)y; sp++;
            seen[y][x] = true;
            while (sp) {
                sp--;
                int cx = st[sp][0], cy = st[sp][1];
                comp[n][0] = (short)cx; comp[n][1] = (short)cy; n++;
                bool edge = false;
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = cx + dx, ny = cy + dy;
                        if (!cv_ok(nx, ny)) continue;
                        if (!dx != !dy && !cv->orig->p[ny][nx].a) edge = true;
                        if (seen[ny][nx] || !orig_hex(cv, nx, ny, 0x131313)) continue;
                        seen[ny][nx] = true;
                        st[sp][0] = (short)nx; st[sp][1] = (short)ny; sp++;
                    }
                out += edge;
            }
            if (n < 5 || out * 2 < n) continue;
            for (int i = 0; i < n; i++) cv_clear(cv, comp[i][0], comp[i][1]);
            for (int i = 0; i < n; i++) {
                int cx = comp[i][0], cy = comp[i][1], nb = 0;
                Color c = {0};
                static const int d4[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
                for (int k = 0; k < 4; k++) {
                    int nx = cx + d4[k][0], ny = cy + d4[k][1];
                    if (cv_ok(nx, ny) && cv->a[ny][nx].a) { nb++; c = cv->a[ny][nx]; }
                }
                if (nb >= 3) { cv->a[cy][cx] = c; cv->tag[cy][cx] = T_BODY; }
            }
        }
}

/* Parada, correndo, pulando ou apanhando: a lança em pé na mão da frente, a ponta
   um pouco para a frente e o pé da haste perto do chão. A mão é a pele do punho
   (e69c69 no pack) mais à frente, abaixo da cabeça. */
static void rest_lance(Canvas *cv, const Char *ch, int idx) {
    int top = CH;
    for (int y = 0; y < CH && top == CH; y++)
        for (int x = 0; x < CW; x++)
            if (cv->orig->p[y][x].a) { top = y; break; }
    int hx = -1, hy = 0;
    for (int y = top + 12; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (orig_hex(cv, x, y, 0xe69c69) && x > hx) { hx = x; hy = y; }
    if (hx < 0) return;
    rest_weapon_at(cv, ch, hx, hy, idx);
}

/* ----- golpes desenhados aqui -------------------------------------------- */
/* Tapa os buracos que ficaram dentro da figura (o corpo fechado com raio 2), de
   fora para dentro, com a cor mais comum em volta. */
static void fill_gone(Canvas *cv, bool (*gone)[CW]) {
    static Mask body, dil, inv, dil2;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) body[y][x] = cv->a[y][x].a && cv->tag[y][x] == T_BODY;
    mask_dilate(dil, body, 2, false);
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) inv[y][x] = !dil[y][x];
    mask_dilate(dil2, inv, 2, false);
    for (int pass = 0; pass < 8; pass++) {
        static Color put[CH][CW];
        static bool has[CH][CW];
        memset(has, 0, sizeof has);
        for (int y = 1; y < CH - 1; y++)
            for (int x = 1; x < CW - 1; x++) {
                if (!gone[y][x] || dil2[y][x] || cv->a[y][x].a) continue;
                Color nb[8];
                int n = 0;
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++)
                        if ((dx || dy) && cv->a[y + dy][x + dx].a && cv->tag[y + dy][x + dx] == T_BODY) nb[n++] = cv->a[y + dy][x + dx];
                if (n < 3) continue;
                int best = 0, bc = 0;
                for (int i = 0; i < n; i++) {
                    int c = 0;
                    for (int j = 0; j < n; j++) c += nb[i].r == nb[j].r && nb[i].g == nb[j].g && nb[i].b == nb[j].b;
                    if (c > bc) { bc = c; best = i; }
                }
                put[y][x] = nb[best];
                has[y][x] = true;
            }
        bool any = false;
        for (int y = 0; y < CH; y++)
            for (int x = 0; x < CW; x++)
                if (has[y][x]) { cv->a[y][x] = put[y][x]; cv->tag[y][x] = T_BODY; any = true; }
        if (!any) break;
    }
}

/* Tira o corte de katana do pack: as cores de rastro_pack (as mesmas da camisa)
   a mais de 2 px do resto do corpo (cabelo, pele, hakama, contorno), ou abaixo da
   faixa, onde camisa não há. */
static void desmear(Canvas *cv, const Char *ch) {
    static bool core[CH][CW], trail[CH][CW];
    static short dist[CH][CW], q[CW * CH][2];
    int head = 0, tail = 0, belt = CH;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            Color o = cv->orig->p[y][x];
            bool t = false;
            for (int i = 0; i < 3 && !t; i++)
                t = rgb_set(ch->rastro_pack[i]) && o.a && o.r == ch->rastro_pack[i].r && o.g == ch->rastro_pack[i].g &&
                    o.b == ch->rastro_pack[i].b;
            trail[y][x] = t;
            core[y][x] = o.a && !t;
            dist[y][x] = core[y][x] ? 0 : 99;
            if (core[y][x]) { q[tail][0] = (short)x; q[tail][1] = (short)y; tail++; }
            /* a faixa: a primeira linha com a hakama do pack */
            if (belt == CH && o.a && rgb_set(ch->hakama[0]))
                for (int i = 0; i < 24 && rgb_set(ch->troca[i].de); i++)
                    if (ch->troca[i].de.r == o.r && ch->troca[i].de.g == o.g && ch->troca[i].de.b == o.b &&
                        ch->troca[i].para.r == ch->hakama[0].r && ch->troca[i].para.g == ch->hakama[0].g &&
                        ch->troca[i].para.b == ch->hakama[0].b) { belt = y; break; }
        }
    while (head < tail) {
        int cx = q[head][0], cy = q[head][1];
        head++;
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                int nx = cx + dx, ny = cy + dy;
                if (!cv_ok(nx, ny) || dist[ny][nx] <= dist[cy][cx] + 1) continue;
                dist[ny][nx] = (short)(dist[cy][cx] + 1);
                q[tail][0] = (short)nx; q[tail][1] = (short)ny; tail++;
            }
    }
    static bool gone[CH][CW];
    memset(gone, 0, sizeof gone);
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            /* solto: além das cores do rastro, o que a leitura marcou como rastro (o miolo
               branco do arco, que tem a cor da lâmina e do olho) */
            bool cut = ch->rastro_pack_solto ? (trail[y][x] && lab_at(cv, x, y) != BLADE) || lab_at(cv, x, y) == SMEAR
                                             : trail[y][x] && (dist[y][x] > 2 || y > belt + 2);
            if (cut) { cv_clear(cv, x, y); gone[y][x] = true; }
        }
    if (!ch->rastro_pack_solto) return;
    /* o rastro passava na frente do corpo: o buraco dentro da figura (fechada com
       raio 2) é tapado de fora para dentro com a cor mais comum em volta */
    fill_gone(cv, gone);
}

/* Garras, foices e adagas no corpo de duas espadas: o que sobra das katanas do
   pack (o pedaço que passa na frente do corpo e não entrou na lâmina lida, e o
   cabo preto) sai, e o buraco é tapado com o corpo. O branco do olho (ao lado do
   verde) fica. */
static void scrub_pack_blades(Canvas *cv) {
    static const uint32_t cols[5] = {0xffffff, 0xf8f8f8, 0xc7cfdd, 0x92a1b9, 0x131313};
    static bool gone[CH][CW];
    memset(gone, 0, sizeof gone);
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            Color o = cv->orig->p[y][x], c = cv->a[y][x];
            if (!o.a || cv->wpx[y][x] || cv->tag[y][x] != T_BODY || c.r != o.r || c.g != o.g || c.b != o.b) continue;
            bool white = false;
            for (int i = 0; i < 5 && !white; i++) white = orig_hex(cv, x, y, cols[i]);
            if (!white) continue;
            bool eye = false;
            for (int dy = -1; dy <= 1 && !eye; dy++)
                for (int dx = -1; dx <= 1 && !eye; dx++) eye = cv_ok(x + dx, y + dy) && orig_hex(cv, x + dx, y + dy, 0x5ac54f);
            if (eye) continue;
            cv_clear(cv, x, y);
            gone[y][x] = true;
        }
    fill_gone(cv, gone);
}

/* A mão da frente num pack: o punho (e69c69) mais à frente, a média dos pixels
   dele. */
static bool pack_fist(const Canvas *cv, double *hx, double *hy) {
    int bx = -1, by = 0;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (orig_hex(cv, x, y, 0xe69c69) && x > bx) { bx = x; by = y; }
    if (bx < 0) return false;
    double sx = 0, sy = 0;
    int n = 0;
    for (int y = by - 2; y <= by + 2; y++)
        for (int x = bx - 3; x <= bx; x++)
            if (cv_ok(x, y) && orig_hex(cv, x, y, 0xe69c69)) { sx += x; sy += y; n++; }
    *hx = sx / n;
    *hy = sy / n;
    return true;
}

static void fx_put(Canvas *cv, int x, int y, Rgb c) {
    if (cv_ok(x, y) && cv->a[y][x].a == 0) { cv_put(cv, x, y, c); mark(cv, x, y); }
}
/* O mesmo sem marcar como arma: o brilho e a poeira do elemento não grudam nele. */
static void fx_put_clean(Canvas *cv, int x, int y, Rgb c) {
    if (cv_ok(x, y) && cv->a[y][x].a == 0) cv_put(cv, x, y, c);
}

/* Rastro reto de estocada, de (x0, y0) até a ponta (x1, y1): fino nas pontas e
   grosso no meio, claro perto da ponta; `fade` no quadro seguinte (só a cauda). */
static void thrust_trail(Canvas *cv, const Char *ch, double x0, double y0, double x1, double y1, bool fade, bool thin) {
    double dx = x1 - x0, dy = y1 - y0, L = sqrt(dx * dx + dy * dy);
    if (L < 1) return;
    double ux = dx / L, uy = dy / L, nx = -uy, ny = ux;
    int n = (int)ceil(L);
    for (int i = 0; i <= n; i++) {
        double t = (double)i / n;
        if (fade && t > 0.55) break;
        double px = x0 + dx * t, py = y0 + dy * t;
        int w = fade || (thin && (t < 0.4 || t > 0.75)) ? 0 : (t > 0.15 && t < 0.85) ? 1 : 0;
        Rgb core = fade ? ch->rastro[2] : t > 0.7 ? ch->rastro[0] : ch->rastro[1];
        fx_put(cv, pyround(px), pyround(py), core);
        for (int k = 1; k <= w; k++) {
            fx_put(cv, pyround(px + nx * k), pyround(py + ny * k), t > 0.7 ? ch->rastro[1] : ch->rastro[2]);
            fx_put(cv, pyround(px - nx * k), pyround(py - ny * k), ch->rastro[2]);
        }
    }
    if (fade) return;
    /* linhas de velocidade acima e abaixo */
    for (int s2 = -1; s2 <= 1; s2 += 2)
        for (int i = 0; i < n * 6 / 10; i++) {
            double t = 0.15 + (double)i / n;
            if ((i & 1) || t > 0.75) continue;
            fx_put(cv, pyround(x0 + dx * t + nx * 4 * s2), pyround(y0 + dy * t + ny * 4 * s2), ch->rastro[2]);
        }
}

/* Varrida: o arco que a ponta da lança descreve em volta da mão, de a0 a a1
   (graus, y para baixo), achatado na altura (`flat`): largo e baixo, com a frente
   clara e a cauda escura. */
static void sweep_trail(Canvas *cv, const Char *ch, double cx, double cy, double r, double a0, double a1, double flat,
                        bool fade) {
    int n = (int)(fabs(a1 - a0) * r * 0.035) + 8;
    for (int i = 0; i <= n; i++) {
        double t = (double)i / n;              /* 0 = começo do arco, 1 = a ponta agora */
        if (fade && t < 0.45) continue;
        double a = (a0 + (a1 - a0) * t) * 3.14159265 / 180;
        int w = fade ? 1 : t > 0.5 ? 3 : t > 0.2 ? 2 : 1;
        for (int k = 0; k < w; k++) {
            double rr = r - k;
            Rgb c = fade ? ch->rastro[2] : k == 0 ? (t > 0.6 ? ch->rastro[0] : ch->rastro[1]) : k == 1 ? ch->rastro[1] : ch->rastro[2];
            fx_put(cv, pyround(cx + cos(a) * rr), pyround(cy + sin(a) * rr * flat), c);
        }
    }
}

/* Lança nos golpes: a lança na mão da frente, reta, estocando (ATTACK_1 e o
   especial, na horizontal; ATTACK_3 um pouco para cima) ou varrendo baixo
   (ATTACK_2: de trás, rente ao chão, até a frente). */
static void lance_attack(Canvas *cv, const Char *ch, const Ctx *ctx) {
    const Seg *s = cv->seg;
    bool foil = ch->arma.kind == W_FLORETE;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (s->lab[y][x] == BLADE) erase_px(cv, x, y);
    double hx, hy;
    if (!pack_fist(cv, &hx, &hy)) return;
    int ph = ctx->phase;
    bool sweep = !foil && !strcmp(ctx->anim, "ATTACK_2");
    double ang = !strcmp(ctx->anim, "ATTACK_3") ? -14 : foil && !strcmp(ctx->anim, "ATTACK_2") ? 24 : 0;
    if (sweep) ang = ph == PH_ANTICIPATION ? 160 : ph == PH_STRIKE ? 168 : ph == PH_CONTACT ? 16 : ctx->idx == ctx->contact + 1 ? 8 : 0;
    double a = ang * 3.14159265 / 180, escala = ch->arma.escala > 0 ? ch->arma.escala : 1.0, tip = KATANA * escala;
    Blade b = {0};
    b.u[0] = cos(a);
    b.u[1] = sin(a);
    b.hilt[0] = hx;
    b.hilt[1] = hy;
    cv->pen = T_WEAPON;
    if (foil) florete(cv, ch, &b, tip);
    else lance(cv, ch, &b, tip, ctx->idx);
    bool contact = ph == PH_CONTACT, after = ph == PH_RECOVERY && ctx->idx == ctx->contact + 1;
    if (!contact && !after) return;
    if (sweep) {
        sweep_trail(cv, ch, hx, hy, tip - 1, 168, ang, 0.45, after);
    } else {
        /* o rastro acaba na ponta: o alcance medido é o da lança */
        double tx = hx + b.u[0] * tip, ty = hy + b.u[1] * tip;
        thrust_trail(cv, ch, hx - b.u[0] * 14, hy - b.u[1] * 14, tx - b.u[0] * 2, ty - b.u[1] * 2, after, foil);
    }
}

/* Florete de esgrima ao longo de b: lâmina reta e fina, a ponta mais clara, e o
   copo (a campânula) na mão, com as bordas voltadas para ela. */
static void florete(Canvas *cv, const Char *ch, const Blade *b, double len) {
    Rgb core = ch->lamina[0], edge = ch->lamina[1];
    stroke(cv, b, 2, len - 2, core, NULL, 0, 0, 2, false);
    stroke(cv, b, len - 2, len, edge, NULL, 0, 0, 2, false);
    double n[2];
    perp(b->u, n);
    for (int k = -2; k <= 2; k++) {
        double back = abs(k) == 2 ? 0.6 : 1.6;
        int x = pyround(b->hilt[0] + b->u[0] * back + n[0] * k), y = pyround(b->hilt[1] + b->u[1] * back + n[1] * k);
        if (cv_ok(x, y) && (weapon_ok(cv, x, y) || lab_at(cv, x, y) == HANDLE || cv->a[y][x].a == 0))
            cv_put(cv, x, y, abs(k) == 2 ? ch->destaque[1] : ch->destaque[0]);
    }
}

/* A arma na mão da frente, fora dos golpes: a lança em pé, o florete em guarda,
   apontado para a frente e para baixo. */
static void rest_weapon_at(Canvas *cv, const Char *ch, double hx, double hy, int idx) {
    double escala = ch->arma.escala > 0 ? ch->arma.escala : 1.0;
    Blade b = {0};
    b.hilt[0] = hx;
    b.hilt[1] = hy;
    if (ch->arma.kind == W_FLORETE) {
        b.u[0] = 0.77; b.u[1] = 0.64;
        florete(cv, ch, &b, KATANA * escala);
    } else {
        b.u[0] = 0.30; b.u[1] = -0.954;
        lance(cv, ch, &b, KATANA * escala, idx);
    }
}

/* Caindo: a lança continua na mão enquanto o corpo está em pé ou de joelhos; no
   chão (a figura mais larga que alta), fica deitada no chão na frente dela. */
static void death_lance(Canvas *cv, const Char *ch, int idx) {
    int x0 = CW, x1 = -1, y0 = CH, y1 = -1;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (cv->orig->p[y][x].a) {
                if (x < x0) x0 = x;
                if (x > x1) x1 = x;
                if (y < y0) y0 = y;
                if (y > y1) y1 = y;
            }
    if (x1 < 0) return;
    if (y1 - y0 >= (x1 - x0) * 8 / 10) { rest_lance(cv, ch, idx); return; }
    Blade b = {0};
    b.u[0] = 1;
    b.u[1] = 0;
    b.hilt[0] = (x0 + x1) / 2.0 - 4;
    b.hilt[1] = y1;
    double escala = ch->arma.escala > 0 ? ch->arma.escala : 1.0;
    if (ch->arma.kind == W_FLORETE) florete(cv, ch, &b, KATANA * escala);
    else lance(cv, ch, &b, KATANA * escala, 0);   /* sem a fita balançando */
}

/* O ponto do golpe: a ponta da arma mais à frente (o pixel de arma mais à
   direita), ou o punho. */
static bool strike_point(const Canvas *cv, double *tx, double *ty) {
    int bx = -1, by = 0;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (cv->wpx[y][x] && x > bx) { bx = x; by = y; }
    if (bx >= 0) { *tx = bx; *ty = by; return true; }
    return pack_fist(cv, tx, ty);
}

/* Linha reta de rastro de (x0, y0) a (x1, y1): miolo claro no meio, pontas no tom
   do meio; `dim` só no tom escuro. */
static void trail_line(Canvas *cv, const Char *ch, double x0, double y0, double x1, double y1, bool dim) {
    double dx = x1 - x0, dy = y1 - y0;
    int n = (int)ceil(fmax(fabs(dx), fabs(dy)));
    for (int i = 0; i <= n; i++) {
        double t = n ? (double)i / n : 0;
        Rgb c = dim ? ch->rastro[2] : (t > 0.2 && t < 0.8) ? ch->rastro[0] : ch->rastro[1];
        fx_put_clean(cv, pyround(x0 + dx * t), pyround(y0 + dy * t), c);
    }
}

/* Garras: três riscos paralelos e curtos, de arranhão, na frente das garras (de
   cima para baixo no ATTACK_1, de baixo para cima no ATTACK_2, quase deitados no
   ATTACK_3), e no quadro seguinte o mesmo mais curto e escuro. */
static void claw_scratches(Canvas *cv, const Char *ch, const Ctx *ctx) {
    bool contact = ctx->phase == PH_CONTACT, after = ctx->phase == PH_RECOVERY && ctx->idx == ctx->contact + 1;
    if (!contact && !after) return;
    double tx, ty;
    if (!strike_point(cv, &tx, &ty)) return;
    double ux = 0.45, uy = 0.89;
    if (!strcmp(ctx->anim, "ATTACK_2")) uy = -0.89;
    else if (!strcmp(ctx->anim, "ATTACK_3")) { ux = 0.96; uy = 0.28; }
    double nx = -uy, ny = ux, L = after ? 4 : 7, cx = tx + 3, cy = ty;
    cv->pen = T_WEAPON;
    for (int k = -1; k <= 1; k++) {
        double ox = cx + nx * k * 3, oy = cy + ny * k * 3, l = L - (k ? 1 : 0);
        trail_line(cv, ch, ox - ux * l, oy - uy * l, ox + ux * l, oy + uy * l, after);
        if (!after) trail_line(cv, ch, ox - ux * l + nx, oy - uy * l + ny, ox + ux * (l - 2) + nx, oy + uy * (l - 2) + ny, true);
    }
}

/* Foices: dois arcos curtos e finos, um de cada foice, girando em volta da mão até
   a lâmina de agora (de cima no ATTACK_1, no ATTACK_3 e no especial; de baixo no
   ATTACK_2); no quadro seguinte, só o fim de cada um, no tom escuro. */
static void kama_arcs(Canvas *cv, const Char *ch, const Ctx *ctx) {
    bool contact = ctx->phase == PH_CONTACT, after = ctx->phase == PH_RECOVERY && ctx->idx == ctx->contact + 1;
    if (!contact && !after) return;
    bool below = !strcmp(ctx->anim, "ATTACK_2");
    int pen = cv->pen;
    cv->pen = T_FX;
    for (int k = 0; k < g_nkama; k++) {
        double a1 = atan2(g_kama[k].uy, g_kama[k].ux);
        /* "de cima": o giro que leva a ponta para cima (depende do lado para onde aponta) */
        double up = cos(a1) >= 0 ? -1 : 1, dir = below ? -up : up;
        double span = (g_kama[k].back ? 60 : 80) * 3.14159265 / 180, a0 = a1 + dir * span;
        double r = g_kama[k].len + (g_kama[k].back ? 2 : 3);
        int n = (int)(span * r * 1.6) + 6;
        for (int i = 0; i <= n; i++) {
            double t = (double)i / n;
            if (after && t < 0.55) continue;
            double a = a0 + (a1 - a0) * t;
            int wd = !after && t > 0.55 ? 2 : 1;
            for (int e = 0; e < wd; e++) {
                Rgb c = after || g_kama[k].back ? ch->rastro[e ? 2 : 1] : e ? ch->rastro[1] : t > 0.5 ? ch->rastro[0] : ch->rastro[1];
                if (!after && t < 0.25) c = ch->rastro[2];
                fx_put_clean(cv, pyround(g_kama[k].hx + cos(a) * (r - e)), pyround(g_kama[k].hy + sin(a) * (r - e)), c);
            }
        }
    }
    cv->pen = pen;
}

/* Espada e espada curta: dois arcos cruzados em X na frente da mão, de tamanhos
   diferentes: o da espada (grande, claro) desce de cima para a frente, o da curta
   (menor, escuro) sobe de baixo; no quadro seguinte, só o fim de cada um. */
static void crossed_arcs(Canvas *cv, const Char *ch, const Ctx *ctx, double hx, double hy) {
    bool contact = ctx->phase == PH_CONTACT, after = ctx->phase == PH_RECOVERY && ctx->idx == ctx->contact + 1;
    if (!contact && !after) return;
    const Weapon *w = &ch->arma;
    double escala = w->escala > 0 ? w->escala : 1.0, L = KATANA * escala;
    int pen = cv->pen;
    cv->pen = T_FX;
    for (int k = 0; k < 2; k++) {
        /* começo, fim e barriga (curva para fora do corpo) de cada arco */
        double s0 = k ? 0.62 : 1.0;
        double p0x = hx + 1, p0y = hy + (k ? 7 : -11) * s0, p2x = hx + L * (k ? 0.72 : 0.92), p2y = hy + (k ? -5 : 5) * s0;
        double mx = (p0x + p2x) / 2, my = (p0y + p2y) / 2, dx = p2x - p0x, dy = p2y - p0y, dl = sqrt(dx * dx + dy * dy);
        double nx = dy / dl, ny = -dx / dl;           /* normal para a frente */
        if (nx < 0) { nx = -nx; ny = -ny; }
        double bulge = k ? 2.5 : 4.0, p1x = mx + nx * bulge, p1y = my + ny * bulge;
        int n = (int)(dl * 1.6) + 6;
        for (int i = 0; i <= n; i++) {
            double t = (double)i / n;
            if (after && t < 0.55) continue;
            double u = 1 - t, X = u * u * p0x + 2 * u * t * p1x + t * t * p2x, Y = u * u * p0y + 2 * u * t * p1y + t * t * p2y;
            int wd = !after && t > 0.45 && !(k && t < 0.6) ? 2 : 1;
            for (int e = 0; e < wd; e++) {
                Rgb c = after ? ch->rastro[2] : k ? ch->rastro[e ? 2 : 1] : e ? ch->rastro[1] : t > 0.5 ? ch->rastro[0] : ch->rastro[1];
                if (!after && t < 0.2) c = ch->rastro[2];
                fx_put_clean(cv, pyround(X - nx * e), pyround(Y - ny * e), c);
            }
        }
    }
    cv->pen = pen;
}

/* os punhos das adagas desenhadas no quadro (para os cortes dos golpes) */
static struct { double hx, hy; bool back; } g_dag[4];
static int g_ndag;

/* Adaga em empunhadura invertida (num pack): o pomo sai 2 px na frente do punho, a
   guarda fica atrás dele e a lâmina volta ao longo do antebraço (o contrário da
   espada do pack), 1 px para fora do braço, por cima da manga, com um contorno
   escuro por dentro. A de trás não cobre a da frente. */
static void reverse_dagger(Canvas *cv, const Char *ch, double hx, double hy, const double u[2], double len, Rgb core,
                           Rgb edge, bool rear) {
    const Weapon *w = &ch->arma;
    double d[2] = {-u[0], -u[1]}, n[2] = {-d[1], d[0]};
    if (n[1] > 0 || (fabs(n[1]) < 1e-9 && n[0] < 0)) { n[0] = -n[0]; n[1] = -n[1]; }   /* para fora: para cima */
    Rgb grip = ch->cabo, guard = rgb_set(w->guarda) ? w->guarda : ch->destaque[1], dark = ch->hakama[4];
    if (g_ndag < 4) { g_dag[g_ndag].hx = hx; g_dag[g_ndag].hy = hy; g_dag[g_ndag].back = rear; g_ndag++; }
#define DAG_OK(X, Y) (cv_ok(X, Y) && !orig_hex(cv, X, Y, 0xf6ca9f) && !(rear && cv->wpx[Y][X]))
    for (int i = 1; i <= 2; i++) {   /* o pomo na frente do punho */
        int x = pyround(hx + u[0] * i), y = pyround(hy + u[1] * i);
        if (DAG_OK(x, y)) { cv_put(cv, x, y, grip); mark(cv, x, y); }
    }
    for (int k = -1; k <= 1; k += 2) {   /* a guarda, atravessada, atrás do punho */
        int x = pyround(hx + d[0] * 1.2 + n[0] * k), y = pyround(hy + d[1] * 1.2 + n[1] * k);
        if (DAG_OK(x, y)) { cv_put(cv, x, y, guard); mark(cv, x, y); }
    }
    static Pts p;
    double bx = hx + n[0], by = hy + n[1];
    line_pts(&p, bx + d[0] * 2, by + d[1] * 2, bx + d[0] * len, by + d[1] * len);
    for (int i = 0; i < p.n; i++) {
        int x = p.x[i], y = p.y[i];
        if (!DAG_OK(x, y)) continue;
        cv_put(cv, x, y, i == p.n - 1 ? (Rgb){255, 255, 255} : i < p.n * 2 / 3 ? core : edge);
        mark(cv, x, y);
        /* o fio, por fora */
        int ex = pyround(x + n[0]), ey = pyround(y + n[1]);
        if (i < p.n - 2 && DAG_OK(ex, ey) && !cv->wpx[ey][ex]) { cv_put(cv, ex, ey, edge); mark(cv, ex, ey); }
        /* por dentro, sobre o corpo, o contorno escuro que separa a lâmina da manga */
        int ix = pyround(x - n[0]), iy = pyround(y - n[1]);
        if (cv_ok(ix, iy) && cv->a[iy][ix].a && !cv->wpx[iy][ix] && !orig_hex(cv, ix, iy, 0xe69c69) &&
            !orig_hex(cv, ix, iy, 0xf6ca9f))
            cv_put(cv, ix, iy, dark);
    }
#undef DAG_OK
}

/* Adagas: cortes pequenos e secos, retos, na frente do punho (descendo no ATTACK_1 e
   no especial, subindo no ATTACK_2, quase deitado no ATTACK_3), com um brilho no
   meio; o da mão de trás mais curto, logo atrás. No quadro seguinte, só o fim. */
static void dagger_cuts(Canvas *cv, const Char *ch, const Ctx *ctx) {
    bool contact = ctx->phase == PH_CONTACT, after = ctx->phase == PH_RECOVERY && ctx->idx == ctx->contact + 1;
    if ((!contact && !after) || !g_ndag) return;
    int f = -1;
    for (int k = 0; k < g_ndag; k++)
        if (f < 0 || g_dag[k].hx > g_dag[f].hx) f = k;
    double ux = 0.8, uy = 0.6;
    if (!strcmp(ctx->anim, "ATTACK_2")) uy = -0.6;
    else if (!strcmp(ctx->anim, "ATTACK_3")) { ux = 0.97; uy = -0.26; }
    cv->pen = T_WEAPON;
    for (int k = 0; k < 2; k++) {
        double L = k ? 2.5 : 4.5, cx = g_dag[f].hx + 6 - k * 3, cy = g_dag[f].hy + k * 3;
        double t0 = after ? L * 0.3 : -L;
        for (double t = t0; t <= L + 1e-9; t += 0.5) {
            int x = pyround(cx + ux * t), y = pyround(cy + uy * t);
            Rgb c = after ? ch->rastro[2] : fabs(t) < 1 && !k ? (Rgb){255, 255, 255} : fabs(t) < L * 0.6 ? ch->rastro[0] : ch->rastro[1];
            fx_put_clean(cv, x, y, c);
            if (!after && !k) fx_put_clean(cv, x, y + 1, ch->rastro[2]);
        }
    }
}

/* Duas espadas da tempestade: cada lâmina deixa um raio em zigue-zague no caminho
   da ponta (um arco quebrado em volta da mão, de cima no ATTACK_1, no ATTACK_3 e no
   especial, de baixo no ATTACK_2), com uma faísca solta; no quadro seguinte, só o
   fim, no azul escuro. */
static void bolt_arcs(Canvas *cv, const Char *ch, const Ctx *ctx) {
    bool contact = ctx->phase == PH_CONTACT, after = ctx->phase == PH_RECOVERY && ctx->idx == ctx->contact + 1;
    if (!contact && !after) return;
    const Seg *s = cv->seg;
    bool below = !strcmp(ctx->anim, "ATTACK_2");
    int pen = cv->pen, nb = 0;
    cv->pen = T_FX;
    for (int bi = 0; bi < s->nblades && nb < 2; bi++) {
        const Blade *b = &s->blades[bi];
        if (b->nearest > 8 || b->loose || b->farthest < 5) continue;
        nb++;
        double a1 = atan2(b->u[1], b->u[0]), up = cos(a1) >= 0 ? -1 : 1, dir = below ? -up : up;
        double span = 75 * 3.14159265 / 180, a0 = a1 + dir * span, R = b->farthest;
        int segs = 6;
        double px = 0, py = 0;
        for (int k = 0; k <= segs; k++) {
            double t = (double)k / segs, a = a0 + (a1 - a0) * t;
            double r = R + (k == 0 || k == segs ? 0 : (k % 2 ? 1.8 : -1.8));
            double x = b->hilt[0] + cos(a) * r, y = b->hilt[1] + sin(a) * r;
            if (k > 0 && (!after || t > 0.6)) {
                static Pts p;
                line_pts(&p, px, py, x, y);
                for (int i = 0; i < p.n; i++) {
                    Rgb c = after ? ch->rastro[2] : t > 0.5 ? ch->rastro[0] : ch->rastro[1];
                    fx_put_clean(cv, p.x[i], p.y[i], c);
                    /* o brilho azul em volta do miolo branco */
                    if (!after && t > 0.3)
                        for (int e = -1; e <= 1; e += 2)
                            if (cv_ok(p.x[i], p.y[i] + e) && cv->a[p.y[i] + e][p.x[i]].a == 0)
                                fx_put_clean(cv, p.x[i], p.y[i] + e, ch->rastro[2]);
                }
            }
            /* uma faísca solta saindo de um dos cotovelos do raio */
            if (!after && k == 3) {
                double ox = cos(a) * 3, oy = sin(a) * 3;
                fx_put_clean(cv, pyround(x + ox), pyround(y + oy), ch->rastro[1]);
                fx_put_clean(cv, pyround(x + ox * 1.6 + 1), pyround(y + oy * 1.6), ch->rastro[1]);
            }
            px = x;
            py = y;
        }
    }
    cv->pen = pen;
}

/* Kojiro com a katana na bainha (EMBAINHADO, e o começo do DESEMBAINHAR): some a
   lâmina e o cabo da guarda; o cabo sai da boca da bainha, na cintura, para a frente
   e para cima (o contrário da bainha), com a tsuba na boca. No DESEMBAINHAR, o
   polegar solta a tsuba (um brilho na boca), a espada sai num corte subindo com o
   clarão do saque e desce até a guarda do IDLE (o último quadro é o IDLE 0). */
static void iai(Canvas *cv, const Char *ch, const Ctx *ctx) {
    const Seg *s = cv->seg;
    bool draw = !strcmp(ctx->anim, "DESEMBAINHAR");
    int idx = ctx->idx;
    /* a espada da guarda: a lâmina na mão, a mais longa */
    int gb = -1;
    for (int bi = 0; bi < s->nblades; bi++)
        if (blade_at_hand(&s->blades[bi]) && (gb < 0 || s->blades[bi].farthest > s->blades[gb].farthest)) gb = bi;
    if (gb < 0) return;
    const Blade *g = &s->blades[gb];
    double hx = g->hilt[0], hy = g->hilt[1], L = g->farthest;
    /* as cores: o cabo e a lâmina como estão na guarda */
    Rgb wrap = {40, 36, 44}, core = {240, 244, 250}, edge = {160, 170, 190};
    int bestl = -1;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            Color c = cv->a[y][x];
            if (!c.a) continue;
            if (s->lab[y][x] == HANDLE) wrap = (Rgb){c.r, c.g, c.b};
            if (s->lab[y][x] == BLADE) {
                int l = c.r + c.g + c.b;
                if (l > bestl) { bestl = l; core = (Rgb){c.r, c.g, c.b}; }
            }
        }
    if (rgb_set(ch->lamina[0])) core = ch->lamina[0];
    if (rgb_set(ch->lamina[1])) edge = ch->lamina[1];
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (s->lab[y][x] == BLADE || s->lab[y][x] == HANDLE || s->lab[y][x] == SMEAR) erase_px(cv, x, y);
    /* a boca da bainha: o pixel da bainha mais perto da mão; o eixo, dela até o meio da bainha */
    double mx = -1, my = 0, md = 1e9, sx = 0, sy = 0;
    int ns = 0;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (s->lab[y][x] == SAYA) {
                double d = (x - hx) * (x - hx) + (y - hy) * (y - hy);
                if (d < md) { md = d; mx = x; my = y; }
                sx += x; sy += y; ns++;
            }
    /* na bainha o cabo fica na frente da faixa escura: trama clara (ito) com os losangos escuros */
    Rgb tsuba = {200, 160, 72}, kashira = {150, 130, 90}, ito = {214, 206, 184};
    cv->pen = T_WEAPON;
    bool sheathed = !draw || idx <= 1;
    if (sheathed) {
        if (mx < 0 || ns < 3) return;
        double ax = mx - sx / ns, ay = my - sy / ns, al = sqrt(ax * ax + ay * ay);
        if (al < 1e-6) return;
        ax /= al;
        ay /= al;
        double pull = draw && idx == 1 ? 1.5 : 0;   /* o polegar empurra a tsuba */
        for (int k = -1; k <= 1; k += 2) {
            int x = pyround(mx + ax * (0.5 + pull) - ay * k), y = pyround(my + ay * (0.5 + pull) + ax * k);
            if (cv_ok(x, y)) cv_put(cv, x, y, tsuba);
        }
        static Pts p;
        line_pts(&p, mx + ax * (1 + pull), my + ay * (1 + pull), mx + ax * (7 + pull), my + ay * (7 + pull));
        for (int i = 0; i < p.n; i++) {
            cv_put(cv, p.x[i], p.y[i], i == p.n - 1 ? kashira : i % 2 ? wrap : ito);
            /* a outra metade da grossura do cabo, com os losangos trocados */
            int ux = p.x[i], uy = p.y[i] - 1;
            if (i < p.n - 1 && cv_ok(ux, uy)) cv_put(cv, ux, uy, i % 2 ? ito : wrap);
        }
        if (draw && idx == 1) {   /* o primeiro dedo de lâmina aparece na boca */
            cv_put(cv, pyround(mx + ax * 0.5), pyround(my + ay * 0.5), (Rgb){255, 255, 255});
            fx_put_clean(cv, pyround(mx + ax * 0.5 + 1), pyround(my + ay * 0.5 - 2), core);
        }
        return;
    }
    /* sacando: 2 subindo na frente (-32 graus), 3 quase reta (-6), 4 descendo (+14);
       a guarda do IDLE fica em torno de +27 */
    double ang = (idx == 2 ? -32 : idx == 3 ? -6 : 14) * 3.14159265 / 180;
    Blade b = *g;
    b.u[0] = cos(ang);
    b.u[1] = sin(ang);
    b.hilt[0] = hx;
    b.hilt[1] = hy;
    g_over_body = true;   /* a espada passa na frente do corpo */
    stroke(cv, &b, -5, -1, wrap, NULL, 0, 0, 2, false);
    stroke(cv, &b, 1, L, core, &edge, 0, 0, 1, false);
    g_over_body = false;
    double n[2];
    perp(b.u, n);
    for (int k = -1; k <= 1; k += 2) {
        int x = pyround(hx + b.u[0] * 0.3 + n[0] * k), y = pyround(hy + b.u[1] * 0.3 + n[1] * k);
        if (cv_ok(x, y)) cv_put(cv, x, y, tsuba);
    }
    /* o clarão do saque: arco da bainha (embaixo, atrás) até a lâmina */
    cv->pen = T_FX;
    if (idx == 4) {   /* já na descida: só um brilho na ponta */
        int x = pyround(hx + b.u[0] * (L + 1)), y = pyround(hy + b.u[1] * (L + 1));
        fx_put_clean(cv, x, y - 1, (Rgb){255, 255, 255});
        fx_put_clean(cv, x + 1, y, (Rgb){199, 207, 221});
        return;
    }
    if (idx == 3) {   /* o rastro do saque ficou para cima: um risco fino por cima da lâmina */
        double nx = b.u[1], ny = -b.u[0];
        if (ny > 0) { nx = -nx; ny = -ny; }
        for (double t = L * 0.4; t <= L * 0.95; t += 0.5)
            fx_put_clean(cv, pyround(hx + b.u[0] * t + nx * 2), pyround(hy + b.u[1] * t + ny * 2), (Rgb){146, 161, 185});
        return;
    }
    /* 2: o arco do saque, de baixo (da bainha) até a ponta */
    double a0 = 62, a1 = ang * 180 / 3.14159265, R = L + 1;
    int nn = (int)(fabs(a1 - a0) * L * 0.03) + 6;
    for (int i = 0; i <= nn; i++) {
        double t = (double)i / nn, a = (a0 + (a1 - a0) * t) * 3.14159265 / 180;
        int wd = idx == 2 && t > 0.45 ? 2 : 1;
        for (int e = 0; e < wd; e++) {
            Rgb c = idx == 3 ? (t > 0.5 ? (Rgb){199, 207, 221} : (Rgb){146, 161, 185})
                             : e == 0 ? (t > 0.55 ? (Rgb){255, 255, 255} : t > 0.2 ? (Rgb){199, 207, 221} : (Rgb){146, 161, 185})
                                      : (Rgb){146, 161, 185};
            fx_put_clean(cv, pyround(hx + cos(a) * (R - e)), pyround(hy + sin(a) * (R - e)), c);
        }
    }
}

static void weapons(Canvas *cv, const Char *ch, const Ctx *ctx) {
    const char *anim = ctx->anim;
    int idx = ctx->idx;
    const Seg *s = cv->seg;
    const Weapon *w = &ch->arma;
    Rgb core = ch->lamina[0], edge = ch->lamina[1];
    double escala = w->escala > 0 ? w->escala : 1.0;
    /* parado, correndo, caindo: a arma descansa (a foice vira o cabo para cima) */
    bool rest = !strstr(anim, "ATTACK") && !strstr(anim, "ESPECIAL") && !strstr(anim, "DEFEND") && !strstr(anim, "THROW");
    const char *dbg = getenv("DBG_LAB");
    if (dbg) {
        char key[64];
        snprintf(key, sizeof key, "%s:%d", anim, idx);
        if (!strcmp(dbg, key))
            for (int y = 0; y < CH; y++) {
                char line[CW + 1];
                bool any = false;
                for (int x = 0; x < CW; x++) {
                    line[x] = ".TFhfsCDYHBSo"[s->lab[y][x]];
                    any |= s->lab[y][x] != NONE;
                }
                line[CW] = 0;
                if (any) fprintf(stderr, "%3d %s\n", y, line);
            }
    }
    memset(cv->wpx, 0, sizeof cv->wpx);
    g_nkama = g_ndag = 0;
    cv->pen = T_WEAPON;
    if (!strcmp(anim, "EMBAINHADO") || !strcmp(anim, "DESEMBAINHAR")) {
        iai(cv, ch, ctx);
        return;
    }
    if (ch->pack && (w->kind == W_LANCA || ch->golpe == GP_FLORETE)) drop_pack_hilt(cv);
    if ((ch->golpe == GP_LANCA || ch->golpe == GP_FLORETE) && (strstr(anim, "ATTACK") || strstr(anim, "ESPECIAL"))) {
        lance_attack(cv, ch, ctx);
        if (ch->elemento != EL_NONE) {
            cv->pen = T_FX;
            blade_fx(cv, ch, anim, idx);
        }
        return;
    }
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
        if (getenv("DBG_BLADES"))
            fprintf(stderr, "%s:%d blade %d n=%d hilt=(%.1f,%.1f) u=(%.2f,%.2f) near=%.1f far=%.1f loose=%d hand=%d\n", anim, idx,
                    bi, b->n, b->hilt[0], b->hilt[1], b->u[0], b->u[1], b->nearest, b->farthest, b->loose, b->hand);
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
                    /* wakizashi: a mesma espada, mais curta (contada de onde a lâmina aparece
                       quando a mão cobre o começo dela, como na guarda em pé) */
                    double off = b->nearest > 3 ? b->nearest - 1 : 0;
                    for (int i = 0; i < b->n; i++) {
                        if (b->dist[i] - off > KATANA * escala) erase_px(cv, b->x[i], b->y[i]);
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
                   e fina, do cabo à ponta, com o copo (a campânula) na mão. Num pack, fora
                   dos golpes a espada está na bainha: o que o separador leu como lâmina é
                   a manga branca da camisa, e fica como está. */
                if (ch->pack && !strstr(anim, "ATTACK") && !strstr(anim, "ESPECIAL") && !strstr(anim, "DEFEND") &&
                    !strstr(anim, "THROW"))
                    break;
                for (int i = 0; i < b->n; i++) erase_px(cv, b->x[i], b->y[i]);
                if (!at_hand) break;
                florete(cv, ch, b, fmax(full, KATANA * escala));
                break;
            }
            case W_ADAGA: case W_CURTA: {
                /* a lâmina curta é desenhada inteira: cabo, guarda e lâmina */
                double keep = w->comprimento > 0 ? w->comprimento : 7;
                for (int i = 0; i < b->n; i++) erase_px(cv, b->x[i], b->y[i]);
                if (ch->pack && ch->pack_par && w->reverso) {
                    /* no pack: o punho é onde a lâmina do pack aparece (a mão cobre o começo
                       dela na guarda em pé; no bloqueio a faísca esconde a mão) */
                    skip_pair = true;
                    if (b->loose && strcmp(anim, "DEFEND")) break;
                    double off = b->nearest > 3 ? b->nearest - 1 : 0;
                    if (off > 15) break;
                    double hx = b->hilt[0] + b->u[0] * off, hy = b->hilt[1] + b->u[1] * off;
                    Rgb c2 = rgb_set(w->cor_par) ? w->cor_par : edge;
                    bool rear = s->nblades >= 2 && bi != front_blade;
                    reverse_dagger(cv, ch, hx, hy, b->u, rear ? keep - 1 : keep, rear ? c2 : core, edge, rear);
                    /* só uma lâmina à vista: a outra adaga na outra mão, logo atrás */
                    if (w->par && s->nblades == 1)
                        reverse_dagger(cv, ch, hx - 3, hy + 2, b->u, keep - 1, c2, edge, true);
                    break;
                }
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
                for (int i = 0; i < b->n; i++) erase_px(cv, b->x[i], b->y[i]);
                skip_pair = true;
                /* num pack, fora dos golpes a lança descansa em pé na mão (desenhada abaixo) */
                if (!at_hand || (ch->pack && rest)) break;
                lance(cv, ch, b, KATANA * escala, ctx->idx);
                break;
            }
            case W_FOICE: {
                for (int i = 0; i < b->n; i++) erase_px(cv, b->x[i], b->y[i]);
                skip_pair = true;
                /* no bloqueio a faísca esconde a mão; no pack, a mão que cobre o começo da
                   lâmina: a foice sai de onde a lâmina aparece (como as garras) */
                if (b->loose && strcmp(anim, "DEFEND")) break;
                Blade m = *b;
                if (m.nearest > 3) {
                    if (!ch->pack_par || m.nearest > 16) break;
                    m.hilt[0] += m.u[0] * (m.nearest - 1);
                    m.hilt[1] += m.u[1] * (m.nearest - 1);
                    m.nearest = 1;
                }
                /* foice pequena (kama): quase tudo é cabo, e a lâmina curva fica na ponta */
                double len = w->comprimento > 0 ? w->comprimento : KATANA * escala * 0.8;
                Rgb c2 = rgb_set(w->cor_par) ? w->cor_par
                                             : (Rgb){(unsigned char)((core.r + edge.r) / 2), (unsigned char)((core.g + edge.g) / 2),
                                                     (unsigned char)((core.b + edge.b) / 2)};
                /* duas lâminas no pack: a foice da mão de trás, mais escura, atrás da da frente */
                bool rear = ch->pack_par && s->nblades >= 2 && bi != front_blade;
                kama(cv, &m, rear ? len - 1 : len, w, rear ? c2 : core, edge, rear, rest);
                /* a outra foice na mão esquerda (no pack, quando só uma lâmina aparece: a
                   mão de trás junto da da frente) */
                if (w->par && (!ch->pack_par || s->nblades == 1)) {
                    Blade o = ch->pack_par ? m : other_hand(&m, 0.8);
                    if (ch->pack_par) { o.hilt[0] -= 3; o.hilt[1] += 2; }
                    kama(cv, &o, len - 1, w, c2, edge, true, rest);
                }
                break;
            }
            case W_GARRAS: {
                for (int i = 0; i < b->n; i++) erase_px(cv, b->x[i], b->y[i]);
                /* no quadro do bloqueio a faísca esconde a mão, mas a lâmina em pé é a da guarda */
                if (b->loose && strcmp(anim, "DEFEND")) { skip_pair = true; break; }
                /* espada em pé (a guarda): a mão cobre o começo da lâmina e o cabo fica
                   estimado lá embaixo; as garras saem de onde a lâmina aparece */
                Blade m = *b;
                if (m.nearest > 3) {
                    if (m.nearest > 16) { skip_pair = true; break; }
                    m.hilt[0] += m.u[0] * (m.nearest - 1);
                    m.hilt[1] += m.u[1] * (m.nearest - 1);
                    m.nearest = 1;
                }
                double size = w->comprimento > 0 ? w->comprimento : 8;
                /* duas lâminas no pack: a garra da mão de trás é mais escura e fica atrás
                   da da frente (as duas apontando para o mesmo lado viravam um borrão) */
                if (ch->pack_par && s->nblades >= 2 && bi != front_blade) {
                    Rgb dim = {(unsigned char)((core.r + edge.r) / 2), (unsigned char)((core.g + edge.g) / 2),
                               (unsigned char)((core.b + edge.b) / 2)};
                    claws(cv, &m, size - 1, rgb_set(w->cor_par) ? w->cor_par : dim, edge, true);
                    skip_pair = true;
                    break;
                }
                claws(cv, &m, size, core, edge, false);
                /* garras também na mão esquerda (ou na de trás, quando só uma lâmina aparece) */
                if (w->par && (!ch->pack_par || s->nblades == 1)) {
                    Blade o = ch->pack_par ? m : other_hand(&m, 0.8);
                    if (ch->pack_par) { o.hilt[0] -= 3; o.hilt[1] += 2; }
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
    /* Duas espadas num pack em que só uma lâmina aparece na guarda (as duas mãos
       juntas no cabo): a outra sai das mesmas mãos, aberta para a frente, em V */
    bool fallen = !strcmp(anim, "DEATH");
    if (ch->pack && ch->pack_par && ((w->kind == W_KATANA && w->par_comprimento > 0) || w->kind == W_DUPLA) &&
        (!strcmp(anim, "DEFEND") || fallen)) {
        int nh = 0, only = -1;
        for (int bi = 0; bi < s->nblades; bi++) {
            const Blade *b = &s->blades[bi];
            if (b->farthest < 5) continue;
            nh++;
            only = bi;
        }
        if (nh == 1) {
            const Blade *b = &s->blades[only];
            Blade o = {0};
            double off = b->nearest > 3 ? b->nearest - 1 : 0;
            o.hilt[0] = b->hilt[0] + b->u[0] * off + 1;
            o.hilt[1] = b->hilt[1] + b->u[1] * off + 1;
            o.u[0] = 0.62;
            o.u[1] = -0.78;
            /* caído: as duas no chão, a curta junto da espada, um pouco abaixo */
            if (fallen) {
                o.u[0] = b->u[0];
                o.u[1] = b->u[1];
                o.hilt[0] = b->hilt[0] + b->u[0] * off + 2;
                o.hilt[1] = b->hilt[1] + b->u[1] * off + 2;
            }
            Rgb guard = rgb_set(w->guarda) ? w->guarda : ch->destaque[1];
            double len2 = w->par_comprimento > 0 ? w->par_comprimento - 2 : KATANA * escala - 3;
            draw_short_blade(cv, &o, 0, 0, len2, rgb_set(w->cor_par) ? w->cor_par : core, edge, guard, ch->cabo, true);
        }
    }
    /* e no golpe em que a espada do pack ainda está na bainha (a preparação) também */
    bool held = ch->pack && (w->kind == W_LANCA || (w->kind == W_FLORETE && ch->golpe == GP_FLORETE));
    if (held && w->kind == W_FLORETE && rest)
        for (int y = 0; y < CH; y++)   /* a manga branca lida como lâmina fica como está */
            for (int x = 0; x < CW; x++) cv->wpx[y][x] = false;
    if (held && strcmp(anim, "DEATH")) {
        bool drawn = false;
        for (int y = 0; y < CH && !drawn; y++)
            for (int x = 0; x < CW && !drawn; x++) drawn = cv->wpx[y][x];
        if (rest || !drawn) rest_lance(cv, ch, idx);
    }
    if (held && !strcmp(anim, "DEATH")) death_lance(cv, ch, idx);
    if (ch->pack && ch->pack_par && (w->kind == W_GARRAS || w->kind == W_FOICE || w->kind == W_ADAGA)) scrub_pack_blades(cv);
    if (ch->golpe == GP_GARRAS && (strstr(anim, "ATTACK") || strstr(anim, "ESPECIAL"))) claw_scratches(cv, ch, ctx);
    if (ch->golpe == GP_KAMA && (strstr(anim, "ATTACK") || strstr(anim, "ESPECIAL"))) kama_arcs(cv, ch, ctx);
    if (ch->golpe == GP_ADAGAS && (strstr(anim, "ATTACK") || strstr(anim, "ESPECIAL"))) dagger_cuts(cv, ch, ctx);
    if (ch->golpe == GP_RAIO && (strstr(anim, "ATTACK") || strstr(anim, "ESPECIAL"))) bolt_arcs(cv, ch, ctx);
    if (ch->golpe == GP_DUAS && (strstr(anim, "ATTACK") || strstr(anim, "ESPECIAL"))) {
        double hx, hy;
        if (front_blade >= 0) { hx = s->blades[front_blade].hilt[0]; hy = s->blades[front_blade].hilt[1]; }
        else if (!pack_fist(cv, &hx, &hy)) hx = -1;
        if (hx >= 0) crossed_arcs(cv, ch, ctx, hx, hy);
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
        case EL_GELO: {
            /* frio: um brilho azul-claro, flocos descendo devagar em volta e, na frente
               da boca, o bafo gelado saindo em nuvenzinhas que andam e somem */
            glow(cv, ctx, 0.09 * pw, ymid, (Rgb){214, 244, 255}, (Rgb){120, 190, 235}, false);
            particles(cv, ctx, "neve", (int)(4 * pw), 12, bx0 - 4, bx1 + 4, by0 - 6, ymid, 0.15, 0.9, 0.7,
                      (Rgb){255, 255, 255}, (Rgb){200, 236, 255}, (Rgb){130, 190, 235}, 1);
            int fx = -1, fy = by0 + 7;
            for (int x = bx1; x >= bx0 && fx < 0; x--)
                for (int y = by0; y <= by0 + 9 && y < CH; y++)
                    if (cv->tag[y][x] == T_BODY && cv->a[y][x].a) { fx = x; fy = y + 1; break; }
            if (fx >= 0 && fy < ymid) {
                int t = ctx->idx % 6;
                for (int i = 0; i < 3; i++) {
                    int x = fx + 2 + t + i, y = fy - (t + i) / 3;
                    if (t + i > 6) break;
                    Rgb c = t < 2 ? (Rgb){236, 250, 255} : t < 4 ? (Rgb){196, 230, 250} : (Rgb){150, 196, 230};
                    if (cv_ok(x, y) && cv->a[y][x].a == 0 && (i != 1 || t < 4)) cv_put(cv, x, y, c);
                }
            }
            break;
        }
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
   linhas na cintura. Tudo em pixel inteiro; os pés continuam no chão. A cabeça sem
   chapéu (as linhas até `head_y`) não alarga: a coluna repetida cairia no meio do
   rosto e o deixaria torto. */
static void resize_body(Canvas *cv, int cx, int wy, int dw, int dh, int head_y) {
    static Color a[CH][CW];
    static unsigned char t[CH][CW];
    if (dw && cx > 0 && cx < CW - 1) {
        memcpy(a, cv->a, sizeof a);
        memcpy(t, cv->tag, sizeof t);
        for (int y = 0; y < CH; y++) {
            bool head = y <= head_y;
            for (int x = head ? 0 : cx + 1; x < CW; x++) {
                int sx = x <= cx ? x : dw > 0 ? (x - dw > cx ? x - dw : cx) : x - dw;
                /* na altura da cabeça só a arma e os efeitos andam; o corpo fica */
                if (head) {
                    bool moved = sx < CW && t[y][sx] != T_BODY && a[y][sx].a;
                    if (moved) { cv->a[y][x] = a[y][sx]; cv->tag[y][x] = t[y][sx]; }
                    else if (t[y][x] == T_BODY) { cv->a[y][x] = a[y][x]; cv->tag[y][x] = T_BODY; }
                    else { cv->a[y][x] = (Color){0, 0, 0, 0}; cv->tag[y][x] = T_NONE; }
                } else if (sx < CW) { cv->a[y][x] = a[y][sx]; cv->tag[y][x] = t[y][sx]; }
                else { cv->a[y][x] = (Color){0, 0, 0, 0}; cv->tag[y][x] = T_NONE; }
            }
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
        case FX_CRISTAL: {
            /* cristais de gelo explodindo do ponto da estocada: seis pontas que crescem,
               brancas no meio e azul-claras na borda, e lascas subindo do chão */
            if (age > 3) break;
            int L = 3 + age * 2;
            for (int a = 0; a < 6; a++) {
                double ang = a * 3.14159 / 3 + 0.26;
                for (int i = 1; i <= L; i++) {
                    int x = tx + (int)floor(cos(ang) * i + 0.5), y = ty + (int)floor(sin(ang) * i + 0.5);
                    fx_px(cv, x, y, i < L - 1 ? (Rgb){246, 252, 255} : c1);
                    if (i == L / 2 && age < 3) {
                        fx_px(cv, x + (int)floor(cos(ang + 1.1) * 2 + 0.5), y + (int)floor(sin(ang + 1.1) * 2 + 0.5), c1);
                        fx_px(cv, x + (int)floor(cos(ang - 1.1) * 2 + 0.5), y + (int)floor(sin(ang - 1.1) * 2 + 0.5), c1);
                    }
                }
            }
            for (int k = 0; k < 4; k++) {
                int x = tx - 10 + (int)(hsh4("gelo", "fx", 0, k, 0) * 20), h = 2 + age + (int)(hsh4("gelo", "fx", 0, k, 1) * 3);
                for (int j = 0; j < h; j++) fx_px(cv, x, gy - j, j == h - 1 ? (Rgb){246, 252, 255} : c1);
                if (h > 3) fx_px(cv, x + 1, gy, c2);
            }
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
/* Postura da lua: o rastro do golpe vira uma lua crescente. O disco do tamanho do
 * rastro, menos um disco deslocado para o lado do corpo (a curva do corte abraça
 * quem golpeia): branco no meio, prata na borda de fora, azulado na de dentro. */
static void moon_slash(Canvas *cv, const Char *ch, const char *anim) {
    if (!strstr(anim, "ATTACK") && !strstr(anim, "ESPECIAL")) return;
    static Mask tr;
    int n = 0, x0 = CW, x1 = -1, y0 = CH, y1 = -1, nb = 0;
    double sx = 0, sy = 0, bxs = 0, bys = 0;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            Color c = cv->a[y][x];
            bool trail = false;
            if (c.a && cv->seg->lab[y][x] != BLADE)
                for (int i = 0; i < 3 && !trail; i++) trail = c.r == ch->rastro[i].r && c.g == ch->rastro[i].g && c.b == ch->rastro[i].b;
            trail = trail && (cv->seg->lab[y][x] == SMEAR || cv->tag[y][x] == T_WEAPON || cv->tag[y][x] == T_FX || ch->pack);
            tr[y][x] = trail;
            if (trail) {
                n++; sx += x; sy += y;
                if (x < x0) x0 = x;
                if (x > x1) x1 = x;
                if (y < y0) y0 = y;
                if (y > y1) y1 = y;
            } else if (c.a && cv->tag[y][x] == T_BODY) {
                nb++; bxs += x; bys += y;
            }
        }
    if (n < 14 || nb == 0) return;
    double cx = (x0 + x1) / 2.0, cy = (y0 + y1) / 2.0, R = fmax(x1 - x0, y1 - y0) / 2.0 + 1;
    if (R < 6) R = 6;
    if (R > 26) R = 26;
    double dx = bxs / nb - sx / n, dy = bys / nb - sy / n, dl = sqrt(dx * dx + dy * dy);
    if (dl < 1e-6) return;
    dx /= dl; dy /= dl;
    double ix = cx + dx * R * 0.55, iy = cy + dy * R * 0.55, ir = R * 0.9;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (tr[y][x]) erase_px(cv, x, y);
    cv->pen = T_WEAPON;
    for (int y = (int)(cy - R) - 1; y <= (int)(cy + R) + 1; y++)
        for (int x = (int)(cx - R) - 1; x <= (int)(cx + R) + 1; x++) {
            if (!cv_ok(x, y)) continue;
            double o = hypot(x - cx, y - cy), in = hypot(x - ix, y - iy);
            if (o > R || in < ir) continue;
            if (cv->a[y][x].a && cv->tag[y][x] == T_BODY) continue;   /* o corpo fica na frente */
            Rgb c = o > R - 1.3 ? ch->rastro[1] : in < ir + 1.3 ? ch->rastro[2] : ch->rastro[0];
            cv_put(cv, x, y, c);
        }
}

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
    /* duas foices, dois cortes de vento pequenos, um acima do outro (o de baixo um
       pouco atrás) */
    for (int j = 0; j < 2; j++) {
        int jx = cx - j * 3, jy = cy + (j ? 3 : -3), jr = (j ? 2 : 3) + age;
        for (int k = -12; k <= 12; k++) {
            double t = k * 0.11;
            for (int e = 0; e < (age < 2 && !j ? 2 : 1); e++) {
                double rr = jr - e;
                int x = jx + (int)floor(cos(t) * rr * 0.55 + 0.5), y = jy + (int)floor(sin(t) * rr + 0.5);
                if (x >= cellw || !cv_ok(x, y) || cv->a[y][x].a) continue;
                cv_put(cv, x, y, e ? ch->rastro[1] : abs(k) > 9 || j ? ch->rastro[2] : ch->rastro[0]);
            }
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

static const char *g_dbg_anim = "";
static int g_dbg_idx;
/* ----- Oboro sem o elmo ---------------------------------------------------- */
/* O elmo de oni do pack Demon (os chifres, o capacete com as placas da nuca e a
 * máscara) é sempre o mesmo desenho, só deslocado de quadro em quadro. Três moldes
 * o acham: de frente (linha 0 = a dos olhos amarelos, coluna 0 = 9 px antes do olho
 * de trás), de costas (só os chifres por cima) e caído de bruços no fim da DEATH.
 * Achado, o elmo inteiro sai e entra uma cabeça de gente: cabelo preto solto e o
 * rosto liso como o do Kojiro, emendado na barba grande que o pack já desenha. */
#define ONI_TOP 12
static const char *const ONI[] = {
    ".......A.....A....", "......A.......A...", ".....AA.......AA..", ".....AA.......AA..",
    "....BCABBDD...AC..", "...EECCADDFG.ACC..", "..EBBBCCAFHFGCC...", ".BBEBDDCCHFHGCD...",
    ".EEBDBDIJCHGCDD...", "KKBDBDJAAACCCAAD..", "EBAADJLLAKKCKACDC.", "BEDDCMALLLACLLDC..",
    "EDBDNOAKLPCAPLC...", "DBDLQCLAKCAKACL...", "ADDLLCCLLKACCLL...", "LCCLLCACRLLLLRLL..",
    "LLLLLSCARFKAFRL...", "LLLLLLSCAKAACLLL..", "CCTTLLSSCASSCLDLL.",
};
#define ONI_ROWS ((int)(sizeof ONI / sizeof ONI[0]))

static uint32_t oni_color(char k) {
    switch (k) {
        case 'A': return 0x891e2b; case 'B': return 0x00396d; case 'C': return 0x571c27; case 'D': return 0x0c2e44;
        case 'E': return 0x0069aa; case 'F': return 0xc7cfdd; case 'G': return 0x657392; case 'H': return 0x92a1b9;
        case 'I': return 0x090008; case 'J': return 0x050213; case 'K': return 0xc42430; case 'L': return 0x131313;
        case 'M': return 0x00020e; case 'N': return 0x02000d; case 'O': return 0x05020d; case 'P': return 0xffc825;
        case 'Q': return 0x01010d; case 'R': return 0xffffff; case 'S': return 0x272727; case 'T': return 0x1b1b1b;
        case 'W': return 0xf6ca9f; case 'X': return 0xe69c69; case 'V': return 0x8a4836; case 'Y': return 0x3d3d3d;
        case 'u': return 0xbf6f4a; case 'g': return 0x6a6660; case 'y': return 0x4e4a48; case 'o': return 0x3a3330;
        default: return 0;
    }
}

static bool orig_is(const Canvas *cv, int x, int y, char k) {
    if (!cv_ok(x, y)) return false;
    if (k != '.' && lab_at(cv, x, y) == SMEAR) return false;   /* o rastro vermelho da fúria não é elmo */
    Color o = cv->orig->p[y][x];
    if (k == '.') return o.a == 0;
    uint32_t c = oni_color(k);
    return o.a && o.r == (c >> 16 & 0xff) && o.g == (c >> 8 & 0xff) && o.b == (c & 0xff);
}

/* De costas, só os chifres e o alto do elmo. */
static const char *const CHIFRES[] = {
    "........A.........", ".......AA......A..", ".......A.......AA.", "......AA........A.",
    "......AA........AA", "......CA........AA", "......CCABEEEE..AC", ".......CEEBBBBEACC",
    ".......EBBBDDBBEC.",
};
/* Caído de bruços no fim da DEATH: a cabeça deitada, o alto do elmo para a direita. */
static const char *const DEITADO[] = {
    "LLLLL.L.LKB.............", "LLLLLLLLLKBE............", "LLELLLLLABEBE...........",
    "LLEAELLLADEBEK..........", "YLEAEALCDBDEBKEB........", "YSBCEAECDDBDABEBE.......",
    "SSBCBCELLLDDADBEBE......", "SSDCLLLLLQNCDBDBBEB.....", "TSLLLLSCCCOMJDBDBCCAA...",
    "TLLLSSCACLAALJDDCCAAAA..", "LLSSSCACLAKLLAICCAB...A.", "LSSSCARRLKLLAAJCADB.....",
    "SSSSAKFLKCLLKACHFDD.....", "LSSSSAKLAACAKCHFHFD.....", "RRSSSAALCKACCCGHFG......",
};

/* O que é elmo, placa, chifre ou máscara no pack (o resto é cabelo, barba, pele). */
static bool helm_px(Color o) {
    static const uint32_t H[] = {0x891e2b, 0x00396d, 0x571c27, 0x0c2e44, 0x0069aa, 0xc7cfdd, 0x657392, 0x92a1b9, 0x090008,
                                 0x050213, 0xc42430, 0x00020e, 0x02000d, 0x05020d, 0xffc825, 0x01010d, 0xffffff, 0x03001b};
    if (!o.a) return false;
    uint32_t v = (uint32_t)o.r << 16 | (uint32_t)o.g << 8 | o.b;
    for (size_t i = 0; i < sizeof H / sizeof H[0]; i++)
        if (H[i] == v) return true;
    return false;
}

/* Acha o molde no quadro original pelos vermelhos e pelos olhos das primeiras
 * `match` linhas; devolve o canto de cima à esquerda. */
static bool oni_find(const Canvas *cv, const char *const *oni, int rows, int match, int *bx, int *by) {
    int best = 0, total = 0, w = (int)strlen(oni[0]);
    for (int r = 0; r < match; r++)
        for (int c = 0; oni[r][c]; c++) total += in_set(oni[r][c], "ACKP");
    for (int y = 0; y + rows <= CH && best < total; y++)
        for (int x = 0; x + w <= CW && best < total; x++) {
            int n = 0;
            for (int r = 0; r < match; r++)
                for (int c = 0; oni[r][c]; c++)
                    if (in_set(oni[r][c], "ACKP") && orig_is(cv, x + c, y + r, oni[r][c])) n++;
            if (n > best) { best = n; *bx = x; *by = y; }
        }
    return best * 100 >= total * 85;
}

/* De costas, o molde dos chifres só vale com o azul do alto do elmo embaixo deles (o
   rastro vermelho da fúria também tem os vermelhos dos chifres). */
static bool chifres_find(const Canvas *cv, int *bx, int *by) {
    int rows = (int)(sizeof CHIFRES / sizeof CHIFRES[0]);
    if (!oni_find(cv, CHIFRES, rows, 9, bx, by)) return false;
    int n = 0, total = 0;
    for (int r = 0; r < rows; r++)
        for (int c = 0; CHIFRES[r][c]; c++)
            if (in_set(CHIFRES[r][c], "BED")) {
                total++;
                n += orig_is(cv, *bx + c, *by + r, CHIFRES[r][c]);
            }
    return n * 100 >= total * 60;
}

/* Pixel de uma lâmina que sai da mão (o enfeite dourado do elmo o separador também
   lê como lâmina, mas solta, longe das mãos). */
static bool hand_blade_px(const Canvas *cv, int x, int y) {
    if (lab_at(cv, x, y) != BLADE) return false;
    const Seg *s = cv->seg;
    for (int i = 0; i < s->nblades; i++) {
        if (!blade_at_hand(&s->blades[i])) continue;
        for (int k = 0; k < s->blades[i].n; k++)
            if (s->blades[i].x[k] == x && s->blades[i].y[k] == y) return true;
    }
    return false;
}

/* Tira o elmo numa caixa (em relação a um ponto) e troca o que era máscara por barba. */
static void clear_helm(Canvas *cv, int ox, int oy, int c0, int c1, int r0, int r1) {
    for (int r = r0; r <= r1; r++)
        for (int c = c0; c <= c1; c++) {
            int x = ox + c, y = oy + r;
            if (cv_ok(x, y) && helm_px(cv->orig->p[y][x]) && !hand_blade_px(cv, x, y) && lab_at(cv, x, y) != SMEAR)
                cv_clear(cv, x, y);   /* a espada na frente do rosto (a guarda) fica */
        }
}

static void paint_px(Canvas *cv, int x, int y, char k) {
    if (k == '.' || !cv_ok(x, y)) return;
    if (k == '-') { cv_clear(cv, x, y); return; }
    uint32_t v = oni_color(k);
    cv_put(cv, x, y, (Rgb){(unsigned char)(v >> 16), (unsigned char)(v >> 8), (unsigned char)v});
}


/* A cabeça do Oboro sem o elmo, de perfil (virada para a direita), construída como
 * a do Hanzo, mas moço: cabelo preto curto rente ao crânio (com os fios em cinza
 * escuro), a nuca e a orelha à mostra, a testa e o nariz na luz, a sobrancelha
 * pesada sobre o olho firme, e a barba preta curta e cheia, do queixo às
 * costeletas, com o bigode. Coluna 0 = olho de trás do elmo menos 9, a primeira
 * linha é a -6 (a dos olhos é a 0). */
#define OBORO_TOPO 6
static const char *const OBORO_CABECA[] = {
    "........LLLLL.......",
    "......LLSYSYSLL.....",
    ".....LSYSSYSSYSL....",
    "....LSSSYSSSSYSLL...",
    "....LSYSSSSSSSLWWL..",
    "....LSSSSXuSLLLWWX..",
    "....LSSSXVuXXRLXWX..",
    ".....LSSuVXXXXXXWWX.",
    ".....LVuXXXXXXXLLu..",
    "......VuSXXSYSSLLL..",
    "......LSSSSYSSSYSL..",
    ".......LSSYSSSYSSL..",
    "........LLSSSSSLL...",
    ".........VuuuuV.....",
};
#define OBORO_LINHAS ((int)(sizeof OBORO_CABECA / sizeof OBORO_CABECA[0]))

/* O cabelo comprido e a barba longa do pack (os pretos e cinzas escuros) presos à
   cabeça: a mancha que sai da caixa da cabeça, sem descer além de `r1`. */
static bool oboro_dark(Color o) {
    uint32_t v = (uint32_t)o.r << 16 | (uint32_t)o.g << 8 | o.b;
    return o.a && (v == 0x131313 || v == 0x272727 || v == 0x1b1b1b || v == 0x3d3d3d);
}

/* Onde o cabelo e a barba saíram e o corpo fica por trás (entre o primeiro e o
   último pixel que sobrou na linha), a armadura do pack em faixas: azul claro, azul,
   azul, azul escuro, com o cordão vermelho nas escuras e a borda escura. */
static void armor_fill(Canvas *cv, bool (*gone)[CW], int ex, int ey, int r0, int r1, int c0, int c1) {
    for (int r = r0; r <= r1; r++) {
        int y = ey + r, a = -1, b = -1;
        if (y < 0 || y >= CH) continue;
        for (int x = ex + c0; x <= ex + c1; x++)
            if (x >= 0 && x < CW && cv->a[y][x].a) {
                if (a < 0) a = x;
                b = x;
            }
        if (a < 0 || b <= a) continue;
        for (int x = a; x <= b; x++) {
            if (!gone[y][x] || cv->a[y][x].a) continue;
            int m = ((r % 4) + 4) % 4;
            char k = m == 0 ? 'E' : m == 3 ? 'D' : 'B';
            if (m == 3 && ((x - ex) % 4 + 4) % 4 == 1) k = 'A';
            if (x == a || x == b) k = 'D';
            paint_px(cv, x, y, k);
            cv->tag[y][x] = T_BODY;
        }
    }
}

/* Troca o elmo (e o cabelo comprido e a barba longa) pela cabeça nova, com a
   armadura por baixo do que era barba e cabelo. (ex, ey): a coluna 0 e a linha dos
   olhos do molde de frente. `r1`: até onde descem o cabelo e a barba do pack. */
static void oboro_head(Canvas *cv, int ex, int ey, int r1) {
    static bool gone[CH][CW];
    static short st[CW * CH][2];
    memset(gone, 0, sizeof gone);
    clear_helm(cv, ex, ey, -6, 19, -13, 2);
    for (int r = -13; r <= 2; r++)
        for (int c = -6; c <= 19; c++) {
            int x = ex + c, y = ey + r;
            if (cv_ok(x, y) && helm_px(cv->orig->p[y][x]) && !cv->a[y][x].a) gone[y][x] = true;
        }
    /* a mancha escura presa à cabeça: começa no que está na caixa da cabeça */
    int sp = 0;
    for (int r = -13; r <= 4; r++)
        for (int c = -8; c <= 20; c++) {
            int x = ex + c, y = ey + r;
            if (cv_ok(x, y) && oboro_dark(cv->orig->p[y][x]) && !gone[y][x] && !hand_blade_px(cv, x, y)) {
                gone[y][x] = true;
                st[sp][0] = (short)x;
                st[sp++][1] = (short)y;
            }
        }
    while (sp > 0) {
        sp--;
        int cx = st[sp][0], cy = st[sp][1];
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                int x = cx + dx, y = cy + dy;
                if (!cv_ok(x, y) || gone[y][x] || y > ey + r1 || x < ex - 16 || x > ex + 26) continue;
                if (!oboro_dark(cv->orig->p[y][x]) || hand_blade_px(cv, x, y)) continue;
                gone[y][x] = true;
                st[sp][0] = (short)x;
                st[sp++][1] = (short)y;
            }
    }
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (gone[y][x] && cv->a[y][x].a && oboro_dark(cv->orig->p[y][x])) cv_clear(cv, x, y);
    armor_fill(cv, gone, ex, ey, 3, r1, -16, 26);
    for (int r = 0; r < OBORO_LINHAS; r++)
        for (int c = 0; OBORO_CABECA[r][c]; c++) {
            int x = ex + c, y = ey + r - OBORO_TOPO;
            if (OBORO_CABECA[r][c] == '.' || !cv_ok(x, y)) continue;
            if (hand_blade_px(cv, x, y) || lab_at(cv, x, y) == SMEAR) continue;   /* a espada passa na frente */
            paint_px(cv, x, y, OBORO_CABECA[r][c]);
            cv->tag[y][x] = T_BODY;
        }
}

/* Caído de bruços (o fim da DEATH): a mesma cabeça deitada, girada um quarto de volta
   (o alto da cabeça para a direita, o rosto para o chão, a barba para o corpo), de
   olho fechado. O cabelo comprido que se espalhava em volta sai. (bx, by): o canto
   do molde DEITADO. */
static void oboro_head_lying(Canvas *cv, int bx, int by) {
    static bool gone[CH][CW];
    static short st[CW * CH][2];
    memset(gone, 0, sizeof gone);
    clear_helm(cv, bx, by, 3, 24, 0, 14);
    int sp = 0;
    for (int r = -2; r <= 15; r++)
        for (int c = -2; c <= 26; c++) {
            int x = bx + c, y = by + r;
            if (cv_ok(x, y) && oboro_dark(cv->orig->p[y][x]) && !gone[y][x]) {
                gone[y][x] = true;
                st[sp][0] = (short)x;
                st[sp++][1] = (short)y;
            }
        }
    while (sp > 0) {
        sp--;
        int cx = st[sp][0], cy = st[sp][1];
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                int x = cx + dx, y = cy + dy;
                if (!cv_ok(x, y) || gone[y][x] || x < bx - 10 || y < by - 6) continue;
                if (!oboro_dark(cv->orig->p[y][x])) continue;
                gone[y][x] = true;
                st[sp][0] = (short)x;
                st[sp++][1] = (short)y;
            }
    }
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (gone[y][x] && cv->a[y][x].a && oboro_dark(cv->orig->p[y][x])) cv_clear(cv, x, y);
    int X0 = bx + 9, Y0 = by - 5;
    for (int r = 0; r < OBORO_LINHAS; r++)
        for (int c = 0; OBORO_CABECA[r][c]; c++) {
            char k = OBORO_CABECA[r][c];
            if (k == '.') continue;
            if (k == 'R') k = 'u';   /* o olho fechado */
            int x = X0 - (r - OBORO_TOPO), y = Y0 + c;
            if (!cv_ok(x, y)) continue;
            paint_px(cv, x, y, k);
            cv->tag[y][x] = T_BODY;
        }
}

/* Olhos acesos (o fim da DEATH) ou os vermelhos do elmo trocados pelo fogo (a fúria):
   o elmo é o mesmo, meio px fora do lugar; acha pelos vermelhos e azuis só dos
   chifres e do alto do elmo, com menos exigência. */
static bool oni_find_loose(const Canvas *cv, int *bx, int *by) {
    int best = 0, total = 0, w = (int)strlen(ONI[0]);
    for (int r = 0; r < ONI_TOP; r++)
        for (int c = 0; ONI[r][c]; c++) total += in_set(ONI[r][c], "ABCDEK");
    for (int y = 0; y + ONI_ROWS <= CH; y++)
        for (int x = 0; x + w <= CW; x++) {
            int n = 0;
            for (int r = 0; r < ONI_TOP; r++)
                for (int c = 0; ONI[r][c]; c++)
                    if (in_set(ONI[r][c], "ABCDEK") && orig_is(cv, x + c, y + r, ONI[r][c])) n++;
            if (n > best) { best = n; *bx = x; *by = y; }
        }
    return best * 100 >= total * 50;
}

/* Oboro de máscara, de costas (o giro do ATTACK_1, o começo do ATTACK_2): o elmo vira
 * de perfil por cima do ombro, com a máscara de oni à mostra, no lugar do elmo visto
 * de trás. O cabelo comprido fica. */
static void mask_turn(Canvas *cv) {
    int bx, by;
    if (oni_find(cv, ONI, ONI_ROWS, ONI_TOP + 3, &bx, &by)) return;
    if (!chifres_find(cv, &bx, &by)) return;
    clear_helm(cv, bx, by, -3, 20, 0, 11);
    for (int r = 0; r < ONI_ROWS; r++)
        for (int c = 0; ONI[r][c]; c++) {
            int x = bx + 1 + c, y = by + r;
            if (ONI[r][c] == '.' || !cv_ok(x, y)) continue;
            if (hand_blade_px(cv, x, y) || lab_at(cv, x, y) == SMEAR) continue;
            paint_px(cv, x, y, ONI[r][c]);
            cv->tag[y][x] = T_BODY;
        }
}

/* De frente, de costas ou caído: o primeiro molde que achar vale. De costas ele vira
 * a cabeça para a frente (o rosto por cima do ombro) e o cabelo comprido sai, com a
 * armadura das costas por baixo. */
static void unmask(Canvas *cv) {
    int bx, by;
    if (oni_find(cv, ONI, ONI_ROWS, ONI_TOP + 3, &bx, &by)) {
        oboro_head(cv, bx, by + ONI_TOP, 16);   /* coluna 0 = olho de trás menos 9, linha 0 = a dos olhos */
        return;
    }
    if (chifres_find(cv, &bx, &by)) {
        if (getenv("DBG_UNMASK")) fprintf(stderr, "unmask: costas %s:%d\n", g_dbg_anim, g_dbg_idx);
        clear_helm(cv, bx, by, -3, 20, 0, 11);
        oboro_head(cv, bx + 1, by + ONI_TOP, 18);
        return;
    }
    if (oni_find(cv, DEITADO, (int)(sizeof DEITADO / sizeof DEITADO[0]), 15, &bx, &by)) {
        if (getenv("DBG_UNMASK")) fprintf(stderr, "unmask: deitado %s:%d\n", g_dbg_anim, g_dbg_idx);
        oboro_head_lying(cv, bx, by);
        return;
    }
    if (oni_find_loose(cv, &bx, &by)) {
        if (getenv("DBG_UNMASK")) fprintf(stderr, "unmask: solto %s:%d\n", g_dbg_anim, g_dbg_idx);
        oboro_head(cv, bx, by + ONI_TOP, 16);
        return;
    }
    if (getenv("DBG_UNMASK")) fprintf(stderr, "unmask: nada em %s:%d\n", g_dbg_anim, g_dbg_idx);
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
            /* a bainha inteira sai; dos pedaços pequenos, os vermelhos (a ponta da bainha
               atrás da mão) também: as botas e os punhos são do marrom escuro */
            bool red = false;
            for (int i = 0; i < n && !red; i++) red = cv->a[comp[i][1]][comp[i][0]].r == 0x57 && cv->a[comp[i][1]][comp[i][0]].g == 0x1c;
            if (x1 - x0 >= 6 || red)
                for (int i = 0; i < n; i++) erase_px(cv, comp[i][0], comp[i][1]);
        }
}

/* Troca exata de cor no que sobrou do pack (a cor original decide). */
static void swap_colors(Canvas *cv, const Char *ch) {
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            Color o = cv->orig->p[y][x];
            if (!cv->a[y][x].a || !o.a) continue;
            for (int i = 0; i < 24 && rgb_set(ch->troca[i].de); i++)
                if (ch->troca[i].de.r == o.r && ch->troca[i].de.g == o.g && ch->troca[i].de.b == o.b) {
                    set_rgb(cv, x, y, ch->troca[i].para);
                    break;
                }
        }
}

/* Hanzo de máscara: acha os olhos (a faixa escura no alto da cabeça) e a frente do
 * rosto, e põe ali o mesmo elmo de oni do Oboro, copiado do molde do pack Demon
 * (chifres, capacete e máscara), do tamanho que é: maior que a cabeça dele. O
 * cabelo e a barba do Oboro que o molde traz ficam de fora; a barba branca do
 * Hanzo aparece por baixo. */
static void oni_face(Canvas *cv) {
    int top = -1;
    for (int y = 0; y < CH && top < 0; y++)
        for (int x = 0; x < CW; x++)
            if (cv->orig->p[y][x].a) { top = y; break; }
    if (top < 0) return;
    int ey = -1, fx = -1;
    for (int y = top; y < top + 10 && y < CH && ey < 0; y++)
        for (int x = 0; x < CW; x++) {
            Color o = cv->orig->p[y][x];
            if (o.a && ((o.r == 0x13 && o.g == 0x13 && o.b == 0x13) || (o.r == 0xbf && o.g == 0x6f && o.b == 0x4a))) { ey = y; break; }
        }
    if (ey < 0) return;   /* o clarão do golpe: sem rosto */
    for (int y = ey - 2; y <= ey + 2; y++)
        for (int x = 0; x < CW; x++) {
            if (!cv_ok(x, y)) continue;
            Color o = cv->orig->p[y][x];
            bool face = o.a && ((o.r == 0xe6 && o.g == 0x9c && o.b == 0x69) || (o.r == 0xf6 && o.g == 0xca && o.b == 0x9f) ||
                                (o.r == 0xbf && o.g == 0x6f && o.b == 0x4a) || (o.r == 0x13 && o.g == 0x13 && o.b == 0x13));
            if (face && x > fx) fx = x;
        }
    if (fx < 0) return;
    /* a frente da máscara (coluna 14 do molde, na linha dos olhos) cobre a frente do rosto.
       A cabeça do Hanzo é menor que a do Oboro: o domo do elmo perde duas linhas (a 5 e
       a 7), para o elmo assentar no crânio em vez de ficar alto, flutuando por cima */
    int ox = fx + 1 - 14, oy = ey - (ONI_TOP - 2);
    for (int r = 0, rr = 0; r < ONI_TOP + 6; r++) {
        if (r == 5 || r == 7) continue;
        for (int c = 0; ONI[r][c]; c++) {
            char k = ONI[r][c];
            int er = r - ONI_TOP;
            if (k == '.' || k == 'S' || k == 'T') continue;
            if (k == 'L' && !(er >= -3 && er <= 5 && c >= 5 && c <= 15)) continue;   /* o cabelo do Oboro */
            if (c < 4) continue;   /* as placas da nuca e a aba do elmo de trás (soltas, no Hanzo) */
            paint_px(cv, ox + c, oy + rr, k);
        }
        rr++;
    }
    /* a fita vermelha do cabelo do Hanzo ficava solta atrás do elmo */
    for (int y = 0; y < ey + 2 && y < CH; y++)
        for (int x = 0; x < CW; x++) {
            Color c = cv->a[y][x], o = cv->orig->p[y][x];
            if (c.a && c.r == o.r && c.g == o.g && c.b == o.b && (orig_hex(cv, x, y, 0x571c27) || orig_hex(cv, x, y, 0x391f21)))
                cv_clear(cv, x, y);
        }
}

/* O rosto do Hanzo, velho e sábio (o pack desenha os olhos como uma faixa preta, que
 * parecia óculos escuros): a testa com uma ruga, as sobrancelhas brancas grossas e
 * caídas nas pontas, os olhos semicerrados (só a linha da pálpebra), os pés de
 * galinha, o nariz na luz e o bigode branco emendando na barba. A barba longa ganha
 * fios cinza. Coluna 0 = a frente do rosto na linha dos olhos, a primeira linha é a
 * -2 (a dos olhos é a 0). */
static const char *const HANZO_ROSTO[] = {
    "...impii..",
    "..deeieed.",
    "..dpiipd..",
    "..pimmip..",
    "...eeeee..",
};
static void hanzo_face(Canvas *cv) {
    int top = -1;
    for (int y = 0; y < CH && top < 0; y++)
        for (int x = 0; x < CW; x++)
            if (cv->orig->p[y][x].a) { top = y; break; }
    if (top < 0) return;
    int ey = -1, fx = -1;
    for (int y = top; y < top + 10 && y < CH && ey < 0; y++)
        for (int x = 0; x < CW; x++)
            if (orig_hex(cv, x, y, 0x131313) || orig_hex(cv, x, y, 0xbf6f4a)) { ey = y; break; }
    if (ey < 0) return;   /* o clarão do golpe: sem rosto */
    for (int y = ey - 2; y <= ey + 2; y++)
        for (int x = 0; x < CW; x++)
            if (cv_ok(x, y) && (orig_hex(cv, x, y, 0xe69c69) || orig_hex(cv, x, y, 0xf6ca9f) || orig_hex(cv, x, y, 0xbf6f4a) ||
                                orig_hex(cv, x, y, 0x131313)) && x > fx)
                fx = x;
    if (fx < 0) return;
    for (int r = 0; r < 5; r++)
        for (int c = 0; HANZO_ROSTO[r][c]; c++) {
            char k = HANZO_ROSTO[r][c];
            int x = fx - 6 + c, y = ey - 2 + r;
            if (k == '.' || !cv_ok(x, y)) continue;
            uint32_t v = k == 'i' ? 0xe69c69 : k == 'm' ? 0xf6ca9f : k == 'p' ? 0xbf6f4a : k == 'w' ? 0x8a5040
                       : k == 'e' ? 0xffffff : 0xd8d8d4;
            cv_put(cv, x, y, (Rgb){(unsigned char)(v >> 16), (unsigned char)(v >> 8), (unsigned char)v});
        }
    /* os fios da barba: cinza claro em diagonais soltas no branco */
    for (int y = ey + 3; y <= ey + 14; y++)
        for (int x = fx - 9; x <= fx + 2; x++)
            if (cv_ok(x, y) && orig_hex(cv, x, y, 0xffffff) && ((x * 2 + y) % 5 == 0))
                cv_put(cv, x, y, (Rgb){0xc8, 0xc8, 0xc8});
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
        swap_colors(cv, ch);
        if (ch->mascara_oni) oni_face(cv);
        else hanzo_face(cv);
        return;
    }
    recolor(cv, ch, anim);
    g_dbg_anim = anim;
    g_dbg_idx = idx;
    if (ch->sem_mascara) unmask(cv);
    else if (ch->ecos) mask_turn(cv);
    accessories(cv, ch, idx);
    if (ch->sem_pano) s5_face(cv, ch);
    if (ch->cab_frente) s5_hair(cv, ch, idx, anim);
    if (ch->mechas) s5_mechas(cv, ch);
    if (ch->topete) s5_topete(cv, ch);
    if (ch->kasa) s5_kasa(cv, anim);
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
    if (ch->sem_rastro_pack && (strstr(anim, "ATTACK") || strstr(anim, "ESPECIAL"))) desmear(cv, ch);
    if (ch->sem_arma) {
        if (ch->pack && (ch->arma.kind == W_LANCA || ch->golpe == GP_FLORETE)) drop_pack_hilt(cv);
        no_weapon(cv, ch);
    } else {
        weapons(cv, ch, ctx);
        cv->pen = T_FX;
        paint_smear(cv, ch, anim, idx);
        smear_style(cv, ch);
        aura(cv, ch, ctx);
        if (ch->elemento == EL_VENTO) wind_slash(cv, ch, ctx);
        if (ch->elemento == EL_LUA) moon_slash(cv, ch, anim);
    }
    for (int i = 0; i < seg->nerase; i++)
        for (int y = seg->erase[i][1] < 0 ? 0 : seg->erase[i][1]; y <= seg->erase[i][3] && y < CH; y++)
            for (int x = seg->erase[i][0] < 0 ? 0 : seg->erase[i][0]; x <= seg->erase[i][2] && x < CW; x++) {
                cv->a[y][x] = (Color){0, 0, 0, 0};
                cv->tag[y][x] = T_NONE;
            }
    /* o corpo do Samurai #3 ganha largura, altura e passo; os packs próprios já vêm animados */
    if (!ch->pack) {
        resize_body(cv, seg->ox + 8, seg->oy + 19, ch->largura, ch->altura, seg->has_hat && !ch->chapeu ? seg->oy + 10 : -1);
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

/* A prancha sai 16 px mais larga que o quadro do pack (até a largura da tela de
 * desenho): o rastro e os efeitos que passam da borda da frente não são cortados.
 * A âncora dos pés conta da esquerda, então não muda. */
#define OUT_W(sc) ((sc)->cw + 16 < CW ? (sc)->cw + 16 : CW)

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
/* Um quadro do Hanzo em seiza a partir do quadro em pé já pronto (ver SENTADO): o
   tronco vai até as mãos (que pendem ao lado do corpo e, sentado, ficam pousadas nas
   coxas) e desce até elas ficarem sobre as coxas; as coxas saem dos quadris para a
   frente até o joelho redondo, a luz por cima, a sombra embaixo e atrás, uma prega
   do colo ao joelho, e as solas escuras dos pés dobrados por baixo, atrás. */
static void seated_frame(const Frame *in, Frame *out, const Char *ch) {
    memset(out, 0, sizeof *out);
    int top = -1, bot = -1;
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            if (in->p[y][x].a) {
                if (top < 0) top = y;
                bot = y;
            }
    if (top < 0) return;
    /* as mãos: a pele mais baixa do tronco */
    int hip = top + (bot - top) * 62 / 100;
    for (int y = top + 15; y < bot; y++)
        for (int x = 0; x < CW; x++) {
            Color c = in->p[y][x];
            for (int k = 0; k < 3; k++)
                if (c.a && c.r == ch->pele[k].r && c.g == ch->pele[k].g && c.b == ch->pele[k].b && y > hip) hip = y;
            if (c.a && ((c.r == 0xe6 && c.g == 0x9c && c.b == 0x69) || (c.r == 0xf6 && c.g == 0xca && c.b == 0x9f) ||
                        (c.r == 0xbf && c.g == 0x6f && c.b == 0x4a)) && y > hip)
                hip = y;
        }
    int legs = 7, shift = bot - legs - hip;
    if (shift < 0) shift = 0;
    for (int y = top; y <= hip; y++)
        for (int x = 0; x < CW; x++)
            if (y + shift < CH) out->p[y + shift][x] = in->p[y][x];
    int xb = CW, front = -1;
    for (int x = 0; x < CW; x++)
        if (in->p[hip - 2][x].a) {
            if (x < xb) xb = x;
            if (x > front) front = x;
        }
    if (front < 0) return;
    /* as pernas dobradas, desenhadas: as costas arredondadas sobre os calcanhares, a coxa
       descendo devagar até o joelho redondo na frente, a luz por cima e a sombra
       embaixo e atrás, as solas por baixo. Coluna 0 = uma antes das costas, a última
       linha é o chão. */
    static const char *const PERNAS[] = {
        "..kkkkkk..........",
        ".akkkkcccwww......",
        "aakkkkccccwwwww...",
        "aakkkkkcccccwwwc..",
        "gaakkkkkcccccccwc.",
        "gaaakkkkkkkcccccka",
        "ggaaaaaakkkkkkkkaa",
        "gggaaaaaaaaaaaaaa.",
    };
    int rows = (int)(sizeof PERNAS / sizeof PERNAS[0]);
    int span = (int)strlen(PERNAS[0]), x0 = xb - 1;
    if (front + 7 - x0 > span) x0 = front + 7 - span;   /* o joelho uns 6 px na frente da barriga */
    for (int r = 0; r < rows; r++)
        for (int c = 0; PERNAS[r][c]; c++) {
            char k = PERNAS[r][c];
            int x = x0 + c, y = bot - (rows - 1) + r;
            if (k == '.' || x < 0 || x >= CW || y < 0 || y >= CH) continue;
            Rgb v = k == 'w' ? ch->camisa[0] : k == 'c' ? ch->camisa[1] : k == 'k' ? ch->camisa[2] : k == 'a' ? ch->camisa[3]
                                                                                                            : (Rgb){74, 70, 68};
            out->p[y][x] = (Color){v.r, v.g, v.b, 255};
        }
    /* entre o tronco e as pernas não pode sobrar buraco */
    for (int y = hip + shift + 1; y < bot - (rows - 1); y++)
        for (int x = xb; x <= front; x++)
            if (!out->p[y][x].a) out->p[y][x] = (Color){ch->camisa[2].r, ch->camisa[2].g, ch->camisa[2].b, 255};
}

static bool skip_strip(const Char *ch, const char *name) {
    /* o grito do pack mostra a máscara acendendo: sem ela, o jogo usa o GRITO montado */
    if (ch->sem_mascara && !strcmp(name, "SHOUT")) return true;
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

/* O quadro j da prancha `st`: o dela mesma ou, se o personagem remonta essa
   prancha, o quadro indicado da outra. */
static void source_frame(const Source *sc, const Char *ch, const Strip *st, int j, const Frame **f, const Seg **sg) {
    for (int r = 0; r < 3 && ch->remonta[r].anim; r++) {
        if (strcmp(ch->remonta[r].anim, st->name)) continue;
        int jj = j < 12 ? j : 11;
        while (jj > 0 && !ch->remonta[r].f[jj].de) jj--;   /* lista mais curta: repete o último */
        const Strip *de = ch->remonta[r].f[jj].de ? source_strip(sc, ch->remonta[r].f[jj].de) : NULL;
        if (!de) break;
        int q = ch->remonta[r].f[jj].q;
        if (q < 0 || q >= de->nframes) q = de->nframes - 1;
        *f = &de->frames[q];
        *sg = &de->segs[q];
        return;
    }
    *f = &st->frames[j];
    *sg = &st->segs[j];
}

static Source *SOURCES[MAX_PACKS + 1];
static int NSOURCES;

/* O fogo da lâmina na fúria (Demon): o pack desenha uma nuvem de pontos vermelhos
 * espalhados, e em cima da armadura, também vermelha, o fogo some no meio do corpo.
 * O fogo é o que muda do quadro normal para o da fúria nas cores da máscara. Saem os
 * pontos soltos (manchas com menos de 6 px), sai o fogo em cima do corpo longe da
 * lâmina, e o que fica ganha uma rampa pela distância até a lâmina: laranja no fio,
 * vermelho vivo perto, vermelho escuro longe e a borda mais escura. Nos golpes (o rastro
 * vermelho tem outro desenho, e às vezes outro número de quadros) só saem as manchas
 * soltas no ar, de até 23 px (o arco do rastro é bem maior). */
static bool fury_red(Color c) {
    uint32_t v = (uint32_t)c.r << 16 | (uint32_t)c.g << 8 | c.b;
    return c.a && (v == 0x891e2b || v == 0x571c27 || v == 0xc42430);
}
static bool fury_blade(Color c) {
    uint32_t v = (uint32_t)c.r << 16 | (uint32_t)c.g << 8 | c.b;
    return c.a && (v == 0xc7cfdd || v == 0xffffff || v == 0x92a1b9);
}
static bool same_px(Color a, Color b) { return a.a == b.a && (!a.a || (a.r == b.r && a.g == b.g && a.b == b.b)); }

static void clean_fury(Source *s) {
    static const char *const which[] = {"IDLE", "RUN", "HURT", "ATTACK_1", "ATTACK_2", "ATTACK_3", "STRONG_ATTACK"};
    static bool fire[CH][CW], seen[CH][CW];
    static short dist[CH][CW], stack[CW * CH][2], comp[CW * CH][2];
    for (int w = 0; w < 7; w++) {
        bool strike = w >= 3;
        const Strip *base = NULL;
        Strip *fu = NULL;
        char name[64];
        snprintf(name, sizeof name, "%s_FURIA", which[w]);
        for (int i = 0; i < s->ns; i++) {
            if (!strcmp(s->strips[i].name, which[w])) base = &s->strips[i];
            if (!strcmp(s->strips[i].name, name)) fu = &s->strips[i];
        }
        if (!base || !fu) continue;
        int W = fu->cw, H = fu->ch, nf = fu->nframes < base->nframes ? fu->nframes : base->nframes;
        for (int k = 0; k < nf; k++) {
            Color (*p)[CW] = fu->frames[k].p;
            Color (*b)[CW] = base->frames[k].p;
            for (int y = 0; y < H; y++)
                for (int x = 0; x < W; x++) fire[y][x] = fury_red(p[y][x]) && !same_px(p[y][x], b[y][x]);
            /* os pontos soltos voltam a ser o que o quadro normal tem ali */
            memset(seen, 0, sizeof seen);
            for (int y = 0; y < H; y++)
                for (int x = 0; x < W; x++) {
                    if (!fire[y][x] || seen[y][x]) continue;
                    int sp = 0, n = 0;
                    stack[sp][0] = (short)x; stack[sp][1] = (short)y; sp++;
                    seen[y][x] = true;
                    while (sp) {
                        sp--;
                        int cx = stack[sp][0], cy = stack[sp][1];
                        comp[n][0] = (short)cx; comp[n][1] = (short)cy; n++;
                        for (int dy = -1; dy <= 1; dy++)
                            for (int dx = -1; dx <= 1; dx++) {
                                int nx = cx + dx, ny = cy + dy;
                                if (nx < 0 || ny < 0 || nx >= W || ny >= H || !fire[ny][nx] || seen[ny][nx]) continue;
                                seen[ny][nx] = true;
                                stack[sp][0] = (short)nx; stack[sp][1] = (short)ny; sp++;
                            }
                    }
                    if (n < (strike ? 24 : 6))
                        for (int i = 0; i < n; i++) {
                            int cx = comp[i][0], cy = comp[i][1];
                            fire[cy][cx] = false;
                            if (!strike) { p[cy][cx] = b[cy][cx]; continue; }
                            /* no golpe: só o ponto solto no ar (quase sem vizinho que não seja fogo) */
                            int solid = 0;
                            for (int q = 0; q < 4; q++) {
                                static const int dq[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
                                int nx = cx + dq[q][0], ny = cy + dq[q][1];
                                solid += nx >= 0 && ny >= 0 && nx < W && ny < H && p[ny][nx].a && !fury_red(p[ny][nx]);
                            }
                            if (solid < 2) p[cy][cx] = (Color){0, 0, 0, 0};
                        }
                }
            if (strike) continue;
            /* os furos de um pixel no meio do fogo fecham */
            static bool add[CH][CW];
            memset(add, 0, sizeof add);
            for (int y = 1; y < H - 1; y++)
                for (int x = 1; x < W - 1; x++) {
                    if (fire[y][x] || (p[y][x].a && !fury_red(p[y][x]))) continue;
                    add[y][x] = fire[y][x + 1] + fire[y][x - 1] + fire[y + 1][x] + fire[y - 1][x] >= 3;
                }
            for (int y = 0; y < H; y++)
                for (int x = 0; x < W; x++)
                    if (add[y][x]) fire[y][x] = true;
            /* distância (em passos de 8 vizinhos) até a lâmina */
            int head = 0, tail = 0;
            static short q[CW * CH][2];
            for (int y = 0; y < H; y++)
                for (int x = 0; x < W; x++) {
                    dist[y][x] = 99;
                    if (!fire[y][x] && fury_blade(p[y][x])) { dist[y][x] = 0; q[tail][0] = (short)x; q[tail][1] = (short)y; tail++; }
                }
            while (head < tail) {
                int cx = q[head][0], cy = q[head][1];
                head++;
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = cx + dx, ny = cy + dy;
                        if (nx < 0 || ny < 0 || nx >= W || ny >= H || dist[ny][nx] <= dist[cy][cx] + 1) continue;
                        dist[ny][nx] = (short)(dist[cy][cx] + 1);
                        q[tail][0] = (short)nx; q[tail][1] = (short)ny; tail++;
                    }
            }
            /* em cima do corpo, só o fogo colado na lâmina fica */
            for (int y = 0; y < H; y++)
                for (int x = 0; x < W; x++)
                    if (fire[y][x] && b[y][x].a && dist[y][x] > 2) { fire[y][x] = false; p[y][x] = b[y][x]; }
            static const int d4[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            static Color out[CH][CW];
            for (int y = 0; y < H; y++)
                for (int x = 0; x < W; x++) {
                    out[y][x] = p[y][x];
                    if (!fire[y][x]) continue;
                    bool edge = false;
                    for (int i = 0; i < 4 && !edge; i++) {
                        int nx = x + d4[i][0], ny = y + d4[i][1];
                        edge = nx < 0 || ny < 0 || nx >= W || ny >= H || (!fire[ny][nx] && !p[ny][nx].a);
                    }
                    uint32_t v = dist[y][x] <= 1 ? 0xff6a20 : edge ? 0x571c27 : dist[y][x] <= 4 ? 0xc42430 : 0x891e2b;
                    out[y][x] = (Color){(unsigned char)(v >> 16), (unsigned char)(v >> 8), (unsigned char)v, 255};
                }
            for (int y = 0; y < H; y++) memcpy(p[y], out[y], sizeof(Color) * (size_t)W);
        }
    }
}

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
    clean_fury(s);
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

/* ------------------------------------------------------------------------ */
/* Grade de conferência: todos os quadros de todas as animações              */
/* ------------------------------------------------------------------------ */
/* Lê uma pasta já gerada (sprite.txt e as tiras) e desenha cada animação numa
 * linha, com o nome à esquerda e o índice em cima de cada quadro. Marcas: cruz
 * ciano na âncora, borda amarela no quadro segurado (hold), vermelha no contato e
 * azul na soltura (THROW), e um ponto magenta no alcance medido no contato. */
static const char *const GLYPHS[] = {
    "A.#.#.#####.##.#", "B##.#.###.#.###.", "C.###..#..#...##", "D##.#.##.##.###.", "E####..##.#..###",
    "F####..##.#..#..", "G.###..#.##.#.##", "H#.##.#####.##.#", "I###.#..#..#.###", "J..#..#..##.#.#.",
    "K#.##.###.#.##.#", "L#..#..#..#..###", "M#.########.##.#", "N##.#.##.##.##.#", "O.#.#.##.##.#.#.",
    "P##.#.###.#..#..", "Q.#.#.##.###..##", "R##.#.###.#.##.#", "S.###...#...###.", "T###.#..#..#..#.",
    "U#.##.##.##.####", "V#.##.##.##.#.#.", "W#.##.########.#", "X#.##.#.#.#.##.#", "Y#.##.#.#..#..#.",
    "Z###..#.#.#..###", "0####.##.##.####", "1.#.##..#..#.###", "2##...#.#.#..###", "3##...#.#...###.",
    "4#.##.####..#..#", "5####..##...###.", "6.###..####.####", "7###..#.#..#..#.", "8####.#####.####",
    "9####.####..###.", "_............###", "-......###......", ":....#.....#....", " ...............",
};
static void grid_char(Image *im, int x, int y, char c, int z, Color col) {
    if (c >= 'a' && c <= 'z') c = (char)(c - 32);
    for (size_t g = 0; g < sizeof GLYPHS / sizeof GLYPHS[0]; g++) {
        if (GLYPHS[g][0] != c) continue;
        const char *b = GLYPHS[g] + 1;
        for (int r = 0; r < 5; r++)
            for (int k = 0; k < 3; k++)
                if (b[r * 3 + k] == '#') ImageDrawRectangle(im, x + k * z, y + r * z, z, z, col);
        return;
    }
}
static void grid_text(Image *im, int x, int y, const char *t, int z, Color col) {
    for (int i = 0; t[i]; i++) grid_char(im, x + i * 4 * z, y, t[i], z, col);
}

typedef struct { char name[64]; int hold, contact, release, dx, dy; bool reach; } GridAnim;

/* mode 0: tudo; 1: sem os ecos do Oboro; 2: só os ecos */
static bool grid_dir(const char *dir, const char *title, const char *out, int mode) {
    char mp[PATHLEN];
    path_join(mp, dir, "sprite.txt");
    char *txt = LoadFileText(mp);
    if (!txt) return false;
    static GridAnim an[128];
    int na = 0, cw = 0, chh = 0, ax = 0, ay = 0;
    for (char *line = strtok(txt, "\n"); line; line = strtok(NULL, "\n")) {
        char w0[64];
        if (sscanf(line, "%63s", w0) != 1 || w0[0] == '#') continue;
        if (!strcmp(w0, "cell")) sscanf(line, "%*s %d %d", &cw, &chh);
        else if (!strcmp(w0, "ancora")) sscanf(line, "%*s %d %d", &ax, &ay);
        else if (!strcmp(w0, "anim") && na < 128) {
            GridAnim *g = &an[na];
            memset(g, 0, sizeof *g);
            g->hold = g->contact = g->release = -1;
            sscanf(line, "%*s %63s", g->name);
            bool eco = strstr(g->name, "_ECO_") != NULL;
            if ((mode == 1 && eco) || (mode == 2 && !eco)) continue;
            char *q;
            if ((q = strstr(line, " hold "))) g->hold = atoi(q + 6);
            if ((q = strstr(line, " contact "))) g->contact = atoi(q + 9);
            if ((q = strstr(line, " release "))) g->release = atoi(q + 9);
            if ((q = strstr(line, " alcance "))) g->reach = sscanf(q + 9, "%d %d", &g->dx, &g->dy) == 2;
            na++;
        }
    }
    UnloadFileText(txt);
    if (!cw || !na) return false;
    static Image strips[128];
    int nf[128], maxf = 1, maxlen = (int)strlen(title);
    for (int i = 0; i < na; i++) {
        char fn[PATHLEN], pp[PATHLEN];
        snprintf(fn, sizeof fn, "%.60s.png", an[i].name);
        path_join(pp, dir, fn);
        strips[i] = LoadImage(pp);
        nf[i] = 0;
        if (!strips[i].data) continue;
        ImageFormat(&strips[i], PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        nf[i] = strips[i].width / cw;
        if (nf[i] > maxf) maxf = nf[i];
        if ((int)strlen(an[i].name) > maxlen) maxlen = (int)strlen(an[i].name);
    }
    const int z = 2, gap = 3 * z, label = 16 + maxlen * 4 * z, head = 9 * z;
    int cellw = cw * z, cellh = chh * z;
    int W = label + maxf * (cellw + gap) + gap, H = 16 * z + na * (head + cellh + gap);
    Image im = GenImageColor(W, H, (Color){30, 28, 36, 255});
    grid_text(&im, 8, 4 * z, title, z, (Color){255, 236, 190, 255});
    grid_text(&im, 8 + ((int)strlen(title) + 2) * 4 * z, 4 * z, "HOLD", z, (Color){255, 214, 60, 255});
    grid_text(&im, 8 + ((int)strlen(title) + 7) * 4 * z, 4 * z, "CONTATO", z, (Color){255, 70, 70, 255});
    grid_text(&im, 8 + ((int)strlen(title) + 15) * 4 * z, 4 * z, "ANCORA", z, (Color){90, 230, 255, 255});
    grid_text(&im, 8 + ((int)strlen(title) + 22) * 4 * z, 4 * z, "ALCANCE", z, (Color){255, 90, 255, 255});
    Color *dst = im.data;
    for (int i = 0; i < na; i++) {
        int y0 = 16 * z + i * (head + cellh + gap);
        grid_text(&im, 8, y0 + head + cellh / 2 - 5, an[i].name, z, (Color){235, 235, 235, 255});
        for (int k = 0; k < nf[i]; k++) {
            int x0 = label + k * (cellw + gap);
            char num[8];
            snprintf(num, sizeof num, "%d", k);
            grid_text(&im, x0 + 2, y0 + 2 * z, num, z, (Color){170, 170, 180, 255});
            Color border = k == an[i].contact ? (Color){255, 70, 70, 255}
                         : k == an[i].hold    ? (Color){255, 214, 60, 255}
                         : k == an[i].release ? (Color){80, 140, 255, 255}
                                              : (Color){58, 54, 68, 255};
            ImageDrawRectangle(&im, x0 - z, y0 + head - z, cellw + 2 * z, cellh + 2 * z, border);
            ImageDrawRectangle(&im, x0, y0 + head, cellw, cellh, (Color){70, 66, 80, 255});
            const Color *src = strips[i].data;
            for (int y = 0; y < chh; y++)
                for (int x = 0; x < cw; x++) {
                    Color c = src[y * strips[i].width + k * cw + x];
                    if (c.a < 128) continue;
                    for (int dy = 0; dy < z; dy++)
                        for (int dxx = 0; dxx < z; dxx++) dst[(y0 + head + y * z + dy) * W + x0 + x * z + dxx] = c;
                }
            /* âncora: cruz ciano; alcance no contato: ponto magenta */
            for (int d = -2; d <= 2; d++) {
                ImageDrawRectangle(&im, x0 + (ax + d) * z, y0 + head + ay * z, z, z, (Color){90, 230, 255, 255});
                ImageDrawRectangle(&im, x0 + ax * z, y0 + head + (ay + d) * z, z, z, (Color){90, 230, 255, 255});
            }
            if (k == an[i].contact && an[i].reach) {
                int rx = ax + an[i].dx, ry = ay + an[i].dy;
                ImageDrawRectangle(&im, x0 + (rx - 1) * z, y0 + head + (ry - 1) * z, 3 * z, 3 * z, (Color){255, 90, 255, 255});
            }
        }
    }
    for (int i = 0; i < na; i++)
        if (strips[i].data) UnloadImage(strips[i]);
    bool ok = ExportImage(im, out);
    UnloadImage(im);
    return ok;
}

/* A grade de um personagem gerado: <saida>/_folhas/grade_<id>.png (no Oboro, os
   ecos numa grade à parte). */
static void grid_char_dir(const char *saida, const char *id) {
    char d[PATHLEN], fol[PATHLEN], fn[128], out[PATHLEN];
    path_join(d, saida, id);
    path_join(fol, saida, "_folhas");
    make_dir(fol);
    snprintf(fn, sizeof fn, "grade_%.60s.png", id);
    path_join(out, fol, fn);
    if (grid_dir(d, id, out, 1)) printf("  grade: %s\n", out);
    char mp[PATHLEN];
    snprintf(fn, sizeof fn, "grade_%.60s_ecos.png", id);
    path_join(mp, fol, fn);
    if (grid_dir(d, id, mp, 2)) printf("  grade: %s\n", mp);
}

static void usage(void) {
    printf("uso: personagens [--entrada pasta] [--saida pasta] [--so nome ...] [--folhas] [--grade] [--lista]\n"
           "       personagens --grade-de pasta saida.png\n"
           "  --entrada  pranchas originais (padrão: <saida>/_original, copiada de <saida>/musashi na primeira vez)\n"
           "  --saida    onde sai uma pasta por personagem (padrão: assets/sprites)\n"
           "  --so       só estes personagens\n"
           "  --folhas   também as folhas de conferência em <saida>/_folhas\n"
           "  --grade    também a grade de cada um (todos os quadros de todas as animações) em <saida>/_folhas\n"
           "  --grade-de só a grade de uma pasta já gerada (ex.: uma cópia de antes), e sai\n"
           "  --lista    mostra os personagens e sai\n");
}

int main(int argc, char **argv) {
    SetTraceLogLevel(LOG_WARNING);
    const char *entrada = NULL, *saida = "assets/sprites";
    const char *only[64];
    int nonly = 0;
    bool folhas = false, lista = false, grade = false;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--grade-de") && i + 2 < argc) {
            const char *dir = argv[i + 1], *out = argv[i + 2];
            const char *slash = strrchr(dir, '/');
            char title[64];
            snprintf(title, sizeof title, "%s", slash && slash[1] ? slash + 1 : dir);
            return grid_dir(dir, title, out, 0) ? 0 : 1;
        }
        if (!strcmp(argv[i], "--entrada") && i + 1 < argc) entrada = argv[++i];
        else if (!strcmp(argv[i], "--grade")) grade = true;
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
            /* parado próprio: o quadro 0 de outra prancha do pack (a guarda do Garfiel,
               com as mãos e as garras à frente), respirando: o tronco desce 1 px a cada
               dois quadros */
            const Strip *pose = NULL;
            if (ch->parado && !strcmp(st->name, "IDLE"))
                for (int t = 0; t < sc->ns; t++)
                    if (!strcmp(sc->strips[t].name, ch->parado)) pose = &sc->strips[t];
            for (int j = 0; j < st->nframes; j++) {
                Ctx cx = make_ctx(st->name, j, st->nframes, info, k);
                if (pose) render(&pose->frames[0], &pose->segs[0], ch, &cx, &cv);
                else {
                    const Frame *sf;
                    const Seg *sg;
                    source_frame(sc, ch, st, j, &sf, &sg);
                    render(sf, sg, ch, &cx, &cv);
                }
                memcpy(r->frames[j].p, cv.a, sizeof cv.a);
                if (pose && (j / 2) % 2) {
                    int top = CH, bot = 0;
                    for (int y = 0; y < CH; y++)
                        for (int x = 0; x < CW; x++)
                            if (r->frames[j].p[y][x].a) { if (y < top) top = y; if (y > bot) bot = y; }
                    int waist = bot - (bot - top) * 42 / 100;
                    for (int y = waist - 1; y >= top; y--) memcpy(r->frames[j].p[y + 1], r->frames[j].p[y], sizeof r->frames[j].p[y]);
                    memset(r->frames[j].p[top], 0, sizeof r->frames[j].p[top]);
                }
                if (j == k) has_reach[nr] = reach(&cv, ax, ay, &reachv[nr][0], &reachv[nr][1]);
            }
            /* o quadro de flash do pack (a silhueta toda branca do golpe recebido) vira a
               silhueta branca do quadro vizinho já pronto, com o cabelo e a arma novos */
            for (int j = 0; ch->pack && !pose && j < st->nframes && st->nframes > 1; j++) {
                const Frame *sf;
                const Seg *sg;
                source_frame(sc, ch, st, j, &sf, &sg);
                int n = 0, wh = 0;
                for (int y = 0; y < CH; y++)
                    for (int x = 0; x < CW; x++) {
                        Color o = sf->p[y][x];
                        n += o.a != 0;
                        wh += o.a && o.r == 255 && o.g == 255 && o.b == 255;
                    }
                if (n == 0 || wh * 10 < n * 6) continue;
                const Frame *nb = &r->frames[j + 1 < st->nframes ? j + 1 : j - 1];
                for (int y = 0; y < CH; y++)
                    for (int x = 0; x < CW; x++)
                        r->frames[j].p[y][x] = nb->p[y][x].a ? (Color){255, 255, 255, 255} : (Color){0, 0, 0, 0};
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
            save_strip(p, r->frames, r->n, OUT_W(sc), sc->ch);
            nr++;
        }
        path_join(p, d, "sprite.txt");
        write_manifest(p, &sc->man, sc->strips, sc->ns, contact, (const int (*)[2])reachv, has_reach, ax, ay,
                       has_guard ? guard : NULL, frame_ms(ch), OUT_W(sc), sc->ch, ch);
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
        if (nst > 0 && nr < MAX_REND) {
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
                {
                    const Frame *sf;
                    const Seg *sg;
                    source_frame(sc, ch, st, steps[k].frame, &sf, &sg);
                    render(sf, sg, ch, &cx, &cv);
                }
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
            save_strip(p, r->frames, nst, OUT_W(sc), sc->ch);
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
                if (ch->ecos) f1 = ss->nframes / 3; /* o Oboro ainda de cabeça erguida (depois ela pende) */
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
                save_strip(p, r->frames, r->n, OUT_W(sc), sc->ch);
                if (mf) {
                    if (r->n > 1) fprintf(mf, "anim %-13s  stop %d\n", out, r->n - 1);
                    else fprintf(mf, "anim %s\n", out);
                }
                nr++;
            }
        }

        /* Kojiro: o parado com a katana na bainha (fora da luta) e o saque, que termina
           no quadro 0 do IDLE (a guarda em que a luta começa) */
        const Strip *idl = ch->saque ? source_strip(sc, "IDLE") : NULL;
        int iq = -1;
        for (int q = 0; q < nr && idl; q++)
            if (!strcmp(rend[si][q].name, "IDLE")) iq = q;
        if (idl && iq >= 0 && nr + 2 <= MAX_REND) {
            const AnimInfo *info = find_anim(&sc->man, "IDLE");
            Rendered *r = &rend[si][nr];
            r->name = "EMBAINHADO";
            r->n = idl->nframes;
            r->frames = calloc((size_t)r->n, sizeof(Frame));
            for (int j = 0; j < idl->nframes; j++) {
                Ctx cx = make_ctx("EMBAINHADO", j, idl->nframes, info, -1);
                render(&idl->frames[j], &idl->segs[j], ch, &cx, &cv);
                memcpy(r->frames[j].p, cv.a, sizeof cv.a);
            }
            path_join(p, d, "EMBAINHADO.png");
            save_strip(p, r->frames, r->n, OUT_W(sc), sc->ch);
            if (mf) fprintf(mf, "anim EMBAINHADO     loop\n");
            nr++;
            r = &rend[si][nr];
            r->name = "DESEMBAINHAR";
            r->n = 6;
            r->frames = calloc(6, sizeof(Frame));
            for (int j = 0; j < 5; j++) {
                Ctx cx = make_ctx("DESEMBAINHAR", j, 6, info, -1);
                render(&idl->frames[0], &idl->segs[0], ch, &cx, &cv);
                memcpy(r->frames[j].p, cv.a, sizeof cv.a);
            }
            memcpy(r->frames[5].p, rend[si][iq].frames[0].p, sizeof r->frames[5].p);
            path_join(p, d, "DESEMBAINHAR.png");
            save_strip(p, r->frames, r->n, OUT_W(sc), sc->ch);
            if (mf) fprintf(mf, "anim DESEMBAINHAR   stop 5  ms 70\n");
            nr++;
        }

        /* Hanzo sentado em seiza, junto da fogueira: o tronco do IDLE (o rosto, a barba,
           as mãos) desce até ficar sobre as pernas dobradas, desenhadas aqui: as coxas
           para a frente até os joelhos, os pés por baixo, atrás, e as mãos no colo */
        int iq2 = -1;
        for (int q = 0; q < nr && ch->sentado; q++)
            if (!strcmp(rend[si][q].name, "IDLE")) iq2 = q;
        if (iq2 >= 0 && nr < MAX_REND) {
            const Rendered *src = &rend[si][iq2];
            Rendered *r = &rend[si][nr];
            r->name = "SENTADO";
            r->n = src->n;
            r->frames = calloc((size_t)r->n, sizeof(Frame));
            for (int j = 0; j < src->n; j++) seated_frame(&src->frames[j], &r->frames[j], ch);
            path_join(p, d, "SENTADO.png");
            save_strip(p, r->frames, r->n, OUT_W(sc), sc->ch);
            if (mf) fprintf(mf, "anim SENTADO        loop\n");
            nr++;
        }

        /* Oboro: cada ataque em cada uma das onze posturas, e a cena do grito */
        if (ch->ecos) {
            static const char *atk[] = {"ATTACK_1", "ATTACK_2", "ATTACK_3"};
            for (int a = 0; a < NCHARS; a++) {
                const Char *ap = &CHARS[a];
                if (!strcmp(ap->id, "kojiro") || ap->sem_arma || ap->ecos) continue;
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
                    save_strip(p, r->frames, r->n, OUT_W(sc), sc->ch);
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
                save_strip(p, r->frames, r->n, OUT_W(sc), sc->ch);
                if (mf) fprintf(mf, "anim GRITO                     ms 90\n");
                nr++;
            }
        }
        if (mf) fclose(mf);
        nrend[si] = nr;
        if (grade) grid_char_dir(saida, ch->id);

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
