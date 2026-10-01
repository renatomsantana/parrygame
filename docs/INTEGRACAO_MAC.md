# Integração com a `mac-integracao`

Conferência da ponta da branch da outra sessão (`mac-integracao`, `a11cdbc`) contra esta (`claude/ajustes-visuais-2`). Nada foi mudado na branch dela, e nenhum arquivo dela
foi commitado aqui: o que precisa mexer num arquivo dela está num patch (`docs/mac/`).

## Como as duas branches se relacionam

- O `a11cdbc` é uma **junção** da minha branch até o `ebb27d2` (a curva, o robô da primeira vez, `bladeMax`) na linha de trabalho dela. Por isso a curva e o `tests/curva.c` já estão lá.
- Os meus 9 commits seguintes (Windows, `make ritmo`, testes do carimbo, `docs/FLUIDEZ.md`, `docs/WINDOWS.md`, os dois patches) **não** estão na dela.
- Nenhum PNG de `assets/sprites/` em nenhum commit dela (conferido no diff; só entram `sprite.txt` das pranchas de origem e o `.glb` da katana).
- Junção simulada numa pasta à parte (os meus 9 commits sobre a ponta dela): **um** conflito, no `Makefile`, e é aditivo (ela criou `presentation_test`, `test-visual`, `teste-vitoria`; eu criei `teste-windows`, `ritmo`, `test-ritmo`). A resolução é manter os dois lados.

## Resultado dos testes na ponta dela (com os meus 9 commits por cima)

| Teste | Resultado |
|---|---|
| `make test` (núcleo, fonte, save, entrada, desempenho, curva, curva-alvo, ritmo) | passa; 11455 verificações do núcleo, 0 falhas |
| fuzz, taxas de quadros, `curva-ordem` (100 mil lutas), `curva-alvo` (100 mil lutas), números, avisos gcc e clang, Windows (MinGW + Wine) | passam |
| `test-visual` (apresentação, 39634 verificações) | passa |
| `teste_save`, `teste_rastro`, `teste_desempenho`, `teste_cenas`, `teste_carimbo` | passam (com a arte local; sem os PNGs o `teste_rastro` reprova, e isso é a pasta sem arte, não código). O `teste_carimbo` reprovava de vez em quando com a máquina ocupada, **na minha branch também** (abaixo) |
| `teste_vitoria`, cenário 5 (golpe final do Oboro) | **reprova: o jogo cai** (abaixo) |

## O bug: o jogo cai ao abrir a luta final com o jogo recém-aberto

`mestre_do_rastro()` (`main.c`, commit `e996bfe`, "Reforça rastros por arma e elemento") chama `duel_move(&G.duel)` para o Oboro. Na abertura da luta final o duelo ainda não foi iniciado
(`G.duel` zerado), e o `duel_move` lê `d->m->moveCount` com `d->m` nulo: **falha de segmentação** (`duel_move` ← `draw_world` ← `main`, conferido com o gdb).
Num jogo contínuo isso passa despercebido (sobra o duelo do mestre anterior); abrindo o jogo já com a trilha completa e indo ao Oboro, ele cai.

O teste que pegou foi o `teste_vitoria.sh` (cenário 5: save com os doze aprendizes vencidos, o jogo joga sozinho até o golpe final). Correção de uma linha em `docs/mac/mestre_do_rastro.patch`
(aplica limpo na ponta dela, `git apply docs/mac/mestre_do_rastro.patch`): só olha o golpe do duelo se o duelo é o do mestre em cena.

Com o patch, o `teste_vitoria` passa nos 6 cenários. Sem ele, qualquer `make teste` completo na branch dela reprova.

## O efeito dela na curva: o Oboro do casual cai 3 a 4 pontos

| casual (%) | minha branch | mac-integracao | faixa alvo |
|---|---|---|---|
| jinshi | 55,9 | 55,9 | 52 a 58 |
| oboro (100 mil lutas) | 41,2 | 37,8 | 37 a 43 |

Comparei a tabela inteira dos cinco robôs nos 13 mestres (10 mil lutas): das 65 células, só esta muda; as outras 64 são idênticas. A causa é a reformulação da Shizuku (9 golpes para 4): o `eco do gelo` do Oboro copia a dupla do florete e passou de 4 golpes para 2
(`test_florete` exige isso). Continua **dentro** da faixa (37 a 43) e a ordem se mantém, mas na borda de baixo (o degrau jinshi → oboro vai de 14,7 para 18,1 pontos).
Não mexi em nada: se o Oboro precisar voltar para perto de 40, é decisão de roster (postura ou peso dos ecos), a medir.

## Outras anotações

- `docs/JOGO.md` ainda lista os 9 golpes antigos da Shizuku (a tabela do roster tem 4).
- O `Move` dela ganhou `thrustOnly` e `feint`, e o núcleo `move_contact_look()`: são só apresentação (o `test_florete` prova 60 lutas idênticas com e sem a finta). Os golpes novos do item 2 usam esses mesmos ganchos.

## O teste do carimbo e a máquina ocupada (defeito do meu teste, corrigido)

Numa rodada o `teste_carimbo` reprovou na ponta dela e, medindo, reprovou também na minha branch: com a CPU disputada o jogo desenha quadros de 50 a 70 ms e o raylib perde o
clique mais curto que um quadro (os 40 carimbos chegam, o duelo vê 13 a 18 cliques). Com um laço de CPU por núcleo reprovava 12 de 12 rodadas; com a máquina parada, 0 de 12. Não é o carimbo
nem código dela. A conta dos apertos agora olha a mediana do quadro que o jogo registra (`passo_ms`): se reprova com mediana acima de 40 ms (em repouso, sob o Xvfb, é de 28 a 31 ms), o cenário se
repete até 3 vezes, e se a máquina não deixar, só ele é pulado, em voz alta. Com quadro normal uma falha continua sendo falha (conferido com casos sintéticos nos dois sentidos).
