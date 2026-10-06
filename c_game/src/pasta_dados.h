/*
 * pasta_dados.h - onde ficam os arquivos do jogador (progresso e opções), sem raylib.
 *
 * Os dados do jogador não ficam mais ao lado do executável (uma pasta de programa pode ser protegida ou
 * compartilhada), e sim na pasta de dados do usuário:
 *   Windows  %LOCALAPPDATA%\Apara        (ou %APPDATA%\Apara; ou %USERPROFILE%\AppData\Local\Apara)
 *   macOS    ~/Library/Application Support/Apara
 *   Linux    $XDG_DATA_HOME/apara        (ou ~/.local/share/apara)
 * APARA_DADOS=<pasta> manda no resto (testes, instalação portátil).
 *
 * Quem tinha jogado com o save ao lado do executável não perde nada: pasta_dados_migrar copia o arquivo antigo para
 * a pasta nova na primeira vez (o original fica onde estava, intacto), desde que o antigo seja válido.
 */
#ifndef APARA_PASTA_DADOS_H
#define APARA_PASTA_DADOS_H

#include <stdbool.h>
#include <stddef.h>

typedef enum { SISTEMA_WINDOWS, SISTEMA_MAC, SISTEMA_LINUX } Sistema;

/* O que o sistema diz sobre onde moram os dados do usuário (NULL ou "" = não definido). Separado de getenv para
 * os testes simularem os três sistemas, com ou sem cada variável. */
typedef struct {
    Sistema sistema;
    const char *forcada;                /* APARA_DADOS */
    const char *localappdata, *appdata, *userprofile;   /* Windows */
    const char *home;                   /* macOS e Linux */
    const char *xdg;                    /* XDG_DATA_HOME, Linux */
} AmbienteDados;

/* O ambiente de verdade: o sistema da compilação e as variáveis de ambiente. */
AmbienteDados pasta_dados_ambiente(void);

/* A pasta de dados do jogador em `out`. false se nenhuma variável permite achá-la (ou se não cabe em `n`). */
bool pasta_dados_caminho(const AmbienteDados *a, char *out, size_t n);

/* `pasta` + separador do sistema + `nome`. false se não cabe. */
bool pasta_dados_arquivo(Sistema s, const char *pasta, const char *nome, char *out, size_t n);

typedef enum {
    MIGROU_NADA,                        /* não havia o que copiar (já existe o novo, ou não existe o antigo) */
    MIGROU_COPIOU,                      /* o antigo era válido e foi copiado; o original continua lá */
    MIGROU_INVALIDO,                    /* o antigo existe mas não é um arquivo do jogo: não foi copiado nem tocado */
    MIGROU_FALHOU                       /* era para copiar e não deu (`aviso` diz por quê); o antigo continua lá */
} Migracao;

/* Copia `legado` para `novo` se `novo` não existe e `legado` existe e passa em `valido` (texto de até `max` bytes).
 * Nunca mexe no `legado`, nunca sobrescreve o `novo`. A pasta de `novo` tem de existir. `aviso` (curto, para a faixa do
 * jogo: só o nome do arquivo, sem as pastas) vem preenchido em MIGROU_FALHOU. */
Migracao pasta_dados_migrar(const char *legado, const char *novo, bool (*valido)(const char *texto), size_t max, char *aviso, size_t n);

#endif
