/*
 * gravar.h - gravar os arquivos do jogador (progresso, opções) sem estragar o que já existe, sem raylib.
 *
 * Gravar é atômico: escreve em <arquivo>.tmp, confere cada passo (fputs, fflush, fsync, fclose) e só então
 * troca pelo arquivo de verdade (rename; no Windows, MoveFileEx com substituição). Uma queda no meio, um disco
 * cheio ou um arquivo bloqueado deixam o arquivo antigo intacto e o .tmp é apagado.
 */
#ifndef APARA_GRAVAR_H
#define APARA_GRAVAR_H

#include <stdbool.h>
#include <stddef.h>

#define CAMINHO_MAX 1024            /* o maior caminho que o jogo monta (a pasta de dados + o nome + .tmp/.bak) */

/* Troca `para` por `de`. Nunca apaga o destino antes: se a troca falha, o destino fica como estava. 0 se deu certo. */
int gravar_troca(const char *de, const char *para);

/* Grava `n` bytes de `dados` em `caminho`, de forma atômica. false se falhou; `erro` diz por quê. */
bool gravar_atomico(const char *caminho, const char *dados, size_t n, char *erro, size_t nerro);

/* Guarda um arquivo estragado em <caminho>.bak (sem apagar um .bak antigo antes da troca). */
bool gravar_guardar_bak(const char *caminho);

/* Cria `pasta` e as que faltam acima dela. true se ela existe como pasta no fim. */
bool gravar_pasta(const char *pasta, char *erro, size_t nerro);

/* O nome do arquivo de `caminho`, sem a pasta (para avisos curtos ao jogador: o caminho inteiro vai para o terminal). */
const char *gravar_nome(const char *caminho);

/* O arquivo (ou pasta) existe? */
bool gravar_existe(const char *caminho);

/* Lê `caminho` inteiro em `buf` (de `max` + 1 bytes, terminado em 0). Devolve o tamanho, ou -1 se não abriu (errno diz
 * por quê), -2 se passa de `max` bytes (lixo: não é um arquivo do jogo), -3 se deu erro de leitura. */
long gravar_ler_pequeno(const char *caminho, char *buf, size_t max);

#endif
