#include "opcoes.h"

#include "ajuste.h"
#include "gravar.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MS_POR_SEGUNDO 1000

static int limite_ms(void) { return (int)lroundf(AJ_LATENCIA_MAX * MS_POR_SEGUNDO); }

static int dentro(int ms) {
    int max = limite_ms();
    return ms < 0 ? 0 : (ms > max ? max : ms);
}

void opcoes_formatar(const Opcoes *o, char *out, size_t n) {
    snprintf(out, n, "atraso_video_ms %d\natraso_audio_ms %d\n", dentro(o->atrasoVideoMs), dentro(o->atrasoAudioMs));
}

/* Consome a palavra `nome` de `*p` (depois de espaços). */
static bool palavra(const char **p, const char *nome) {
    const char *s = *p;
    while (isspace((unsigned char)*s)) s++;
    size_t n = strlen(nome);
    if (strncmp(s, nome, n) != 0 || (s[n] && !isspace((unsigned char)s[n]))) return false;
    *p = s + n;
    return true;
}

/* Consome um inteiro (com sinal opcional) de `*p`. false se não há, se não cabe num int ou se tem lixo colado. */
static bool inteiro(const char **p, int *out) {
    const char *s = *p;
    while (isspace((unsigned char)*s)) s++;
    errno = 0;
    char *fim;
    long v = strtol(s, &fim, 10);
    if (fim == s || errno == ERANGE || v < INT_MIN || v > INT_MAX) return false;
    if (*fim && !isspace((unsigned char)*fim)) return false;
    *p = fim;
    *out = (int)v;
    return true;
}

bool opcoes_interpretar(const char *texto, Opcoes *o) {
    const char *p = texto;
    int video, audio;
    if (!palavra(&p, "atraso_video_ms") || !inteiro(&p, &video)) return false;
    if (!palavra(&p, "atraso_audio_ms") || !inteiro(&p, &audio)) return false;
    while (isspace((unsigned char)*p)) p++;
    if (*p) return false;                                   /* lixo depois do último número */
    o->atrasoVideoMs = dentro(video);
    o->atrasoAudioMs = dentro(audio);
    return true;
}

bool opcoes_gravar(const char *caminho, const Opcoes *o, char *erro, size_t n) {
    char texto[OPCOES_MAX_BYTES];
    opcoes_formatar(o, texto, sizeof texto);
    return gravar_atomico(caminho, texto, strlen(texto), erro, n);
}

OpcoesLeitura opcoes_ler(const char *caminho, Opcoes *o, char *aviso, size_t n) {
    char buf[OPCOES_MAX_BYTES + 1];
    long len = gravar_ler_pequeno(caminho, buf, OPCOES_MAX_BYTES);
    if (len == -1) {
        if (errno == ENOENT) return OPCOES_NAO_EXISTE;
        snprintf(aviso, n, "não consegui abrir as opções: %s", strerror(errno));
        return OPCOES_ILEGIVEL;
    }
    if (len == -3) {
        snprintf(aviso, n, "não consegui ler as opções (%s)", strerror(errno));
        return OPCOES_ILEGIVEL;
    }
    Opcoes lidas;
    if (len >= 0 && memchr(buf, 0, (size_t)len) == NULL && opcoes_interpretar(buf, &lidas)) {
        *o = lidas;
        return OPCOES_OK;
    }
    if (gravar_guardar_bak(caminho)) snprintf(aviso, n, "as opções estavam corrompidas: cópia em %s.bak; uso a calibração padrão", gravar_nome(caminho));
    else snprintf(aviso, n, "as opções estavam corrompidas e não consegui guardar a cópia: uso a calibração padrão");
    return OPCOES_CORROMPIDA;
}
