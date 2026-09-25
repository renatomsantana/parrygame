# aparar — a trilha dos doze aprendizes

Duelo de parry de um botão, em C puro com [raylib](https://www.raylib.com/).
Sem motor: o loop, a leitura do clique, as animações, os cenários e o som são
todos código daqui. História em `../aparar_lore.md`; direção de arte em
`../parrygame_arte.md`.

## Rodar

```sh
brew install raylib   # uma vez
make packs ZIP=all_the_animations.zip   # põe as tiras dos packs nas pastas (uma vez)
make sprites          # gera os lutadores em pixel art
make run              # compila e abre o jogo
make test             # regras do núcleo, sem janela
```

Os lutadores, os efeitos e as teclas vêm de packs pagos da Mattz Art, que não
vão para o git (o repositório é público). `make packs ZIP=...` pega o zip com
todas as animações e põe cada tira no lugar certo (`tools/instalar_packs.sh`):
o Samurai #3 em `assets/sprites/_original/`, os outros packs em
`assets/sprites/_packs/<pack>/` (os `sprite.txt` já estão lá), os efeitos em
`_fx/` e as teclas em `_ui/`. Depois, `make sprites` gera os lutadores. Sem as
tiras, o jogo roda com os bonecos de `src/rig.c` e as partículas de sempre.

Controles: **clique, Espaço, J ou Enter** aparam e avançam as falas.
Esc pausa (T volta à trilha, M volta ao menu, Q sai). Na trilha, o botão "menu"
(ou Esc) volta ao menu. F liga e desliga o tremor. F11 alterna a tela cheia.
O progresso fica em `apara_save.txt`, ao lado do executável.

## História

Hattori Hanzo criou a Arte do Aparar e reuniu treze aprendizes em busca de um
sucessor. Oboro, o mais promissor, venceu o mestre depois de incontáveis
desafios e tomou o dojo, como manda a tradição, mas percebeu que Hanzo lhe
concedera a abertura. Passou anos aprendendo as posturas dos outros doze.
Anos depois, Hanzo encontra Kojiro, e Kojiro sobe a trilha para mostrar que o
estilo do mestre é inabalável. A tela de título mostra Kojiro de costas em cima de um
morro, à noite, com a espada na bainha e o vento nos fiapos do coque, olhando
para o dojo de Hanzo, pequeno e aceso no pico da serra (desenhado pixel a pixel
em `src/lore.c`); a abertura é narrada sobre essa mesma serra (segurar
Esc enche um anel e pula; segurar o clique acelera).

## Regras

- **Kojiro tem vida; o adversário, postura.** Errar tira vida de kojiro; aparar
  quebra a postura do mestre. A vida no fim faz a borda da tela pulsar.
- Nenhum adversário finta, e o jogo não avisa os golpes: o jogador aprende o moveset.
- Entre um gesto e outro há 0,3 s de espera, que zera a cada golpe novo.

| Resultado | Vida de kojiro | Postura do adversário |
|---|---:|---:|
| Perfeito | +20 | −20 (+2 por aprendiz vencido) |
| Bom | −4 | −6 (+0,5 por aprendiz vencido) |
| Ruim (cedo ou sem defesa) | um "golpe" (tabela abaixo) | +20 do 5º em diante |

kojiro começa com 250 de vida e ganha +25 a cada aprendiz vencido (o perfeito
também devolve vida). Com
metade da postura, o adversário acelera a preparação (o ritmo dentro de uma
sequência nunca muda). Quebrar a postura **desarma**: a arma voa, crava no chão
e o adversário cai de joelhos. Oboro tem três selos de 360 e um golpe especial
(25% das vezes) que tira o dobro. Depois de duas derrotas seguidas, dá para
**conversar com hanzo**.

## A trilha

| # | Aprendiz | Postura | Arma | Cenário | Postura dele | Perfeito / Bom | Erros até cair |
|---:|---|---|---|---|---:|---:|---:|
| 1 | daichi | terra | espada pesada | Celeiro | 300 | 90 / 220 ms | 50 |
| 2 | genbu | tartaruga | katana simples | Jardim | 300 | 87 / 215 ms | 45 |
| 3 | raizo | touro | espadão | Pátio do dojo | 330 | 84 / 208 ms | 40 |
| 4 | shizuku | água | florete de esgrima | Cachoeira | 330 | 80 / 200 ms | 35 |
| 5 | garfiel | tigre | garras nas duas mãos | Portão do tigre branco | 360 | 76 / 192 ms | 30 |
| 6 | karasu | corvo | duas wakizashi | Cobertura | 360 | 72 / 185 ms | 25 |
| 7 | hayate | vento (ritmo quebrado) | duas foices pequenas | Trem | 390 | 69 / 178 ms | 22 |
| 8 | enjin | chama | katana de fogo | Forja | 390 | 66 / 172 ms | 20 |
| 9 | suiren | mar (acelerando) | lança | Porto | 420 | 63 / 166 ms | 18 |
| 10 | arashi | tempestade | duas katanas | Salão de espelhos | 420 | 60 / 160 ms | 15 |
| 11 | yoru | noite (apagões) | duas adagas (ao contrário) | Bambuzal | 450 | 58 / 155 ms | 12 |
| 12 | jinshi | montanha (sem som) | katana branca forjada com a lua | Encosta da serra | 450 | 55 / 150 ms | 10 |
| 13 | **oboro** | as doze, trocando a cada duas sequências | katana de hanzo | Dojo de hanzo | 3 × 360 | 70 / 180 → 45 / 130 ms | 10 |

A dificuldade é da posição na trilha, não do personagem: postura, erros até
cair e janelas apertam a cada passo, e do 5º em diante acertar devolve postura
ao adversário. O que é de cada um (golpes, cenário, apagões, ritmo quebrado,
ondas acelerando, silêncio) vai junto quando ele muda de lugar.

## Moveset

Cada lutador tem um repertório fixo de sequências (`moves` em `src/roster.c`),
com intervalos sempre iguais entre um contato e o próximo. A preparação
denuncia o tipo de golpe (alto, baixo em gedan subindo, ou estocada), e cada
vilão tem um sinal próprio no começo de cada sequência (faíscas no chão, gotas
na lâmina, brasas, penas, névoa…). O **golpe forte** é um só: o salto com a
pancada de cima (o STRONG_ATTACK do pack, ou o especial de quem não tem), com a
preparação no ar, longa e bem visível. Além dele, dois jeitos de chegar que
também se leem de longe: **correndo** (recua num pulinho, firma os pés e vem
correndo até o alcance; o golpe sai da corrida) e **saltando** (agacha, salta e
desce cortando; o contato é o instante em que os pés tocam o chão). Os três
primeiros têm seis sequências, nenhuma com mais de dois contatos; do Shizuku em
diante, de sete a nove, com combos de até cinco golpes.

| Aprendiz | Sequências (golpes: intervalos em s) |
|---|---|
| daichi | rocha (1) · desabamento (2: 1,00) · **terremoto** (forte) · raiz (1) · arado (1) correndo · pedregulho (2: 0,90) saltando |
| genbu | casco (1) · mordida (2: 0,42) · carapaça (2: 0,60) · bote da tartaruga (1) correndo · concha (1) · maré lenta (2: 0,75) |
| raizo | corte do touro (1) · investida dupla (2: 0,85) · **chifrada** (forte) · estouro da boiada (1) correndo · coice (1) · pisada (2: 0,70) saltando |
| shizuku | gota (1) · correnteza (2: 0,50) · queda d'água (3: 0,50 0,45) · garoa (2: 0,40) · **tsunami** (forte) · remanso (1) · salto do peixe (2: 0,45) saltando · corredeira (1) correndo |
| garfiel | patada (1) · garras cruzadas (2: 0,40) · bote do tigre (3: 0,40 0,85) · **salto do tigre** (forte) · rasgo (2: 0,40) · caçada (2: 0,40) correndo · rugido (3: 0,45 0,45) · pulo do gato (2: 0,45) saltando |
| karasu | bicada (1) · garra (2: 0,55) · revoada (3: 0,50 0,90) · **mergulho** (forte) · bando (3: 0,40 0,40) · voo rasante (1) correndo · asa quebrada (2: 0,50) saltando |
| hayate | rajada (1) · redemoinho (2: 0,45) · vendaval (3: 0,50 0,90) · **ciclone** (forte) · brisa cortante (2: 0,40) · lufada (1) correndo · tufão (4: 0,40 0,40 0,60) · folha ao vento (2: 0,55) saltando |
| enjin | brasa (2: 0,50) · labareda (3: 0,45 0,45) · incêndio (4: 0,45 0,45 0,80) · **erupção** (forte) · fagulhas (3: 0,40 0,40) · chama viva (1) correndo · cinzas (2: 0,45) · fogo alto (3: 0,45 0,45) saltando |
| suiren | onda (1) · ressaca (2: 0,50) · maremoto (4: 0,55 0,50 0,45) · **arpão** (forte) · maré baixa (2: 0,45) · arrebentação (3: 0,45 0,45) · espuma (1) correndo · salto da baleia (2: 0,60) saltando |
| arashi | faísca (1) · trovoada (2: 0,45) · tormenta (3: 0,40 0,40) · **relâmpago** (forte) · granizo (4: 0,40 0,40 0,40) · trovão (1) correndo · raio duplo (2: 0,40) saltando · ventania (3: 0,40 0,70) · céu partido (5: 0,40 0,40 0,40 0,80) |
| yoru | sombra (1) · presas (2: 0,45) · lua nova (3: 0,45 0,80) · **eclipse** (forte) · vultos (3: 0,40 0,70) · breu (1) correndo · coruja (2: 0,45) saltando · meia-noite (4: 0,40 0,40 0,90) · nevoeiro (2: 0,70) |
| jinshi | pedra (1) · avalanche (3: 0,60 0,60) · cordilheira (4: 0,50 0,50 0,90) · **deslizamento** (forte) · rocha rolante (2: 0,80) · desfiladeiro (1) correndo · cume (2: 0,70) saltando · tremor (3: 0,60 0,60) · monte (5: 0,50 0,50 0,50 0,90) |
| oboro | um eco de cada aprendiz, na postura dele · doze posturas (5, terceiro selo) · passo de hanzo (1) correndo · queda da lua (1) saltando |

## Visual e som

- Tudo em pixel art de 320 × 180, ampliado por número inteiro e sem filtro:
  cenários, trilha, lore, final, lutadores (com contorno escuro e filete de luz na
  cor do cenário) e a interface.
- Os fundos (cenários, título, lore e trilha) passam por uma paleta curta de 32
  cores, tirada da própria cena (`src/pixelize.c`): cada pixel vai para a cor mais
  perto, e só na faixa entre dois tons vizinhos um xadrez de Bayer 4 × 4 mistura
  os dois. Degradê vira faixas com dithering e brilho vira anéis, como em pixel art
  feita à mão; os lutadores e a interface não passam por ali.
- Lutadores com as pranchas geradas (`src/sprites.c`), em tamanho de pixel 1:1.
  A preparação de cada golpe escolhe a prancha (alto desce, baixo sobe, estocada
  é o corte reto ou a investida; o último golpe das sequências longas é o
  especial). O mestre dá um passo curto e rápido para dentro no começo da
  preparação, firma o corpo (o tronco desce 1 px) e dá o bote quando a lâmina
  parte, para a ponta da arma encontrar a guarda de kojiro no quadro de
  contato, que sai junto com o som do choque; o hitstop segura esse quadro.
  Entre os golpes de um combo ele fica onde está; no fim, volta ao lugar num
  pulo para trás. No bote, na corrida e no salto ele deixa silhuetas na cor do
  seu elemento.
  Kojiro apara com um corte de encontro ao golpe (ou com a DEFEND, quando a
  prancha existir). Parry perfeito: o mestre acusa (HURT). Desarme: o mestre fica
  de joelhos sem a arma (DESARMADO) e kojiro avança com a lâmina baixa. Oboro
  ataca com o eco da postura em que está e, a cada selo quebrado, grita (GRITO)
  e volta em fúria. Quem não tem prancha parada respira (o tronco desce 1 px).
- Sem as pranchas: bonecos por articulação (`src/rig.c`), com poses interpoladas,
  IK, respiração e cansaço.
- Parry perfeito: estrela branca e dourada, anel de choque, flash e sino, e os
  raios de luz da folha de efeitos atrás do choque. Bom: faíscas douradas. Erro:
  estouro vermelho. Cada mestre abre a sequência com o efeito do elemento dele
  (poeira, casco, respingo, garras, redemoinho, labareda, onda, raios, noite,
  montanha; oboro, o do aprendiz da postura, em vermelho). Os efeitos de energia
  ficam atrás dos lutadores e somam luz.
  Quebra de postura: tela sem cor, bordas escuras, rachadura branca, cerâmica e
  taiko, meio segundo de silêncio. Execução: tela em duas cores e um traço de
  corte atravessando.
- Katana 3D (`assets/katana`, convertida do FBX com assimp) na espada que voa no
  desarme (e nas mãos dos bonecos).
- Interface em janelas de pergaminho de pixel, no jeito RPG Maker (cantos
  cortados, borda de tinta de 1 px, rolos de madeira, gauges e cursor em pixel),
  na fonte de pixel Tiny5 (`assets/fonts`, licença OFL), toda em minúsculo. O
  layout é pensado em 1280 × 720, mas tudo cai na grade de 320 × 180.
- As falas do duelo ficam numa caixa no alto, para os lutadores aparecerem
  inteiros; na cena de hanzo e no final, hanzo e kojiro também são os sprites.
- Teclas de pixel na pausa e, no primeiro duelo, a dica de como aparar.
- Trilha sonora sintetizada por cenário; vitória e derrota com sons curtos e sutis.

Da direção de arte ficaram de fora os golpes imbloqueáveis e a esquiva (o jogo é
de um botão só).

## Código

| Arquivo | O quê |
|---|---|
| `src/core.c`, `core.h` | Regras puras: relógio, tentativa, postura, selos, moveset, trilha. Sem raylib. |
| `src/roster.c` | Os treze lutadores (doze aprendizes e oboro), falas, conselhos de hanzo e a lore. **Balanceamento é aqui.** |
| `src/main.c` | Telas, coreografia, interface, visuais de cada lutador (`MASTER_LOOKS`) |
| `src/sprites.c` | Lê as pranchas geradas (`sprite.txt` e tiras) e desenha e toca os lutadores em pixel art; efeitos em folha e teclas |
| `src/rig.c` | Bonecos (quando faltam as pranchas): poses, passos, cansaço, roupas, armas |
| `src/katana3d.c` | A katana 3D dentro do mundo em pixel |
| `src/arenas.c` | Os treze cenários dos duelos (e o do título) |
| `src/pixelize.c` | Paleta curta por cena e dithering ordenado nos fundos |
| `src/lore.c` | Tela de título, trilha, final e a cena do sensei |
| `src/fx.c` | Faíscas, estrela, anéis, arcos, flash, tremor |
| `src/audio.c` | Efeitos e trilha ambiente sintetizados |
| `tests/core_test.c` | Verificações do núcleo (`make test`) |
| `tools/instalar_packs.sh` | Põe o zip das animações nas pastas do gerador e do jogo (`make packs ZIP=...`) |
| `tools/personagens.c` | Gera os 15 lutadores (cabeça, arma, corpo, rastro, aura, golpe especial, pose desarmada), uma pasta por nome (kojiro, raizo, yoru, garfiel...), a partir do Samurai #3 e dos packs de `assets/sprites/_packs/` (Raizo com o espadão, Shizuku, Arashi, Oboro com as posturas dos outros e o grito): `make sprites`; ver `../docs/PERSONAGENS.md` |

Para testar sem jogar: `./apara --master 13 --duel --demo` põe um robô aparando
contra oboro; `--shot arquivo.png 5` salva uma captura depois de 5 segundos;
`--state pause|defeat|finisher|cleared` (com `--master N --duel`) abre direto a
pausa, a derrota, o desarme ou a vitória; `--rec pasta 1 5` salva os quadros de
1 a 5 segundos (30 por segundo, tempo fixo), para GIFs.
