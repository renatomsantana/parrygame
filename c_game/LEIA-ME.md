# aparar — a trilha dos doze aprendizes

Duelo de parry de um botão, em C11 com [raylib](https://www.raylib.com/). As regras, a trilha,
o moveset e o visual estão em `../docs/JOGO.md`; a história, em `../aparar_lore.md`.

## Rodar

```sh
brew install raylib                     # uma vez (Linux: o pacote raylib do sistema)
make packs ZIP=all_the_animations.zip   # uma vez: põe as tiras dos packs pagos nas pastas
make run                                # compila e abre o jogo
```

Os packs pagos não entram no Git. `tools/instalar_packs.sh` organiza as tiras em
`assets/sprites/_original/`, `_packs/`, `_fx/` e `_ui/`. Os personagens são
desenhados apenas pelos PNGs de animação; sem os packs, eles não aparecem.
Os cenários e as partículas embutidas continuam disponíveis.
O progresso fica em `apara_save.txt` e a calibração em `apara_opcoes.txt`, ao lado do executável.
Se o save estiver estragado, o jogo avisa, guarda o arquivo em `apara_save.txt.bak` e começa de novo.

## No macOS: o carimbo do clique

O jogo usa o instante em que o macOS recebeu o clique (`NSEvent.timestamp`, em `src/entrada_mac.m`) e não o meio do
quadro. Esse arquivo foi escrito sem um Mac à mão: **nunca foi compilado nem rodado**. Confira nesta ordem:

1. `make` (precisa das Command Line Tools). Compila o `src/entrada_mac.m` junto com o resto.
2. `./apara --carimbo` e clique 20 vezes (depois, 20 vezes com Espaço). A 60 Hz o mínimo sai perto de 0, a média perto de
   8 ms e o máximo perto de 16 ms. Sempre perto de 0: o carimbo está sendo tirado no poll, e não no clique (defeito).
   Aviso vermelho "esta plataforma não dá o instante do clique": o monitor não instalou.
3. Num duelo (`./apara --teste --master 6 --duel`), aperte **F3**: a linha "carimbo do clique" conta os apertos "no
   instante do clique" e os "no meio do quadro". Os do meio do quadro têm de ficar perto de zero.
4. Compare o toque: `APARA_SEM_CARIMBO=1 ./apara --teste --master 6 --duel` aperta no meio do quadro, como antes.

**Se o `.m` não compilar:** `make clean && make ENTRADA_PLAT=src/entrada_stub.c` compila sem o carimbo (o jogo fica como era, com o
aperto no meio do quadro) e não perde mais nada. Mande as primeiras linhas do erro do compilador. No Windows e no Linux sem
XInput2 (Wayland puro) o Makefile já usa esse mesmo `src/entrada_stub.c`: lá o aperto vale no meio do quadro, de propósito.

## Teclas

| Tecla | O que faz |
|---|---|
| clique, **Espaço**, **J**, **Enter** | apara e avança as falas |
| **Esc** | pausa (**T** trilha, **L** calibra o atraso, **M** menu, **Q** sai) |
| **F3** (ou `APARA_DEBUG=1`) | overlay de debug: janelas, linha do tempo do golpe, os seis últimos apertos (perfeito, bom, cedo, tarde, e o erro em ms), fase, posturas, vida, e quantos apertos usaram o instante de hardware do clique |
| **F** / **F11** | liga e desliga o tremor / tela cheia |
| segurar Esc, segurar o clique | na abertura: pula / acelera |

## Testar à mão

```sh
./apara --teste --master 13 --fase 3     # direto na 3ª fase do oboro, F3 ligado, sem salvar
./apara --master 13 --duel --demo        # um robô apara sozinho contra o oboro
./apara --state pause|defeat|finisher|cleared --master 5 --duel
./apara --shot arquivo.png 5             # captura depois de 5 s;  --rec pasta 1 5  salva os quadros (GIF)
./apara --carimbo                        # clique 20 vezes: mostra quanto depois do clique o jogo o leu
```

Com `--teste`, na luta ou na derrota: **R** recomeça, **V** enche a vida, **P** enche a postura
do mestre, **1 2 3** escolhem a fase do oboro, **N** / **B** vão ao próximo / anterior mestre.
`--teste`, `--master`, `--duel`, `--state`, `--fase`, `--final` e `--demo` nunca gravam o progresso.

## Gravar vídeos sem áudio

```sh
tools/gravar_video.sh saida.mp4 T0 T1 "linha 1" "linha 2" APARA_DEBUG=1 APARA_SEMENTE=11 -- --demo --master 1 --duel
```

Roda o jogo sob xvfb com passo fixo (`--rec`), escreve cada quadro em RGB cru para o `ffmpeg` (`APARA_REC_RAW`) e faz um mp4 de 60
quadros por segundo de tempo de jogo, por mais lento que o vídeo por software seja, com uma faixa de legenda em cima e o tempo de
jogo no canto. `T0` e `T1` são segundos de jogo. Sem som: com `APARA_DEBUG=1` o F3 mostra o aviso, o contato e o resultado de cada
parry. Precisa do `ffmpeg` (com o libass; `FFMPEG=/caminho`). Ganchos para os vídeos: `APARA_ROBO=cedo|tarde|casual|spam|nunca`
(o robô do `--demo`: `cedo` aperta 0,6 s antes do contato, `tarde` 0,1 s depois), `APARA_CLIQUE_PERIODO=0,05` (fora do duelo o robô
clica a cada 50 ms e um ponto vermelho no canto mostra cada clique, também os que a trava ignora), `APARA_REC_FPS`.
`LENTO=4` faz câmera lenta de verdade (o jogo roda a 4 x 60 passos por segundo, sem quadros repetidos) e `SEM_LEGENDA=1` grava só o jogo, para juntar
vídeos lado a lado. Para cenas (o desarme, a quebra do selo, os finais) rode antes com `APARA_AUTO=1 APARA_SEMENTE=11`: o jogo escreve
`TESTE_MARCO nome t=segundos` a cada momento, e esses tempos valem para o vídeo com as mesmas variáveis e argumentos.

## Ajustar

- `src/ajuste.h`: todas as constantes globais de equilíbrio e de sensação (vida, janelas, aviso,
  tolerância tardia, recarga do aperto cedo, hitstop, ritmo, tremor, os tempos das cenas). Cada uma
  tem um comentário; nenhum número solto nas regras (`make numeros` confere).
- `src/roster.c`: os treze lutadores. **O balanceamento de cada mestre é aqui** (janelas, aviso,
  postura, erros até cair, sequências, falas).

## Desempenho

```sh
APARA_PERF=30 ./apara --demo --master 13 --duel --fase 3   # joga 30 s com o robô, escreve o relatório e sai
tools/medir_desempenho.sh                                  # a tabela dos 13 mestres (SEGUNDOS=30 MESTRES="1 13" RASTRO=1)
```

Meça no seu computador, com a janela em foco e o vsync ligado (o jogo liga sozinho); sob xvfb o vídeo é por software e só a lógica e
a carga valem. Rode também com `RASTRO=1`, que compara o oboro na 3ª fase e o karasu com e sem o rastro fantasma.

**Como ler a tabela** (uma linha por mestre, o robô do demo jogando `SEGUNDOS` s, sem os 30 primeiros quadros):

| Coluna | O que é | Sinal de problema |
|---|---|---|
| médio, p50, p95, p99, máx (ms) | o quadro, de um poll ao seguinte | a 60 Hz o esperado é ~16,7 (o vsync manda); médio abaixo disso é vsync desligado ou tela mais rápida |
| quadros > 16,7 ms | "A de N": quantos passaram do orçamento de 60 Hz | a meta é 0 (ou perto). Passar de 1% num mestre é o que interessa |
| mundo, composição, swap (ms, média) | onde o quadro gasta: o cenário e os lutadores; a ampliação, o pós-processo e a interface; a espera do vsync | `mundo` de um mestre bem acima dos outros é o fundo do cenário dele. `swap` alto é espera, não trabalho |
| CPU do processo | uso de CPU do jogo, com o driver de vídeo | perto de 100% de um núcleo com o vsync ligado indica que o jogo não sobra tempo |

Trabalho do jogo por quadro = lógica + mundo + interface + composição (sem o swap): tem de caber em 16,7 ms com folga. Um máximo
alto com p99 bom é um pico só: rode `APARA_PERF=30 ./apara --demo --master N --duel` e leia as linhas `PERF pico`, que dizem
o estado do jogo e o tempo. Pico no começo de uma cena é a carga (paleta e sprites da primeira exibição); pico no meio da
luta é o que se conserta. O relatório também traz a carga até o primeiro quadro (janela, áudio, sprites...) e avisa se
o jogo esteve pausado durante a medida (então ela não vale). Mande a tabela inteira e a linha "carga até o primeiro quadro".

## Os robôs (curva de dificuldade)

```sh
make robos                          # tabela por mestre: perfeito, nunca defende, spam, reação 200/250/300, humano casual
make robos LUTAS=1000 HZ=144        # mais lutas, outra taxa de quadros (a tabela não muda: os robôs apertam em ms)
make robos QUADROS=1                # o aperto no meio do quadro, como o clique do jogo (depende da taxa)
make robos-taxas                    # a mesma luta a 30, 60, 120, 144 e 240 Hz tem de dar o mesmo resultado
make fuzz N=3000                    # cenários sorteados contra os invariantes do núcleo
```

## Testes: um comando

`make teste` roda tudo e sai com erro se algo falhar:

| Parte | O que confere |
|---|---|
| `make test` | `core_test` (regras, janela viável em todo golpe, curva, Oboro, taxa de quadros), `test-fonte` (todo caractere não ASCII tem glifo), `test-save` (formato, saves corrompidos, gravação atômica), `test-entrada` (o carimbo do clique: conversão, carimbo inválido, lutas dos robôs), `test-desempenho` (médias e percentis do `APARA_PERF`) |
| `make fuzz-rapido`, `make robos-taxas` | invariantes em cenários sorteados; mesma luta em cinco taxas de quadros |
| `make numeros`, `make avisos` | nenhum número mágico nas regras; nenhum aviso do gcc e do clang em -O1, -O2 e -O3 |
| `make teste-jogo` | o jogo de verdade sob `xvfb-run` (precisa dele e da libX11; sem eles pula): teclas de teste só com `--teste`, o rastro é só visual, a vitória é salva no golpe final e sobrevive a kill, Esc+Q e fechar a janela, o carimbo do clique chega ao duelo no instante certo (cliques do XTest; precisa de libXtst), e o `APARA_PERF` mede e sai |

Ganchos só para os testes (variáveis de ambiente): `APARA_AUTO`, `APARA_SEMENTE`, `APARA_TECLAS`,
`APARA_RASTRO`, `APARA_LOG_IMPACTOS`, `APARA_VITORIA_LENTA`, `APARA_SEM_UI`, `APARA_SFX`, `APARA_FPS` (limita
os quadros por segundo), `APARA_LOG_CARIMBOS` (escreve cada carimbo e cada aperto do duelo) e
`APARA_SEM_CARIMBO=1` (o aperto vale no meio do quadro, como antes: para comparar à mão) e `APARA_PERF=segundos`
(mede o desempenho e sai).

Mapa do código: tabela em `../docs/JOGO.md`. O núcleo (`core`, `roster`, `ajuste`, `robo`, `salvar`,
`fonte`, `entrada`, `desempenho`) não usa raylib.

## Clipes das lutas com impactos

`python3 tools/gravar_lutas.py --seconds 6` gera amostras MP4 dos treze mestres
em `videos-lutas/`, sem alterar o save. Os clipes usam os quadros do jogo a
60 fps e os efeitos de impacto sintetizados pelo próprio áudio; não incluem a
trilha ou a mixagem completa da partida. Requer `ffmpeg`.
