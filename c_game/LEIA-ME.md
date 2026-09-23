# APARA em C — A Trilha dos Doze Mestres

Versão em C puro com [raylib](https://www.raylib.com/). Sem motor: o loop,
a leitura do clique, as animações, os cenários e o som são todos código daqui.

## Rodar

```sh
brew install raylib   # uma vez
make run              # compila e abre o jogo
make test             # regras do núcleo, sem janela
```

Controles: **clique, Espaço, J ou Enter** aparam e avançam as falas.
Esc pausa (T volta à trilha, M volta ao menu, Q sai). Na trilha, o botão "menu" (ou Esc) volta ao menu. F liga e desliga o tremor. F11 alterna a tela cheia.
O progresso fica em `apara_save.txt`, ao lado do executável.

## O que mudou em relação à versão Unity/RPG Maker

- **Não existe vida, só postura.** musashi e o mestre têm uma barra de postura cada.
  Quem chega a zero cai.
- **Doze mestres e o BIG BOSS** (décimo terceiro), cada um com cenário próprio e animado.
- **Abertura narrada**: parágrafos sobem sobre o pôr do sol da serra, como página de livro,
  com duas citações em dourado. A história: os doze mestres e Oboro eram discípulos de Hanzo e
  nunca aceitaram o estilo dele. Oboro venceu o mestre à traição, no meio de uma reverência, e
  cortou o braço dele; pelo costume, os doze passaram a segui-lo. Musashi, que chegou ao dojo
  criança e que Hanzo viu cedo que seria um prodígio, desce a serra não por vingança, mas para
  mostrar que o estilo do mestre é inabalável.
  Segurar Esc enche um anel e pula; segurar o clique acelera.
- Tudo é desenhado em **320 × 180** e ampliado por número inteiro, sem filtro: pixel nítido.

## Estilo

Pixel art nítida em 320 × 180, ampliada só por número inteiro, no espírito de
Katana Zero: cada lutador tem contorno escuro de 1 px e um filete de luz neon do
lado de trás, na cor do cenário (`arena_rim`); golpes rápidos deixam rastros de
silhueta em ciano e magenta; os impactos acendem uma aberração cromática, e a
imagem final tem scanlines e vinheta sutis (`POST_FS` em `src/main.c`). A katana 3D
é desenhada dentro dessa pixel art. A tela de título é só paisagem.

## Arte provisória

Os lutadores são bonecos montados por articulações (`src/rig.c`). Cada pose é um
punhado de números (inclinação, agachamento, mãos, ângulo da espada, passos) e o
movimento é a interpolação entre poses, com duração exata: o corte do mestre
chega na pose de contato no instante do contato. Faixa da testa e barra do
casaco têm movimento secundário.

A aparência de cada lutador (cores, chapéu, tamanho, comprimento da lâmina) fica
em `REN_LOOK` e `MASTER_LOOKS`, no começo de `src/main.c`. Quando a pixel art
final chegar, ela entra no lugar do `rig_draw` com os mesmos momentos de pose.

## Postura

Nenhum mestre finta por enquanto. Entre um gesto e outro há 0,3 s de espera, que zera a cada
golpe novo: um gesto feito no intervalo nunca rouba a defesa do golpe seguinte.

| Resultado | musashi | Mestre |
|---|---:|---:|
| Perfeito | +20 | −20 (+2 por mestre já vencido) |
| Bom | −4 | −6 (+0,5 por mestre já vencido) |
| Ruim (cedo ou sem defesa) | um "golpe" (tabela abaixo) | +20 do 5º mestre em diante |

musashi começa com 250 de postura e ganha +25 a cada mestre vencido. Cada mestre
derruba musashi num número fixo de erros: 50, 40, 35, 30, 25, 22, 20, 18, 15, 12, 10, 10 e 10.
A postura dos mestres vai de 300 (tetsu) a 450 (magna e sombra); o oboro tem 360 em cada um dos três selos.
O oboro tem um **golpe especial** (25% das vezes, com o corpo ardendo em vermelhão)
que tira o dobro. Com metade da postura, o mestre acelera 10%.

Quebrar a postura do mestre **desarma**: a espada dele voa girando, crava no chão,
ele cai de joelhos e musashi aponta a lâmina. O oboro tem três selos; os dois primeiros
só o fazem cambalear.

Depois de duas derrotas seguidas contra o mesmo mestre, o painel da derrota
oferece **conversar com hanzo**: dois conselhos filosóficos sobre aquele mestre,
e o duelo recomeça.

## A trilha

| # | Mestre | Postura de combate | Cenário | Perfeito / Bom | Estilo |
|---:|---|---|---|---:|---|
| 1 | tetsu, antigo capitão de musashi | touro | Pátio da velha guarda | 90 / 220 ms | Ritmo direto |
| 2 | neon jax | macaco | Rave | 85 / 210 ms | Sincopado |
| 3 | cavan | urso | Celeiro | 80 / 200 ms | Golpes pesados, segundo atrasado |
| 4 | vance | raposa | Cobertura | 75 / 190 ms | Pausa traiçoeira no meio |
| 5 | kaelen | névoa | Galeria | 72 / 185 ms | Tinta, sinal fraco |
| 6 | taiko | maré | Porto | 70 / 180 ms | Acelerando |
| 7 | eleonor | garça | Salão de espelhos | 65 / 170 ms | Estocadas longas |
| 8 | kira | trovão | Trem-bala | 62 / 165 ms | Golpe duplo |
| 9 | hayate | serpente | Cachoeira | 60 / 160 ms | Sem som de aviso |
| 10 | yoru | lobo | Bambuzal | 58 / 155 ms | Apagão |
| 11 | magna | fogo | Forja | 55 / 150 ms | Golpe triplo |
| 12 | sombra | espelho | Jardim de Vidro | 50 / 140 ms | O ritmo de musashi, mais curto |
| 13 | **oboro (BIG BOSS)** | dragão: tartaruga e tigre | Cidadela | 70/180 · 45/130 | Selos, compostos, especial |

## Moveset

Os três primeiros mestres têm só dois golpes cada, e na fala de desafio dizem
exatamente o que fazem. Daí em diante o repertório cresce e as dicas ficam mais
curtas. A pressa (metade da postura, terceiro selo) acelera só a preparação: o
ritmo dentro de uma sequência é sempre o mesmo.

Cada mestre tem um repertório fixo de **sequências** (`moves` em `src/roster.c`):
de 1 a 5 golpes seguidos, com intervalos sempre iguais entre um contato e o
próximo. A preparação denuncia a sequência (alta, baixa ou estocada), e os
golpes seguintes alternam de altura. Como num soulslike, o jogo não avisa nada:
nem "perfeito", nem "cedo demais", nem "golpe duplo". O jogador aprende o
moveset de cada mestre de tanto enfrentá-lo, e o hanzo ajuda quem travar.

| Mestre | Sequências (golpes, intervalos em s) |
|---|---|
| tetsu | chifrada (1) · dois chifres (2: 0,80) |
| neon jax | tapa (1) · passo quebrado (2: 0,55) |
| cavan | patada (1) · peso morto (2: 1,00) |
| vance | proposta (1) · contrato (2: 0,55) · cláusula (3: 0,50 0,90) |
| kaelen | pincelada (1) · respingo (2: 0,60) · borrão (3: 0,45 0,70) |
| taiko | batida (1) · dois tempos (2: 0,50) · rufar (4: 0,55 0,50 0,45) |
| eleonor | estocada (1) · estocada dupla (2: 0,42) · valsa (3: 0,45 0,45) |
| kira | relâmpago (1) · trovão duplo (2: 0,45) · tempestade (3: 0,40 0,40) |
| hayate | bote (1) · dupla picada (2: 0,70) · serpente (3: 0,50 0,80) |
| yoru | mordida (1) · matilha (2: 0,50) · caçada (3: 0,60 0,40) |
| magna | martelo (2) · bigorna (3) · forja (4: 0,45 0,45 0,80) |
| sombra | reflexo (1) · eco (2: 0,40) · espelho partido (3: 0,40 0,70) |
| oboro | tartaruga: casco (1), maré lenta (2) · tigre: garra (2), salto do tigre (3), fúria do tigre (4, 2º selo) · sopro do dragão (5, 3º selo) |

## Katana 3D

`assets/katana/katana.glb` é o modelo de `katana 3d/` (FBX original), convertido com
`assimp export katanaFINAL1_low.fbx katana.glb -fglb2 -ptv`, com a textura base
reduzida para 256 px. O jogo desenha a katana 3D na mão de musashi e dos mestres, na
espada que voa no desarme (girando no próprio eixo) e girando devagar na tela de
título. Ela é renderizada dentro do mundo de 320 × 180, com luz simples e cores
reduzidas a poucos tons, para parecer pixel art. Sem os arquivos, o jogo volta à
lâmina desenhada.

## Interface

Os cenários são desenhados em 1280 × 720, com degradês, sombras, madeira, reflexos e
objetos de cena; os lutadores continuam em pixel art de 320 × 180 por cima. A interface vai
por cima em alta resolução, no
jeito RPG Maker, em janelas de pergaminho (textura gerada no início: papel
manchado com bordas queimadas, borda dupla de tinta, bastões de madeira nas
pontas). No duelo, duas placas pequenas: o mestre em cima, musashi embaixo, cada uma
com nome e gauge de postura. As falas têm a caixa do nome separada e a seta de
continuar; os menus têm cursor de seleção. Fonte Montserrat (Medium e SemiBold,
`assets/fonts`, licença OFL), tudo em minúsculo na interface.

## Roupas e katanas dos mestres

Cada mestre se veste de acordo com o seu lugar (`MASTER_LOOKS` em `src/main.c`):
tetsu de haori com ombreiras e hakama, neon jax de jaqueta com neon e visor,
cavan de colete de palha e lenço, vance de sobretudo, gravata e óculos, kaelen
de avental de pintor e cachecol, taiko de happi e faixa, eleonor de vestido de
baile, kira de uniforme de maquinista, hayate de túnica de monge com contas,
yoru de capuz e cachecol, magna de avental de couro e braços de fora, oboro de
armadura com capa. A katana 3D muda por mestre: cor da lâmina, cor do cabo,
largura e comprimento (nodachi do cavan e do oboro, lâmina fina da eleonor,
larga e em brasa da magna, negra do vance e da yoru, neon do jax).

## musashi (protagonista)

Casaco e mangas em laranja, calça escura, cabelo preto longo que balança com o
movimento (`hairTail` em `src/rig.c`).

## Código

| Arquivo | O quê |
|---|---|
| `src/core.c`, `core.h` | Regras puras: relógio, tentativa, postura, selos, trilha (a finta existe no núcleo, mas nenhum mestre usa). Sem raylib. |
| `src/roster.c` | Os treze mestres, nomes, falas e a lore. **Balanceamento é aqui.** |
| `src/main.c` | Telas, coreografia das animações, HUD, entrada e progresso |
| `src/katana3d.c` | Carrega e desenha a katana 3D no mundo em pixel |
| `src/rig.c` | Bonecos provisórios: poses, interpolação, faixa, casaco, rastro da lâmina |
| `src/arenas.c` | Os treze cenários, em 320 × 180 |
| `src/lore.c` | Ilustrações da abertura, do mapa da trilha e do final |
| `src/fx.c` | Faíscas, anéis, arcos de corte, flash, tremor, soco de câmera |
| `src/audio.c` | Efeitos e trilha ambiente sintetizados, uma por cenário |
| `tests/core_test.c` | 846 verificações do núcleo |

O clique é aplicado no meio do quadro em que chegou (o mundo avança meio quadro,
recebe o clique e avança o resto), e o jogo roda no vsync do monitor.
Hitstop congela o duelo; o golpe final roda em câmera lenta.
Nomes, falas e lore ficam em `src/roster.c`.

Para testar sem jogar: `./apara --master 13 --duel --demo` põe um robô aparando
no tempo perfeito contra o BIG BOSS. `--shot arquivo.png 5` salva uma captura
depois de 5 segundos e sai.
