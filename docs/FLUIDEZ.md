# Fluidez da luta

Só a luta: sem cenas, cutscenes nem mensagens. Quatro frentes, e o que já está feito, o que é patch e o que espera a sua decisão.

| Frente | Estado |
|---|---|
| Travadas e FPS | **causa achada e medida**; a pré-carga já está no jogo (`01eeb13` da outra sessão) e o teste que a guarda está no repositório (`tests/teste_desempenho.sh`) |
| Ritmo do duelo | ferramenta e teste no repositório (`make ritmo`, `make test-ritmo`); duas mudanças propostas, **nenhuma aplicada** |
| Resposta do aperto | o julgamento já é exato (carimbo); o que sobra é visual e está no `main.c`: análise abaixo |
| Fluxo visual dos golpes | o corpo agora desliza até o contato em vez de saltar (seção 4) |

## 1. Travadas: o jogo carrega cada folha de efeito na primeira vez que a usa

**O que acontece.** `spr_fx()` (`sprites.c`) carrega o PNG do efeito (decodifica e sobe a textura) **na primeira vez que o duelo pede**. Os pedidos
caem nos dois instantes que mais importam: o primeiro aviso de cada mestre (`tell_fx`) e o primeiro parry perfeito, bom ou erro (`on_impact`).

**Como medi.** Cronometrei por dentro do `handle_events()` numa cópia do jogo (Linux, demo, `APARA_PERF`). Sob o Xvfb a placa de vídeo é por software,
então só a seção **lógica** vale (mundo e composição não); a lógica de um quadro normal leva 0,01 ms.

| Quando | Folha | Custo no quadro |
|---|---|---|
| 1º aviso do daichi | `70` | 3,9 ms |
| 1º parry perfeito (todos os mestres) | `652` | 6,0 a 7,6 ms |
| 1º aviso do oboro | `197` | 10,4 ms |
| onda de choque do oboro | `184` | 5,6 ms |

O orçamento de um quadro é 16,7 ms a 60 Hz e 6,9 ms a 144 Hz. Não medi com placa de vídeo de verdade, mas 4 a 10 ms de CPU a mais no meio de um quadro arriscam
perdê-lo a 60 Hz e perdem a 144 Hz, justamente no primeiro aviso e no primeiro parry perfeito.

**A correção** é carregar as folhas de efeito na abertura, e **já está no jogo**: o commit `01eeb13` da outra sessão ("Carrega efeitos e ícones antes da luta", `preload_runtime_art()`) faz isso, então o patch que eu tinha preparado (que carregava 15 folhas) não foi
aplicado. O que entrou foi o **teste** dele (`tests/teste_desempenho.sh`): a lógica de um quadro nunca passa de 3 ms na luta (mestres 1 e 13, até 3 tentativas cada, para a máquina ocupada não reprovar). Conferido nos dois sentidos: com a pré-carga dela, o pior quadro de lógica é
0,04 ms (daichi) e 0,07 ms (oboro); desligando a chamada, o oboro chega a 11,2 ms e o teste reprova. Sem pré-carga, o que medi antes: 9,3 ms (daichi), 9,5 ms (oboro) e 11,1 ms (karasu).

| | lógica máxima numa luta de 8 s |
|---|---|
| sem a pré-carga | daichi 9,3 ms, oboro 9,5 ms (karasu 11,1 ms) |
| com a pré-carga | daichi 0,04 ms, oboro 0,07 ms |

**No Windows**, para medir de verdade (com a placa de vídeo real): `set APARA_PERF=20` e `apara.exe --demo --master 6 --duel`; o relatório vem
no terminal. Olhe `PERF lógica` (o máximo tem de ficar abaixo de 3 ms) e os `PERF pico`.

**Teste de longa duração** (Oboro, demo de 60 s, o jogo no Linux): a memória do `malloc` fica **plana durante a luta** (85,3 → 86,2 MB em 40 s: sem vazamento). O crescimento
de memória que apareceu no primeiro teste (155 → 178 MB) eram as folhas de efeito sendo carregadas aos poucos, uma a cada primeiro uso; com a pré-carga ele some. A mesma pré-carga
tira um travamento de 27,9 ms que aconteceu no meio da luta do oboro (aos 39,7 s, num efeito de fase). Sobra um pico de 80 a 140 ms **na cena depois da luta** (o desarme), que está fora do escopo.

## 2. Ritmo do duelo

`make ritmo` mede, em **tempo real** (núcleo mais o que o hitstop congela) e com o robô perfeito:

| # | Mestre | luta (s) | golpes/min | abertura (s) | contato→aviso (s) mediana / p95 | aviso→contato (s) mín / mediana | na sequência (s) mín / mediana | congelado |
|---|---|---|---|---|---|---|---|---|
| 1 | daichi | 21,0 | 42,9 | 1,20 | 0,99 / 1,14 | 0,45 / 0,45 | 1,00 / 1,00 | 6,8% |
| 2 | genbu | 16,0 | 56,2 | 1,00 | 0,81 / 0,90 | 0,44 / 0,44 | 0,42 / 0,60 | 8,9% |
| 3 | raizo | 17,7 | 50,8 | 1,01 | 0,84 / 0,88 | 0,42 / 0,42 | 0,70 / 0,85 | 8,0% |
| 4 | shizuku | 12,7 | 70,8 | 0,86 | 0,69 / 0,74 | 0,41 / 0,41 | 0,40 / 0,40 | 11,2% |
| 5 | garfiel | 12,8 | 89,3 | 0,86 | 0,69 / 0,74 | 0,40 / 0,40 | 0,40 / 0,40 | 13,9% |
| 6 | karasu | 15,6 | 65,2 | 0,93 | 0,71 / 0,86 | 0,38 / 0,38 | 0,45 / 0,50 | 10,2% |
| 7 | hayate | 16,5 | 72,8 | 0,87 | 0,70 / 0,84 | 0,37 / 0,37 | 0,40 / 0,45 | 11,3% |
| 8 | enjin | 15,3 | 82,3 | 0,83 | 0,66 / 0,71 | 0,36 / 0,36 | 0,40 / 0,45 | 12,8% |
| 9 | suiren | 21,2 | 67,9 | 1,05 | 0,75 / 0,88 | 0,35 / 0,35 | 0,45 / 0,50 | 10,5% |
| 10 | arashi | 11,5 | 78,0 | 0,89 | 0,72 / 0,77 | 0,33 / 0,33 | 0,40 / 0,40 | 12,3% |
| 11 | yoru | 21,0 | 71,3 | 0,94 | 0,75 / 0,83 | 0,32 / 0,32 | 0,40 / 0,45 | 11,0% |
| 12 | jinshi | 13,0 | 69,5 | 1,00 | 0,81 / 1,00 | 0,35 / 0,35 | 0,40 / 0,50 | 11,0% |
| 13 | oboro | 48,8 | 75,1 | 1,03 | 0,82 / 2,96 | 0,32 / 0,35 | 0,40 / 0,45 | 11,7% |

Leitura:

- **A abertura** (do começo da luta ao primeiro aviso) é de 0,83 a 1,20 s, e **o tempo sem nada a reagir** (do contato ao aviso seguinte) de 0,66 a 0,99 s (mediana).
  O daichi é o mais lento: 43 golpes por minuto, abertura de 1,20 s, 1,00 s entre os dois golpes da sequência dele (os outros ficam em 0,40 a 0,85 s).
- **O aviso** (do aviso ao contato do primeiro golpe) fica em 0,32 a 0,45 s: nunca abaixo dos 300 ms da regra (o piso é 320).
- **Os golpes de uma sequência** nunca ficam a menos de 0,40 s um do outro (`AJ_CADEIA_MIN`).
- **O hitstop congela de 7% (daichi) a 14% (garfiel) da luta.** Nos mestres rápidos, cada golpe de uma sequência de 0,40 s tem 0,09 s de congelamento dentro:
  o corpo para um quinto do tempo da sequência.

**Duas mudanças propostas (nenhuma aplicada; preciso do seu OK):**

| | O que muda | Efeito medido |
|---|---|---|
| A. Daichi mais ágil | só a espera dele antes do aviso (`waitScale`, `roster.c`): ×0,50 → ×0,30 | 42,9 → 48,7 golpes/min; abertura 1,20 → 0,98 s; contato→aviso 1,01 → 0,80 s. Não mexe no aviso nem nas janelas |
| B. Hitstop menor dentro da sequência | ×0,6 nos golpes que ainda têm outro depois (o último da sequência e a quebra de postura ficam inteiros); `duel_hitstop_for` ganha o argumento "em sequência" | tempo congelado: garfiel 14,0 → 10,4%, enjin 12,8 → 10,0%, jinshi 11,0 → 8,5%, oboro 11,7 → 8,6%; daichi 6,8 → 6,3% (quase não muda) |

Nenhuma das duas toca o aviso, as janelas perfeita e boa nem a menor partida da lâmina.

**B foi implementada no núcleo para medir e revertida** (nada ficou no repositório). O tempo real entre os contatos não muda: o que o hitstop deixa de congelar volta para a
preparação do golpe seguinte. Mas essa preparação mais longa endurece o casual, que mede o intervalo a partir do começo dela. Com 100 mil lutas por mestre:

| casual (%) | sem a B | com a B (×0,6) |
|---|---|---|
| garfiel | 99,8 | 99,6 |
| hayate | 84,1 | 83,6 |
| enjin | 79,7 | 78,9 |
| suiren | 72,4 | 71,8 |
| arashi | 63,8 | 63,2 |
| yoru | 59,0 | 57,8 |
| jinshi | 55,9 | 55,0 |
| oboro | 41,2 | 39,9 |

O efeito satura logo: ×0,95 já desloca o casual (oboro 41,4 → 40,7 em 20 mil lutas) e ×0,8 dá o mesmo que ×0,6, então **não existe uma versão branda que deixe a curva
intacta**. Consequência: o `test_curva` do `core_test.c` (jinshi do casual ≥ 55%) reprova e o degrau yoru → jinshi cai de 3,1 para 2,8 (o mínimo que eu mesmo coloquei é 3,0).
Pela regra de parar quando algo quebra a curva sem pedido, revertei. Para a B valer, é preciso decidir entre (i) aceitar a curva 1 ponto mais dura nos mestres do fim e baixar o
piso do jinshi no `test_curva` de 55 para 52 (a faixa alvo que você aprovou é 52 a 58) e o degrau mínimo para 2,5, ou (ii) compensar com a postura do jinshi e do yoru, o que são
mais duas mudanças.

**A também esbarra numa regra:** o `core_test.c` exige que só o hayate e o jinshi tenham `waitScale` próprio (a exceção aprovada na rodada de ritmo). Dar um ao daichi muda esse
teste. A curva não sente (o casual do daichi já é 100%), mas é mudar uma regra aprovada, e por isso também espera o seu OK.

**O que o teste garante agora** (`make test-ritmo`, dentro do `make test`): o aviso do primeiro golpe nunca baixa de 300 ms; dois contatos de uma sequência nunca
ficam mais perto que `AJ_CADEIA_MIN`; nunca há mais de 3 s sem aviso (fora a pausa do selo do oboro); o hitstop nunca passa de 20% de uma luta. Conferido por
três mutações (piso do aviso mais alto, `AJ_CADEIA_MIN` mais alto, hitstop de 0,9 s).

## 3. Resposta do aperto

O que já é exato: o clique é carimbado pelo sistema (Linux, macOS e agora Windows) e o núcleo o julga no instante dele, em qualquer taxa de quadros; o hitstop é gasto em
tempo real. Ver `docs/CURVA.md` e `docs/WINDOWS.md`.

O que sobra é **visual**, e fica no `main.c` (que não toquei). Conferi o caminho que o jogo desenha de fato: o Kojiro sempre aparece pelos sprites (o rig só é desenhado quando
o lutador não tem prancha; `rig_pose(..., 0.06f)` no `EV_PRESS` não aparece na tela), então o que vale é `sprite_press()`:

- o clique é lido no começo do quadro seguinte (até 1 quadro depois: 16,7 ms a 60 Hz, 6,9 ms a 144 Hz) e o `EV_PRESS` põe a defesa **nesse mesmo quadro**: o quadro 0 da `DEFEND`
  já sai no vblank seguinte (até mais 1 quadro);
- o quadro de contato da defesa (o quadro 1 do `DEFEND` do Kojiro, `contact 1`) vem **25 ms depois** (`f_add(f, a, 0, c, 0.05f)`: dois quadros em 50 ms), e fica 0,25 s parado.

Do clique ao quadro 0 na tela: 7 a 14 ms a 144 Hz (média 10) e 17 a 33 ms a 60 Hz (média 25). Do quadro 0 ao de contato: 28 ms a 144 Hz (4 quadros) e 33 ms a 60 Hz (2 quadros: 25 ms não cabe
em um). Ou seja, o parry "fecha" na tela, em média, 38 ms depois do clique a 144 Hz e 58 ms a 60 Hz. Nada disso toca o julgamento.

Das duas saídas que eu tinha citado, só a 1 vale: **monitor de 144 Hz ou mais** (corta o pior caso para menos da metade) e, no Windows, janela sem borda ou tela cheia (o compositor do
sistema soma um quadro). A de encurtar a pose do rig para 30 a 40 ms **não faz nada na tela** (o rig não é desenhado) e a retiro. Se você quiser o fechamento mais rápido a 60 Hz, é uma
linha: `f_add(f, a, 0, c, 0.05f)` para `0.034f` (o quadro 0 dura um quadro de 60 Hz, o de contato chega no seguinte). É mexer na duração de um quadro da animação, então **não proponho
por conta própria**: só se o parry parecer "responder devagar" jogando.

Não dá para medir isso aqui (sem tela de verdade). Num Windows com câmera de celular a 240 fps, ou com o `apara --carimbo` (que mostra o atraso do poll), dá.

## 4. Fluxo visual dos golpes

**O que se via.** O golpe chega ao contato num quadro só: a prancha traz o avanço do corpo pronto no quadro de contato (sem quadro no meio), então o mestre ficava parado
na pose de preparação e **saltava** para a pose do golpe no instante do choque. Medi o meio do corpo (da cintura para baixo) em todos os quadros de golpe das pranchas: do quadro
anterior ao de contato o corpo salta **12 px de mediana**, e em 63 dos 78 golpes `ATTACK_*` o salto passa de 6 px (o `DASH_ATTACK` dos mestres de investida chega a **37 px**, o `ESPECIAL`
do Enjin e do Daichi a 25 px). O jogo parava o corpo no lugar e deixava o resto para o rastro fantasma.

**O que mudou (só desenho, `AJ_DESLIZE_GOLPE` em `ajuste.h`, 0 desliga).** Na partida da lâmina o mestre passa a avançar esse salto, acelerando (o passo cresce com o quadrado da
partida), e **no contato está exatamente onde a prancha o põe**: o quadro de contato entra com o deslize zerado, na mesma conta que já alinhava a ponta da lâmina com a guarda do Kojiro.
O núcleo, as janelas, o contato, as âncoras, as caixas e a duração dos quadros não mudam. Saltos menores que `AJ_DESLIZE_MIN` (6 px) ficam como a prancha tem, e o avanço passa de
`AJ_DESLIZE_MAX` (40 px) nunca. Quem olha para a esquerda avança para a esquerda; se o corpo recua no contato (o Jinshi no `ATTACK_2`), o deslize recua. A sombra, o rastro e os efeitos
presos à lâmina acompanham, porque o deslize entra no mesmo deslocamento do mestre.

O maior passo de um quadro (a 60 Hz, na partida de um `DASH_ATTACK`) cai de 38 px para uns 5 px. Comparei antes e depois quadro a quadro (Daichi, Genbu, Jinshi) e o golpe agora chega
deslizando em vez de aparecer colado no Kojiro. Não deslizei o que vem **depois** do contato (o corpo volta uns 17 px no `DASH_ATTACK`), porque ali o hitstop já segura o quadro: fica
como proposta se parecer duro jogando.

## 5. Como as três decisões ficaram

1. **Pré-carga de efeitos:** só o teste entrou (a pré-carga já estava no jogo, vinda da outra sessão).
2. **Ritmo:** nenhuma das duas mudanças (B e A) foi aplicada: a B endurece o casual em até 1,3 ponto e mexe em duas regras aprovadas, e a A mexe na regra do `waitScale`.
3. **Pose de parry:** resolvida na seção 3 (não há o que mudar no rig).
