# Integração com a `mac-integracao`

A branch `mac-integracao` (`a11cdbc`) foi conferida e **integrada** a esta. A `mac-integracao` não foi mexida: a junção está só aqui, e a ordem dos commits é a abaixo.

## Como ficou, em commits

| Commit | O que |
|---|---|
| `e89b45c` | **Junção** da `mac-integracao`. Um conflito só, no `Makefile`, aditivo (ela criou `presentation_test`, `test-visual`, `test-assets`, `teste-vitoria`; eu criei `teste-windows`, `ritmo`, `test-ritmo`): mantidos os dois lados. Vêm com ela a remoção de dois PNGs de protótipo do Unity e o encolhimento do `script.md` (a limpeza dos protótipos antigos dela, `1a47d08` e `7f16d35`). |
| `bf5b47b` | **Conserto do bug que derruba o jogo** na luta final (abaixo), em `main.c`. |
| `b314f8c` | **Windows**: o jogo inteiro compila para Windows (as duas correções de `main.c` e `katana3d.c` que estavam em patch). |
| `c14eeea` | **Teste de desempenho**: a lógica de um quadro nunca passa de 3 ms na luta (a pré-carga em si é o commit `01eeb13`). |
| depois | as regras de teste do item 2 e os golpes novos, um mestre por commit (`docs/GOLPES_NOVOS.md`). |

A `mac-integracao` já continha esta branch até o `ebb27d2` (a curva, o robô da primeira vez, `bladeMax`); o `a11cdbc` é uma junção dela. Nenhum PNG de `assets/sprites/` em nenhum commit dela.

## O bug: o jogo caía ao abrir a luta final com o jogo recém-aberto (corrigido em `bf5b47b`)

`mestre_do_rastro()` (`main.c`, commit `e996bfe`, "Reforça rastros por arma e elemento") chamava `duel_move(&G.duel)` para o Oboro. Na abertura da luta final o duelo ainda não foi iniciado
(`G.duel` zerado), e o `duel_move` lê `d->m->moveCount` com `d->m` nulo: **falha de segmentação** (`duel_move` ← `draw_world` ← `main`, conferido com o gdb).
Num jogo contínuo isso passa despercebido (sobra o duelo do mestre anterior); abrindo o jogo já com a trilha completa e indo ao Oboro, ele caía.

O teste que pegou foi o `teste_vitoria.sh` (cenário 5: save com os doze aprendizes vencidos, o jogo joga sozinho até o golpe final): reprovava com 3 falhas e passa nos 6 cenários. A correção é de uma linha: só olha o golpe do duelo se o duelo é o do mestre em cena.

## O efeito da branch dela na curva: o Oboro do casual cai 3 a 4 pontos

| casual (%) | antes da junção | com a `mac-integracao` | faixa alvo |
|---|---|---|---|
| jinshi | 55,9 | 55,9 | 52 a 58 |
| oboro (100 mil lutas) | 41,2 | 37,8 | 37 a 43 |

Comparei a tabela inteira dos cinco robôs nos 13 mestres (10 mil lutas): das 65 células, só esta muda; as outras 64 são idênticas. A causa é a reformulação da Shizuku (9 golpes para 4): o `eco do gelo` do Oboro copia a dupla do florete e passou de 4 golpes para 2
(`test_florete` exige isso). Continua **dentro** da faixa (37 a 43) e a ordem se mantém, mas na borda de baixo (o degrau jinshi → oboro vai de 14,7 para 18,1 pontos).
Não mexi: se o Oboro precisar voltar para perto de 40, é decisão de roster (postura ou peso dos ecos), a medir.

## O teste do carimbo e a máquina ocupada (defeito do meu teste, corrigido)

Numa rodada o `teste_carimbo` reprovou na ponta dela e, medindo, reprovou também na minha branch: com a CPU disputada o jogo desenha quadros de 50 a 70 ms e o raylib perde o
clique mais curto que um quadro (os 40 carimbos chegam, o duelo vê 13 a 18 cliques). Com um laço de CPU por núcleo reprovava 12 de 12 rodadas; com a máquina parada, 0 de 12. Não é o carimbo
nem código dela. A conta dos apertos agora olha a mediana do quadro que o jogo registra (`passo_ms`): se reprova com mediana acima de 40 ms (em repouso, sob o Xvfb, é de 28 a 31 ms), o cenário se
repete até 3 vezes, e se a máquina não deixar, só ele é pulado, em voz alta. Com quadro normal uma falha continua sendo falha (conferido com casos sintéticos nos dois sentidos).

## Outras anotações

- `docs/JOGO.md` ainda lista os 9 golpes antigos da Shizuku (a tabela do roster tem 4): é a linha dela, e não mexi.
- O `Move` dela ganhou `thrustOnly` e `feint`, e o núcleo `move_contact_look()`: são só apresentação (o `test_florete` prova 60 lutas idênticas com e sem a finta). Os golpes novos do item 2 não usam esses ganchos (só o olhar e o duplo que já existiam).
- Os testes de jogo (`teste_rastro`, `teste_vitoria`, `teste_cenas`...) rodam com a arte local; sem os PNGs (que não vão para o git) o `teste_rastro` reprova, e isso é a pasta sem arte, não código.
