# aparar — a trilha dos doze aprendizes

Duelo de parry de um botão, em C puro com [raylib](https://www.raylib.com/).
Sem motor: o loop, a leitura do clique, as animações, os cenários e o som são
todos código daqui. História em `../aparar_lore.md`; direção de arte em
`../parrygame_arte.md`.

## Rodar

```sh
brew install raylib   # uma vez
make run              # compila e abre o jogo
make test             # regras do núcleo, sem janela
```

Controles: **clique, Espaço, J ou Enter** aparam e avançam as falas.
Esc pausa (T volta à trilha, M volta ao menu, Q sai). Na trilha, o botão "menu"
(ou Esc) volta ao menu. F liga e desliga o tremor. F11 alterna a tela cheia.
O progresso fica em `apara_save.txt`, ao lado do executável.

## História

Hattori Hanzo criou a Arte do Aparar e reuniu doze aprendizes em busca de um
sucessor. Oboro, o mais promissor, venceu o mestre depois de incontáveis
desafios e tomou o dojo, como manda a tradição, mas percebeu que Hanzo lhe
concedera a abertura. Passou anos aprendendo as posturas dos outros onze.
Anos depois, Hanzo encontra Musashi, e Musashi sobe a trilha para mostrar que o
estilo do mestre é inabalável. A abertura é narrada sobre o pôr do sol da serra
(segurar Esc enche um anel e pula; segurar o clique acelera).

## Regras

- **Não existe vida, só postura.** Musashi e o adversário têm uma barra de postura cada.
- Nenhum adversário finta, e o jogo não avisa os golpes: o jogador aprende o moveset.
- Entre um gesto e outro há 0,3 s de espera, que zera a cada golpe novo.

| Resultado | musashi | Adversário |
|---|---:|---:|
| Perfeito | +20 | −20 (+2 por aprendiz vencido) |
| Bom | −4 | −6 (+0,5 por aprendiz vencido) |
| Ruim (cedo ou sem defesa) | um "golpe" (tabela abaixo) | +20 do 5º em diante |

musashi começa com 250 de postura e ganha +25 a cada aprendiz vencido. Com
metade da postura, o adversário acelera a preparação (o ritmo dentro de uma
sequência nunca muda). Quebrar a postura **desarma**: a arma voa, crava no chão
e o adversário cai de joelhos. Oboro tem três selos de 360 e um golpe especial
(25% das vezes) que tira o dobro. Depois de duas derrotas seguidas, dá para
**conversar com hanzo**.

## A trilha

| # | Aprendiz | Postura | Arma | Cenário | Postura dele | Perfeito / Bom | Erros até cair |
|---:|---|---|---|---|---:|---:|---:|
| 1 | raijin | touro | odachi | Pátio do dojo | 300 | 90 / 220 ms | 50 |
| 2 | shizuku | água | florete | Cachoeira | 300 | 85 / 210 ms | 40 |
| 3 | kage | noite (apagões) | duas adagas | Bambuzal | 330 | 80 / 200 ms | 35 |
| 4 | daichi | terra | espada pesada | Celeiro | 330 | 75 / 190 ms | 30 |
| 5 | hayate | vento (ritmo quebrado) | katana leve | Trem | 360 | 72 / 185 ms | 25 |
| 6 | genbu | tartaruga | escudo e espada curta | Jardim | 360 | 70 / 180 ms | 22 |
| 7 | enjin | chama | sabre curvo | Forja | 390 | 65 / 170 ms | 20 |
| 8 | suiren | mar (acelerando) | lança | Porto | 390 | 62 / 165 ms | 18 |
| 9 | karasu | corvo | espada e adaga | Cobertura | 420 | 60 / 160 ms | 15 |
| 10 | arashi | tempestade | duas espadas | Salão de espelhos | 420 | 58 / 155 ms | 12 |
| 11 | jinshi | montanha (sem som) | cajado de ferro | Encosta da serra | 450 | 55 / 150 ms | 10 |
| 12 | **oboro** | as onze, trocando a cada duas sequências | katana de hanzo | Dojo de hanzo | 3 × 360 | 70 / 180 → 45 / 130 ms | 10 |

## Moveset

Cada lutador tem um repertório fixo de sequências (`moves` em `src/roster.c`),
com intervalos sempre iguais entre um contato e o próximo. A preparação
denuncia o tipo de golpe (alto, baixo em gedan subindo, ou estocada), e cada
vilão tem um sinal próprio no começo de cada sequência (faíscas no chão, gotas
na lâmina, brasas, penas, névoa…). Os três primeiros têm só dois golpes.

| Aprendiz | Sequências (golpes: intervalos em s) |
|---|---|
| raijin | corte do touro (1) · investida dupla (2: 0,85) |
| shizuku | gota (1) · correnteza (2: 0,50) |
| kage | sombra (1) · presas (2: 0,45) |
| daichi | rocha (1) · desabamento (2: 1,00) · terremoto (3: 0,80 0,50) |
| hayate | rajada (1) · redemoinho (2: 0,45) · vendaval (3: 0,50 0,90) |
| genbu | casco (1) · mordida (2: 0,42) · carapaça (3: 0,90 0,42) |
| enjin | brasa (2: 0,50) · labareda (3: 0,45 0,45) · incêndio (4: 0,45 0,45 0,80) |
| suiren | onda (1) · ressaca (2: 0,50) · maremoto (4: 0,55 0,50 0,45) |
| karasu | bicada (1) · garra (2: 0,55) · revoada (3: 0,50 0,90) |
| arashi | faísca (1) · trovoada (2: 0,45) · tormenta (3: 0,40 0,40) |
| jinshi | pedra (1) · avalanche (3: 0,60 0,60) · cordilheira (4: 0,50 0,50 0,90) |
| oboro | um eco de cada aprendiz, na postura dele · doze posturas (5, terceiro selo) |

## Visual e som

- Cenários desenhados em 1280 × 720; lutadores em pixel art de 320 × 180 por cima,
  com contorno escuro, filete de luz na cor do cenário e rastros nos golpes.
- Bonecos por articulação (`src/rig.c`): poses interpoladas, braços e pernas por
  IK, pés que dão passos nos golpes.
- A postura aparece no corpo: respiração mais curta e guarda mais baixa depois de
  40% perdidos, ofegante e com a lâmina tremendo depois de 75%.
- Parry perfeito: estrela branca e dourada, anel de choque, flash e sino.
  Quebra de postura: tela sem cor, bordas escuras, rachadura branca, cerâmica e
  taiko, meio segundo de silêncio. Execução: tela em duas cores e um traço de
  corte atravessando.
- Katana 3D (`assets/katana`, convertida do FBX com assimp) nas mãos; lança e
  cajado desenhados; escudo e segunda lâmina na mão de trás.
- Interface em pergaminho escuro, no jeito RPG Maker, em Montserrat
  (`assets/fonts`, licença OFL), toda em minúsculo.
- musashi: casaco laranja escuro, cabelo preto até o ombro com franja, sem bandana.
- Trilha sonora sintetizada por cenário; vitória e derrota com sons curtos e sutis.

Da direção de arte ficaram de fora os golpes imbloqueáveis e a esquiva (o jogo é
de um botão só) e o visual índigo do Musashi (fica o laranja escuro).

## Código

| Arquivo | O quê |
|---|---|
| `src/core.c`, `core.h` | Regras puras: relógio, tentativa, postura, selos, moveset, trilha. Sem raylib. |
| `src/roster.c` | Os doze lutadores, falas, conselhos de hanzo e a lore. **Balanceamento é aqui.** |
| `src/main.c` | Telas, coreografia, interface, visuais de cada lutador (`MASTER_LOOKS`) |
| `src/rig.c` | Bonecos: poses, passos, cansaço, roupas, armas |
| `src/katana3d.c` | A katana 3D dentro do mundo em pixel |
| `src/arenas.c` | Os doze cenários |
| `src/lore.c` | Tela de título, trilha, final e a cena do sensei |
| `src/fx.c` | Faíscas, estrela, anéis, arcos, flash, tremor |
| `src/audio.c` | Efeitos e trilha ambiente sintetizados |
| `tests/core_test.c` | Verificações do núcleo (`make test`) |
| `tools/personagens.py` | Gera os 14 lutadores (cabeça, arma, cores, rastro) a partir das pranchas do Musashi; ver `../docs/PERSONAGENS.md` |

Para testar sem jogar: `./apara --master 12 --duel --demo` põe um robô aparando
contra oboro; `--shot arquivo.png 5` salva uma captura depois de 5 segundos.
