# Personagens: o elenco saído do Samurai #3

> Substitui a seção 4 (troca de paleta) do brief visual v2. Pode colar este
> arquivo inteiro no Claude Code.

Os 14 personagens (Musashi, os 11 aprendizes, Oboro e Hanzo) saem do mesmo
corpo do pack A, mas não são só troca de cor. O programa
`c_game/tools/personagens.c` (C com raylib, igual ao jogo) pega cada quadro das
pranchas do Musashi e:

- **tira o chapéu** e desenha a cabeça no lugar (coque, rabo de cavalo, capuz,
  careca, cabelo em chamas...). Só **dois** ficam de chapéu: Daichi (palha) e
  Arashi (o chapéu largo do Raiden). O Musashi fica **sem chapéu, de coque**;
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
| 1 | **Raijin** (touro) | odachi, 1,5× a katana | dois tufos de chifre, faixa amarela; 2 px mais largo | marrom, amarelo | ouro, fagulhas subindo |
| 2 | **Shizuku** (água) | florete com copo ciano, **estoca** | franja e rabo de cavalo; 1 px mais estreita | azul claro, ciano | água, bolhas |
| 3 | **Kage** (noite) | duas adagas que brilham roxo | capuz ninja, fitas roxas; 1 px mais estreito | preto azulado e roxo | corte duplo, fumaça roxa |
| 4 | **Daichi** (terra) | espada pesada, lâmina larga | **chapéu de palha**, barba; 1 px mais largo | verde oliva, ocre | rastro grosso, poeira no chão |
| 5 | **Hayate** (vento) | katana leve | cabelo espetado, cachecol limão | verde claro, limão | vento, rajadas passando |
| 6 | **Genbu** (tartaruga) | espada curta + casco nas costas | careca, barbicha; atarracado | verde musgo | verde, esporos |
| 7 | **Enjin** (chama) | espada de fogo | cabelo em chamas; 1 px mais largo | vermelho e amarelo | fogo, labaredas e brasas |
| 8 | **Suiren** (mar) | lança de água, **estoca** | cabelo curto, faixa turquesa | azul mar, turquesa | água, bolhas |
| 9 | **Karasu** (corvo) | garras (três lâminas na mão) | cabelo em penas, olho vermelho, trapo; magro e alto | preto e vermelho | três riscos, penas caindo |
| 10 | **Arashi** (tempestade) | duas espadas com raios | **chapéu do Raiden**, olhos brilhando | preto, azul elétrico | eco da segunda espada, raios |
| 11 | **Jinshi** (montanha) | cajado de ferro, ponteiras de osso | cabelo grisalho comprido, barba; mais alto | cinza pedra, branco osso | rastro grosso, pedrisco caindo |
| 12 | **Oboro** | katana de Hanzo, dourada | rabo de cavalo longo, anel de ouro; mais alto | roxo escuro, dourado | sombra e fagulhas de ouro |
| — | **Hanzo** | katana, bainha vermelha | coque branco, **sem barba**; mais baixo | azul escuro | branco, sem aura |

As armas do pedido caíram assim: garras → Karasu, espada maior → Raijin
(odachi), florete → Shizuku, adagas roxas → Kage, espadas com raios → Arashi,
espada de fogo → Enjin, lança de água → Suiren.

**Nomes:** Raijin quer dizer "deus do trovão", mas na lore quem tem duas espadas
e azul elétrico é o Arashi, então o raio e o chapéu do Raiden ficaram com ele.
Se preferir o trovão no Raijin, é só trocar os nomes das duas entradas em
`CHARS` no programa.

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
com um comprimento fixo a partir dele: odachi 1,5×, lança 1,2× mais 14 px de
haste atrás da mão, florete 1,25×, adaga 8 px, espada curta 11 px, garras 10 px.
Adaga e espada curta são desenhadas inteiras (cabo, guarda e lâmina), e a
segunda adaga do Kage fica 4 px atrás da primeira.

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

## Limites conhecidos

- A segunda arma (Kage e Arashi) é uma cópia paralela da primeira, desenhada
  atrás do corpo. O sprite não mostra a outra mão segurando nada.
- A estocada usa os quadros do corte horizontal: o braço é o mesmo, só a arma e
  o rastro mudam. Uma estocada com o braço esticando de verdade pediria
  desenho novo.
- No DASH, a bainha que o Musashi segura na mão de trás continua aparecendo
  para quem não usa bainha (é desenhada com as cores da hakama).
- As pranchas têm que estar viradas para a direita, como vêm no pack.

## Outros samurais da Mattz Art

As referências mandadas (Samurai #2, #4, #5, #6, o de máscara oni) são capas e
vitrines de packs pagos, algumas com a marca "PREVIEW" por cima: não dá para
recortar e usar. Mas são do **mesmo artista e da mesma escala** do Samurai #3,
então, comprados, cada um vira um corpo de verdade, com animações e armas
próprias, e combina com o resto. Sugestão de quem fica com qual:

| Pack | Visual | Personagem |
|---|---|---|
| Samurai #2 | armadura vermelha, kabuto com chifres dourados | Raijin (touro) |
| máscara oni | armadura azul e vermelha, máscara de demônio, espada curva | Oboro |
| Samurai #4 | moça de rabo de cavalo | Shizuku |
| Samurai #5 | mascarado, duas espadas | Arashi ou Kage |
| Samurai #6 | chapéu de palha, corte largo | Daichi |

Para encaixar um pack novo, o programa precisa das pranchas em PNG (tiras de
quadros, como as do #3) para aprender a paleta e as partes daquele corpo.

**Hanzo:** o pack B (a versão grátis, com o Hanzo de barba branca) já é seu. Com
as pranchas dele (`assets/sprites/hanzo/*.png`), a barba sai do sprite original
em vez de o Hanzo vir do corpo do Musashi.

## Arte e repositório

O repositório é público e as pranchas são do pack pago da Mattz Art, então os
PNGs de `c_game/assets/sprites/` (os originais e os gerados, que saem deles)
estão no `.gitignore`. O programa e os `sprite.txt` vão para o git normalmente.
