# Personagens: o elenco saído do Samurai #3

> Substitui a seção 4 (troca de paleta) do brief visual v2. Pode colar este
> arquivo inteiro no Claude Code.

Os 14 personagens (Musashi, os 11 aprendizes, Oboro e Hanzo) saem de quatro
corpos da Mattz Art, e não são só troca de cor. Onze usam o corpo do pack A (o
Samurai #3 do Musashi). Três têm corpo próprio, de outro pack: **Shizuku**
(Samurai #4), **Arashi** (Samurai #5, o de duas espadas) e **Oboro** (o Demon,
de máscara oni). Ver [Corpos de outros packs](#corpos-de-outros-packs-shizuku-arashi-e-oboro).
O programa `c_game/tools/personagens.c` (C com raylib, igual ao jogo) pega cada
quadro das pranchas e:

- **tira o chapéu** e desenha a cabeça no lugar (coque, rabo de cavalo, capuz,
  careca, cabelo em chamas...). Só o Daichi fica de chapéu (palha). O Musashi
  fica **sem chapéu, de coque**;
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
| — | **Musashi** | katana | sem chapéu, coque com fita vermelha | original (branco e preto) | branco, sem aura |
| 1 | **Raijin** (touro) | **espadão**: odachi 1,85× a katana, lâmina larga | dois tufos de chifre, faixa amarela; 2 px mais largo | marrom, amarelo | ouro, fagulhas subindo |
| 2 | **Shizuku** (água) | florete com copo ciano, **estoca** | **corpo do Samurai #4**, cabelo roxo virou azul petróleo | azul claro, ciano, hakama azul | água, bolhas |
| 3 | **Kage** (noite) | **uma adaga em cada mão**, brilho roxo | capuz ninja, fitas roxas; 1 px mais estreito | preto azulado e roxo | corte duplo, fumaça roxa |
| 4 | **Daichi** (terra) | espada pesada, lâmina larga | **chapéu de palha**, barba; 1 px mais largo | verde oliva, ocre | rastro grosso, poeira no chão |
| 5 | **Hayate** (vento) | katana leve | cabelo espetado, cachecol limão | verde claro, limão | vento, rajadas passando |
| 6 | **Genbu** (tartaruga) | espada curta + casco nas costas | careca, barbicha; atarracado | verde musgo | verde, esporos |
| 7 | **Enjin** (chama) | espada de fogo | cabelo em chamas; 1 px mais largo | vermelho e amarelo | fogo, labaredas e brasas |
| 8 | **Suiren** (mar) | lança de água, **estoca** | cabelo curto, faixa turquesa | azul mar, turquesa | água, bolhas |
| 9 | **Karasu** (corvo) | **garras nas duas mãos** (três lâminas em cada) | cabelo em penas, olho vermelho, trapo; magro e alto | preto e vermelho | três riscos, penas caindo |
| 10 | **Arashi** (tempestade) | duas espadas com raios | **corpo do Samurai #5**, cabelo prateado, olhos de raio | preto, azul elétrico | raios nas lâminas e no corpo |
| 11 | **Jinshi** (montanha) | cajado de ferro, ponteiras de osso | cabelo grisalho comprido, barba; mais alto | cinza pedra, branco osso | rastro grosso, pedrisco caindo |
| 12 | **Oboro** | katana de Hanzo, dourada | **corpo do Demon** (máscara oni), com as versões de fúria | azul e vermelho do pack | sombra; nos ecos, o de cada aprendiz |
| — | **Hanzo** | katana, bainha vermelha | coque branco, **sem barba**; mais baixo | azul escuro | branco, sem aura |

As armas do pedido caíram assim: garras → Karasu, espada maior → Raijin
(odachi), florete → Shizuku, adagas roxas → Kage, espadas com raios → Arashi,
espada de fogo → Enjin, lança de água → Suiren.

**Nomes:** Raijin quer dizer "deus do trovão", mas na lore quem tem duas espadas
e azul elétrico é o Arashi, então o raio ficou com ele. Se preferir o trovão no
Raijin, é só trocar os nomes das duas entradas em `CHARS` no programa.

Nos pedidos mais novos apareceram outros nomes: **Kojiro** (o protagonista, aqui
`musashi`), **Garfiel** (garras, aqui `karasu`), **Yoru** (adagas, aqui `kage`)
e **Raizo** (o do espadão, aqui `raijin`). O `id` é o nome da pasta e das tiras
(`ATTACK_1_ECO_KARASU`); o `titulo` é o que aparece nas folhas. Para renomear,
troque os dois em `CHARS`.

**Hanzo:** o pedido era o Hanzo do pack B sem a barba. Esse sprite não estava
aqui, então o Hanzo saiu do corpo do pack A, de coque branco e sem barba. Assim
ele também combina com o resto. O programa **não** apaga o Hanzo do pack B: se
`assets/sprites/hanzo/` já tiver pranchas de outro pack, o gerado vai para
`hanzo_gerado/`.

## Como rodar

```sh
cd c_game
make sprites          # compila tools/personagens.c e gera tudo, com as folhas
```

Ou em partes: `make personagens` e depois `./personagens` (opções `--so enjin
kage`, `--folhas`, `--lista`, `--entrada`, `--saida`). Não abre janela: usa só
as funções de imagem e de arquivo da raylib.

Na primeira vez, o programa copia `assets/sprites/musashi/` (o pack original)
para `assets/sprites/_original/` e passa a ler dali. Depois grava uma pasta por
personagem em `assets/sprites/<nome>/`, cada uma com as mesmas pranchas e um
`sprite.txt` próprio. O `musashi/` passa a ter o Musashi sem chapéu, então o
jogo pega a versão nova sem mudar o carregador.

- Pastas com o arquivo `.gerado` são do programa e podem ser sobrescritas; as
  outras ele não toca.
- Daqui em diante, os números de `hold` e `contact` se editam em
  `_original/sprite.txt`, e o programa copia para todos.

### Folhas de conferência (`assets/sprites/_folhas/`)

| Arquivo | Para quê |
|---|---|
| `elenco.png`, `elenco_pb.png` | Os 14 lado a lado, no tamanho do jogo e ampliados; a versão em preto e branco confere se dá para distinguir pela forma |
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
com um comprimento fixo a partir dele: espadão do Raijin 1,85× (e uma fileira a
mais de largura), lança 1,2× mais 14 px de haste atrás da mão, florete 1,25×,
adaga 8 px, espada curta 11 px, garras 10 px. Adaga e espada curta são
desenhadas inteiras (cabo, guarda e lâmina). Cada pack tem a sua katana medida
(o Demon, por exemplo, tem uma katana bem mais comprida), então a mesma arma
fica proporcional ao corpo que a segura.

**Arma na outra mão.** No corpo do Musashi as duas mãos ficam juntas no cabo.
A adaga do Kage e as garras do Karasu da mão esquerda saem do mesmo punho, um
pouco mais para dentro e mais baixo, abertas em leque (empunhadas ao contrário)
e atrás do corpo: dá para ver uma arma em cada mão em todos os golpes, inclusive
quando o Oboro usa a postura deles.

## O golpe especial

Cada aprendiz ganha um `ESPECIAL.png`, montado com os quadros do pack. É **um
golpe só**: num jogo de parry, cada golpe que aparece tem que bater com um
contato do núcleo, senão o jogador não sabe o que aparar. O que muda é a
preparação (mais longa e mais legível) e o impacto.

| Coreografia | Quadros | Quem usa |
|---|---|---|
| Salto pesado | ATTACK_3: ergue a arma, segura no alto, desce com tudo | Raijin, Daichi, Enjin, Jinshi |
| Investida | DASH_ATTACK: agacha, risca a tela e corta; imagens do corpo ficando para trás | Kage, Karasu, Arashi, Oboro |
| Estocada longa | DASH_ATTACK agachado e a estocada do ATTACK_1 disparada de longe | Shizuku, Suiren |
| Golpe subindo | ATTACK_2: abaixa a guarda e corta para cima | Hayate, Genbu |

| Personagem | Efeito no impacto |
|---|---|
| Raijin | onda de choque de poeira com brilho dourado |
| Daichi | onda de choque, chão rachando e pedras voando |
| Jinshi | pedras caindo do alto |
| Enjin | pilar de fogo que sobe e apaga |
| Kage | corte em X roxo |
| Karasu | três riscos de garra e penas |
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
dois é `alcance do golpe + guarda do Musashi`.** Como o alcance muda por golpe
e por personagem, o mestre se posiciona a cada golpe:

```c
/* Mestre olha para a esquerda; Musashi para a direita. */
int alvo = musashi_x + musashi_guarda_dx + anim_do_golpe->alcance_dx;

/* Durante a antecipação: anda até o alvo em passos inteiros (ex.: 2 px por
   quadro) ou encaixa direto no hold se a preparação for curta. Durante o
   golpe, não mexe: o passo do ATTACK_3, o avanço do DASH_ATTACK e o passo à
   frente das armas já estão desenhados e contados no alcance. */

/* A faísca do parry vai no ponto de encontro: */
Vector2 faisca = {musashi_x + musashi_guarda_dx, chao_y + musashi_guarda_dy};
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

## Corpos de outros packs: Shizuku, Arashi e Oboro

Três personagens usam o corpo de outro pack da Mattz Art, que já vem com os
próprios golpes animados. O programa lê esse pack, separa as partes do mesmo
jeito e aplica o personagem por cima: troca exata de cores, a arma dele, o
rastro e a aura do elemento, o golpe especial.

| Personagem | Pack | Pasta | Quadro | O que muda |
|---|---|---|---|---|
| **Shizuku** | Samurai #4 (moça de rabo de cavalo) | `_packs/samurai4/` | 96 × 96 | cabelo roxo → azul petróleo, camisa azul clara, hakama azul, faixa ciano, olhos ciano; a katana vira **florete** e o ATTACK_1 vira estocada |
| **Arashi** | Samurai #5 (mascarado, duas espadas) | `_packs/samurai5/` | 96 × 64 | roupa verde → preta, cinto e botas azul elétrico, cabelo prateado, olhos de raio; as **duas espadas** do pack ficam, com raios nas lâminas e rastro azul |
| **Oboro** | Demon (máscara oni) | `_packs/demon/` | 128 × 108 | cores do pack; ganha os ecos das onze posturas e a cena do grito |

Como montar a pasta (os PNGs são do pack pago e ficam fora do git; os
`sprite.txt` já estão no repositório):

```
c_game/assets/sprites/_packs/
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
pack não existir, o personagem sai do corpo do Musashi, como antes.

No `CHARS` do programa, os campos do pack são:

- **`pack`**: a pasta em `_packs/`;
- **`troca`**: cor original → cor nova, exata (a lâmina e o rastro não entram:
  eles ganham as cores da arma e do elemento);
- **`leitura`**: como o separador lê as cores que o Samurai #3 não tem (a pele
  mais clara desses packs é pele; o azul escuro da roupa do #5 e do Demon é o
  mesmo do cabo do #3, então ali ele não é cabo);
- **`pack_par`**: o pack já tem uma arma em cada mão (não desenha a segunda);
- **`pack_sem_camisa`**: o pack não tem roupa branca, então todo branco grosso
  é rastro.

Nesses packs a lâmina tem 2 ou 3 px de largura (no #3, 1 px). O separador trata
como camisa só o branco que tem miolo de 3 × 3 e fica dentro do corpo; o que é
comprido e fino, ou encosta numa lâmina, é lâmina; o que é grosso e fica fora
do corpo é rastro. A `deteccao_<nome>.png` das folhas mostra o resultado.

O golpe especial desses três sai de um golpe do próprio pack (ATTACK_1 para
investida e estocada, ATTACK_3 para o salto, ATTACK_2 para o ascendente): a
preparação fica segurada um quadro a mais, um passo para trás antes do bote, e
o avanço no contato, com o efeito do elemento.

### Oboro: as onze posturas e o grito

O Oboro passou anos estudando as posturas dos outros. Além dos golpes do Demon,
o programa gera **cada ataque dele na postura de cada aprendiz**:
`ATTACK_1_ECO_RAIJIN`, `ATTACK_2_ECO_SHIZUKU`, ... até `ATTACK_3_ECO_JINSHI`
(33 tiras). Em cada uma, o corpo e o movimento são do Oboro, mas a arma, o
rastro, a cor da lâmina, a aura e o tempo (`ms`) são do aprendiz: o espadão do
Raijin, o florete da Shizuku estocando, uma adaga em cada mão como o Kage,
garras nas duas mãos como o Karasu, a lança do Suiren, as espadas com raios do
Arashi... Cada eco tem o próprio `alcance` no `sprite.txt`, para o parry.

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

- No corpo do Musashi, a arma da mão esquerda (Kage, Karasu) sai do mesmo
  punho da primeira, porque as duas mãos ficam juntas no cabo; o braço
  esquerdo não se estica sozinho.
- Os packs novos não têm todas as animações do #3 (não há DASH nem
  DASH_ATTACK), e o Demon não tem DEATH. O que o jogo pedir e o pack não tiver
  precisa de um substituto no carregador (por exemplo, HURT segurado).
- A estocada usa os quadros do corte horizontal: o braço é o mesmo, só a arma e
  o rastro mudam. Uma estocada com o braço esticando de verdade pediria
  desenho novo.
- No DASH, a bainha que o Musashi segura na mão de trás continua aparecendo
  para quem não usa bainha (é desenhada com as cores da hakama).
- As pranchas têm que estar viradas para a direita, como vêm no pack.

## Outros samurais da Mattz Art

As capas com a marca "PREVIEW" não servem de sprite, mas os packs comprados
viram corpo de verdade. Já estão encaixados o Samurai #4 (Shizuku), o #5
(Arashi) e o Demon (Oboro). Os que ainda podem entrar:

| Pack | Visual | Personagem |
|---|---|---|
| Samurai #2 | armadura vermelha, kabuto com chifres dourados | Raijin (touro) |
| Samurai #6 | chapéu de palha, corte largo | Daichi |

Para encaixar mais um: pôr as tiras em `_packs/<nome>/` com um `sprite.txt`
(`cell` e os `hold`/`contact` de cada golpe), apontar `pack` na entrada do
personagem em `CHARS` e conferir a `deteccao_<nome>.png`.

**Hanzo:** o pack B (a versão grátis, com o Hanzo de barba branca) já é seu. Com
as pranchas dele (`assets/sprites/hanzo/*.png`), a barba sai do sprite original
em vez de o Hanzo vir do corpo do Musashi.

## Arte e repositório

O repositório é público e as pranchas são do pack pago da Mattz Art, então os
PNGs de `c_game/assets/sprites/` (os originais e os gerados, que saem deles)
estão no `.gitignore`. O programa e os `sprite.txt` vão para o git normalmente.
