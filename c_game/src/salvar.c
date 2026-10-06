#define _POSIX_C_SOURCE 200809L   /* fileno e fsync com -std=c11 */
#include "salvar.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#define SINCRONIZA(f) _commit(_fileno(f))
#else
#include <unistd.h>
#define SINCRONIZA(f) fsync(fileno(f))
#endif

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

/* Os dois caminhos ficam no mesmo diretório/volume. Nunca apagar o destino
 * antes da troca: um arquivo bloqueado ou uma falha deve conservar o save antigo. */
static int troca(const char *de, const char *para) {
#ifdef _WIN32
    if (MoveFileExA(de, para, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return 0;
    DWORD erro = GetLastError();
    errno = erro == ERROR_FILE_NOT_FOUND || erro == ERROR_PATH_NOT_FOUND ? ENOENT :
            erro == ERROR_ACCESS_DENIED || erro == ERROR_SHARING_VIOLATION ? EACCES : EIO;
    return -1;
#else
    return rename(de, para);
#endif
}

bool save_gravar(const char *caminho, const Campaign *c, char *erro, size_t n) {
    char tmp[512], texto[96];
    if (snprintf(tmp, sizeof tmp, "%s.tmp", caminho) >= (int)sizeof tmp) { snprintf(erro, n, "caminho comprido demais"); return false; }
    save_formatar(c, texto, sizeof texto);
    FILE *f = fopen(tmp, "wb");
    if (!f) { descreve(erro, n, "não abriu o arquivo temporário"); return false; }
    bool ok = fputs(texto, f) >= 0;
    if (ok) ok = fflush(f) == 0;
    if (ok) ok = SINCRONIZA(f) == 0;
    if (!ok) descreve(erro, n, "não escreveu");
    if (fclose(f) != 0 && ok) { descreve(erro, n, "não fechou o arquivo"); ok = false; }
    if (ok && troca(tmp, caminho) != 0) { descreve(erro, n, "não trocou pelo save"); ok = false; }
    if (!ok) remove(tmp);
    return ok;
}

/* Guarda o arquivo estragado em <caminho>.bak. */
static bool guarda_bak(const char *caminho) {
    char bak[512];
    if (snprintf(bak, sizeof bak, "%s.bak", caminho) >= (int)sizeof bak) return false;
    return troca(caminho, bak) == 0;
}

SaveLeitura save_ler(const char *caminho, Campaign *c, char *aviso, size_t n) {
    FILE *f = fopen(caminho, "rb");
    if (!f) {
        if (errno == ENOENT) return SAVE_NAO_EXISTE;
        descreve(aviso, n, "não consegui abrir o save");
        return SAVE_ILEGIVEL;
    }
    char buf[SAVE_MAX_BYTES + 1];
    size_t len = fread(buf, 1, sizeof buf - 1, f);
    bool mais = len == sizeof buf - 1 && fgetc(f) != EOF;   /* passou do tamanho de um save */
    bool erro_de_leitura = ferror(f) != 0;
    fclose(f);
    if (erro_de_leitura) {
        snprintf(aviso, n, "não consegui ler o save (%s)", strerror(errno));
        return SAVE_ILEGIVEL;
    }
    buf[len] = 0;
    Campaign lido;
    if (!mais && memchr(buf, 0, len) == NULL && save_interpretar(buf, &lido)) {
        *c = lido;
        return SAVE_OK;
    }
    if (guarda_bak(caminho)) snprintf(aviso, n, "o save estava corrompido: guardei uma cópia em %s.bak e começo sem ele", caminho);
    else snprintf(aviso, n, "o save estava corrompido e não consegui guardar a cópia: começo sem ele");
    return SAVE_CORROMPIDO;
}
