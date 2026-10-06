#include "pasta_dados.h"

#include "gravar.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOME_WINDOWS "Apara"
#define NOME_MAC "Apara"
#define NOME_LINUX "apara"
#define MIGRAR_MAX 512                  /* o maior arquivo que se migra: o save tem uns 40 bytes, as opções uns 40 */

static bool tem(const char *v) { return v && v[0]; }

static char separador(Sistema s) { return s == SISTEMA_WINDOWS ? '\\' : '/'; }

static bool e_separador(Sistema s, char c) { return c == '/' || (s == SISTEMA_WINDOWS && c == '\\'); }

/* As `partes` (terminadas em NULL) unidas pelo separador do sistema, sem separador dobrado nem sobrando no fim. */
static bool monta(Sistema s, char *out, size_t n, const char *const *partes) {
    size_t len = 0;
    for (int i = 0; partes[i]; i++) {
        const char *p = partes[i];
        size_t t = strlen(p);
        while (t > 1 && e_separador(s, p[t - 1])) t--;           /* "a/b/" vale "a/b"; a raiz "/" fica */
        bool raiz = len == 1 && e_separador(s, out[0]);
        if (i > 0 && !raiz) {
            if (len + 1 >= n) return false;
            out[len++] = separador(s);
        }
        if (len + t >= n) return false;
        memcpy(out + len, p, t);
        len += t;
    }
    out[len] = 0;
    return len > 0;
}

AmbienteDados pasta_dados_ambiente(void) {
    AmbienteDados a = {0};
#if defined(_WIN32)
    a.sistema = SISTEMA_WINDOWS;
#elif defined(__APPLE__)
    a.sistema = SISTEMA_MAC;
#else
    a.sistema = SISTEMA_LINUX;
#endif
    a.forcada = getenv("APARA_DADOS");
    a.localappdata = getenv("LOCALAPPDATA");
    a.appdata = getenv("APPDATA");
    a.userprofile = getenv("USERPROFILE");
    a.home = getenv("HOME");
    a.xdg = getenv("XDG_DATA_HOME");
    return a;
}

bool pasta_dados_caminho(const AmbienteDados *a, char *out, size_t n) {
    Sistema s = a->sistema;
    if (tem(a->forcada)) return monta(s, out, n, (const char *const[]){a->forcada, NULL});
    switch (s) {
        case SISTEMA_WINDOWS:
            if (tem(a->localappdata)) return monta(s, out, n, (const char *const[]){a->localappdata, NOME_WINDOWS, NULL});
            if (tem(a->appdata)) return monta(s, out, n, (const char *const[]){a->appdata, NOME_WINDOWS, NULL});
            if (tem(a->userprofile)) return monta(s, out, n, (const char *const[]){a->userprofile, "AppData", "Local", NOME_WINDOWS, NULL});
            return false;
        case SISTEMA_MAC:
            if (!tem(a->home)) return false;
            return monta(s, out, n, (const char *const[]){a->home, "Library", "Application Support", NOME_MAC, NULL});
        case SISTEMA_LINUX:
            if (tem(a->xdg) && a->xdg[0] == '/') return monta(s, out, n, (const char *const[]){a->xdg, NOME_LINUX, NULL});   /* a especificação XDG: só vale caminho absoluto */
            if (!tem(a->home)) return false;
            return monta(s, out, n, (const char *const[]){a->home, ".local", "share", NOME_LINUX, NULL});
    }
    return false;
}

bool pasta_dados_arquivo(Sistema s, const char *pasta, const char *nome, char *out, size_t n) {
    return monta(s, out, n, (const char *const[]){pasta, nome, NULL});
}

Migracao pasta_dados_migrar(const char *legado, const char *novo, bool (*valido)(const char *texto), size_t max, char *aviso, size_t n) {
    if (gravar_existe(novo)) return MIGROU_NADA;                /* o novo manda: nunca se sobrescreve */
    char buf[MIGRAR_MAX + 1];
    if (max > MIGRAR_MAX) max = MIGRAR_MAX;
    long len = gravar_ler_pequeno(legado, buf, max);
    if (len == -1) {
        if (errno == ENOENT) return MIGROU_NADA;
        snprintf(aviso, n, "não consegui abrir o arquivo antigo (%s): %s", gravar_nome(legado), strerror(errno));
        return MIGROU_FALHOU;
    }
    if (len == -3) {
        snprintf(aviso, n, "não consegui ler o arquivo antigo (%s): %s", gravar_nome(legado), strerror(errno));
        return MIGROU_FALHOU;
    }
    if (len < 0 || memchr(buf, 0, (size_t)len) != NULL || !valido(buf)) return MIGROU_INVALIDO;
    char erro[128];
    if (!gravar_atomico(novo, buf, (size_t)len, erro, sizeof erro)) {
        snprintf(aviso, n, "não consegui copiar %s para a pasta de dados: %s", gravar_nome(legado), erro);
        return MIGROU_FALHOU;
    }
    return MIGROU_COPIOU;
}
