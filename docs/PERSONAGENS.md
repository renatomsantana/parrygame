# Personagens: o elenco saído do Samurai #3

> Substitui a seção 4 (troca de paleta) do brief visual v2. Pode colar este
> arquivo inteiro no Claude Code.

Os 15 personagens (o protagonista Kojiro, os 12 aprendizes na ordem da trilha,
Oboro e Hanzo) saem
de quatro corpos da Mattz Art, e não são só troca de cor. A maioria usa o corpo
do pack A (o Samurai #3). Os outros têm corpo de outro pack: **Raizo** (o samurai
do espadão, de chapéu de palha), **Shizuku** (Samurai #4), **Oboro** (o Demon,
de máscara oni) e os cinco que lutam com uma arma em cada mão, **Arashi, Yoru,
Garfiel, Karasu e Hayate**, no Samurai #5 (o de duas espadas), cada um com as
suas cores e a sua arma. Ver
[Corpos de outros packs](#corpos-de-outros-packs-raizo-shizuku-arashi-e-oboro).
O programa `c_game/tools/personagens.c` (C com raylib, igual ao jogo) pega cada
quadro das pranchas e:

- **tira o chapéu** e desenha a cabeça no lugar (coque, rabo de cavalo, capuz,
  careca, cabelo em chamas...). Só o Daichi fica de chapéu (palha). O Kojiro
  fica **sem chapéu e sem máscara, de coque solto**;
- **troca a arma** seguindo a reta da katana em cada quadro, sempre do mesmo
  tamanho, e lança e florete passam a **estocar** no golpe reto em vez de
  cortar em arco;
- **monta um golpe especial** para cada aprendiz (`ESPECIAL.png`), com
  preparação própria e um efeito grande do elemento no impacto;
- **muda o corpo**: mais largo, mais estreito, mais alto ou mais baixo, e um
  passo à frente no contato conforme a arma;
- **dá ao rastro o formato da arma** (adaga corta duplo, garra deixa três
  riscos, arma pesada abre um rastro grosso, duas espadas deixam um eco) e a cor
  do elemento;
- **põe uma aura do elemento** em volta do corpo: labaredas, bolhas, raios,
  fumaça, penas, vento, poeira, fagulhas de ouro. Ela fica mais forte na
  preparação e no contato;
- **acrescenta acessórios atrás do corpo**: cachecol, casco, fitas, rabo de cavalo.

Tudo é pixel inteiro, sem escala nem rotação, e o mesmo comando sempre gera os
mesmos pixels.

## O elenco

| # | Personagem | Arma | Cabeça e corpo | Cores | Rastro e aura |
|---:|---|---|---|---|---|
| — | **Kojiro** | katana | sem chapéu e **sem máscara**: coque solto no alto da cabeça com fita vermelha, mechas na testa, barba rala | original (branco e preto) | branco, sem aura |
| 1 | **Daichi** (terra) | espada pesada, lâmina larga | **chapéu de palha**, barba; 1 px mais largo | verde oliva, ocre | rastro grosso, poeira no chão |
| 2 | **Genbu** (tartaruga) | katana simples + casco nas costas | careca, barbicha; atarracado | verde musgo | verde, esporos |
| 3 | **Raizo** (touro) | **espadão** do próprio pack, do jeito que vem | **corpo do samurai do espadão**, chapéu de palha; a roupa preta virou marrom | marrom, amarelo | ouro, fagulhas subindo |
| 4 | **Shizuku** (água) | **florete de esgrima**: lâmina reta e fina, copo na mão | **corpo do Samurai #4**, cabelo roxo virou azul petróleo | azul claro, ciano, hakama azul | água, bolhas |
| 5 | **Garfiel** (tigre branco, Byakko) | **garras nas duas mãos** (três lâminas em cada) | **corpo do samurai de duas espadas**, cabelo loiro, roupa preta listrada de vermelho | preto e vermelho, cabelo loiro | terra, três riscos de garra |
| 6 | **Karasu** (corvo) | **duas wakizashi** (espadas curtas, 0,72× a katana) | **corpo do samurai de duas espadas**, preto e vermelho, trapo vermelho | preto e vermelho | eco da segunda lâmina, penas caindo |
| 7 | **Hayate** (vento) | **duas foices** (kama) do tamanho de uma espada: cabo longo e a lâmina curva na ponta | **corpo do samurai de duas espadas**, verde claro e limão, cachecol | verde claro, limão | vento; no golpe, **cortes de vento** voam para a frente |
| 8 | **Enjin** (chama) | katana de fogo | cabelo em chamas; 1 px mais largo | vermelho e amarelo | fogo, labaredas e brasas |
| 9 | **Suiren** (mar) | lança com ponta em folha, anel e fita, **estoca** | cabelo curto, faixa turquesa | azul mar, turquesa | água, bolhas |
| 10 | **Arashi** (tempestade) | duas katanas com raios | **corpo do Samurai #5**, cabelo prateado, olhos de raio | preto, azul elétrico | raios nas lâminas e no corpo |
| 11 | **Yoru** (noite) | uma adaga em cada mão, **empunhadas ao contrário** (lâmina para trás), brilho roxo | **corpo do samurai de duas espadas**, ninja preto e roxo | preto azulado e roxo | corte duplo, fumaça roxa |
| 12 | **Jinshi** (montanha) | **katana branca forjada com a lua**, com halo de luar | cabelo grisalho comprido, barba; mais alto | cinza pedra, branco osso | rastro grosso, pedrisco caindo |
| — | **Oboro** | katana de Hanzo, dourada | **corpo do Demon** (máscara oni), com as versões de fúria | azul e vermelho do pack | sombra; nos ecos, o de cada aprendiz |
| — | **Hanzo** | **nenhuma**: um velho que não luta mais | coque branco, **sem barba**; mais baixo | azul escuro | sem rastro nem aura |

As armas do pedido caíram assim: garras → Garfiel, espada maior → Raizo
(odachi), florete → Shizuku, adagas roxas → Yoru, espada e lâmina curta →
Karasu, duas foices → Hayate, espadas com raios → Arashi,
espada de fogo → Enjin, lança de água → Suiren.

**Nomes e pastas:** cada personagem tem uma pasta com o nome atual: `kojiro`,
`raizo`, `shizuku`, `yoru`, `daichi`, `hayate`, `genbu`, `enjin`, `suiren`,
`karasu`, `arashi`, `jinshi`, `garfiel`, `oboro` e `hanzo`. O `id` em `CHARS` é
o nome da pasta e das tiras (`ATTACK_1_ECO_KARASU`); o `titulo` é o que aparece
nas folhas. As pastas geradas com os nomes antigos (`raijin/`, `kage/`) ficam
paradas; o programa avisa e elas podem ser apagadas. Os diálogos do jogo ainda
chamam o protagonista de `musashi` (não mexi no jogo).

**Hanzo:** o pedido era o Hanzo do pack B sem a barba. Esse sprite não estava
aqui, então o Hanzo saiu do corpo do pack A, de coque branco e sem barba. Assim
ele também combina com o resto. Ele **não luta mais**: não tem espada, bainha, rastro nem aura, e só
ganha as pranchas que não são de luta (IDLE, RUN, HURT, DEATH, JUMP...; os
ATTACK, DEFEND, THROW e DASH ficam de fora). O programa **não** apaga o Hanzo do pack B: se
`assets/sprites/hanzo/` já tiver pranchas de outro pack, o gerado vai para
`hanzo_gerado/`.

## Como rodar

```sh
cd c_game
make sprites          # compila tools/personagens.c e gera tudo, com as folhas
```

Ou em partes: `make personagens` e depois `./personagens` (opções `--so enjin
yoru`, `--folhas`, `--lista`, `--entrada`, `--saida`). Não abre janela: usa só
as funções de imagem e de arquivo da raylib.

Na primeira vez, o programa copia `assets/sprites/musashi/` (o pack original)
para `assets/sprites/_original/` e passa a ler dali. Depois grava uma pasta por
personagem em `assets/sprites/<nome>/`, cada uma com as mesmas pranchas e um
`sprite.txt` próprio; o protagonista sai em `kojiro/`. Numa pasta gerada, as
tiras da rodada anterior são apagadas antes (uma animação que deixou de existir,
como os golpes do Hanzo, não fica para trás).

- Pastas com o arquivo `.gerado` são do programa e podem ser sobrescritas; as
  outras ele não toca.
- Daqui em diante, os números de `hold` e `contact` se editam em
  `_original/sprite.txt`, e o programa copia para todos.

### Folhas de conferência (`assets/sprites/_folhas/`)

| Arquivo | Para quê |
|---|---|
| `elenco.png`, `elenco_pb.png` | Os 15 lado a lado, no tamanho do jogo e ampliados; a versão em preto e branco confere se dá para distinguir pela forma |
| `golpes.png` | O quadro de contato de cada golpe, um personagem por linha |
| `<nome>.png` | Todas as pranchas do personagem, quadro a quadro, com o número embaixo |
| `alcance_<nome>.png` | Quadro de contato com a âncora dos pés (vermelho) e a ponta do golpe (ciano) |
| `deteccao.png` | O que o programa achou em cada quadro, em cor chapada: chapéu (azul), camisa (rosa), hakama (cinza), pele, bainha (roxo), cabo, lâmina (ciano), rastro (amarelo) |
| `deteccao_<nome>.png` | O mesmo, para quem tem corpo de outro pack (Shizuku, Arashi, Oboro); verde é cor própria do pack |

**Olhe a `deteccao.png` depois da primeira rodada.** O programa foi afinado nas
pranchas ATTACK_1, ATTACK_2, ATTACK_3, DASH_ATTACK e DASH. IDLE, DEFEND, HURT,
DEATH, STRONG_ATTACK, THROW, RUN e JUMP ele processa do mesmo jeito, mas ainda
não foram vistos. Se algum quadro sair errado, dá para corrigir sem mexer no
código, num `_original/ajustes.txt`:

```
DEATH 7 semchapeu              # a cabeça está sem chapéu neste quadro
DEATH 8 apaga 60 50 80 60      # apaga um retângulo (ex.: o chapéu caindo)
HURT 2 chapeu 44 41            # o chapéu está com o canto em (44, 41)
```

### A arma é do mesmo tamanho em todos os quadros

Na prancha, a katana aparece cortada em alguns quadros (atrás do corpo, borrada
no movimento). O programa mede o comprimento da katana do pack (a mediana das
vezes em que ela aparece inteira: 16,6 px do cabo à ponta) e desenha cada arma
com um comprimento fixo a partir dele: espadão do Raizo 1,45× (e duas fileiras a
mais de largura), wakizashi do Karasu 0,72×, lança 1,2× mais 14 px de haste atrás
da mão, florete 1,15× (reto, com copo),
adaga 8 px (as do Yoru empunhadas ao contrário: a lâmina sai do punho para trás
e, na mão da frente, fica deitada por cima do antebraço), espada curta 11 px, a
lâmina curta do Karasu 12 px, garras 10 px, foice do tamanho da espada, quase
toda cabo (0,8× a katana), com anel de metal e a lâmina curva de 6 px na ponta;
as garras saem de uma barra de metal sobre os nós dos dedos; a katana branca do
Jinshi tem um halo azulado de luar em volta da lâmina. Só a lâmina que sai da mão é
trocada ou alongada; um pedaço solto dela aparecendo no meio do rastro fica como
está. Adaga e espada curta são
desenhadas inteiras (cabo, guarda e lâmina). Cada pack tem a sua katana medida
(o Demon, por exemplo, tem uma katana bem mais comprida), então a mesma arma
fica proporcional ao corpo que a segura.

**Arma na outra mão.** Quem luta com duas armas (Yoru, Garfiel, Karasu, Hayate,
Arashi) usa o corpo do Samurai #5, que já segura uma espada em cada mão: cada
espada vira a arma do personagem na mão em que está, e os golpes de duas armas
do pack ficam como são. No Karasu, a lâmina da mão de trás (a esquerda) fica
mais curta. Sem esse pack, eles voltam para o corpo do Samurai #3, onde as duas
mãos ficam juntas no cabo e a segunda arma sai do mesmo punho, aberta em leque.

**Poder do vento (Hayate).** No contato e nos dois quadros seguintes, uma
meia-lua de vento sai da ponta da foice e voa para a frente, abrindo. É efeito,
não entra no alcance do parry (o parry continua sendo contra a foice).

## O golpe especial

Cada aprendiz ganha um `ESPECIAL.png`, montado com os quadros do pack. É **um
golpe só**: num jogo de parry, cada golpe que aparece tem que bater com um
contato do núcleo, senão o jogador não sabe o que aparar. O que muda é a
preparação (mais longa e mais legível) e o impacto.

| Coreografia | Quadros | Quem usa |
|---|---|---|
| Salto pesado | ATTACK_3: ergue a arma, segura no alto, desce com tudo | Raizo, Daichi, Enjin, Jinshi |
| Investida | DASH_ATTACK: agacha, risca a tela e corta; o contorno do corpo ficando para trás | Yoru, Karasu, Garfiel, Arashi, Oboro |
| Estocada longa | DASH_ATTACK agachado e a estocada do ATTACK_1 disparada de longe | Shizuku, Suiren |
| Golpe subindo | ATTACK_2: abaixa a guarda e corta para cima | Hayate, Genbu |

| Personagem | Efeito no impacto |
|---|---|
| Raizo | onda de choque de poeira com brilho dourado |
| Daichi | onda de choque, chão rachando e pedras voando |
| Jinshi | pedras caindo do alto |
| Enjin | pilar de fogo que sobe e apaga |
| Yoru | corte em X roxo |
| Karasu | corte em X vermelho (as duas lâminas) |
| Garfiel | três riscos de garra |
| Arashi | raio caindo do céu |
| Oboro | corte em X de sombra com fagulhas de ouro |
| Shizuku | coroa de água no ponto da estocada |
| Suiren | onda de água rolando para a frente |
| Hayate | redemoinho de vento |
| Genbu | escudo de casco que se abre e racha |

O quadro de 106 px não chega até o adversário, então o efeito fica no caminho
da lâmina (os de chão a 3/4 do alcance, os de ar um pouco antes da ponta).

No `sprite.txt` sai uma linha como as outras:

```
anim ESPECIAL       hold 3  contact 4  alcance 50 -18  ms 60
```

Para usar: quando o núcleo sortear o golpe especial (`specialChance`), tocar
`ESPECIAL` no lugar de `STRONG_ATTACK` se a pasta do personagem tiver o
arquivo. A sincronia é a mesma dos outros golpes: o `hold` estica até faltar o
tempo do golpe, e o `contact` cai no quadro do contato do núcleo.

## O `sprite.txt` de cada personagem

```
cell 106 84
ancora 54 74                   # pés do IDLE quadro 0 deste personagem
guarda dx dy                   # ponta da lâmina do DEFEND no contato (aparece quando houver DEFEND.png)
anim ATTACK_1  hold 1  contact 2  alcance 38 -21  ms 100
```

- **`alcance dx dy`**: ponta da arma ou do rastro no quadro de contato, a partir
  da âncora (dy negativo = acima dos pés).
- **`ms`**: duração de cada quadro do golpe. Sem `ms`, 80. Adaga, garra e
  florete usam 60 (golpe seco e rápido); odachi, espada pesada e cajado usam
  100 (peso). O `hold` continua esticando até o contato do núcleo; o `ms` muda
  a preparação antes do hold, o golpe entre o hold e o contato e a volta.
- A **âncora** é de cada personagem, porque o corpo muda de largura.
- Quem tem corpo de outro pack sai com o `cell` daquele pack (Shizuku 96 × 96,
  Arashi 96 × 64, Oboro 128 × 108) e com as animações que o pack tem.

### Como o corpo se move no golpe

- **Lança e florete** estocam no ATTACK_1 (o golpe reto, `LOOK_THRUST`) e no
  DASH_ATTACK: a arma fica na horizontal na altura das mãos, recua no `hold` e
  dispara no contato com um rastro reto. Nos golpes de cima e de baixo eles
  giram a arma como os outros.
- **Passo à frente** no quadro de contato, já desenhado: lança 3 px, florete,
  adaga e garra 2 px, odachi e espada pesada 1 px. No quadro seguinte ainda
  fica 1 px à frente (menos nas pesadas), e depois volta ao lugar.
- **Armas pesadas** levantam poeira no chão no contato.

## A espada que não encontra a outra no parry

A causa está nas pranchas. Cada golpe chega a uma distância diferente dos pés.
Com a katana:

| Golpe | Contato | Alcance (px à frente da âncora) | Altura (px acima dos pés) |
|---|---:|---:|---:|
| ATTACK_2 (sobe) | quadro 2 | 30 | 20 |
| ATTACK_1 (horizontal) | quadro 2 | 35 | 21 |
| ATTACK_3 (desce) | quadro 2 | 40 | 20 |
| DASH_ATTACK | quadro 4 | 44 | 18 |

Além disso, no ATTACK_3 o pé da frente **avança uns 20 px dentro do quadro**,
e o DASH_ATTACK **começa 18 px atrás** da âncora (agachado) e dispara para a
frente. Com os dois lutadores a uma distância fixa, o ATTACK_2 não alcança e o
ATTACK_3 passa do ponto. É o "às vezes está atacando longe demais". Com as
armas novas os números mudam por personagem (a estocada da Suiren chega a 52),
e cada `sprite.txt` traz os seus.

Regra para o duelo: **no quadro de contato, a distância entre as âncoras dos
dois é `alcance do golpe + guarda do Kojiro`.** Como o alcance muda por golpe
e por personagem, o mestre se posiciona a cada golpe:

```c
/* Mestre olha para a esquerda; Kojiro para a direita. */
int alvo = kojiro_x + kojiro_guarda_dx + anim_do_golpe->alcance_dx;

/* Durante a antecipação: anda até o alvo em passos inteiros (ex.: 2 px por
   quadro) ou encaixa direto no hold se a preparação for curta. Durante o
   golpe, não mexe: o passo do ATTACK_3, o avanço do DASH_ATTACK e o passo à
   frente das armas já estão desenhados e contados no alcance. */

/* A faísca do parry vai no ponto de encontro: */
Vector2 faisca = {kojiro_x + kojiro_guarda_dx, chao_y + kojiro_guarda_dy};
```

Ao espelhar quem olha para a esquerda, a âncora dentro do quadro também espelha:
`x_desenho = x_mundo - (virado ? cell_w - 1 - ancora_x : ancora_x)`.

## O som do parry

O som já toca no instante do contato do núcleo (`EV_IMPACT`, em `on_impact`
no `main.c`). Para ele cair **exatamente quando as lâminas se chocam**, o que
falta é o quadro `contact` da prancha aparecer no mesmo quadro de jogo desse
evento (regra do `hold` do brief visual). Com isso, som, faísca e desenho saem
juntos.

O choque foi refeito em `src/audio.c`, em camadas que começam todas no primeiro
milissegundo:

- **estalo**: ruído agudo de poucos milissegundos, o "tchk" do contato;
- **metal**: os modos de vibração de uma lâmina (1 : 2,76 : 5,40 : 8,93) em duas
  lâminas um pouco desafinadas entre si, o que faz o brilho pulsar;
- **baque**: um grave que cai de tom e dá peso;
- **faíscas**: estalinhos soltos que vão rareando;
- **nota longa**, só no perfeito: um agudo com vibrato leve e um pouco de sala.

O perfeito dura 1,6 s e o bom 0,5 s, mais seco. Perfeito, bom, erro e o
"whoosh" do golpe têm quatro vozes cada: num combo, um parry não corta a cauda
do anterior. O bom varia um pouco de tom a cada vez, para não soar repetido.
Sem placa de som o jogo segue mudo, como antes.

## Corpos de outros packs: Raizo, Shizuku, Arashi e Oboro

Quatro personagens usam o corpo de outro pack da Mattz Art, que já vem com os
próprios golpes animados. O programa lê esse pack, separa as partes do mesmo
jeito e aplica o personagem por cima: troca exata de cores, a arma dele, o
rastro e a aura do elemento, o golpe especial.

| Personagem | Pack | Pasta | Quadro | O que muda |
|---|---|---|---|---|
| **Raizo** | samurai do espadão (chapéu de palha) | `_packs/espadao/` | 98 × 64 | roupa preta → marrom; o espadão e o chapéu ficam como vêm, com rastro dourado e fagulhas de ouro |
| **Shizuku** | Samurai #4 (moça de rabo de cavalo) | `_packs/samurai4/` | 96 × 96 | cabelo roxo → azul petróleo, camisa azul clara, hakama azul, faixa ciano, olhos ciano; a katana vira **florete** e o ATTACK_1 vira estocada |
| **Yoru, Garfiel, Karasu, Hayate** | Samurai #5 (o mesmo corpo) | `_packs/samurai5/` | 96 × 64 | cada um com as suas cores; cada espada do pack vira a arma dele, **na mão em que está**: adagas, garras, espada e lâmina curta (a da mão de trás encurta), foices |
| **Arashi** | Samurai #5 (mascarado, duas espadas) | `_packs/samurai5/` | 96 × 64 | roupa verde → preta, cinto e botas azul elétrico, cabelo prateado, olhos de raio; as **duas espadas** do pack ficam, com raios nas lâminas e rastro azul |
| **Oboro** | Demon (máscara oni) | `_packs/demon/` | 128 × 108 | cores do pack; ganha os ecos das posturas dos outros e a cena do grito |

Como montar a pasta (os PNGs são do pack pago e ficam fora do git; os
`sprite.txt` já estão no repositório):

```
c_game/assets/sprites/_packs/
  espadao/   sprite.txt  IDLE.png ATTACK_1.png ATTACK_2.png ATTACK_3.png DEFEND.png
             HURT.png DEATH.png RUN.png JUMP.png
  samurai4/  sprite.txt  IDLE.png ATTACK_1.png ATTACK_2.png ATTACK_3.png DEFEND.png
             HURT.png DEATH.png RUN.png JUMP.png THROW.png
  samurai5/  sprite.txt  IDLE.png ATTACK_1.png ATTACK_2.png ATTACK_3.png DEFEND.png
             HURT.png DEATH.png RUN.png JUMP.png
  demon/     sprite.txt  IDLE.png IDLE_FURIA.png ATTACK_1.png ATTACK_1_FURIA.png
             ATTACK_2.png ATTACK_2_FURIA.png ATTACK_3.png ATTACK_3_FURIA.png
             STRONG_ATTACK.png STRONG_ATTACK_FURIA.png DEFEND.png HURT.png
             HURT_FURIA.png RUN.png RUN_FURIA.png
```

Cada PNG é uma tira de quadros na horizontal, na altura do quadro do pack (o
`cell` do `sprite.txt`). O nome do arquivo é o nome da animação. Se a pasta do
pack não existir, o personagem sai do corpo do Samurai #3, como antes.

No `CHARS` do programa, os campos do pack são:

- **`pack`**: a pasta em `_packs/`;
- **`troca`**: cor original → cor nova, exata (a lâmina e o rastro não entram:
  eles ganham as cores da arma e do elemento);
- **`leitura`**: como o separador lê as cores que o Samurai #3 não tem (a pele
  mais clara desses packs é pele; o azul escuro da roupa do #5 e do Demon é o
  mesmo do cabo do #3, então ali ele não é cabo);
- **`pack_par`**: o pack já tem uma arma em cada mão (não desenha a segunda);
- **`pack_sem_camisa`**: o pack não tem roupa branca, então todo branco grosso
  é rastro;
- **`pack_arma`**: a arma do pack fica exatamente como vem (o espadão do Raizo);
  só ganha o brilho do elemento. Na `leitura`, o azul-acinzentado do corpo largo
  do espadão fica marcado como "nem camisa nem rastro".

Nesses packs a lâmina tem 2 ou 3 px de largura (no #3, 1 px). O separador trata
como camisa só o branco que tem miolo de 3 × 3 e fica dentro do corpo; o que é
comprido e fino, ou encosta numa lâmina, é lâmina; o que é grosso e fica fora
do corpo é rastro. A `deteccao_<nome>.png` das folhas mostra o resultado.

Nesses corpos o florete e a lança não viram estocada: o golpe é o corte do
próprio pack, com a arma e o rastro novos (a estocada em cima do corte deles
ficava estranha). O golpe especial desses quatro sai de um golpe do próprio pack (ATTACK_1 para
investida e estocada, ATTACK_3 para o salto, ATTACK_2 para o ascendente): a
preparação fica segurada um quadro a mais, um passo para trás antes do bote, e
o avanço no contato, com o efeito do elemento.

### Oboro: as posturas dos outros e o grito

O Oboro passou anos estudando as posturas dos outros. Além dos golpes do Demon,
o programa gera **cada ataque dele na postura de cada aprendiz**:
`ATTACK_1_ECO_RAIZO`, `ATTACK_2_ECO_SHIZUKU`, ... até `ATTACK_3_ECO_GARFIEL`
(36 tiras). A **espada continua a dele** (a katana dourada do Demon, do mesmo
tamanho e no mesmo tempo); o que muda é a aura: o rastro, o brilho na lâmina e
as partículas do elemento de cada aprendiz (fogo do Enjin, raios do Arashi,
água da Shizuku, os cortes de vento do Hayate...). Cada eco tem o próprio
`alcance` no `sprite.txt`, para o parry.

As versões de **fúria** do pack (`*_FURIA`: lâmina vermelha, rastro de sangue)
são a fase 2. O rastro vermelho tem as mesmas cores da máscara, então o alcance
delas é copiado do golpe normal, que tem o mesmo desenho.

**`GRITO.png`** (14 quadros, 90 ms cada) é a cena final: parado, a fúria sobe
(passa do IDLE para o IDLE_FURIA), ele treme cada vez mais, as linhas do grito
saem da cabeça e anéis vermelhos se abrem em volta do corpo. Foi montada com as
tiras que o pack tem; se chegar a tira própria de raiva do Demon (18 quadros),
é só pôr como `RAIVA.png` na pasta do pack (com o mesmo quadro de 128 × 108),
que ela sai com as cores e a aura do Oboro como as outras.

## Limites conhecidos

- Cinco personagens dividem o corpo do Samurai #5: mudam as cores, as armas, o
  rastro e a aura, mas a silhueta e os golpes são os mesmos. As cabeças próprias
  (capuz, cabelo de tigre, penas) só aparecem no corpo do Samurai #3.
- O Hanzo sai das pranchas do Samurai #3 sem a espada: nas poses em que o
  corpo segurava a katana, as mãos ficam na mesma posição, vazias.
- Os packs novos não têm todas as animações do #3 (não há DASH nem
  DASH_ATTACK), e o Demon não tem DEATH. Do pack do espadão chegaram só
  ATTACK_1, ATTACK_2, ATTACK_3, DEFEND e DEATH; IDLE, RUN, JUMP e HURT entram
  quando forem para a pasta `_packs/espadao/`. O que o jogo pedir e o pack não tiver
  precisa de um substituto no carregador (por exemplo, HURT segurado).
- A estocada usa os quadros do corte horizontal: o braço é o mesmo, só a arma e
  o rastro mudam. Uma estocada com o braço esticando de verdade pediria
  desenho novo.
- No DASH, a bainha que o Kojiro segura na mão de trás continua aparecendo
  para quem não usa bainha (é desenhada com as cores da hakama).
- As pranchas têm que estar viradas para a direita, como vêm no pack.

## Outros samurais da Mattz Art

As capas com a marca "PREVIEW" não servem de sprite, mas os packs comprados
viram corpo de verdade. Já estão encaixados o do espadão (Raizo), o Samurai #4
(Shizuku), o #5 (Arashi) e o Demon (Oboro). Os que ainda podem entrar:

| Pack | Visual | Personagem |
|---|---|---|
| Samurai #2 | armadura vermelha, kabuto com chifres dourados | Daichi ou Jinshi |
| Samurai #6 | chapéu de palha, corte largo | Daichi |

Para encaixar mais um: pôr as tiras em `_packs/<nome>/` com um `sprite.txt`
(`cell` e os `hold`/`contact` de cada golpe), apontar `pack` na entrada do
personagem em `CHARS` e conferir a `deteccao_<nome>.png`.

**Hanzo:** o pack B (a versão grátis, com o Hanzo de barba branca) já é seu. Com
as pranchas dele (`assets/sprites/hanzo/*.png`), a barba sai do sprite original
em vez de o Hanzo vir do corpo do Samurai #3.

## Arte e repositório

O repositório é público e as pranchas são do pack pago da Mattz Art, então os
PNGs de `c_game/assets/sprites/` (os originais e os gerados, que saem deles)
estão no `.gitignore`. O programa e os `sprite.txt` vão para o git normalmente.
