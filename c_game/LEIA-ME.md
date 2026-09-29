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
make test             # regras do núcleo, sem janela (inclui robôs e janelas viáveis)
make robos            # curva de dificuldade: os robôs contra cada mestre (LUTAS=300)
```

Os lutadores, os efeitos e as teclas vêm de packs pagos da Mattz Art, que não
vão para o git (o repositório é público). `make packs ZIP=...` pega o zip com
todas as animações e põe cada tira no lugar certo (`tools/instalar_packs.sh`):
o Samurai #3 em `assets/sprites/_original/`, os outros packs em
`assets/sprites/_packs/<pack>/` (os `sprite.txt` já estão lá), os efeitos em
`_fx/` e as teclas em `_ui/`. Depois, `make sprites` gera os lutadores. Sem as
tiras, o jogo roda com os bonecos de `src/rig.c` e as partículas de sempre.

Controles: **clique, Espaço, J ou Enter** aparam e avançam as falas.
Esc pausa (T volta à trilha, L calibra o atraso, M volta ao menu, Q sai). Na
trilha, o botão "menu" (ou Esc) volta ao menu. F liga e desliga o tremor. F11
alterna a tela cheia. **F3** (ou `APARA_DEBUG=1`) mostra o overlay de debug: as
janelas, a linha do tempo do golpe, o último aperto com o erro em ms, a fase, o
golpe, as posturas e a vida. **L** no título ou na pausa abre a calibração de
latência (um quadrado que pisca, depois um clique; aperte junto). O progresso
fica em `apara_save.txt` e a calibração em `apara_opcoes.txt`, ao lado do
executável. Todas as constantes globais de equilíbrio e de sensação estão em
`src/ajuste.h`; as de cada mestre, em `src/roster.c`.

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
- **Cada golpe:** o mestre prepara (cada sequência tem a sua preparação, sempre a
  mesma), e um **aviso**, som e brilho na lâmina, vem sempre o mesmo tempo antes do
  contato: 450 ms no daichi, descendo até 320 ms (jinshi, sem som, avisa só com o
  brilho da lua, 350 ms antes). A lâmina parte 220 ms antes do contato. Na
  sequência, o aviso de cada golpe é o contato anterior (o ritmo).
- **Janela:** perfeito se o aperto cai na janela perfeita antes do contato; bom na
  janela boa ou **até 30 ms depois do contato** (tolerância tardia).
- **Apertar cedo**, antes do aviso, não trava o golpe: dá uma recarga de no máximo
  0,5 s (que acaba no aviso, nunca na janela boa) e a defesa daquele golpe não sai
  perfeita. Depois do aviso vale uma tentativa só. O jogo mostra "cedo" ou
  "tarde" quando a defesa não pega.
- O **hitstop** congela o duelo no impacto, e o tempo dele sai do golpe seguinte da
  sequência: o ritmo é sempre o mesmo, qualquer que seja o resultado.
- Quando falta **um perfeito** para quebrar a postura, aparece "vantagem". O parry
  que quebra a postura é a execução: sem botão.

| Resultado | Vida de kojiro | Postura do adversário |
|---|---:|---:|
| Perfeito | +3% da vida | −20 (+2 por aprendiz vencido) |
| Bom | −4 | −6 (+0,5 por aprendiz vencido) |
| Erro (cedo, tarde ou sem defesa) | vida ÷ erros até cair (tabela abaixo) | +20 do 5º em diante |

kojiro tem 250 de vida contra todos. Com metade da postura, o aprendiz acelera a
espera antes do aviso (do aviso ao contato, nunca muda). Quebrar a postura
**desarma**: a arma voa, crava no chão e o adversário cai de joelhos. Depois de
duas derrotas seguidas, dá para **conversar com hanzo** (no Oboro, ele só diz
"Confie em você mesmo. Use tudo que aprendeu.").

## A trilha

| # | Aprendiz | Postura | Arma | Cenário | Postura dele | Perfeito / Bom | Aviso | Erros até cair |
|---:|---|---|---|---|---:|---:|---:|---:|
| 1 | daichi | terra (bem devagar) | katana | Celeiro | 300 | 90 / 220 ms | 450 ms | 8 |
| 2 | genbu | tartaruga | katana simples | Jardim de pedras do mosteiro | 330 | 82 / 203 ms | 437 ms | 8 |
| 3 | raizo | touro | espadão | Pátio do dojo | 350 | 75 / 185 ms | 424 ms | 7 |
| 4 | shizuku | gelo | florete de esgrima com geada | Cachoeira | 380 | 68 / 171 ms | 411 ms | 7 |
| 5 | garfiel | tigre | garras nas duas mãos | Portão do tigre branco | 530 | 63 / 158 ms | 398 ms | 4 |
| 6 | karasu | corvo | katana e wakizashi | Telhados da vila na chuva | 490 | 58 / 148 ms | 385 ms | 7 |
| 7 | hayate | vento (ritmo quebrado) | duas foices pequenas | Ponte de corda no desfiladeiro | 450 | 54 / 138 ms | 372 ms | 4 |
| 8 | enjin | chama | katana de fogo | Forja | 460 | 50 / 131 ms | 359 ms | 4 |
| 9 | suiren | mar (acelerando) | lança | Porto | 490 | 47 / 125 ms | 346 ms | 4 |
| 10 | arashi | tempestade (dano 1,2×) | duas katanas | Salão do castelo na tempestade | 550 | 46 / 121 ms | 333 ms | 10 |
| 11 | yoru | noite (apagões) | duas adagas (ao contrário) | Bambuzal | 740 | 44 / 119 ms | 320 ms | 4 |
| 12 | jinshi | lua (sem som) | katana bem branca, forjada com a lua | Encosta da serra | 610 | 44 / 119 ms | 350 ms (brilho) | 5 |
| 13 | **oboro** | hanzo → devorador de posturas → oni (uma por selo) | katana de hanzo | Dojo de hanzo | 360 · 1800 · 600 | 56 → 46 → 38 ms | 380 → 350 → 320 ms | 11 |

A curva foi afinada com os robôs (`make robos`): o humano casual que decora o
ritmo vence os quatro primeiros sempre, e a vitória cai sem degraus até uns 65%
no jinshi e uns 40% no oboro; apertar sem olhar, em qualquer ritmo, perde de
todos (`test_curva`). As janelas apertam pela trilha; os erros até cair corrigem
o que é de cada um (o ritmo fácil do garfiel, os golpes duplos do arashi).

**Oboro**, três selos: na **postura de hanzo**, abre sempre com a *lição
completa*, sete golpes seguidos; no **devorador de posturas** faz os doze padrões
dos aprendizes, um de cada, iguais ao original, na ordem da trilha na primeira
volta e depois sorteados; na **postura do oni**, os mesmos doze mais rápidos
(espera antes do aviso ×0,85) e mais pesados (dano ×1,25), sem o golpe especial
que tira o dobro (esse sai nas duas primeiras fases, 25% das vezes).

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
só com a lança da suiren, **de longe** (ela se afasta e a ponta viaja: entre a
lâmina partir e chegar passa 1,75 vez o tempo dos outros golpes, então quem
aparar no susto da partida chega cedo). O karasu tem ainda o **sumiço**: no meio
da preparação ele vira penas e some, e reaparece na frente de kojiro com a lâmina
no alto, pouco antes de ela partir; é o reaparecer que avisa. **⚔** é o golpe de **duas lâminas**
(o corte cruzado, anunciado por um brilho duplo e um tinido duplo): um parry só
segura as duas se for perfeito; no bom, a segunda entra; no erro, entram as
duas. Todos têm de sete a dez sequências. Daichi, Genbu e Raizo nunca passam de
dois contatos; Garfiel chega a oito golpes seguidos; arashi bate 1,2 vez mais
forte; enjin deixa kojiro **em brasas** a cada erro
(ele perde mais 60% de um golpe ao longo de 3 s, e o parry perfeito apaga); yoru
apaga as luzes em sete de cada dez sequências; jinshi tem o
repertório mais variado e o ritmo irregular.

| Aprendiz | Sequências (golpes: intervalos em s) |
|---|---|
| daichi | rocha (1) · raiz (1) · sulco (1) · desabamento (2: 1,00) · arado (1) correndo · pedregulho (2: 1,10) saltando · **terremoto** (forte) |
| genbu | casco (1) · mordida (2: 0,42) · carapaça (2: 0,60) · concha (1) · bote da tartaruga (1) correndo · maré lenta (2: 0,75) · casco fechado (2: 0,90) |
| raizo | corte do touro (1) · investida dupla (2: 0,85) · coice (1) · marrada (2: 1,00) · estouro da boiada (1) correndo · pisada (2: 0,70) saltando · **chifrada** (forte) |
| shizuku | floco (1) · geada (2: 0,45) · sincelo (2: 0,40) · estalactite (3: 0,45 0,40) · nevasca (4: 0,40 0,40 0,40) · gelo fino (1) · deslize (1) correndo · salto do cristal (2: 0,45) saltando · **glaciar** (forte) |
| garfiel | patada (1) · garras cruzadas (2: 0,40) · rasgo (3: 0,40 0,40) · bote do tigre (3: 0,40 0,85) · fúria do tigre (6: 0,40 0,40 0,40 0,40 0,40) · caçada (7: 0,40 0,40 0,45 0,40 0,40 0,70) correndo · rugido (8: 0,40 0,40 0,40 0,40 0,40 0,40 0,80) · pulo do gato (2: 0,45) saltando · **salto do tigre** (forte) |
| karasu | bicada (1) · garra (2: 0,55) · sumiço (1) sumindo · corvo fantasma (2: 0,55) sumindo · revoada (3: 0,50 0,90) ⚔ no 3º · bando (4: 0,45 0,45 0,45) ⚔ no 4º · duas penas (1) ⚔ · voo rasante (1) correndo · asa quebrada (2: 0,50) saltando · **mergulho** (forte) |
| hayate | rajada (1) · redemoinho (2: 0,45) · vendaval (3: 0,50 0,90) · brisa cortante (2: 0,40) · tufão (5: 0,40 0,40 0,40 0,60) · foices gêmeas (2: 0,45) ⚔ no 2º · lufada (1) correndo · folha ao vento (2: 0,55) saltando · **ciclone** (forte) |
| enjin | brasa (2: 0,50) · labareda (3: 0,45 0,45) · incêndio (4: 0,45 0,45 0,80) · **erupção** (forte) · fagulhas (3: 0,40 0,40) · chama viva (1) correndo · cinzas (2: 0,45) · fogo alto (3: 0,45 0,45) saltando |
| suiren | onda (1) · arpão (1) de longe · linha d'água (2: 0,60) de longe · maré longa (3: 0,60 0,50) de longe · maré baixa (2: 0,45) · arrebentação (3: 0,45 0,45) · maremoto (4: 0,55 0,50 0,45) · espuma (1) correndo · salto da baleia (2: 0,60) saltando · **vagalhão** (forte) |
| arashi | faísca (1) · duas tempestades (1) ⚔ · trovoada (2: 0,45) ⚔ no 2º · tormenta (3: 0,40 0,40) ⚔ no 3º · granizo (4: 0,40 0,40 0,40) · ventania (3: 0,40 0,70) · trovão (1) correndo ⚔ · raio duplo (2: 0,40) saltando ⚔ nos dois · céu partido (5: 0,40 0,40 0,40 0,80) ⚔ no 5º · **relâmpago** (forte) ⚔ |
| yoru | sombra (1) · presas (2: 0,45) · lua nova (3: 0,45 0,80) · **eclipse** (forte) · vultos (3: 0,40 0,70) · breu (1) correndo · coruja (2: 0,45) saltando · meia-noite (4: 0,40 0,40 0,90) · nevoeiro (2: 0,70) |
| jinshi | crescente (1) · minguante (3: 0,60 0,60) · fases da lua (4: 0,50 0,50 0,90) · luar (3: 0,45 1,00) · lua cheia (5: 0,50 0,50 0,50 0,90) · lua branca (6: 0,40 0,90 0,45 0,45 1,00) · noite branca (4: 1,00 0,40 0,40) · reflexo no lago (1) correndo · lua alta (2: 0,70) saltando · **halo** (forte) |
| oboro | **postura de hanzo:** lição completa (7: 0,60 0,50 0,50 0,70 0,45 0,45), sempre a primeira · corte do mestre (1) · lição (2: 0,60) · estocada de hanzo (1) · três lições (3: 0,50 0,60) · passo de hanzo (1) correndo · salto do mestre (2: 0,55) saltando · **devorador de posturas** e **postura do oni:** os doze ecos, cada um igual ao golpe do aprendiz: eco da terra (desabamento) · da tartaruga (mordida) · do touro (investida dupla) · do gelo (nevasca) · do tigre (fúria do tigre) · do corvo (revoada ⚔) · do vento (foices gêmeas ⚔) · da chama (incêndio) · do mar (maré longa, de longe) · da tempestade (tormenta ⚔) · da noite (meia-noite) · da lua (lua cheia) |

## Visual e som

- Tudo em pixel art de 320 × 180, ampliado por número inteiro e sem filtro:
  cenários, trilha, lore, final, lutadores (com contorno escuro e filete de luz na
  cor do cenário) e a interface.
- Todos os cenários são do Japão antigo: casa de fazenda (árvores de outono
  entre a casa e a cerca), jardim de pedras com o laguinho de carpas e a ilha da
  tartaruga (pinheiros velhos dos dois lados do pátio), pátio do dojo (a última
  porta de correr aberta para o jardim no fim da tarde), a cachoeira da Shizuku
  no inverno (rocha azulada com neve nos degraus, pinheiros nevados, colunas e
  pingentes de gelo, o poço de água escura e a neve caindo na frente), o portão
  de dois andares do tigre branco (bandeiras listradas, lanternas de pedra,
  bordos com a copa em massas), telhados da vila do castelo na chuva (uma
  fileira de casas iguais de janelas quadradas, pinheiros molhados nos quintais,
  a lua crescente e os corvos na cumeeira), ponte de corda sobre o desfiladeiro
  no vento (pinheiros tortos no alto dos paredões e o capim alto deitando com as
  rajadas, também na frente), a forja dentro da cratera (lago e cascata de lava
  com línguas de fogo, braseiros de ferro nas pontas, brasas subindo, o barracão
  com a corda sagrada, a fornalha de barro e as katanas esfriando), o porto do
  farol (com o lago de lótus da Suiren), o salão do castelo numa noite de
  tempestade (biombos de ouro, as portas abertas para a chuva e os raios que
  clareiam a luta, com trovão), bambuzal (o luar na borda dos colmos), a encosta
  da serra com a lua cheia grande e o dojo de Hanzo (pinheiros em camadas).
- As árvores têm volume: tronco com o lado da luz, copa em massas de três e
  quatro tons com a borda recortada (pinheiros em camadas de agulha, árvores de
  folha em tufos), e as de trás menores e apagadas na névoa. Os helpers são
  `canopy`, `broadleaf`, `tier_pine`, `snow_pine` e `flame` em `src/arenas.c`.
- A poeira dos pés (o bote, a queda, o passo, a espada caindo) sai menor e na cor
  do chão de cada cenário (`arena_dust`: a palha, o cascalho, a neve, a cinza...).
  O aviso do Jinshi (o brilho da lua antes do golpe) ficou pequeno e lilás, na
  paleta do céu da serra, em vez da nuvem branca grande.
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
  Na cabana, Hanzo fica sentado (`SENTADO`) junto da fogueira: pedras em volta,
  lenha cruzada, brasas, três labaredas, fumaça e fagulhas subindo, e pinheiros
  em camadas atrás.
- **Kojiro saca a espada no começo de cada luta.** Fora da luta (a conversa antes
  do duelo), ele fica com a katana embainhada (`EMBAINHADO`); quando o duelo
  começa, toca o `DESEMBAINHAR` (0,42 s, com o som do corte), que termina
  exatamente na guarda (o quadro 0 do IDLE), antes do primeiro golpe do mestre.
  No código: `ren_sheathed()` é chamada em `setup_actors()`, logo depois de
  carregar o Kojiro, e `ren_draw_sword()` em `start_duel()`, depois de
  `setup_actors()` (as duas em `src/main.c`). Na cabana, Kojiro também fica
  com a katana na bainha (`sprite_person_pose` em `src/lore.c` prefere o
  `EMBAINHADO`).
- Oboro luta de rosto descoberto nas duas primeiras formas: o gerador acha o
  elmo de oni do pack em cada quadro (de frente, de costas e caído), tira o elmo
  inteiro e pinta no lugar a cabeça dele, a mesma em todos os quadros: o rosto do
  Hanzo jovem, cabelo curto e barba curta (`oboro/`); a prancha com a máscara sai
  em `oboro_mascara/`, com o elmo visível em todos os quadros. O Hanzo mascarado
  usa o mesmo elmo, assentado na cabeça dele em cada quadro.
- Entre os aprendizes só o Yoru usa pano no rosto; Garfiel, Karasu, Hayate e
  Arashi lutam de rosto descoberto, cada um com o seu cabelo. A Shizuku é a
  postura do gelo (florete com geada, cabelo prateado); a Suiren, de lança,
  ficou com o visual que era dela.
- Cada arma tem o seu golpe: a lança e o florete estocam, as garras do Garfiel
  deixam três riscos, as foicinhas do Hayate dois arcos curtos, a katana e a
  wakizashi do Karasu dois arcos cruzados em X, as adagas invertidas do Yoru
  cortes curtos e secos, e as espadas do Arashi raios em zigue-zague. Ver
  `../docs/PERSONAGENS.md` (inclusive o alcance novo de cada um).
- A escolha do fim corta a música e deixa só o vento; nos finais, Hanzo entra
  batendo palmas (sim) ou sai do escuro (não).
- Teclas de pixel na pausa e, no primeiro duelo, a dica de como aparar.
- Trilha sonora sintetizada por cenário; vitória e derrota com sons curtos e sutis.

Da direção de arte ficaram de fora os golpes imbloqueáveis e a esquiva (o jogo é
de um botão só).

## Código

| Arquivo | O quê |
|---|---|
| `src/core.c`, `core.h` | Regras puras: relógio, aviso, tentativa, julgamento (com tolerância e latência), hitstop, postura, selos, vantagem, moveset, trilha. Sem raylib. |
| `src/ajuste.h`, `ajuste.c` | **Todas as constantes globais** de equilíbrio e de sensação, com comentário em cada uma (vida, cura, aviso, tolerância, recarga, hitstop, tremor, câmera lenta, recuo, calibração). |
| `src/roster.c` | Os treze lutadores (doze aprendizes e oboro): janelas, aviso, preparação de cada sequência, erros até cair, postura, traços; falas, visitas, lore e cenas. **O balanceamento de cada mestre é aqui.** |
| `src/robo.c`, `robo.h` | Os robôs: o do `--demo`, os dos testes e os da curva de dificuldade (perfeito, sem defesa, spam, reação, humano casual). Sem raylib. |
| `src/main.c` | Telas, coreografia, interface, visuais de cada lutador (`MASTER_LOOKS`) |
| `src/sprites.c` | Lê as pranchas geradas (`sprite.txt` e tiras) e desenha e toca os lutadores em pixel art; efeitos em folha e teclas |
| `src/rig.c` | Bonecos (quando faltam as pranchas): poses, passos, cansaço, roupas, armas |
| `src/katana3d.c` | A katana 3D dentro do mundo em pixel |
| `src/arenas.c` | Os treze cenários dos duelos (e o do título) |
| `src/pixelize.c` | Paleta curta por cena e dithering ordenado nos fundos |
| `src/lore.c` | Tela de título, trilha e a cabana de hanzo |
| `src/fx.c` | Faíscas, estrela, anéis, arcos, flash, tremor |
| `src/audio.c` | Efeitos e trilha ambiente sintetizados |
| `tests/core_test.c` | Verificações do núcleo (`make test`): regras, janelas viáveis em todo golpe, aviso, aperto cedo, ritmo com hitstop, calibração, curva e Oboro |
| `tests/robos.c` | A tabela dos robôs por mestre (`make robos`) |
| `tools/instalar_packs.sh` | Põe o zip das animações nas pastas do gerador e do jogo (`make packs ZIP=...`) |
| `tools/personagens.c` | Gera os 15 lutadores (cabeça, arma, corpo, rastro, aura, golpe especial, pose desarmada), uma pasta por nome (kojiro, raizo, yoru, garfiel...), a partir do Samurai #3 e dos packs de `assets/sprites/_packs/` (Raizo com o espadão, Shizuku, Suiren e Jinshi no Samurai #4, Arashi, Oboro com as posturas dos outros e o grito): `make sprites`; ver `../docs/PERSONAGENS.md` |

Para testar sem jogar: `./apara --master 13 --duel --demo` põe um robô aparando
contra oboro; `--shot arquivo.png 5` salva uma captura depois de 5 segundos;
`--state pause|defeat|finisher|cleared` (com `--master N --duel`) abre direto a
pausa, a derrota, o desarme ou a vitória; `--rec pasta 1 5` salva os quadros de
1 a 5 segundos (30 por segundo, tempo fixo), para GIFs.
