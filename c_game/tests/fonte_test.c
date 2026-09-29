/*
 * fonte_test.c - todo caractere não ASCII das strings do jogo tem glifo na fonte da interface.
 * Sem glifo a raylib desenha um "?" (foi o que aconteceu com as aspas de fechar da abertura).
 *   fonte_test fonte.ttf arquivo.c...      (make teste-fonte)
 * Confere: (1) tudo que o jogo pede à fonte (fonte_pedidos) existe na TTF; (2) todo caractere não
 * ASCII dentro de string literal dos .c foi pedido. Comentários e literais de caractere não contam.
 * A TTF é lida direto (tabela cmap, formato 4), sem raylib nem janela.
 */
#include "../src/fonte.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks = 0, failures = 0;
#define CHECK(cond, ...) do { \
    checks++; \
    if (!(cond)) { failures++; printf("FALHA %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } \
} while (0)

/* ---- a TTF ---------------------------------------------------------- */
static unsigned char *ttf;
static long ttf_size;

static unsigned u16(long off) { return off + 2 <= ttf_size ? (unsigned)(ttf[off] << 8 | ttf[off + 1]) : 0; }
static unsigned long u32(long off) { return ((unsigned long)u16(off) << 16) | u16(off + 2); }

static long tabela(const char *tag) {
    unsigned n = u16(4);
    for (unsigned i = 0; i < n; i++) {
        long rec = 12 + 16L * i;
        if (rec + 16 <= ttf_size && memcmp(ttf + rec, tag, 4) == 0) return (long)u32(rec + 8);
    }
    return -1;
}

/* Subtabela Unicode BMP (formato 4) da cmap; -1 se não achou. */
static long subtabela_unicode(void) {
    long cmap = tabela("cmap");
    if (cmap < 0) return -1;
    unsigned n = u16(cmap + 2);
    for (unsigned i = 0; i < n; i++) {
        long rec = cmap + 4 + 8L * i;
        unsigned plat = u16(rec), enc = u16(rec + 2);
        long sub = cmap + (long)u32(rec + 4);
        if (((plat == 3 && enc == 1) || plat == 0) && u16(sub) == 4) return sub;
    }
    return -1;
}

static int tem_glifo(long sub, int cp) {
    if (cp < 0 || cp > 0xFFFF) return 0;
    unsigned segs = u16(sub + 6) / 2;
    long fim = sub + 14, ini = fim + 2L * segs + 2, delta = ini + 2L * segs, ro = delta + 2L * segs;
    for (unsigned i = 0; i < segs; i++) {
        if ((int)u16(fim + 2L * i) < cp) continue;
        if ((int)u16(ini + 2L * i) > cp) return 0;
        unsigned g;
        if (u16(ro + 2L * i) == 0) g = ((unsigned)cp + u16(delta + 2L * i)) & 0xFFFF;
        else {
            g = u16(ro + 2L * i + u16(ro + 2L * i) + 2L * (cp - (int)u16(ini + 2L * i)));
            if (g) g = (g + u16(delta + 2L * i)) & 0xFFFF;
        }
        return g != 0;
    }
    return 0;
}

/* ---- as strings do C -------------------------------------------------- */
#define MAX_CP 512
static int achados[MAX_CP], nachados;
static char onde[MAX_CP][96];

static void acha(int cp, const char *arq, int linha) {
    for (int i = 0; i < nachados; i++) if (achados[i] == cp) return;
    if (nachados >= MAX_CP) return;
    achados[nachados] = cp;
    snprintf(onde[nachados], sizeof onde[0], "%s:%d", arq, linha);
    nachados++;
}

/* Lê um ponto de código UTF-8 em s; devolve o tamanho (1 se inválido). */
static int utf8(const unsigned char *s, int *cp) {
    if (s[0] < 0x80) { *cp = s[0]; return 1; }
    if ((s[0] & 0xE0) == 0xC0 && (s[1] & 0xC0) == 0x80) { *cp = (s[0] & 0x1F) << 6 | (s[1] & 0x3F); return 2; }
    if ((s[0] & 0xF0) == 0xE0 && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80) { *cp = (s[0] & 0x0F) << 12 | (s[1] & 0x3F) << 6 | (s[2] & 0x3F); return 3; }
    if ((s[0] & 0xF8) == 0xF0 && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80 && (s[3] & 0xC0) == 0x80) { *cp = (s[0] & 0x07) << 18 | (s[1] & 0x3F) << 12 | (s[2] & 0x3F) << 6 | (s[3] & 0x3F); return 4; }
    *cp = s[0];
    return 1;
}

/* Junta os caracteres não ASCII dos literais de string do texto C `t`. */
static void varre(const char *arq, const unsigned char *t) {
    int linha = 1;
    enum { CODIGO, LINHA, BLOCO, TEXTO, CARACTERE } st = CODIGO;
    for (const unsigned char *p = t; *p; p++) {
        if (*p == '\n') linha++;
        switch (st) {
            case CODIGO:
                if (p[0] == '/' && p[1] == '/') { st = LINHA; p++; }
                else if (p[0] == '/' && p[1] == '*') { st = BLOCO; p++; }
                else if (*p == '"') st = TEXTO;
                else if (*p == '\'') st = CARACTERE;
                break;
            case LINHA: if (*p == '\n') st = CODIGO; break;
            case BLOCO: if (p[0] == '*' && p[1] == '/') { st = CODIGO; p++; } break;
            case CARACTERE:
                if (*p == '\\' && p[1]) p++;
                else if (*p == '\'') st = CODIGO;
                break;
            case TEXTO:
                if (*p == '\\' && p[1]) { p++; break; }
                if (*p == '"') { st = CODIGO; break; }
                if (*p >= 0x80) {
                    int cp, n = utf8(p, &cp);
                    acha(cp, arq, linha);
                    p += n - 1;
                }
                break;
        }
    }
}

static void teste_do_varredor(void) {
    nachados = 0;
    varre("x.c", (const unsigned char *)
        "/* comentário com ō e 構 */ // outro ç\n"
        "char a = '\"'; const char *s = \"aspas \\\" escapadas “assim” e ção\"; char b = '\\'';\n"
        "const char *t = \"outra linha ok\"; /* “fora” */\n");
    int tem_ccedilha = 0, tem_abre = 0, tem_fecha = 0, tem_o = 0, tem_kanji = 0;
    for (int i = 0; i < nachados; i++) {
        tem_ccedilha |= achados[i] == 0xE7;
        tem_abre |= achados[i] == 0x201C;
        tem_fecha |= achados[i] == 0x201D;
        tem_o |= achados[i] == 0x14D;
        tem_kanji |= achados[i] == 0x69CB;
    }
    CHECK(tem_ccedilha && tem_abre && tem_fecha, "o varredor acha os caracteres dentro da string, mesmo depois de \\\" e de '\"'");
    CHECK(!tem_o && !tem_kanji, "o varredor ignora comentários");
    CHECK(nachados == 4, "só os quatro caracteres da string: ç, ã e as duas aspas (%d)", nachados);
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "uso: fonte_test fonte.ttf arquivo.c...\n"); return 2; }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { fprintf(stderr, "fonte_test: sem %s\n", argv[1]); return 2; }
    fseek(f, 0, SEEK_END);
    ttf_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    ttf = malloc((size_t)ttf_size + 1);
    if (!ttf || fread(ttf, 1, (size_t)ttf_size, f) != (size_t)ttf_size) { fprintf(stderr, "fonte_test: erro lendo %s\n", argv[1]); return 2; }
    fclose(f);
    long sub = subtabela_unicode();
    CHECK(sub >= 0, "a fonte tem uma cmap Unicode (formato 4)");
    if (sub < 0) return 1;
    CHECK(tem_glifo(sub, 'a') && tem_glifo(sub, 0xE7) && !tem_glifo(sub, 0x69CB), "o leitor da cmap: a e ç têm glifo; o kanji não");

    teste_do_varredor();

    /* 1. tudo que o jogo pede existe na fonte (senão a raylib avisa "glyphs found [x/y]") */
    int pedidos[FONTE_PEDIDOS_MAX], np = fonte_pedidos(pedidos, FONTE_PEDIDOS_MAX);
    int faltam = 0;
    for (int i = 0; i < np; i++)
        if (!tem_glifo(sub, pedidos[i])) { faltam++; printf("  pedido sem glifo na fonte: U+%04X\n", pedidos[i]); }
    CHECK(faltam == 0, "todo caractere pedido à fonte existe nela (%d de %d faltam)", faltam, np);

    /* 2. todo não ASCII das strings do jogo foi pedido e tem glifo */
    nachados = 0;
    for (int a = 2; a < argc; a++) {
        FILE *c = fopen(argv[a], "rb");
        if (!c) { fprintf(stderr, "fonte_test: sem %s\n", argv[a]); return 2; }
        fseek(c, 0, SEEK_END);
        long n = ftell(c);
        fseek(c, 0, SEEK_SET);
        unsigned char *txt = malloc((size_t)n + 1);
        if (!txt || fread(txt, 1, (size_t)n, c) != (size_t)n) { fprintf(stderr, "fonte_test: erro lendo %s\n", argv[a]); return 2; }
        txt[n] = 0;
        fclose(c);
        varre(argv[a], txt);
        free(txt);
    }
    int sem_pedido = 0;
    for (int i = 0; i < nachados; i++) {
        int pedido = 0;
        for (int k = 0; k < np; k++) pedido |= pedidos[k] == achados[i];
        if (!pedido || !tem_glifo(sub, achados[i])) {
            sem_pedido++;
            printf("  U+%04X em %s: %s\n", achados[i], onde[i], pedido ? "pedido, mas a fonte não tem" : "a fonte não pede esse caractere (entre em src/fonte.c)");
        }
    }
    CHECK(nachados > 0, "as strings do jogo têm caracteres não ASCII (o varredor viu %d)", nachados);
    CHECK(sem_pedido == 0, "todo caractere não ASCII das strings do jogo tem glifo (%d sem)", sem_pedido);
    printf("fonte: %d caracteres pedidos; %d não ASCII diferentes nas strings; %d verificações, %d falhas\n", np, nachados, checks, failures);
    free(ttf);
    return failures ? 1 : 0;
}
