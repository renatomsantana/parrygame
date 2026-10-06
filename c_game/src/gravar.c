#define _POSIX_C_SOURCE 200809L   /* fileno, fsync e mkdir com -std=c11 */
#include "gravar.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#define SINCRONIZA(f) _commit(_fileno(f))
#define FAZ_PASTA(p) _mkdir(p)
#define SEPARADOR(c) ((c) == '/' || (c) == '\\')
#else
#include <unistd.h>
#define SINCRONIZA(f) fsync(fileno(f))
#define FAZ_PASTA(p) mkdir((p), 0755)
#define SEPARADOR(c) ((c) == '/')
#endif

static void descreve(char *erro, size_t n, const char *o_que) {
    if (erro && n) snprintf(erro, n, "%s: %s", o_que, strerror(errno));
}

int gravar_troca(const char *de, const char *para) {
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

bool gravar_atomico(const char *caminho, const char *dados, size_t n, char *erro, size_t nerro) {
    char tmp[CAMINHO_MAX];
    if (snprintf(tmp, sizeof tmp, "%s.tmp", caminho) >= (int)sizeof tmp) {
        if (erro && nerro) snprintf(erro, nerro, "caminho comprido demais");
        return false;
    }
    FILE *f = fopen(tmp, "wb");
    if (!f) { descreve(erro, nerro, "não abriu o arquivo temporário"); return false; }
    bool ok = n == 0 || fwrite(dados, 1, n, f) == n;
    if (ok) ok = fflush(f) == 0;
    if (ok) ok = SINCRONIZA(f) == 0;
    if (!ok) descreve(erro, nerro, "não escreveu");
    if (fclose(f) != 0 && ok) { descreve(erro, nerro, "não fechou o arquivo"); ok = false; }
    if (ok && gravar_troca(tmp, caminho) != 0) { descreve(erro, nerro, "não trocou pelo save"); ok = false; }
    if (!ok) remove(tmp);
    return ok;
}

bool gravar_guardar_bak(const char *caminho) {
    char bak[CAMINHO_MAX];
    if (snprintf(bak, sizeof bak, "%s.bak", caminho) >= (int)sizeof bak) return false;
    return gravar_troca(caminho, bak) == 0;
}

const char *gravar_nome(const char *caminho) {
    const char *nome = caminho;
    for (const char *c = caminho; *c; c++) if (SEPARADOR(*c)) nome = c + 1;
    return nome;
}

static bool e_pasta(const char *caminho) {
    struct stat st;
    return stat(caminho, &st) == 0 && S_ISDIR(st.st_mode);
}

bool gravar_existe(const char *caminho) {
    struct stat st;
    return stat(caminho, &st) == 0;
}

bool gravar_pasta(const char *pasta, char *erro, size_t nerro) {
    char p[CAMINHO_MAX];
    size_t n = strlen(pasta);
    if (n == 0 || n >= sizeof p) {
        errno = n == 0 ? EINVAL : ENAMETOOLONG;
        descreve(erro, nerro, n == 0 ? "pasta sem nome" : "caminho comprido demais");
        return false;
    }
    memcpy(p, pasta, n + 1);
    while (n > 1 && SEPARADOR(p[n - 1])) p[--n] = 0;       /* "a/b/" vale "a/b"; a raiz "/" fica */
    /* as de cima, da raiz para baixo; as que já existem (ou que não se pode criar, como "C:") não são erro aqui */
    for (size_t i = 1; i < n; i++) {
        if (!SEPARADOR(p[i]) || SEPARADOR(p[i - 1])) continue;
        char c = p[i];
        p[i] = 0;
        if (!e_pasta(p)) FAZ_PASTA(p);
        p[i] = c;
    }
    if (e_pasta(p)) return true;
    if (FAZ_PASTA(p) != 0 && !e_pasta(p)) { descreve(erro, nerro, "não criou a pasta"); return false; }
    return true;
}

long gravar_ler_pequeno(const char *caminho, char *buf, size_t max) {
    FILE *f = fopen(caminho, "rb");
    if (!f) return -1;
    size_t len = fread(buf, 1, max, f);
    bool mais = len == max && fgetc(f) != EOF;              /* passou do tamanho de um arquivo do jogo */
    bool erro_de_leitura = ferror(f) != 0;
    int guarda = errno;
    fclose(f);
    errno = guarda;
    buf[len] = 0;
    if (erro_de_leitura) return -3;
    if (mais) return -2;
    return (long)len;
}
