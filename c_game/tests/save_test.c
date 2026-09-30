/*
 * save_test.c - o arquivo de progresso (src/salvar.c), sem janela nem raylib. make test-save
 * Cobre: o formato, a validação de cada campo, os oito saves estragados da caça de bugs, o .bak, a
 * gravação atômica (.tmp + rename) e os jeitos de a gravação falhar sem estragar o save antigo.
 */
#define _POSIX_C_SOURCE 200809L
#include "../src/salvar.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "portavel.h"

static int checks = 0, failures = 0;
#define CHECK(cond, ...) do { \
    checks++; \
    if (!(cond)) { failures++; printf("FALHA %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } \
} while (0)

static char dir[256], caminho[300], bak[310], tmp[310];

static void escreve(const char *path, const char *conteudo, size_t n) {
    FILE *f = fopen(path, "wb");
    if (f) { fwrite(conteudo, 1, n, f); fclose(f); }
}

static int le(const char *path, char *out, size_t max) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    size_t n = fread(out, 1, max - 1, f);
    fclose(f);
    out[n] = 0;
    return (int)n;
}

static int existe(const char *path) { struct stat st; return stat(path, &st) == 0; }

static void limpa(void) {
    remove(caminho); remove(bak); remove(tmp);
    rmdir(caminho); rmdir(tmp);
}

/* Os oito saves da caça de bugs, e mais alguns jeitos de estragar. */
typedef struct { const char *nome, *texto; int valido; } Caso;
static const Caso CASOS[] = {
    {"ok",             "APARA-C 2\n5 31 0 1\n", 1},
    {"vazio",          "", 0},
    {"lixo",           "ZZZ\n", 0},
    {"truncado",       "APARA-C 2\n5 31", 0},
    {"fora da trilha", "APARA-C 2\n99 0 0 1\n", 0},
    {"negativo",       "APARA-C 2\n-3 0 0 1\n", 0},
    {"máscara e flags","APARA-C 2\n5 4294967295 7 9\n", 0},
    {"versão 1",       "APARA-C 1\n5 31 0 1\n", 0},
    {"máscara com bit de mestre que não existe", "APARA-C 2\n5 8192 0 1\n", 0},
    {"máscara cheia (os treze)", "APARA-C 2\n12 8191 1 1\n", 1},
    {"máscara negativa", "APARA-C 2\n5 -1 0 1\n", 0},
    {"completa com o mestre no meio da trilha", "APARA-C 2\n5 31 1 1\n", 0},
    {"abertura vista = 2", "APARA-C 2\n5 31 0 2\n", 0},
    {"número que estoura", "APARA-C 2\n5 99999999999999999999999 0 1\n", 0},
    {"lixo depois dos números", "APARA-C 2\n5 31 0 1\nextra\n", 0},
    {"lixo colado no número", "APARA-C 2\n5 31x 0 1\n", 0},
    {"linhas do Windows (CRLF)", "APARA-C 2\r\n5 31 0 1\r\n", 1},
    {"sem a linha de versão", "5 31 0 1\n", 0},
    {"campo faltando", "APARA-C 2\n5 31 0\n", 0},
};

static void teste_interpretar(void) {
    for (unsigned i = 0; i < sizeof CASOS / sizeof CASOS[0]; i++) {
        Campaign c;
        memset(&c, 0x7F, sizeof c);
        bool ok = save_interpretar(CASOS[i].texto, &c);
        CHECK(ok == (CASOS[i].valido != 0), "interpretar \"%s\": esperado %s", CASOS[i].nome, CASOS[i].valido ? "válido" : "inválido");
    }
    Campaign c;
    CHECK(save_interpretar("APARA-C 2\n5 31 0 1\n", &c) && c.index == 5 && c.clearedMask == 31 && !c.completed && c.loreSeen, "os campos saem no lugar certo");
    /* um save inválido não mexe na trilha de quem chamou */
    campaign_reset(&c);
    c.index = 3;
    CHECK(!save_interpretar("APARA-C 2\n99 0 0 1\n", &c) && c.index == 3, "um save inválido deixa a trilha como estava");
}

static uint64_t rs = 88172645463325252ull;
static unsigned rnd(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return (unsigned)(rs >> 16); }

static void teste_ida_e_volta(void) {
    for (int k = 0; k < 20000; k++) {
        Campaign a, b;
        a.index = (int)(rnd() % ROSTER_SIZE);
        a.clearedMask = rnd() & ((1u << ROSTER_SIZE) - 1);
        a.completed = a.index == ROSTER_SIZE - 1 && (rnd() & 1);
        a.loreSeen = rnd() & 1;
        char t[96];
        save_formatar(&a, t, sizeof t);
        bool ok = save_interpretar(t, &b);
        if (!ok || b.index != a.index || b.clearedMask != a.clearedMask || b.completed != a.completed || b.loreSeen != a.loreSeen) {
            CHECK(0, "ida e volta falhou para %d %u %d %d (\"%s\")", a.index, a.clearedMask, a.completed, a.loreSeen, t);
            return;
        }
    }
    CHECK(1, "ida e volta");
}

static void teste_gravar_e_ler(void) {
    limpa();
    Campaign a = {.index = 4, .clearedMask = 15, .completed = false, .loreSeen = true}, b;
    char erro[200] = "", aviso[200] = "", texto[96], lido[300];
    CHECK(save_ler(caminho, &b, aviso, sizeof aviso) == SAVE_NAO_EXISTE, "sem arquivo: a primeira vez não é erro");
    CHECK(save_gravar(caminho, &a, erro, sizeof erro), "gravar (%s)", erro);
    save_formatar(&a, texto, sizeof texto);
    CHECK(le(caminho, lido, sizeof lido) > 0 && strcmp(lido, texto) == 0, "o arquivo tem o texto do formato");
    CHECK(!existe(tmp), "o .tmp não sobra depois de gravar");
    CHECK(save_ler(caminho, &b, aviso, sizeof aviso) == SAVE_OK && b.index == 4 && b.clearedMask == 15 && b.loreSeen, "ler devolve o que se gravou");
    a.index = 5; a.clearedMask = 31;
    CHECK(save_gravar(caminho, &a, erro, sizeof erro), "gravar por cima do save antigo (%s)", erro);
    CHECK(save_ler(caminho, &b, aviso, sizeof aviso) == SAVE_OK && b.index == 5 && b.clearedMask == 31, "o save novo substituiu o antigo");
    CHECK(!existe(bak), "um save bom não gera .bak");
}

static void teste_falhas_de_gravacao(void) {
    limpa();
    Campaign a = {.index = 2, .clearedMask = 3, .completed = false, .loreSeen = true}, b;
    char erro[200] = "", aviso[200] = "", antes[300], depois[300];
    CHECK(save_gravar(caminho, &a, erro, sizeof erro), "save bom para começar");
    le(caminho, antes, sizeof antes);

    /* não dá para criar o .tmp (é uma pasta): falha, e o save antigo fica como estava */
    mkdir(tmp, 0755);
    Campaign n = {.index = 9, .clearedMask = 511, .completed = false, .loreSeen = true};
    erro[0] = 0;
    CHECK(!save_gravar(caminho, &n, erro, sizeof erro) && erro[0], "com o .tmp impossível: falha e diz por quê (\"%s\")", erro);
    le(caminho, depois, sizeof depois);
    CHECK(strcmp(antes, depois) == 0, "e o save antigo continua intacto");
    CHECK(save_ler(caminho, &b, aviso, sizeof aviso) == SAVE_OK && b.index == 2, "e ainda carrega");
    rmdir(tmp);

    /* o destino é uma pasta: o rename falha, o .tmp é apagado */
    remove(caminho);
    mkdir(caminho, 0755);
    erro[0] = 0;
    CHECK(!save_gravar(caminho, &n, erro, sizeof erro) && erro[0], "com o destino impossível: falha e diz por quê (\"%s\")", erro);
    CHECK(!existe(tmp), "o .tmp não sobra quando a troca falha");
    aviso[0] = 0;
    CHECK(save_ler(caminho, &b, aviso, sizeof aviso) == SAVE_ILEGIVEL && aviso[0], "uma pasta no lugar do save: ilegível, com aviso (\"%s\")", aviso);
    CHECK(existe(caminho) && !existe(bak), "e nada é movido para .bak");
    rmdir(caminho);

    /* pasta que não existe */
    char longe[400];
    snprintf(longe, sizeof longe, "%s/nao/existe/save.txt", dir);
    erro[0] = 0;
    CHECK(!save_gravar(longe, &n, erro, sizeof erro) && erro[0], "pasta inexistente: falha e diz por quê (\"%s\")", erro);

    /* uma queda no meio de uma gravação deixou um .tmp pela metade: nada muda para o jogo */
    limpa();
    CHECK(save_gravar(caminho, &a, erro, sizeof erro), "save bom");
    escreve(tmp, "APARA-C 2\n9 5", 13);
    CHECK(save_ler(caminho, &b, aviso, sizeof aviso) == SAVE_OK && b.index == 2, "um .tmp de uma queda antiga não atrapalha a leitura");
    CHECK(save_gravar(caminho, &n, erro, sizeof erro) && save_ler(caminho, &b, aviso, sizeof aviso) == SAVE_OK && b.index == 9, "e a próxima gravação passa por cima dele");
    CHECK(!existe(tmp), "sem sobras");
}

static void teste_corrompidos(void) {
    for (unsigned i = 0; i < sizeof CASOS / sizeof CASOS[0]; i++) {
        limpa();
        size_t n = strlen(CASOS[i].texto);
        escreve(caminho, CASOS[i].texto, n);
        Campaign c;
        char aviso[200] = "", lido[300];
        SaveLeitura r = save_ler(caminho, &c, aviso, sizeof aviso);
        if (CASOS[i].valido) {
            CHECK(r == SAVE_OK && !existe(bak), "\"%s\": lê e não mexe no arquivo", CASOS[i].nome);
            continue;
        }
        CHECK(r == SAVE_CORROMPIDO && aviso[0], "\"%s\": corrompido, com aviso ao jogador", CASOS[i].nome);
        CHECK(!existe(caminho) && le(bak, lido, sizeof lido) == (int)n && memcmp(lido, CASOS[i].texto, n) == 0, "\"%s\": o arquivo estragado foi guardado, byte a byte, no .bak", CASOS[i].nome);
    }
    /* um segundo estragado substitui o .bak do primeiro (o mais recente é o que importa) */
    limpa();
    escreve(caminho, "primeiro", 8);
    Campaign c;
    char aviso[200], lido[300];
    save_ler(caminho, &c, aviso, sizeof aviso);
    escreve(caminho, "segundo lixo", 12);
    CHECK(save_ler(caminho, &c, aviso, sizeof aviso) == SAVE_CORROMPIDO && le(bak, lido, sizeof lido) == 12 && strcmp(lido, "segundo lixo") == 0, "o .bak guarda o estragado mais recente");
    /* arquivo enorme e byte zero no meio */
    limpa();
    char *grande = malloc(1 << 20);
    memset(grande, 'A', 1 << 20);
    escreve(caminho, grande, 1 << 20);
    CHECK(save_ler(caminho, &c, aviso, sizeof aviso) == SAVE_CORROMPIDO, "um arquivo de 1 MB é lixo, sem ler tudo");
    free(grande);
    limpa();
    escreve(caminho, "APARA-C 2\n5 31 0 1\n\0lixo", 24);
    CHECK(save_ler(caminho, &c, aviso, sizeof aviso) == SAVE_CORROMPIDO, "byte zero no meio: corrompido");
    limpa();
}

/* Uma pasta nova só deste processo. Não usa mkdtemp: com -std=c11 e _POSIX_C_SOURCE o macOS não o declara (o make teste
 * parava aqui no Mac); mkdir com o pid no nome (e uma volta se a pasta já existir) funciona em qualquer POSIX. */
static bool pasta_temporaria(const char *base) {
    for (int n = 0; n < 1000; n++) {
        snprintf(dir, sizeof dir, "%s/apara_save_test_%ld_%d", base, (long)getpid(), n);
        if (mkdir(dir, 0700) == 0) return true;
        if (errno != EEXIST) return false;
    }
    return false;
}

int main(void) {
    const char *base = pasta_do_sistema();
    if (!pasta_temporaria(base)) { perror("mkdir"); return 2; }
    snprintf(caminho, sizeof caminho, "%s/apara_save.txt", dir);
    snprintf(bak, sizeof bak, "%s.bak", caminho);
    snprintf(tmp, sizeof tmp, "%s.tmp", caminho);

    teste_interpretar();
    teste_ida_e_volta();
    teste_gravar_e_ler();
    teste_falhas_de_gravacao();
    teste_corrompidos();

    limpa();
    rmdir(dir);
    printf("save: %d verificações, %d falhas\n", checks, failures);
    return failures ? 1 : 0;
}
