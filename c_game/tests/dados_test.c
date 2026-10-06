/*
 * dados_test.c - os arquivos do jogador fora da pasta do jogo (src/gravar.c, src/pasta_dados.c, src/opcoes.c),
 * sem janela nem raylib. make test-dados
 * Cobre: criar pastas (as de cima e as que já existem), a gravação atômica e seus jeitos de falhar, onde fica a pasta de
 * dados em cada sistema (com e sem cada variável de ambiente), a migração do save e das opções que ficavam ao lado
 * do executável (copia, não sobrescreve, não toca o original) e o arquivo de opções (formato, limites, estragado).
 */
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE 1
#endif
#include "../src/gravar.h"
#include "../src/opcoes.h"
#include "../src/pasta_dados.h"
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

static char raiz[256];

static void escreve(const char *path, const char *conteudo) {
    FILE *f = fopen(path, "wb");
    if (f) { fputs(conteudo, f); fclose(f); }
}

static int le(const char *path, char *out, size_t max) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    size_t n = fread(out, 1, max - 1, f);
    fclose(f);
    out[n] = 0;
    return (int)n;
}

static bool e_pasta(const char *path) { struct stat st; return stat(path, &st) == 0 && S_ISDIR(st.st_mode); }

static const char *sub(const char *nome) {
    static char buf[8][512];
    static int i = 0;
    char *b = buf[i++ % 8];
    snprintf(b, sizeof buf[0], "%s/%s", raiz, nome);
    return b;
}

/* ------------------------------------------------------------------ */

static void teste_pastas(void) {
    char erro[200] = "";
    const char *fundo = sub("a/b/c");
    CHECK(gravar_pasta(fundo, erro, sizeof erro) && e_pasta(fundo), "cria a pasta e as de cima (%s)", erro);
    CHECK(gravar_pasta(fundo, erro, sizeof erro), "uma pasta que já existe não é erro");
    char com_barra[300];
    snprintf(com_barra, sizeof com_barra, "%s/", fundo);
    CHECK(gravar_pasta(com_barra, erro, sizeof erro), "uma barra no fim vale a mesma pasta");
    char dupla[300];
    snprintf(dupla, sizeof dupla, "%s/x//y", raiz);
    CHECK(gravar_pasta(dupla, erro, sizeof erro) && e_pasta(sub("x/y")), "barra dobrada no meio");

    /* um arquivo no caminho: não dá para criar a pasta, e o erro diz isso */
    escreve(sub("arquivo.txt"), "oi");
    char dentro[300];
    snprintf(dentro, sizeof dentro, "%s/filho", sub("arquivo.txt"));
    erro[0] = 0;
    CHECK(!gravar_pasta(sub("arquivo.txt"), erro, sizeof erro) && erro[0], "o nome é de um arquivo: falha com motivo (\"%s\")", erro);
    erro[0] = 0;
    CHECK(!gravar_pasta(dentro, erro, sizeof erro) && erro[0], "uma pasta dentro de um arquivo: falha com motivo (\"%s\")", erro);
    erro[0] = 0;
    CHECK(!gravar_pasta("", erro, sizeof erro) && erro[0], "sem nome: falha (\"%s\")", erro);
    char longo[CAMINHO_MAX + 50];
    memset(longo, 'a', sizeof longo - 1);
    longo[sizeof longo - 1] = 0;
    erro[0] = 0;
    CHECK(!gravar_pasta(longo, erro, sizeof erro) && erro[0], "caminho comprido demais: falha (\"%s\")", erro);

    CHECK(gravar_existe(sub("arquivo.txt")) && gravar_existe(fundo) && !gravar_existe(sub("nao_existe")), "gravar_existe");
}

static void teste_ler_pequeno(void) {
    char buf[65];
    escreve(sub("pequeno.txt"), "abc\n");
    CHECK(gravar_ler_pequeno(sub("pequeno.txt"), buf, 64) == 4 && strcmp(buf, "abc\n") == 0, "lê o arquivo inteiro e termina em 0");
    escreve(sub("vazio.txt"), "");
    CHECK(gravar_ler_pequeno(sub("vazio.txt"), buf, 64) == 0 && buf[0] == 0, "arquivo vazio: 0 bytes");
    errno = 0;
    CHECK(gravar_ler_pequeno(sub("nao_existe.txt"), buf, 64) == -1 && errno == ENOENT, "sem arquivo: -1 com ENOENT");
    char exato[65], mais[66];
    memset(exato, 'x', 64); exato[64] = 0;
    memset(mais, 'x', 65); mais[65] = 0;
    escreve(sub("exato.txt"), exato);
    escreve(sub("mais.txt"), mais);
    CHECK(gravar_ler_pequeno(sub("exato.txt"), buf, 64) == 64, "do tamanho do limite: cabe");
    CHECK(gravar_ler_pequeno(sub("mais.txt"), buf, 64) == -2, "um byte além do limite: lixo (-2)");
    long r = gravar_ler_pequeno(sub("a"), buf, 64);       /* uma pasta: não abre (Windows) ou não lê (POSIX) */
    CHECK(r == -1 || r == -3, "uma pasta não se lê (%ld)", r);
}

static void teste_atomico(void) {
    char erro[200] = "", lido[100];
    const char *c = sub("grava.txt");
    CHECK(gravar_atomico(c, "um\n", 3, erro, sizeof erro), "grava (%s)", erro);
    CHECK(le(c, lido, sizeof lido) == 3 && strcmp(lido, "um\n") == 0, "o conteúdo é o gravado");
    CHECK(!gravar_existe(sub("grava.txt.tmp")), "sem .tmp sobrando");
    CHECK(gravar_atomico(c, "dois\n", 5, erro, sizeof erro) && le(c, lido, sizeof lido) == 5 && strcmp(lido, "dois\n") == 0, "grava por cima");
    CHECK(gravar_atomico(c, "", 0, erro, sizeof erro) && le(c, lido, sizeof lido) == 0, "zero bytes: arquivo vazio, sem erro");
    CHECK(gravar_atomico(c, "com\0meio", 8, erro, sizeof erro) && le(c, lido, sizeof lido) == 8 && memcmp(lido, "com\0meio", 8) == 0, "bytes zero no meio passam (é binário)");

    /* falhas: o arquivo antigo fica como estava, e o erro diz o motivo */
    gravar_atomico(c, "bom\n", 4, erro, sizeof erro);
    erro[0] = 0;
    CHECK(!gravar_atomico(sub("nao/existe/g.txt"), "x", 1, erro, sizeof erro) && erro[0], "pasta inexistente: falha com motivo (\"%s\")", erro);
    mkdir(sub("grava.txt.tmp"), 0755);
    erro[0] = 0;
    CHECK(!gravar_atomico(c, "novo\n", 5, erro, sizeof erro) && erro[0], "o .tmp não abre: falha com motivo (\"%s\")", erro);
    CHECK(le(c, lido, sizeof lido) == 4 && strcmp(lido, "bom\n") == 0, "e o arquivo antigo continua intacto");
    rmdir(sub("grava.txt.tmp"));
    mkdir(sub("destino_pasta"), 0755);
    erro[0] = 0;
    CHECK(!gravar_atomico(sub("destino_pasta"), "x", 1, erro, sizeof erro) && erro[0] && !gravar_existe(sub("destino_pasta.tmp")), "o destino é uma pasta: falha e não sobra .tmp (\"%s\")", erro);
    char longo[CAMINHO_MAX + 10];
    memset(longo, 'a', sizeof longo - 1);
    longo[sizeof longo - 1] = 0;
    erro[0] = 0;
    CHECK(!gravar_atomico(longo, "x", 1, erro, sizeof erro) && strstr(erro, "comprido"), "caminho comprido demais (\"%s\")", erro);
    CHECK(gravar_atomico(c, "x", 1, NULL, 0), "sem buffer de erro também funciona");
}

static void teste_nome(void) {
    CHECK(strcmp(gravar_nome("/a/b/apara_save.txt"), "apara_save.txt") == 0, "nome: caminho POSIX");
    CHECK(strcmp(gravar_nome("apara_save.txt"), "apara_save.txt") == 0, "nome: sem pasta");
    CHECK(strcmp(gravar_nome("/apara_save.txt"), "apara_save.txt") == 0, "nome: na raiz");
    CHECK(strcmp(gravar_nome(""), "") == 0, "nome: vazio");
    CHECK(strcmp(gravar_nome("/a/b/"), "") == 0, "nome: caminho de pasta (barra no fim)");
#ifdef _WIN32
    CHECK(strcmp(gravar_nome("C:\\Users\\k\\Apara\\apara_save.txt"), "apara_save.txt") == 0, "nome: caminho do Windows");
#endif
}

static void teste_bak(void) {
    char lido[100];
    escreve(sub("ruim.txt"), "lixo");
    CHECK(gravar_guardar_bak(sub("ruim.txt")) && !gravar_existe(sub("ruim.txt")) && le(sub("ruim.txt.bak"), lido, sizeof lido) == 4, "o estragado vai para .bak");
    escreve(sub("ruim.txt"), "lixo novo");
    CHECK(gravar_guardar_bak(sub("ruim.txt")) && le(sub("ruim.txt.bak"), lido, sizeof lido) == 9 && strcmp(lido, "lixo novo") == 0, "o .bak seguinte substitui o anterior");
    CHECK(!gravar_guardar_bak(sub("nao_existe.txt")), "sem arquivo, sem .bak");
}

/* ------------------------------------------------------------------ */

typedef struct {
    const char *nome;
    AmbienteDados amb;
    const char *esperado;               /* NULL: não acha pasta */
} CasoPasta;

static void teste_pasta_de_dados(void) {
    static const CasoPasta CASOS[] = {
        {"Windows: LOCALAPPDATA", {SISTEMA_WINDOWS, NULL, "C:\\Users\\k\\AppData\\Local", "C:\\Users\\k\\AppData\\Roaming", "C:\\Users\\k", NULL, NULL}, "C:\\Users\\k\\AppData\\Local\\Apara"},
        {"Windows: sem LOCALAPPDATA, usa APPDATA", {SISTEMA_WINDOWS, NULL, NULL, "C:\\Users\\k\\AppData\\Roaming", "C:\\Users\\k", NULL, NULL}, "C:\\Users\\k\\AppData\\Roaming\\Apara"},
        {"Windows: LOCALAPPDATA vazia", {SISTEMA_WINDOWS, NULL, "", "C:\\Users\\k\\AppData\\Roaming", NULL, NULL, NULL}, "C:\\Users\\k\\AppData\\Roaming\\Apara"},
        {"Windows: só USERPROFILE", {SISTEMA_WINDOWS, NULL, NULL, NULL, "C:\\Users\\k", NULL, NULL}, "C:\\Users\\k\\AppData\\Local\\Apara"},
        {"Windows: barra no fim da variável", {SISTEMA_WINDOWS, NULL, "C:\\Users\\k\\AppData\\Local\\", NULL, NULL, NULL, NULL}, "C:\\Users\\k\\AppData\\Local\\Apara"},
        {"Windows: nenhuma variável", {SISTEMA_WINDOWS, NULL, NULL, NULL, NULL, NULL, NULL}, NULL},
        {"Windows: nome com espaço e acento", {SISTEMA_WINDOWS, NULL, "C:\\Users\\José da Silva\\AppData\\Local", NULL, NULL, NULL, NULL}, "C:\\Users\\José da Silva\\AppData\\Local\\Apara"},
        {"macOS: HOME", {SISTEMA_MAC, NULL, NULL, NULL, NULL, "/Users/k", NULL}, "/Users/k/Library/Application Support/Apara"},
        {"macOS: HOME com barra no fim", {SISTEMA_MAC, NULL, NULL, NULL, NULL, "/Users/k/", NULL}, "/Users/k/Library/Application Support/Apara"},
        {"macOS: sem HOME", {SISTEMA_MAC, NULL, NULL, NULL, NULL, NULL, NULL}, NULL},
        {"Linux: XDG_DATA_HOME", {SISTEMA_LINUX, NULL, NULL, NULL, NULL, "/home/k", "/dados/k"}, "/dados/k/apara"},
        {"Linux: sem XDG, usa HOME", {SISTEMA_LINUX, NULL, NULL, NULL, NULL, "/home/k", NULL}, "/home/k/.local/share/apara"},
        {"Linux: XDG relativa não vale (diz a especificação)", {SISTEMA_LINUX, NULL, NULL, NULL, NULL, "/home/k", "dados"}, "/home/k/.local/share/apara"},
        {"Linux: XDG vazia", {SISTEMA_LINUX, NULL, NULL, NULL, NULL, "/home/k", ""}, "/home/k/.local/share/apara"},
        {"Linux: nenhuma variável", {SISTEMA_LINUX, NULL, NULL, NULL, NULL, NULL, NULL}, NULL},
        {"Linux: HOME na raiz", {SISTEMA_LINUX, NULL, NULL, NULL, NULL, "/", NULL}, "/.local/share/apara"},
        {"APARA_DADOS manda no Linux", {SISTEMA_LINUX, "/tmp/meus", NULL, NULL, NULL, "/home/k", "/dados/k"}, "/tmp/meus"},
        {"APARA_DADOS manda no Windows", {SISTEMA_WINDOWS, "D:\\jogos\\dados\\", "C:\\x", NULL, NULL, NULL, NULL}, "D:\\jogos\\dados"},
        {"APARA_DADOS manda mesmo sem nenhuma variável", {SISTEMA_MAC, "/tmp/meus/", NULL, NULL, NULL, NULL, NULL}, "/tmp/meus"},
        {"APARA_DADOS vazia não vale", {SISTEMA_LINUX, "", NULL, NULL, NULL, "/home/k", NULL}, "/home/k/.local/share/apara"},
        {"APARA_DADOS é a raiz", {SISTEMA_LINUX, "/", NULL, NULL, NULL, NULL, NULL}, "/"},
    };
    for (unsigned i = 0; i < sizeof CASOS / sizeof CASOS[0]; i++) {
        char out[CAMINHO_MAX] = "sobra";
        bool achou = pasta_dados_caminho(&CASOS[i].amb, out, sizeof out);
        if (CASOS[i].esperado) CHECK(achou && strcmp(out, CASOS[i].esperado) == 0, "%s: esperado \"%s\", saiu \"%s\"", CASOS[i].nome, CASOS[i].esperado, achou ? out : "(nada)");
        else CHECK(!achou, "%s: não devia achar pasta, achou \"%s\"", CASOS[i].nome, out);
    }

    /* não cabe no buffer: falha em vez de cortar o caminho no meio */
    AmbienteDados a = {SISTEMA_LINUX, NULL, NULL, NULL, NULL, "/home/k", NULL};
    char curto[10];
    CHECK(!pasta_dados_caminho(&a, curto, sizeof curto), "buffer curto: falha");
    CHECK(!pasta_dados_caminho(&a, curto, 0), "buffer de zero bytes: falha");
    char justo[] = "/home/k/.local/share/apara";
    char cabe[sizeof justo], nao_cabe[sizeof justo - 1];
    CHECK(pasta_dados_caminho(&a, cabe, sizeof cabe) && strcmp(cabe, justo) == 0, "cabe no byte");
    CHECK(!pasta_dados_caminho(&a, nao_cabe, sizeof nao_cabe), "falta um byte: falha");

    char arq[CAMINHO_MAX];
    CHECK(pasta_dados_arquivo(SISTEMA_WINDOWS, "C:\\a\\Apara", "apara_save.txt", arq, sizeof arq) && strcmp(arq, "C:\\a\\Apara\\apara_save.txt") == 0, "arquivo no Windows: barra invertida");
    CHECK(pasta_dados_arquivo(SISTEMA_LINUX, "/a/apara/", "apara_save.txt", arq, sizeof arq) && strcmp(arq, "/a/apara/apara_save.txt") == 0, "arquivo no Linux: sem barra dobrada");
    CHECK(pasta_dados_arquivo(SISTEMA_WINDOWS, "C:/a/Apara/", "x", arq, sizeof arq) && strcmp(arq, "C:/a/Apara\\x") == 0, "o Windows aceita barra normal na pasta");
    CHECK(pasta_dados_arquivo(SISTEMA_LINUX, "/", "x", arq, sizeof arq) && strcmp(arq, "/x") == 0, "arquivo na raiz");
    CHECK(!pasta_dados_arquivo(SISTEMA_LINUX, "/a/apara", "apara_save.txt", arq, 10), "arquivo que não cabe: falha");

    /* o ambiente de verdade: ou acha uma pasta absoluta, ou diz que não acha (sem ler lixo) */
    AmbienteDados real = pasta_dados_ambiente();
    char out[CAMINHO_MAX];
    if (pasta_dados_caminho(&real, out, sizeof out)) CHECK(out[0] != 0, "o ambiente de verdade dá uma pasta");
}

/* ------------------------------------------------------------------ */

/* Os avisos vão para uma faixa pequena do jogo: citam o arquivo, nunca a pasta (que pode ser comprida). */
static void teste_avisos_curtos(void) {
    char aviso[300] = "";
    Campaign c;
    escreve(sub("apara_save.txt"), "lixo");
    CHECK(save_ler(sub("apara_save.txt"), &c, aviso, sizeof aviso) == SAVE_CORROMPIDO, "save estragado");
    CHECK(strstr(aviso, "apara_save.txt.bak") && !strstr(aviso, raiz), "o aviso do save cita o .bak e não a pasta (\"%s\")", aviso);
    remove(sub("apara_save.txt.bak"));
    mkdir(sub("apara_save.txt"), 0755);
    aviso[0] = 0;
    CHECK(save_ler(sub("apara_save.txt"), &c, aviso, sizeof aviso) == SAVE_ILEGIVEL && !strstr(aviso, raiz), "o aviso do save ilegível não cita a pasta (\"%s\")", aviso);
    rmdir(sub("apara_save.txt"));

    Opcoes o;
    escreve(sub("apara_opcoes.txt"), "lixo");
    CHECK(opcoes_ler(sub("apara_opcoes.txt"), &o, aviso, sizeof aviso) == OPCOES_CORROMPIDA, "opções estragadas");
    CHECK(strstr(aviso, "apara_opcoes.txt.bak") && !strstr(aviso, raiz), "o aviso das opções cita o .bak e não a pasta (\"%s\")", aviso);
    remove(sub("apara_opcoes.txt.bak"));
}

static bool valido_save(const char *t) { Campaign c; return save_interpretar(t, &c); }

static void teste_migracao(void) {
    char aviso[256] = "", lido[100], erro[100];
    const char *bom = "APARA-C 2\n5 31 0 1\n";
    gravar_pasta(sub("ao_lado"), erro, sizeof erro);
    gravar_pasta(sub("dados"), erro, sizeof erro);

    /* nada a migrar: sem arquivo antigo */
    CHECK(pasta_dados_migrar(sub("ao_lado/s.txt"), sub("dados/s.txt"), valido_save, SAVE_MAX_BYTES, aviso, sizeof aviso) == MIGROU_NADA && !gravar_existe(sub("dados/s.txt")), "sem arquivo antigo: nada");

    /* o caso de verdade: copia, e o original fica */
    escreve(sub("ao_lado/s.txt"), bom);
    CHECK(pasta_dados_migrar(sub("ao_lado/s.txt"), sub("dados/s.txt"), valido_save, SAVE_MAX_BYTES, aviso, sizeof aviso) == MIGROU_COPIOU, "copia o save antigo (%s)", aviso);
    CHECK(le(sub("dados/s.txt"), lido, sizeof lido) > 0 && strcmp(lido, bom) == 0, "a cópia é igual byte a byte");
    CHECK(le(sub("ao_lado/s.txt"), lido, sizeof lido) > 0 && strcmp(lido, bom) == 0, "o original continua lá, intacto");
    CHECK(!gravar_existe(sub("dados/s.txt.tmp")), "sem .tmp sobrando");
    Campaign c;
    CHECK(save_ler(sub("dados/s.txt"), &c, aviso, sizeof aviso) == SAVE_OK && c.index == 5 && c.clearedMask == 31, "o jogo carrega a cópia como o save dele");

    /* o novo manda: não se sobrescreve, nem se o antigo mudou */
    gravar_atomico(sub("dados/s.txt"), "APARA-C 2\n9 511 0 1\n", 19, erro, sizeof erro);
    CHECK(pasta_dados_migrar(sub("ao_lado/s.txt"), sub("dados/s.txt"), valido_save, SAVE_MAX_BYTES, aviso, sizeof aviso) == MIGROU_NADA, "já existe o novo: nada");
    CHECK(le(sub("dados/s.txt"), lido, sizeof lido) > 0 && strncmp(lido, "APARA-C 2\n9 511", 15) == 0, "e o novo não foi tocado");
    /* um novo estragado também não é sobrescrito: quem trata isso é o save_ler (.bak + aviso) */
    escreve(sub("dados/estragado.txt"), "lixo");
    CHECK(pasta_dados_migrar(sub("ao_lado/s.txt"), sub("dados/estragado.txt"), valido_save, SAVE_MAX_BYTES, aviso, sizeof aviso) == MIGROU_NADA, "um novo estragado também não é sobrescrito");

    /* o antigo não é um arquivo do jogo: não copia, não toca */
    const char *ruins[] = {"", "lixo\n", "APARA-C 2\n99 0 0 1\n", "APARA-C 1\n5 31 0 1\n", "APARA-C 2\n5 31 0 1\nextra\n"};
    for (unsigned i = 0; i < sizeof ruins / sizeof ruins[0]; i++) {
        remove(sub("dados/r.txt"));
        escreve(sub("ao_lado/r.txt"), ruins[i]);
        CHECK(pasta_dados_migrar(sub("ao_lado/r.txt"), sub("dados/r.txt"), valido_save, SAVE_MAX_BYTES, aviso, sizeof aviso) == MIGROU_INVALIDO, "antigo inválido #%u: não copia", i);
        CHECK(!gravar_existe(sub("dados/r.txt")), "antigo inválido #%u: nada na pasta nova", i);
        CHECK(le(sub("ao_lado/r.txt"), lido, sizeof lido) == (int)strlen(ruins[i]), "antigo inválido #%u: original intacto", i);
    }
    char grande[1000];
    memset(grande, '1', sizeof grande - 1);
    grande[sizeof grande - 1] = 0;
    escreve(sub("ao_lado/g.txt"), grande);
    CHECK(pasta_dados_migrar(sub("ao_lado/g.txt"), sub("dados/g.txt"), valido_save, SAVE_MAX_BYTES, aviso, sizeof aviso) == MIGROU_INVALIDO && !gravar_existe(sub("dados/g.txt")), "antigo enorme: lixo, não copia");
    gravar_atomico(sub("ao_lado/z.txt"), "APARA-C 2\n5 31 0 1\0lixo", 22, erro, sizeof erro);
    CHECK(pasta_dados_migrar(sub("ao_lado/z.txt"), sub("dados/z.txt"), valido_save, SAVE_MAX_BYTES, aviso, sizeof aviso) == MIGROU_INVALIDO, "byte zero no meio: lixo");
    /* o limite pedido acima do que a migração aceita (512) é cortado, não estoura o buffer */
    CHECK(pasta_dados_migrar(sub("ao_lado/g.txt"), sub("dados/g.txt"), valido_save, 1u << 20, aviso, sizeof aviso) == MIGROU_INVALIDO, "limite grande demais é cortado");

    /* o antigo é uma pasta: não dá para ler, e quem chama é avisado para seguir com ele */
    mkdir(sub("ao_lado/pasta.txt"), 0755);
    aviso[0] = 0;
    CHECK(pasta_dados_migrar(sub("ao_lado/pasta.txt"), sub("dados/pasta.txt"), valido_save, SAVE_MAX_BYTES, aviso, sizeof aviso) == MIGROU_FALHOU && aviso[0] && !strstr(aviso, raiz), "antigo ilegível: falha com aviso, sem a pasta (\"%s\")", aviso);

    /* a pasta nova não existe: não dá para gravar */
    aviso[0] = 0;
    CHECK(pasta_dados_migrar(sub("ao_lado/s.txt"), sub("nao/existe/s.txt"), valido_save, SAVE_MAX_BYTES, aviso, sizeof aviso) == MIGROU_FALHOU && aviso[0] && !strstr(aviso, raiz), "sem a pasta nova: falha com aviso, sem a pasta (\"%s\")", aviso);
    CHECK(le(sub("ao_lado/s.txt"), lido, sizeof lido) > 0 && strcmp(lido, bom) == 0, "e o original continua intacto");
}

/* ------------------------------------------------------------------ */

typedef struct { const char *nome, *texto; int valido, video, audio; } CasoOpcao;

static bool valido_opcoes(const char *t) { Opcoes o; return opcoes_interpretar(t, &o); }

static void teste_opcoes(void) {
    static const CasoOpcao CASOS[] = {
        {"o que o jogo grava",              "atraso_video_ms 40\natraso_audio_ms 25\n", 1, 40, 25},
        {"sem a quebra de linha no fim",    "atraso_video_ms 40\natraso_audio_ms 25", 1, 40, 25},
        {"linhas do Windows (CRLF)",        "atraso_video_ms 40\r\natraso_audio_ms 25\r\n", 1, 40, 25},
        {"na mesma linha (o fscanf antigo aceitava)", "atraso_video_ms 40 atraso_audio_ms 25", 1, 40, 25},
        {"zeros",                           "atraso_video_ms 0\natraso_audio_ms 0\n", 1, 0, 0},
        {"no limite",                       "atraso_video_ms 120\natraso_audio_ms 120\n", 1, 120, 120},
        {"acima do limite vale o limite",   "atraso_video_ms 500\natraso_audio_ms 121\n", 1, 120, 120},
        {"negativo vale zero",              "atraso_video_ms -30\natraso_audio_ms -1\n", 1, 0, 0},
        {"com sinal de mais",               "atraso_video_ms +40\natraso_audio_ms 25\n", 1, 40, 25},
        {"vazio",                           "", 0, 0, 0},
        {"lixo",                            "ZZZ\n", 0, 0, 0},
        {"só o vídeo",                      "atraso_video_ms 40\n", 0, 0, 0},
        {"só o áudio",                      "atraso_audio_ms 25\n", 0, 0, 0},
        {"na ordem trocada",                "atraso_audio_ms 25\natraso_video_ms 40\n", 0, 0, 0},
        {"sem número",                      "atraso_video_ms\natraso_audio_ms 25\n", 0, 0, 0},
        {"número com lixo colado",          "atraso_video_ms 40ms\natraso_audio_ms 25\n", 0, 0, 0},
        {"decimal",                         "atraso_video_ms 40.5\natraso_audio_ms 25\n", 0, 0, 0},
        {"número que estoura",              "atraso_video_ms 99999999999999999999\natraso_audio_ms 25\n", 0, 0, 0},
        {"nome colado",                     "atraso_video_msx 40\natraso_audio_ms 25\n", 0, 0, 0},
        {"lixo depois",                     "atraso_video_ms 40\natraso_audio_ms 25\nextra\n", 0, 0, 0},
        {"um terceiro campo",               "atraso_video_ms 40\natraso_audio_ms 25\natraso_x 3\n", 0, 0, 0},
    };
    for (unsigned i = 0; i < sizeof CASOS / sizeof CASOS[0]; i++) {
        Opcoes o = {-7, -7};
        bool ok = opcoes_interpretar(CASOS[i].texto, &o);
        CHECK(ok == (CASOS[i].valido != 0), "interpretar \"%s\": esperado %s", CASOS[i].nome, CASOS[i].valido ? "válido" : "inválido");
        if (CASOS[i].valido) CHECK(o.atrasoVideoMs == CASOS[i].video && o.atrasoAudioMs == CASOS[i].audio, "\"%s\": saiu %d/%d, esperado %d/%d", CASOS[i].nome, o.atrasoVideoMs, o.atrasoAudioMs, CASOS[i].video, CASOS[i].audio);
        else CHECK(o.atrasoVideoMs == -7 && o.atrasoAudioMs == -7, "\"%s\": inválido não mexe nas opções", CASOS[i].nome);
    }

    /* formatar: o formato de sempre, e sempre dentro dos limites */
    char t[OPCOES_MAX_BYTES];
    Opcoes o = {40, 25};
    opcoes_formatar(&o, t, sizeof t);
    CHECK(strcmp(t, "atraso_video_ms 40\natraso_audio_ms 25\n") == 0, "o formato é o das versões antigas (\"%s\")", t);
    o = (Opcoes){-5, 9999};
    opcoes_formatar(&o, t, sizeof t);
    CHECK(strcmp(t, "atraso_video_ms 0\natraso_audio_ms 120\n") == 0, "formatar corta nos limites (\"%s\")", t);
    for (int v = 0; v <= 120; v += 7) for (int a = 0; a <= 120; a += 11) {
        Opcoes in = {v, a}, volta = {-1, -1};
        opcoes_formatar(&in, t, sizeof t);
        if (!opcoes_interpretar(t, &volta) || volta.atrasoVideoMs != v || volta.atrasoAudioMs != a) { CHECK(0, "ida e volta falhou para %d/%d", v, a); return; }
    }
    CHECK(1, "ida e volta");

    /* arquivo: gravar, ler, estragado, ilegível */
    char erro[200] = "", aviso[300] = "", lido[200];
    const char *c = sub("apara_opcoes.txt");
    Opcoes b = {-1, -1};
    CHECK(opcoes_ler(c, &b, aviso, sizeof aviso) == OPCOES_NAO_EXISTE, "sem arquivo: a primeira vez não é erro");
    o = (Opcoes){60, 35};
    CHECK(opcoes_gravar(c, &o, erro, sizeof erro), "gravar (%s)", erro);
    CHECK(opcoes_ler(c, &b, aviso, sizeof aviso) == OPCOES_OK && b.atrasoVideoMs == 60 && b.atrasoAudioMs == 35, "ler devolve o que se gravou");
    CHECK(!gravar_existe(sub("apara_opcoes.txt.tmp")) && !gravar_existe(sub("apara_opcoes.txt.bak")), "sem .tmp nem .bak");
    o = (Opcoes){10, 5};
    CHECK(opcoes_gravar(c, &o, erro, sizeof erro) && opcoes_ler(c, &b, aviso, sizeof aviso) == OPCOES_OK && b.atrasoVideoMs == 10, "gravar por cima");

    /* gravar impossível: diz o motivo e o arquivo bom continua */
    mkdir(sub("apara_opcoes.txt.tmp"), 0755);
    erro[0] = 0;
    CHECK(!opcoes_gravar(c, &(Opcoes){99, 99}, erro, sizeof erro) && erro[0], "o .tmp não abre: falha com motivo (\"%s\")", erro);
    CHECK(le(c, lido, sizeof lido) > 0 && strcmp(lido, "atraso_video_ms 10\natraso_audio_ms 5\n") == 0, "e o arquivo antigo continua intacto");
    rmdir(sub("apara_opcoes.txt.tmp"));
    erro[0] = 0;
    CHECK(!opcoes_gravar(sub("nao/existe/o.txt"), &o, erro, sizeof erro) && erro[0], "pasta inexistente: falha com motivo (\"%s\")", erro);

    /* estragado: vai para .bak, com aviso, e as opções de quem chamou não mudam */
    escreve(c, "lixo qualquer");
    b = (Opcoes){33, 44};
    aviso[0] = 0;
    CHECK(opcoes_ler(c, &b, aviso, sizeof aviso) == OPCOES_CORROMPIDA && aviso[0] && b.atrasoVideoMs == 33 && b.atrasoAudioMs == 44, "estragado: corrompido, com aviso (\"%s\"), sem mexer nas opções", aviso);
    CHECK(!gravar_existe(c) && le(sub("apara_opcoes.txt.bak"), lido, sizeof lido) == 13 && strcmp(lido, "lixo qualquer") == 0, "e o arquivo estragado foi guardado byte a byte no .bak");
    char grande[600];
    memset(grande, '9', sizeof grande - 1);
    grande[sizeof grande - 1] = 0;
    escreve(c, grande);
    CHECK(opcoes_ler(c, &b, aviso, sizeof aviso) == OPCOES_CORROMPIDA, "arquivo enorme: lixo, sem ler tudo");
    gravar_atomico(c, "atraso_video_ms 4\0 atraso_audio_ms 2", 36, erro, sizeof erro);
    CHECK(opcoes_ler(c, &b, aviso, sizeof aviso) == OPCOES_CORROMPIDA, "byte zero no meio: corrompido");

    /* uma pasta no lugar do arquivo: ilegível, e nada vai para .bak */
    remove(c);
    remove(sub("apara_opcoes.txt.bak"));
    mkdir(c, 0755);
    aviso[0] = 0;
    OpcoesLeitura r = opcoes_ler(c, &b, aviso, sizeof aviso);
    CHECK(r == OPCOES_ILEGIVEL && aviso[0] && !gravar_existe(sub("apara_opcoes.txt.bak")), "uma pasta no lugar das opções: ilegível, com aviso (\"%s\")", aviso);
    rmdir(c);

    /* a migração das opções antigas passa pela mesma validação */
    escreve(sub("ao_lado/apara_opcoes.txt"), "atraso_video_ms 40\natraso_audio_ms 25\n");
    CHECK(pasta_dados_migrar(sub("ao_lado/apara_opcoes.txt"), sub("dados/apara_opcoes.txt"), valido_opcoes, OPCOES_MAX_BYTES, aviso, sizeof aviso) == MIGROU_COPIOU, "copia as opções antigas (%s)", aviso);
    CHECK(opcoes_ler(sub("dados/apara_opcoes.txt"), &b, aviso, sizeof aviso) == OPCOES_OK && b.atrasoVideoMs == 40 && b.atrasoAudioMs == 25, "e o jogo lê a cópia");
    remove(sub("dados/apara_opcoes.txt"));
    escreve(sub("ao_lado/apara_opcoes.txt"), "atraso_video_ms 40\n");
    CHECK(pasta_dados_migrar(sub("ao_lado/apara_opcoes.txt"), sub("dados/apara_opcoes.txt"), valido_opcoes, OPCOES_MAX_BYTES, aviso, sizeof aviso) == MIGROU_INVALIDO && !gravar_existe(sub("dados/apara_opcoes.txt")), "opções antigas incompletas: não copia");
}

/* O mkdir com o pid no nome (e uma volta se já existir) funciona em qualquer POSIX e no Windows. */
static bool pasta_temporaria(const char *base) {
    for (int n = 0; n < 1000; n++) {
        snprintf(raiz, sizeof raiz, "%s/apara_dados_test_%ld_%d", base, (long)getpid(), n);
        if (mkdir(raiz, 0700) == 0) return true;
        if (errno != EEXIST) return false;
    }
    return false;
}

static void apaga_tudo(void) {
    /* o que os testes criam, de dentro para fora */
    static const char *ARQUIVOS[] = {
        "arquivo.txt", "pequeno.txt", "vazio.txt", "exato.txt", "mais.txt", "grava.txt", "ruim.txt", "ruim.txt.bak", "apara_opcoes.txt", "apara_opcoes.txt.bak", "nao_existe.txt",
        "ao_lado/s.txt", "ao_lado/r.txt", "ao_lado/g.txt", "ao_lado/z.txt", "ao_lado/apara_opcoes.txt", "dados/s.txt", "dados/estragado.txt", "dados/r.txt", "dados/g.txt", "dados/z.txt",
        "dados/apara_opcoes.txt"};
    for (unsigned i = 0; i < sizeof ARQUIVOS / sizeof ARQUIVOS[0]; i++) remove(sub(ARQUIVOS[i]));
    static const char *PASTAS[] = {"destino_pasta", "grava.txt.tmp", "ao_lado/pasta.txt", "ao_lado", "dados", "a/b/c", "a/b", "a", "x/y", "x"};
    for (unsigned i = 0; i < sizeof PASTAS / sizeof PASTAS[0]; i++) rmdir(sub(PASTAS[i]));
    rmdir(raiz);
}

int main(void) {
    if (!pasta_temporaria(pasta_do_sistema())) { perror("mkdir"); return 2; }
    teste_pastas();
    teste_ler_pequeno();
    teste_atomico();
    teste_nome();
    teste_bak();
    teste_pasta_de_dados();
    teste_avisos_curtos();
    teste_migracao();
    teste_opcoes();
    apaga_tudo();
    printf("dados: %d verificações, %d falhas\n", checks, failures);
    return failures ? 1 : 0;
}
