# Personagens: o elenco saído do Samurai #3

> Substitui a seção 4 (troca de paleta) do brief visual v2. Pode colar este
> arquivo inteiro no Claude Code.

Os 14 personagens (Musashi, os 11 aprendizes, Oboro e Hanzo) saem do mesmo
corpo do pack A, mas não são mais só troca de cor. O script
`c_game/tools/personagens.py` pega cada quadro das pranchas do Musashi e:

- **tira o chapéu** e desenha a cabeça no lugar (coque, rabo de cavalo, capuz,
  careca, cabelo em chamas...). Só **dois** ficam de chapéu: Daichi (palha) e
  Arashi (o chapéu largo do Raiden). O Musashi fica **sem chapéu, de coque**;
- **troca a arma** seguindo a reta da katana em cada quadro;
- **pinta o rastro do golpe** com o elemento de cada um e solta partículas;
- **acrescenta acessórios atrás do corpo**: cachecol, casco, fitas, rabo de cavalo.

Tudo é pixel inteiro, sem escala nem rotação, e o mesmo comando sempre gera os
mesmos pixels.

## O elenco

| # | Personagem | Arma | Cabeça | Cores | Rastro |
|---:|---|---|---|---|---|
| — | **Musashi** | katana | sem chapéu, coque com fita vermelha | original (branco e preto) | branco |
| 1 | **Raijin** (touro) | odachi, 1,5× a katana | dois tufos de chifre, faixa amarela | marrom, destaque amarelo | ouro |
| 2 | **Shizuku** (água) | florete fino com copo ciano | franja e rabo de cavalo | azul claro, destaque ciano | água |
| 3 | **Kage** (noite) | duas adagas que brilham roxo | capuz ninja, fresta dos olhos, fitas roxas | preto azulado e roxo | roxo |
| 4 | **Daichi** (terra) | espada pesada, lâmina larga | **chapéu de palha**, barba | verde oliva, ocre | terra |
| 5 | **Hayate** (vento) | katana leve | cabelo espetado pelo vento, cachecol limão | verde claro, limão | vento |
| 6 | **Genbu** (tartaruga) | espada curta + casco nas costas (escudo) | careca, barbicha grisalha | verde musgo | verde |
| 7 | **Enjin** (chama) | espada de fogo | cabelo em chamas | vermelho e amarelo | fogo com brasas |
| 8 | **Suiren** (mar) | lança de água | cabelo curto, faixa turquesa | azul mar, turquesa | água |
| 9 | **Karasu** (corvo) | garras (três lâminas na mão) | cabelo em penas, olho vermelho, trapo vermelho | preto e vermelho | penas |
| 10 | **Arashi** (tempestade) | duas espadas com raios | **chapéu do Raiden**, olhos brilhando | preto, azul elétrico | raio |
| 11 | **Jinshi** (montanha) | cajado de ferro, ponteiras de osso | cabelo grisalho comprido, barba | cinza pedra, branco osso | osso |
| 12 | **Oboro** | katana de Hanzo, dourada | rabo de cavalo longo com anel de ouro | roxo escuro, dourado | roxo e ouro |
| — | **Hanzo** | katana, bainha vermelha | coque branco, **sem barba** | azul escuro | branco |

As armas do pedido caíram assim: garras → Karasu, espada maior → Raijin
(odachi), florete → Shizuku, adagas roxas → Kage, espadas com raios → Arashi,
espada de fogo → Enjin, lança de água → Suiren.

**Nomes:** Raijin quer dizer "deus do trovão", mas na lore quem tem duas espadas
e azul elétrico é o Arashi, então o raio e o chapéu do Raiden ficaram com ele.
Se preferir o trovão no Raijin, é só trocar os nomes das duas entradas em
`CHARS` no script.

**Hanzo:** o pedido era o Hanzo do pack B sem a barba. Esse sprite não estava
aqui, então o Hanzo saiu do corpo do pack A, de coque branco e sem barba. Assim
ele também combina com o resto. O script **não** apaga o Hanzo do pack B: se
`assets/sprites/hanzo/` já tiver pranchas de outro pack, o gerado vai para
`hanzo_gerado/`.

## Como rodar (no PC)

```sh
cd c_game
pip install pillow numpy
python3 tools/personagens.py --folhas
```

Na primeira vez, o script copia `assets/sprites/musashi/` (o pack original)
para `assets/sprites/_original/` e passa a ler dali. Depois grava uma pasta por
personagem em `assets/sprites/<nome>/`, cada uma com as mesmas pranchas e um
`sprite.txt` próprio. O `musashi/` passa a ter o Musashi sem chapéu, então o
jogo pega a versão nova sem mudar o carregador.

- `--so enjin kage`: gera só alguns.
- `--lista`: mostra quem é quem.
- Pastas com o arquivo `.gerado` são do script e podem ser sobrescritas; as
  outras ele não toca.
- Daqui em diante, os números de `hold` e `contact` se editam em
  `_original/sprite.txt`, e o script copia para todos.

### Folhas de conferência (`assets/sprites/_folhas/`)

| Arquivo | Para quê |
|---|---|
| `elenco.png`, `elenco_pb.png` | Os 14 lado a lado, no tamanho do jogo e ampliados; a versão em preto e branco confere se dá para distinguir pela forma |
| `golpes.png` | O quadro de contato de cada golpe, um personagem por linha |
| `<nome>.png` | Todas as pranchas do personagem, quadro a quadro, com o número embaixo |
| `alcance_<nome>.png` | Quadro de contato com a âncora dos pés (vermelho) e a ponta do golpe (ciano) |
| `deteccao.png` | O que o script achou em cada quadro, em cor chapada: chapéu (azul), camisa (rosa), hakama (cinza), pele, bainha (roxo), cabo, lâmina (ciano), rastro (amarelo) |

**Olhe a `deteccao.png` depois da primeira rodada.** O script foi afinado nas
pranchas ATTACK_1, ATTACK_2, ATTACK_3, DASH_ATTACK e DASH. IDLE, DEFEND, HURT,
DEATH, STRONG_ATTACK, THROW, RUN e JUMP ele processa do mesmo jeito, mas ainda
não foram vistos. Se algum quadro sair errado, dá para corrigir sem mexer no
código, num `_original/ajustes.txt`:

```
DEATH 7 semchapeu              # a cabeça está sem chapéu neste quadro
DEATH 8 apaga 60 50 80 60      # apaga um retângulo (ex.: o chapéu caindo)
HURT 2 chapeu 44 41            # o chapéu está com o canto em (44, 41)
```

## A espada que não encontra a outra no parry

A causa está nas pranchas. Cada golpe chega a uma distância diferente dos pés:

| Golpe | Contato | Alcance (px à frente da âncora) | Altura (px acima dos pés) |
|---|---:|---:|---:|
| ATTACK_2 (sobe) | quadro 2 | 30 | 20 |
| ATTACK_1 (horizontal) | quadro 2 | 35 | 21 |
| ATTACK_3 (desce) | quadro 2 | 40 | 20 |
| DASH_ATTACK | quadro 4 | 44 | 18 |

Além disso, no ATTACK_3 o pé da frente **avança uns 20 px dentro do quadro**,
e o DASH_ATTACK **começa 18 px atrás** da âncora (agachado) e dispara para a
frente. Com os dois lutadores a uma distância fixa, o ATTACK_2 não alcança e o
ATTACK_3 passa do ponto. É o "às vezes está atacando longe demais".

O script mede isso e escreve no `sprite.txt` de cada personagem:

```
ancora 53 74
guarda dx dy                   # ponta da lâmina do DEFEND no contato (aparece quando houver DEFEND.png)
anim ATTACK_1  hold 1  contact 2  alcance 35 -21
```

Regra para o duelo: **no quadro de contato, a distância entre as âncoras dos
dois é `alcance do golpe + guarda do Musashi`.** Como o alcance muda por golpe,
o mestre se posiciona a cada golpe:

```c
/* Mestre olha para a esquerda; Musashi para a direita. */
int alvo = musashi_x + musashi_guarda_dx + anim_do_golpe->alcance_dx;

/* Durante a antecipação: anda até o alvo em passos inteiros (ex.: 2 px por
   quadro de 0,08 s) ou encaixa direto no hold se a preparação for curta.
   Durante o golpe, não mexe: o passo do ATTACK_3 e o avanço do DASH_ATTACK
   já estão desenhados e contados no alcance. */

/* A faísca do parry vai no ponto de encontro: */
Vector2 faisca = {musashi_x + musashi_guarda_dx, chao_y + musashi_guarda_dy};
```

Ao espelhar quem olha para a esquerda, a âncora dentro do quadro também espelha:
`x_desenho = x_mundo - (virado ? cell_w - 1 - ancora_x : ancora_x)`.

## Limites conhecidos

- O rastro tem o formato do rastro da katana em todo mundo, só muda a cor. Para
  adaga e garra ele fica maior que a arma; no clima do Katana Zero isso passa
  como efeito.
- A segunda arma (Kage e Arashi) é uma cópia paralela da primeira, desenhada
  atrás do corpo. O sprite não mostra a outra mão segurando nada.
- No DASH, a bainha que o Musashi segura na mão de trás continua aparecendo
  para quem não usa bainha (é desenhada com as cores da hakama).
- As pranchas têm que estar viradas para a direita, como vêm no pack.

## Arte e repositório

O repositório é público e as pranchas são do pack pago da Mattz Art, então os
PNGs de `c_game/assets/sprites/` (os originais e os gerados, que saem deles)
estão no `.gitignore`. O script e os `sprite.txt` vão para o git normalmente.
