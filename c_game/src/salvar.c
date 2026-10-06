#include "salvar.h"

#include "gravar.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define VERSAO 2

void save_formatar(const Campaign *c, char *out, size_t n) {
    snprintf(out, n, "APARA-C %d\n%d %u %d %d\n", VERSAO, c->index, (unsigned)c->clearedMask, c->completed ? 1 : 0, c->loreSeen ? 1 : 0);
}

/* Próximo número inteiro sem sinal de `*p` (depois de espaços). false se não há, se passa de `max`
 * ou se tem sinal ou lixo colado. */
static bool proximo(const char **p, unsigned long long max, unsigned long long *out) {
    const char *s = *p;
    while (isspace((unsigned char)*s)) s++;
    if (!isdigit((unsigned char)*s)) return false;
    unsigned long long v = 0;
    while (isdigit((unsigned char)*s)) {
        v = v * 10 + (unsigned)(*s - '0');
        if (v > max) return false;
        s++;
    }
    if (*s && !isspace((unsigned char)*s)) return false;
    *p = s;
    *out = v;
    return true;
}

bool save_interpretar(const char *texto, Campaign *c) {
    const char *p = texto;
    while (isspace((unsigned char)*p)) p++;
    static const char CABECA[] = "APARA-C";
    if (strncmp(p, CABECA, sizeof CABECA - 1) != 0) return false;
    p += sizeof CABECA - 1;
    unsigned long long versao, indice, mascara, completa, abertura;
    const unsigned long long todos = (1ull << ROSTER_SIZE) - 1;
    if (!proximo(&p, VERSAO, &versao) || versao != VERSAO) return false;
    if (!proximo(&p, ROSTER_SIZE - 1, &indice)) return false;
    if (!proximo(&p, todos, &mascara)) return false;
    if (!proximo(&p, 1, &completa)) return false;
    if (!proximo(&p, 1, &abertura)) return false;
    while (isspace((unsigned char)*p)) p++;
    if (*p) return false;                                   /* lixo depois do último número */
    if (completa && indice != ROSTER_SIZE - 1) return false;    /* a trilha só se completa no último mestre */
    c->index = (int)indice;
    c->clearedMask = (uint32_t)mascara;
    c->completed = completa != 0;
    c->loreSeen = abertura != 0;
    return true;
}

static void descreve(char *erro, size_t n, const char *o_que) {
    snprintf(erro, n, "%s: %s", o_que, strerror(errno));
}

bool save_gravar(const char *caminho, const Campaign *c, char *erro, size_t n) {
    char texto[96];
    save_formatar(c, texto, sizeof texto);
    return gravar_atomico(caminho, texto, strlen(texto), erro, n);
}

SaveLeitura save_ler(const char *caminho, Campaign *c, char *aviso, size_t n) {
    char buf[SAVE_MAX_BYTES + 1];
    long len = gravar_ler_pequeno(caminho, buf, SAVE_MAX_BYTES);
    if (len == -1) {
        if (errno == ENOENT) return SAVE_NAO_EXISTE;
        descreve(aviso, n, "não consegui abrir o save");
        return SAVE_ILEGIVEL;
    }
    if (len == -3) {
        snprintf(aviso, n, "não consegui ler o save (%s)", strerror(errno));
        return SAVE_ILEGIVEL;
    }
    Campaign lido;
    if (len >= 0 && memchr(buf, 0, (size_t)len) == NULL && save_interpretar(buf, &lido)) {
        *c = lido;
        return SAVE_OK;
    }
    if (gravar_guardar_bak(caminho)) snprintf(aviso, n, "o save estava corrompido: cópia em %s.bak; começo sem ele", gravar_nome(caminho));
    else snprintf(aviso, n, "o save estava corrompido e não consegui guardar a cópia: começo sem ele");
    return SAVE_CORROMPIDO;
}
