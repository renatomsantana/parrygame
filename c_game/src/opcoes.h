/*
 * opcoes.h - o arquivo de opções (apara_opcoes.txt, na pasta de dados do jogador), sem raylib.
 *
 * Formato (texto):   atraso_video_ms <0..120>
 *                    atraso_audio_ms <0..120>
 *
 * Mesmas garantias do progresso (src/salvar.h): gravar é atômico (um disco cheio ou um arquivo bloqueado deixam o
 * arquivo antigo intacto) e diz o motivo quando falha; ler valida tudo, e um arquivo estragado vai para <arquivo>.bak
 * em vez de ser sobrescrito calado. O valor fora do intervalo não é erro: vale o limite (a calibração nunca passa dele).
 */
#ifndef APARA_OPCOES_H
#define APARA_OPCOES_H

#include <stdbool.h>
#include <stddef.h>

#define OPCOES_MAX_BYTES 128            /* um arquivo de verdade tem uns 40 bytes; o que passar disso é lixo */

typedef struct {
    int atrasoVideoMs;                  /* calibração: quanto a tela demora para mostrar o que o jogo desenha */
    int atrasoAudioMs;                  /* e quanto o som demora para sair */
} Opcoes;

typedef enum {
    OPCOES_OK,
    OPCOES_NAO_EXISTE,                  /* primeira vez: nada a avisar */
    OPCOES_CORROMPIDA,                  /* lido, mas sem sentido: guardado em .bak */
    OPCOES_ILEGIVEL                     /* não deu para ler (permissão, é uma pasta...) */
} OpcoesLeitura;

/* O texto das opções `o`, já dentro dos limites. */
void opcoes_formatar(const Opcoes *o, char *out, size_t n);
/* Interpreta o texto de um arquivo de opções. false se não tem exatamente os dois campos, na ordem, com inteiros
 * (sem lixo colado, sem lixo depois). Valores fora de 0..120 ms ficam no limite. */
bool opcoes_interpretar(const char *texto, Opcoes *o);

/* Grava em `caminho` (atômico). false se falhou; `erro` diz por quê. */
bool opcoes_gravar(const char *caminho, const Opcoes *o, char *erro, size_t n);
/* Lê `caminho`. Em OPCOES_CORROMPIDA o arquivo foi para "<caminho>.bak". `aviso` (para o jogador) vem preenchido em
 * OPCOES_CORROMPIDA e OPCOES_ILEGIVEL. `o` só muda em OPCOES_OK. */
OpcoesLeitura opcoes_ler(const char *caminho, Opcoes *o, char *aviso, size_t n);

#endif
