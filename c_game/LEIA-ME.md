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

A abertura conta a história como Kojiro acredita nela: Hattori Hanzo criou a
Arte do Aparar, Oboro, o aluno mais promissor, venceu o mestre e tomou o dojo,
e anos depois Hanzo recolheu Kojiro, um órfão que viu o pai morrer pelas mãos de
um homem com máscara de oni. Kojiro sobe a trilha pela vingança. Cada aprendiz
pensa uma coisa de Hanzo e de Oboro (uns falam bem do velho, outros devem a vida
a Oboro, outros chamam Hanzo de monstro, e dois viram que o último duelo não foi
justo), e depois de cada vitória Kojiro volta à cabana de Hanzo na serra, que
comenta o vencido e fala do próximo. Na luta final, Oboro fala a cada selo
quebrado, põe a máscara de oni no terceiro e, de joelhos, a tira: "Eu achei isso
no baú dele." Aí vem a escolha, **DESEJA MATAR O OBORO?**, sem nada marcado e sem
tempo, e um final para cada resposta. A história inteira está em
`../aparar_lore.md`.

A tela de título mostra Kojiro de costas em cima de um
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
**conversar com hanzo** (no Oboro, ele só diz "Confie em você mesmo. Use tudo
que aprendeu.").

## A trilha

| # | Aprendiz | Postura | Arma | Cenário | Postura dele | Perfeito / Bom | Erros até cair |
|---:|---|---|---|---|---:|---:|---:|
| 1 | daichi | terra (bem devagar) | katana | Celeiro | 300 | 90 / 220 ms | 50 |
| 2 | genbu | tartaruga | katana simples | Jardim de pedras do mosteiro | 300 | 87 / 215 ms | 45 |
| 3 | raizo | touro | espadão | Pátio do dojo | 330 | 84 / 208 ms | 40 |
| 4 | shizuku | água | florete de esgrima | Cachoeira | 330 | 80 / 200 ms | 35 |
| 5 | garfiel | tigre | garras nas duas mãos | Portão do tigre branco | 360 | 76 / 192 ms | 30 |
| 6 | karasu | corvo | katana e wakizashi | Telhados da vila na chuva | 360 | 72 / 185 ms | 25 |
| 7 | hayate | vento (ritmo quebrado) | duas foices pequenas | Ponte de corda no desfiladeiro | 390 | 69 / 178 ms | 22 |
| 8 | enjin | chama | katana de fogo | Forja | 390 | 66 / 172 ms | 20 |
| 9 | suiren | mar (acelerando) | lança | Porto | 420 | 63 / 166 ms | 18 |
| 10 | arashi | tempestade (dano 1,5×) | duas katanas | Salão do castelo na tempestade | 420 | 60 / 160 ms | 15 |
| 11 | yoru | noite (apagões) | duas adagas (ao contrário) | Bambuzal | 450 | 58 / 155 ms | 12 |
| 12 | jinshi | lua (sem som) | katana bem branca, forjada com a lua | Encosta da serra | 450 | 55 / 150 ms | 10 |
| 13 | **oboro** | hanzo → devorador de posturas → oni (uma por selo) | katana de hanzo | Dojo de hanzo | 3 × 360 | 70 / 180 → 45 / 130 ms | 10 |

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
preparação no ar, longa e bem visível. Além dele, jeitos de chegar que se leem de
longe: **correndo** (recua num pulinho, firma os pés e vem correndo até o
alcance), **saltando** (agacha, salta e desce cortando; o contato é o pouso) e,
só com a lança do suiren, **de longe** (ele se afasta e a ponta viaja: entre a
lâmina partir e chegar passa 1,75 vez o tempo dos outros golpes, então quem
aparar no susto da partida chega cedo). O karasu tem ainda o **sumiço**: no meio
da preparação ele vira penas e some, e reaparece na frente de kojiro com a lâmina
no alto, pouco antes de ela partir; é o reaparecer que avisa. **⚔** é o golpe de **duas lâminas**
(o corte cruzado, anunciado por um brilho duplo e um tinido duplo): um parry só
segura as duas se for perfeito; no bom, a segunda entra; no erro, entram as
duas. Todos têm de sete a dez sequências. Daichi, Genbu e Raizo nunca passam de
dois contatos; Garfiel chega a oito golpes seguidos; arashi bate 1,5 vez mais
forte; enjin deixa kojiro **em brasas** a cada erro
(ele perde mais 60% de um golpe ao longo de 3 s, e o parry perfeito apaga); yoru
apaga as luzes em sete de cada dez sequências; jinshi tem o
repertório mais variado e o ritmo irregular.

| Aprendiz | Sequências (golpes: intervalos em s) |
|---|---|
| daichi | rocha (1) · raiz (1) · sulco (1) · desabamento (2: 1,00) · arado (1) correndo · pedregulho (2: 1,10) saltando · **terremoto** (forte) |
| genbu | casco (1) · mordida (2: 0,42) · carapaça (2: 0,60) · concha (1) · bote da tartaruga (1) correndo · maré lenta (2: 0,75) · casco fechado (2: 0,90) |
| raizo | corte do touro (1) · investida dupla (2: 0,85) · coice (1) · marrada (2: 1,00) · estouro da boiada (1) correndo · pisada (2: 0,70) saltando · **chifrada** (forte) |
| shizuku | gota (1) · correnteza (2: 0,45) · garoa (2: 0,40) · queda d'água (3: 0,45 0,40) · chuvisco (4: 0,40 0,40 0,40) · remanso (1) · corredeira (1) correndo · salto do peixe (2: 0,45) saltando · **tsunami** (forte) |
| garfiel | patada (1) · garras cruzadas (2: 0,40) · rasgo (3: 0,40 0,40) · bote do tigre (3: 0,40 0,85) · fúria do tigre (6: 0,40 0,40 0,40 0,40 0,40) · caçada (7: 0,40 0,40 0,45 0,40 0,40 0,70) correndo · rugido (8: 0,40 0,40 0,40 0,40 0,40 0,40 0,80) · pulo do gato (2: 0,45) saltando · **salto do tigre** (forte) |
| karasu | bicada (1) · garra (2: 0,55) · sumiço (1) sumindo · corvo fantasma (2: 0,55) sumindo · revoada (3: 0,50 0,90) ⚔ no 3º · bando (4: 0,45 0,45 0,45) ⚔ no 4º · duas penas (1) ⚔ · voo rasante (1) correndo · asa quebrada (2: 0,50) saltando · **mergulho** (forte) |
| hayate | rajada (1) · redemoinho (2: 0,45) · vendaval (3: 0,50 0,90) · brisa cortante (2: 0,40) · tufão (5: 0,40 0,40 0,40 0,60) · foices gêmeas (2: 0,45) ⚔ no 2º · lufada (1) correndo · folha ao vento (2: 0,55) saltando · **ciclone** (forte) |
| enjin | brasa (2: 0,50) · labareda (3: 0,45 0,45) · incêndio (4: 0,45 0,45 0,80) · **erupção** (forte) · fagulhas (3: 0,40 0,40) · chama viva (1) correndo · cinzas (2: 0,45) · fogo alto (3: 0,45 0,45) saltando |
| suiren | onda (1) · arpão (1) de longe · linha d'água (2: 0,60) de longe · maré longa (3: 0,60 0,50) de longe · maré baixa (2: 0,45) · arrebentação (3: 0,45 0,45) · maremoto (4: 0,55 0,50 0,45) · espuma (1) correndo · salto da baleia (2: 0,60) saltando · **vagalhão** (forte) |
| arashi | faísca (1) · duas tempestades (1) ⚔ · trovoada (2: 0,45) ⚔ no 2º · tormenta (3: 0,40 0,40) ⚔ no 3º · granizo (4: 0,40 0,40 0,40) · ventania (3: 0,40 0,70) · trovão (1) correndo ⚔ · raio duplo (2: 0,40) saltando ⚔ nos dois · céu partido (5: 0,40 0,40 0,40 0,80) ⚔ no 5º · **relâmpago** (forte) ⚔ |
| yoru | sombra (1) · presas (2: 0,45) · lua nova (3: 0,45 0,80) · **eclipse** (forte) · vultos (3: 0,40 0,70) · breu (1) correndo · coruja (2: 0,45) saltando · meia-noite (4: 0,40 0,40 0,90) · nevoeiro (2: 0,70) |
| jinshi | crescente (1) · minguante (3: 0,60 0,60) · fases da lua (4: 0,50 0,50 0,90) · luar (3: 0,45 1,00) · lua cheia (5: 0,50 0,50 0,50 0,90) · lua branca (6: 0,40 0,90 0,45 0,45 1,00) · noite branca (4: 1,00 0,40 0,40) · reflexo no lago (1) correndo · lua alta (2: 0,70) saltando · **halo** (forte) |
| oboro | **postura de hanzo:** corte do mestre (1) · lição (2: 0,60) · estocada de hanzo (1) · três lições (3: 0,50 0,60) · passo de hanzo (1) correndo · salto do mestre (2: 0,55) saltando · **devorador de posturas:** um eco de cada aprendiz, com o elemento dele (eco da terra, da tartaruga, do touro, da água, do tigre, do corvo, do vento, da chama, do mar, da tempestade, da noite, da lua) · **postura do oni** (de máscara, lâmina em chamas): fúria do oni (4: 0,40 0,40 0,70) · chama do oni (3: 0,45 0,45) · doze posturas (6: 0,40 0,40 0,40 0,40 0,90) · investida do oni (1) correndo · mergulho do oni (2: 0,50) saltando · **golpe do oni** (forte) |

## Visual e som

- Tudo em pixel art de 320 × 180, ampliado por número inteiro e sem filtro:
  cenários, trilha, lore, final, lutadores (com contorno escuro e filete de luz na
  cor do cenário) e a interface.
- Todos os cenários são do Japão antigo: casa de fazenda, jardim de pedras com o
  laguinho de carpas e a ilha da tartaruga, pátio do dojo, cachoeira entre
  árvores, o portão de dois andares do tigre branco (bandeiras listradas,
  lanternas de pedra, bordos), telhados da vila do castelo na chuva (uma fileira
  de casas iguais de janelas quadradas, a lua crescente e os corvos na cumeeira),
  ponte de corda sobre o desfiladeiro no vento (serras de picos na névoa), a forja
  dentro da cratera (lago e cascata de lava, o barracão com a corda sagrada, a
  fornalha de barro e as katanas esfriando), o porto do farol, o salão do castelo
  numa noite de tempestade (biombos de ouro, as portas abertas para a chuva e os
  raios que clareiam a luta, com trovão), bambuzal, a encosta da serra com a lua
  cheia grande e o dojo de Hanzo.
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
  inteiros; na cabana de hanzo e nos finais, hanzo e kojiro também são os sprites.
- Oboro luta de rosto descoberto nas duas primeiras formas: o gerador acha a
  máscara de oni do pack em cada quadro e pinta um rosto cansado no lugar
  (`oboro/`); a prancha com a máscara sai em `oboro_mascara/`.
- A escolha do fim corta a música e deixa só o vento; nos finais, Hanzo entra
  batendo palmas (sim) ou sai do escuro (não).
- Teclas de pixel na pausa e, no primeiro duelo, a dica de como aparar.
- Trilha sonora sintetizada por cenário; vitória e derrota com sons curtos e sutis.

Da direção de arte ficaram de fora os golpes imbloqueáveis e a esquiva (o jogo é
de um botão só).

## Código

| Arquivo | O quê |
|---|---|
| `src/core.c`, `core.h` | Regras puras: relógio, tentativa, postura, selos, moveset, trilha. Sem raylib. |
| `src/roster.c` | Os treze lutadores (doze aprendizes e oboro), falas, visitas e conselhos de hanzo, a lore e as cenas da luta final e dos finais. **Balanceamento é aqui.** |
| `src/main.c` | Telas, coreografia, interface, visuais de cada lutador (`MASTER_LOOKS`) |
| `src/sprites.c` | Lê as pranchas geradas (`sprite.txt` e tiras) e desenha e toca os lutadores em pixel art; efeitos em folha e teclas |
| `src/rig.c` | Bonecos (quando faltam as pranchas): poses, passos, cansaço, roupas, armas |
| `src/katana3d.c` | A katana 3D dentro do mundo em pixel |
| `src/arenas.c` | Os treze cenários dos duelos (e o do título) |
| `src/pixelize.c` | Paleta curta por cena e dithering ordenado nos fundos |
| `src/lore.c` | Tela de título, trilha e a cabana de hanzo |
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
