/*
 * salvar.h - o arquivo de progresso (apara_save.txt), sem raylib.
 *
 * Formato (texto):   APARA-C 2
 *                    <mestre atual> <máscara dos vencidos> <trilha completa 0/1> <abertura vista 0/1>
 *
 * Gravar é atômico: escreve em <arquivo>.tmp, confere cada passo (fprintf, fflush, fsync, fclose) e
 * só então troca pelo arquivo de verdade (rename). Uma queda no meio deixa o save antigo intacto.
 * Ler valida tudo; um save estragado vai para <arquivo>.bak em vez de ser sobrescrito calado.
 */
#ifndef APARA_SALVAR_H
#define APARA_SALVAR_H

#include "core.h"

#include <stdbool.h>
#include <stddef.h>

#define SAVE_MAX_BYTES 256          /* um save de verdade tem menos de 40; o que passar disso é lixo */

typedef enum {
    SAVE_OK,
    SAVE_NAO_EXISTE,                /* primeira vez: nada a avisar */
    SAVE_CORROMPIDO,                /* lido, mas sem sentido: guardado em .bak */
    SAVE_ILEGIVEL                   /* não deu para ler (permissão, é uma pasta...) */
} SaveLeitura;

/* O texto do save de `c`. */
void save_formatar(const Campaign *c, char *out, size_t n);
/* Interpreta o texto de um save. false se qualquer campo estiver fora do que o jogo grava:
 * versão diferente de 2, mestre fora da trilha, máscara com bit de mestre que não existe, os dois
 * 0/1 com outro valor, trilha "completa" fora do último mestre, lixo depois dos quatro números. */
bool save_interpretar(const char *texto, Campaign *c);

/* Grava em `caminho` (atômico). false se falhou; `erro` diz por quê. */
bool save_gravar(const char *caminho, const Campaign *c, char *erro, size_t n);
/* Lê `caminho`. Em SAVE_CORROMPIDO o arquivo foi para "<caminho>.bak". `aviso` (para o jogador)
 * vem preenchido em SAVE_CORROMPIDO e SAVE_ILEGIVEL. `c` só muda em SAVE_OK. */
SaveLeitura save_ler(const char *caminho, Campaign *c, char *aviso, size_t n);

#endif
