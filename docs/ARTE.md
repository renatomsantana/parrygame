# Arte e animações

## Protagonista

O ponto de partida é um samurai ágil em pixel art. A referência de roupa
é a ideia de shinobi de Sekiro, com mudanças próprias: **casaco laranja
assimétrico**, frente esquerda mais comprida, mangas claras, faixa de tecido
escuro e costura discreta. O cabelo é curto, preto e um pouco bagunçado.
O laranja ocupa a maior área de cor do personagem; não fica restrito a um
detalhe ou acessório.

| Área | Direção de cor |
|---|---|
| Casaco | Laranja queimado, luz âmbar |
| Camadas claras | Linho e marfim |
| Faixa e calça | Carvão e cinza escuro |
| Katana | Aço frio com luz clara no fio |
| Impacto perfeito | Dourado pálido e branco quente |
| Boss | Armadura escura, cordas vinho e violeta, máscara clara |
| Arena | Indigo, roxo, neon ciano/magenta e lanternas laranja |

## Assets incluídos

| Caminho dentro do projeto | Conteúdo |
|---|---|
| `Assets/Resources/Art/hero_orange.png` | Prancha de 1536 × 1024, 24 poses do herói |
| `Assets/Resources/Art/boss.png` | Prancha de 1536 × 1024, 24 poses do boss |
| `Assets/Resources/Art/arena.png` | Fundo lateral de 1672 × 941 |
| `Assets/Resources/Art/frames.json` | Retângulos e âncoras de todos os frames (âncoras em coordenadas da prancha) |

Os PNGs dos personagens têm canal alfa. A cor armazenada nos pixels
transparentes pode aparecer em leitores que ignoram esse canal; no jogo,
o importador deve preservar a transparência. Não use chroma key.

Cada prancha segue a organização visual de seis colunas e quatro linhas.
Os recortes usados pelo projeto são individuais, registrados em
`frames.json`, porque as poses não têm exatamente a mesma largura e altura.
A linha 2 do herói é o parry; a linha 3 traz poses de corte; a linha 4,
dano e derrota. A segunda linha do boss é a preparação do golpe.

## Reprodução e refinamento

Modo usado: **geração de imagens integrada**, sem CLI. Os PNGs foram
copiados para o projeto sem pintar ou alterar seus pixels. Recortes e
âncoras são recursos do jogo. As pranchas são uma base inicial para o
artista refinar alinhamento, pixels vizinhos e continuidade entre poses.

Os prompts abaixo registram a especificação final dos assets. A ferramenta
pode variar dimensões e detalhes; os tamanhos efetivamente entregues são
os da tabela acima. Gerar novamente não garante imagens idênticas.

### Prompt do herói laranja

```text
Create a usable retro PIXEL ART sprite atlas for a 2D action game. PNG with REAL ALPHA TRANSPARENCY. EXACT canvas 1536x1024. EXACT grid: SIX columns by FOUR rows, 24 equal cells of 256x256. Each cell contains ONE sequential animation pose. No background of any kind, no checkerboard painted in, no shadow, no floor, no labels, gridlines, lettering or frame numbers.
Original samurai hero: SHORT tousled BLACK HAIR, no ponytail, no topknot, no long hair. Orange is strongly dominant: bold burnt-ORANGE asymmetrical short shinobi coat, left front panel longer than the right, charcoal fabric waist belt, small ivory stitching, pale linen sleeves, dark slate trousers, wrapped lower legs, sandals. One steel katana. Outfit inspired by feudal shinobi clothing but a new character, not an exact Sekiro costume. Orange coat covers most of torso and upper legs. Clear chunky square pixel art similar to 16-bit side scrolling action games, not painted realistic art. Proportions like an agile adult sprite, not oversized chibi head.
All poses face RIGHT. Same identity, clothes, scale throughout. Anchor the character's rear foot at local x=105,y=220 in every cell. Whole sprite including katana fits inside its own cell with at least 8px transparent margin. Character standing height 155px inside each 256px cell.
Row 1: SIX different subtle IDLE breathing frames with katana ready in front.
Row 2: SIX frames of a fast PARRY cut: brace, draw back, slash up-forward, diagonal sword blocking contact with tiny gold spark, recoil, return to ready. Strongly distinct sequential poses.
Row 3: SIX frames of an ATTACK: wind-up, draw katana behind shoulder, fast cut forward, extended cut with thin ivory blade trail, follow-through, return to ready.
Row 4: SIX DEFEAT frames: hit recoil, stagger backward, kneel, fall, lying down, still lying down. No gore.
Strict 6x4 sheet. All 24 sprites present and no sprite crosses cell boundaries. Square pixels and a clean transparent silhouette, suitable for nearest-neighbor rendering.
```

### Prompt do boss

```text
Use case: stylized-concept. Asset type: production pixel-art boss sprite atlas for a 2D samurai parry duel.
Generate one PNG atlas, real transparent background, exactly 1536x1024 pixels, exactly 6 columns and 4 rows, 24 equal 256x256 cells with NO padding or gaps between cells. One full-body pose per cell, no numbers, captions, text, border, guides, grid or scenery.
An original tall armored samurai boss wearing charcoal lacquered armor with muted violet and burgundy cords, a simple ivory face mask, a low angular helmet without large horns, a tattered short plum cape, wielding a long steel katana. Side-view, always facing RIGHT. Polished sharp retro pixel art with small square pixel clusters, limited palette, consistent costume, proportions and scale across every frame. Figure around 155 pixels tall inside each cell, foot anchor x=112 y=220, all blade tips remain inside cell with 10 pixel margins.
Row 1: six idle breathing/ready frames, sword forward, cape subtly moving, last frame returns toward first.
Row 2: six anticipation frames for a telegraphed strike, sword gradually draws back and lifts above shoulder, increasing tension from first to sixth, no contact or swing yet.
Row 3: six attack frames, 1 launches, 2 accelerates, 3 decisive horizontal cut forward, 4 contact and thin white blade arc, 5 follow-through, 6 recovery. Very distinct sequential action poses.
Row 4: six stagger/defeat frames, first visibly recoils from deflection, second stumbles backward, third kneels, fourth collapses, fifth lies on floor, sixth still on floor. No gore. Ground anchor remains y=220 in each cell.
Exactly 6 columns and 4 rows, never add extra characters between cells. Transparent alpha behind all poses.
```

### Prompt da arena

```text
Use case: stylized-concept. Asset type: background image for a side-view 2D pixel-art samurai game.
A stunning polished retro pixel art nighttime alley combining old Japanese tiled roofs and rain-soaked cyberpunk neon. Wide 16:9 image, 1536x864 pixels if possible. Fixed orthographic SIDE VIEW, not isometric, no deep perspective floor. Purple and indigo atmosphere, warm orange lanterns, cyan and magenta reflected signs with abstract glyphs, hanging cables, layered apartments, shutters, tiled shrine eaves, pipes and a distant hazy city. The foreground action lane is a straight horizontal stone-and-metal walkway with its walking surface at exactly 79% of the image height; below this a dark detailed foundation fills the bottom 21%. Flat uninterrupted walkway across the full image. Center combat area relatively low contrast so two small samurai silhouettes will stand out. Upper half rich environmental detail, silhouettes of roofs, a moon partially veiled by clouds. Tight square pixel clusters and clear architectural edges, evocative atmospheric game background, no smooth painted brushwork. No people, no characters, no weapons, no interface, no health bars, no logo, no watermark. Ready to put animated characters in front.
```

## Próximos frames para o artista

Depois de testar o duelo, priorize o contato da defesa, a recuperação da
katana e a reação do boss. Mantenha os pés no mesmo chão, faça o laranja
continuar legível no fundo roxo e preserve a antecipação antes do contato.
As poses de corrida e salto só serão necessárias quando houver exploração.

## Os sete mestres com uma prancha só

Até existir arte própria, os mestres 1 a 6 usam `art/boss.png` com uma
tonalidade por mestre (`tint`) e uma tonalidade do cenário (`arena_tint`),
ambas em `Assets/Scripts/Core/BossRoster.cs`. A Sombra usa `art/hero_orange.png`
espelhada e escurecida. Quando houver pranchas próprias, basta apontar o
mestre para a prancha nova em `ActorView.Load` e devolver as tonalidades a branco.

| Mestre | Sprite | Tom do sprite | Tom do cenário |
|---|---|---|---|
| Gorou | boss.png | Neutro | Quente, lanternas |
| Neon Jax | boss.png | Magenta | Roxo saturado, neon |
| Cavan | boss.png | Terroso | Âmbar de entardecer |
| Vance | boss.png | Azul frio | Azul de escritório |
| Kaelen | boss.png | Violeta claro | Saturado, tintas |
| Eleonor | boss.png | Pálido, dourado | Dourado de salão |
| Sombra | hero_orange.png | Silhueta escura | Dessaturado, escuro |

Próximas pranchas, em ordem de valor: a Sombra com paleta invertida de
verdade; Eleonor com florete; Cavan com foice. Cada cenário novo substitui
`Assets/Resources/Art/arena.png` e a tonalidade daquele mestre.

## Cenários por mestre (arte para depois)

O código já espera uma imagem por mestre em `Assets/Resources/Art/Arenas/`,
mesmo formato da arena atual (16:9, visão lateral, chão a cerca de 74% da
altura). Enquanto o arquivo não existe, a arena comum entra com a tonalidade
do mestre.

| Mestre | Arquivo | Cenário do roteiro |
|---|---|---|
| Gorou | `arena_dojo.png` | Dojo clássico de tatame e bambu |
| Neon Jax | `arena_balada.png` | Clube noturno rave, luzes estroboscópicas |
| Cavan | `arena_campo.png` | Campo aberto ou celeiro rústico ao entardecer |
| Vance | `arena_escritorio.png` | Escritório corporativo no topo de um arranha-céu |
| Kaelen | `arena_galeria.png` | Galeria de arte surrealista, telas rasgadas e tintas brilhantes |
| Eleonor | `arena_salao.png` | Salão de baile nobre com espelhos |
| Sombra | `arena_jardim.png` | Jardim de Vidro, Altar da Alma |
| Mestre Supremo (provisório) | `arena_liga.png` | Salão da Liga, sete gemas vazias |

Pranchas próprias por mestre entram por `SheetAsset`: acrescente a chave em
`frames.json` (mesmo formato de `hero` e `boss`) e o PNG ao lado. No RPG
Maker, o equivalente é um mapa por mestre e um SV battler por mestre.

## Referências de arte

- **KATANA ZERO**: visão lateral, neon, contraste forte, o que já guia a arena atual.
- **Children of Morta**: pixel art pintado, luz quente e volumes suaves nos
  personagens, cenários com profundidade e detalhe sem perder a leitura.
  É a direção para as pranchas e os sete cenários quando a arte entrar:
  silhuetas limpas para o duelo, acabamento pintado para o fundo.
