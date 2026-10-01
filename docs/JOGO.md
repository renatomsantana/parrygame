# aparar — o jogo por dentro

O desenho do jogo, as regras, a trilha, o moveset e o visual. Para rodar, os
controles e os testes, veja `../c_game/LEIA-ME.md` (uma página). Os caminhos
`src/`, `tests/` e `tools/` daqui são relativos a `c_game/`. História em
`../aparar_lore.md`; direção de arte em `../parrygame_arte.md`; os lutadores
gerados em `PERSONAGENS.md`.

Os packs de arte (Mattz Art) são pagos e não vão para o git: `make packs ZIP=...`
põe as tiras de cada pack no lugar (`tools/instalar_packs.sh`): o Samurai #3 em
`assets/sprites/_original/`, os outros packs em `assets/sprites/_packs/<pack>/`,
os efeitos em `_fx/` e as teclas em `_ui/`. Depois, `make sprites` gera os
lutadores. Sem as tiras, o jogo roda com os bonecos de `src/rig.c` e as
partículas de sempre.

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
  brilho da lua, 350 ms antes). Nos quatro primeiros, a lâmina parte (o bote e o
  assobio) 220 ms antes do contato; do garfiel em diante, um tempo sorteado a cada
  golpe, entre 140 e 320 ms (240 na sequência): reagir à lâmina não basta, vale o
  ritmo do aviso. O aviso e o contato não mudam, nem os quadros do golpe
  (`AJ_LAMINA_VARIAVEL` em `src/ajuste.h`; 0 deixa fixa). Numa sequência, com o atraso
  calibrado alto, a lâmina parte no máximo o tempo que falta até o contato (nunca menos
  de 140 ms): o ritmo não atrasa com atraso até 120 ms. Na sequência, o aviso de
  cada golpe é o contato anterior (o ritmo).
- **Janela:** perfeito se o aperto cai na janela perfeita antes do contato; bom na
  janela boa ou **até 30 ms depois do contato** (tolerância tardia).
- **Apertar cedo**, antes do aviso, não trava o golpe: dá uma recarga de no máximo
  0,5 s (que acaba no aviso, nunca na janela boa) e a defesa daquele golpe não sai
  perfeita. Depois do aviso vale uma tentativa só. O jogo mostra "cedo" ou
  "tarde" quando a defesa não pega.
- **Taxa de quadros:** o núcleo do duelo não depende dela. O tempo avança em
  `duel_tick(delta)`; a preparação começa e a recuperação conta a partir do instante
  certo (`phaseEnd`, o impacto), nunca do quadro em que o relógio o passou, e o
  julgamento usa o instante do aperto e o do contato. Os mesmos apertos, nos
  mesmos ms, dão o mesmo resultado a 30, 60, 120, 144 e 240 Hz (`test_taxa_de_quadros`).
  O clique do jogo entra no **instante em que o sistema o carimbou**
  (`src/entrada.c`, `entrada_plat.h`): o macOS dá `NSEvent.timestamp`, o X11 os eventos brutos do
  XInput2, e o carimbo é convertido para o passo do núcleo (`entrada_no_quadro`), com câmera lenta e
  hitstop, e entra no `duel_step_at` como o dos robôs. Sem carimbo (Wayland, Windows, gamepad), com um
  carimbo fora do quadro (mais de 5 ms) ou em demonstração, o clique entra no meio do quadro
  (`duel_step`), com ±½ quadro de imprecisão: nesse caso, nos mestres finais o casual vence uns 3
  a 6 pontos menos a 60 Hz do que a 144 Hz (±8 ms contra ±3 ms); com o carimbo, a tabela do casual é a mesma
  a qualquer taxa (`make test-entrada`). O F3 mostra quantos apertos usaram o carimbo e quantos caíram no meio
  do quadro; `./apara --carimbo` mede o atraso do poll em 20 cliques. Os robôs decidem em ms e apertam no instante
  exato (`duel_step_at`, `robo_aperto_em`): `make robos HZ=144` dá a mesma tabela que
  60; `QUADROS=1` põe o aperto no meio do quadro, como no jogo. `make robos-taxas`
  (dentro do `make teste`) confere que a mesma luta dá o mesmo resultado a 30, 60, 120,
  144 e 240 Hz. Uma exceção conhecida, que não é regra: a queimadura do enjin é somada
  por quadro, antes de os eventos do quadro serem julgados, e por isso uns 0,4% das lutas
  contra ele mudam de uma taxa para outra (o teste aceita até 2%).
- **Rastro fantasma do golpe:** na partida da lâmina, o mestre deixa fantasmas do quadro que
  já está mostrando, esticados para trás, que crescem até o contato e somem nele. O quadro
  de contato só aparece no impacto, no instante do julgamento. É só desenho
  (`AJ_RASTRO_FANTASMA`, `_FANTASMAS`, `_ESPACO` e `_ALFA` em `src/ajuste.h`; 0 desliga) e
  `tests/teste_rastro.sh` (`make teste-jogo`) confere, no jogo, que os impactos saem idênticos
  com ele ligado e desligado.
- **Ritmo:** o mestre não fica parado à toa. A espera antes do aviso (a
  preparação segurada) vale ×0,50 do que estava no roster (piso de 100 ms; ×0,60 no hayate e no jinshi, de ritmo
  irregular, que já batem no piso) e a pausa depois de cada sequência é de 0,40 s: no casual, 6 a 13% menos tempo de
  duelo em cada mestre, com a curva de vitórias igual. Do aviso ao contato, as janelas e o intervalo dentro da sequência
  não mudam (`AJ_ESPERA_X`, `AJ_ESPERA_X_IRREGULAR` e `AJ_PAUSA_SEQUENCIA`, em `src/ajuste.h`; 1,0 e 0,8 voltam ao
  antigo). A pausa não pode cair abaixo do fim da recuperação do mestre (0,32 s) nem deixar de ter um tempo depois da
  janela de "tarde" (`test_aperto_cedo`, no `core_test`).
  Nas cenas, do golpe final à fala do vencido passam 2,6 s (o hitstop, a câmera lenta do desarme, a espada que crava e
  `AJ_ESPADA_CRAVADA_ESPERA`) e, do selo quebrado do oboro à cena de fala, 1,6 s (`AJ_QUEBRA_ATE_A_CENA`); a
  câmera lenta e o grito de depois da cena (`AJ_PAUSA_APOS_CENA_SELO`, do tamanho da animação) ficam como estavam.
  As telas de resultado seguram o clique um pouco menos: a derrota aceita clique 1,2 s depois de aparecer (a palavra
  "derrota" entra aos 0,8 s, para estar inteira antes disso, como já estava) e a vitória, 0,7 s depois; quem aperta sem
  parar vê o resultado inteiro antes de o clique valer (`AJ_DERROTA_*`, `AJ_VITORIA_TRAVA`, `AJ_FADE_RESULTADO`).
- O **hitstop** congela o duelo no impacto, e o tempo dele sai do golpe seguinte da
  sequência: o ritmo é sempre o mesmo, qualquer que seja o resultado. O jogo congela o
  tempo real pelo mesmo número de segundos (`hitstop_passo`), e o que sobra do quadro
  em que o congelamento acaba volta ao duelo, em vez de se perder.
- A **agenda** de cada golpe (lançamento da lâmina, aviso, som do aviso) dispara na hora
  certa mesmo com um quadro grande: `duel_tick` e `duel_press` disparam o que já venceu.
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
| 3 | raizo | montanha | odachi | Pátio do dojo | 350 | 75 / 185 ms | 424 ms | 7 |
| 4 | shizuku | gelo | florete de esgrima com geada | Cachoeira | 380 | 68 / 171 ms | 411 ms | 7 |
| 5 | garfiel | tigre | garras nas duas mãos | Portão do tigre branco | 530 | 63 / 158 ms | 398 ms | 6 |
| 6 | karasu | corvo | katana e wakizashi | Telhados da vila na chuva | 490 | 58 / 148 ms | 385 ms | 7 |
| 7 | hayate | vento (ritmo quebrado) | duas foices pequenas | Ponte de corda no desfiladeiro | 620 | 49 / 138 ms | 372 ms | 6 |
| 8 | enjin | chama | katana de fogo | Forja | 690 | 46 / 131 ms | 359 ms | 6 |
| 9 | suiren | mar (acelerando) | lança | Porto | 840 | 46 / 125 ms | 346 ms | 6 |
| 10 | arashi | tempestade (dano 1,2×) | duas katanas | Salão do castelo na tempestade | 550 | 46 / 121 ms | 333 ms | 10 |
| 11 | yoru | noite (apagões) | duas adagas (ao contrário) | Bambuzal | 970 | 44 / 119 ms | 320 ms | 5 |
| 12 | jinshi | lua (sem som) | katana bem branca, forjada com a lua | Encosta da serra | 610 | 43 / 119 ms | 350 ms (brilho) | 5 |
| 13 | **oboro** | hanzo → devorador de posturas → oni (uma por selo) | katana de hanzo | Dojo de hanzo | 360 · 1800 · 450 | 62 → 51 → 42 ms | 380 → 350 → 320 ms | 5 · 10 · 4 |

A curva foi afinada com os robôs (`make robos`, o núcleo exato, que não depende da
taxa de quadros): o humano casual que decora o ritmo vence os cinco primeiros
(garfiel com 6 erros já vai a uns 100%), e a vitória cai sem degraus até uns 60% no
jinshi e uns 41% no oboro (100, 100, 100, 100, 100, 91, 84, 80, 72, 64, 59, 58, 41); apertar sem olhar, em qualquer ritmo, perde de
todos (`test_curva`). Do karasu ao oboro a curva cai com folga de 0,5 ponto de um mestre para o seguinte, conferida com 100 mil
lutas por mestre (`make curva-ordem`); a janela perfeita do jinshi, 43 ms (a do yoru é 44), é o que o põe abaixo do yoru. As janelas apertam pela trilha; os erros até cair corrigem
o que é de cada um (o ritmo fácil do garfiel, os golpes duplos do arashi).

**Oboro**, três selos: na **postura de hanzo**, abre sempre com a *lição
completa*, sete golpes seguidos; no **devorador de posturas** faz os doze padrões
dos aprendizes, um de cada, iguais ao original, na ordem da trilha na primeira
volta e depois sorteados; na **postura do oni**, os mesmos doze mais rápidos
(espera antes do aviso ×0,85) e mais pesados (dano ×1,25). Cada selo quebrado
devolve a vida inteira, e cada fase é uma prova: kojiro cai com 5 erros na
primeira, 10 na segunda (dano ×0,5: é a mais comprida, os doze padrões inteiros) e
4 na terceira. Nenhuma fase tem o golpe especial que tira o dobro.

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
| raizo | corte do cume (1) · fenda dupla (2: 0,85) · rasgo na laje (1) · ponta da serra (2: 1,00) · avalanche (1) correndo · queda de pedras (2: 0,70) saltando · **montanha partida** (forte). Todos com a odachi do pack e sinais cinza-pedra/ocre na lâmina. |
| shizuku | floco (1) · geada (2: 0,45) · sincelo (2: 0,40) · estalactite (3: 0,45 0,40) · nevasca (4: 0,40 0,40 0,40) · gelo fino (1) · deslize (1) correndo · salto do cristal (2: 0,45) saltando · **glaciar** (forte) |
| garfiel | patada (1) · garras cruzadas (2: 0,40) · rasgo (3: 0,40 0,40) · bote do tigre (3: 0,40 0,85) · fúria do tigre (6: 0,40 0,40 0,40 0,40 0,40) · caçada (7: 0,40 0,40 0,45 0,40 0,40 0,70) correndo · rugido (8: 0,40 0,40 0,40 0,40 0,40 0,40 0,80) · pulo do gato (2: 0,45) saltando · **salto do tigre** (forte) |
| karasu | bicada (1) · garra (2: 0,55) · sumiço (1) sumindo · corvo fantasma (2: 0,55) sumindo · revoada (3: 0,50 0,90) ⚔ no 3º · bando (4: 0,45 0,45 0,45) ⚔ no 4º · duas penas (1) ⚔ · voo rasante (1) correndo · asa quebrada (2: 0,50) saltando · cruz de penas (1) ⚔ · corte curto (2: 0,40) · **mergulho** (forte) |
| hayate | rajada (1) · redemoinho (2: 0,45) · vendaval (3: 0,50 0,90) · brisa cortante (2: 0,40) · tufão (5: 0,40 0,40 0,40 0,60) · foices gêmeas (2: 0,45) ⚔ no 2º · lufada (1) correndo · folha ao vento (2: 0,55) saltando · gancho duplo (2: 0,40) · ceifada em X (1) ⚔ · vento partido (3: 0,40 0,45) · **ciclone** (forte) |
| enjin | brasa (2: 0,50) · labareda (3: 0,45 0,45) · incêndio (4: 0,45 0,45 0,80) · **erupção** (forte) · fagulhas (3: 0,40 0,40) · chama viva (1) correndo · cinzas (2: 0,45) · fogo alto (3: 0,45 0,45) saltando |
| suiren | onda (1) · arpão (1) de longe · linha d'água (2: 0,60) de longe · maré longa (3: 0,60 0,50) de longe · maré baixa (2: 0,45) · arrebentação (3: 0,45 0,45) · maremoto (4: 0,55 0,50 0,45) · espuma (1) correndo · salto da baleia (2: 0,60) saltando · **vagalhão** (forte) |
| arashi | faísca (1) · duas tempestades (1) ⚔ · trovoada (2: 0,45) ⚔ no 2º · tormenta (3: 0,40 0,40) ⚔ no 3º · granizo (4: 0,40 0,40 0,40) · ventania (3: 0,40 0,70) · trovão (1) correndo ⚔ · raio duplo (2: 0,40) saltando ⚔ nos dois · céu partido (5: 0,40 0,40 0,40 0,80) ⚔ no 5º · **relâmpago** (forte) ⚔ |
| yoru | sombra (1) · presas (2: 0,45) · lua nova (3: 0,45 0,80) · **eclipse** (forte) · vultos (3: 0,40 0,70) · breu (1) correndo · coruja (2: 0,45) saltando · meia-noite (4: 0,40 0,40 0,90) · nevoeiro (2: 0,70) |
| jinshi | crescente (1) · minguante (3: 0,60 0,60) · fases da lua (4: 0,50 0,50 0,90) · luar (3: 0,45 1,00) · lua cheia (5: 0,50 0,50 0,50 0,90) · lua branca (6: 0,40 0,90 0,45 0,45 1,00) · noite branca (4: 1,00 0,40 0,40) · reflexo no lago (1) correndo · lua alta (2: 0,70) saltando · **halo** (forte) |
| oboro | **postura de hanzo:** lição completa (7: 0,60 0,50 0,50 0,70 0,45 0,45), sempre a primeira · corte do mestre (1) · lição (2: 0,60) · estocada de hanzo (1) · três lições (3: 0,50 0,60) · passo de hanzo (1) correndo · salto do mestre (2: 0,55) saltando · **devorador de posturas** e **postura do oni:** os doze ecos, cada um igual ao golpe do aprendiz: eco da terra (desabamento) · da tartaruga (mordida) · da montanha (fenda dupla) · do gelo (nevasca) · do tigre (fúria do tigre) · do corvo (revoada ⚔) · do vento (foices gêmeas ⚔) · da chama (incêndio) · do mar (maré longa, de longe) · da tempestade (tormenta ⚔) · da noite (meia-noite) · da lua (lua cheia) |

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
  `PERSONAGENS.md` (inclusive o alcance novo de cada um).
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
| `src/entrada.c`, `entrada.h` | O instante do clique no relógio do núcleo: converte o carimbo do sistema para o passo do duelo (câmera lenta, hitstop) e cai no meio do quadro se ele não for confiável. Sem raylib. |
| `src/entrada_plat.h`, `entrada_linux.c`, `entrada_mac.m`, `entrada_stub.c`, `entrada_fila.h` | De onde vem o carimbo: X11 (eventos brutos do XInput2 numa thread), macOS (monitor local de `NSEvent`), o resto sem carimbo. O Makefile escolhe o arquivo. |
| `src/desempenho.c`, `desempenho.h` | A medida de quadros do `APARA_PERF`: guarda o tempo de cada seção do laço (lógica, mundo, interface, composição, swap) e resume em média, mediana, p95, p99 e máximo. Sem raylib. |
| `src/salvar.c`, `salvar.h` | O arquivo de progresso, puro: formato, leitura estrita (máscara, vencidos e lore vistos), `.bak` do save corrompido, gravação atômica (`.tmp` + `fsync` + `rename`). Sem raylib. |
| `src/fonte.c`, `fonte.h` | Quais caracteres pedir à fonte: os ASCII e todo caractere não ASCII de qualquer string do jogo. Sem raylib. |
| `src/vozes.h` | Quantas vozes cada som precisa para uma cauda não cortar a outra (o PERFECT tem 8). |
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
| `tests/core_test.c` | Verificações do núcleo (`make test`): regras, janelas viáveis em todo golpe, aviso, aperto cedo, ritmo com hitstop, calibração, curva, Oboro, taxa de quadros, vitória idempotente, vozes |
| `tests/robos.c` | A tabela dos robôs por mestre (`make robos`) e a mesma luta em cinco taxas (`make robos-taxas`) |
| `tests/fuzz.c` | Fuzz do núcleo: cenários sorteados, com invariantes (`make fuzz N=3000`) |
| `tests/entrada_test.c` | O carimbo (`make test-entrada`): a conversão, todo carimbo inválido no meio do quadro, um fuzz de 1.000.000 entradas e lutas dos robôs com carimbo exato = ms exato e com carimbo inválido = o de antes |
| `tests/teste_carimbo.sh`, `xclique.c` | O jogo de verdade sob xvfb: cliques e Espaço pelo XTest em instantes medidos; o carimbo chega a menos de 1 ms e o duelo o aplica onde ele manda |
| `tests/desempenho_test.c`, `teste_desempenho.sh` | Os percentis com números que se conferem de cabeça, o aquecimento, a capacidade cheia; e o jogo de verdade sob xvfb medindo 4 s e saindo sozinho |
| `tests/save_test.c`, `fonte_test.c` | O save (formato, saves corrompidos, gravação atômica) e os glifos da fonte |
| `tests/teste_save.sh`, `teste_rastro.sh`, `teste_vitoria.sh`, `xtecla.c` | O jogo de verdade sob xvfb (`make teste-jogo`): nada grava fora do jogo normal, o rastro é só visual, a vitória é salva no golpe final e sobrevive a kill, ESC+Q e fechar a janela |
| `tests/numeros.sh`, `avisos.sh` | Nenhum número mágico nas regras; nenhum aviso do gcc e do clang em -O1, -O2 e -O3 |
| `tools/medir_desempenho.sh` | A tabela de desempenho dos 13 mestres com o robô do demo (`APARA_PERF`); com `RASTRO=1`, o oboro na 3ª fase e o karasu com e sem o rastro fantasma |
| `tools/instalar_packs.sh` | Põe o zip das animações nas pastas do gerador e do jogo (`make packs ZIP=...`) |
| `tools/personagens.c` | Gera os 15 lutadores (cabeça, arma, corpo, rastro, aura, golpe especial, pose desarmada), uma pasta por nome (kojiro, raizo, yoru, garfiel...), a partir do Samurai #3 e dos packs de `assets/sprites/_packs/` (Raizo com o espadão, Shizuku, Suiren e Jinshi no Samurai #4, Arashi, Oboro com as posturas dos outros e o grito): `make sprites`; ver `PERSONAGENS.md` |

Como rodar, os atalhos de teste e os robôs estão em `../c_game/LEIA-ME.md`.
