# Roteiro completo — APARA: A Trilha dos Sete Mestres

## Direção aprovada

A base é **2D de visão lateral**, com pixel art e iluminação de neon
inspiradas na referência de KATANA ZERO. O protagonista, **Ren**, usa roupa
de shinobi inspirada na ideia de Sekiro, redesenhada: cabelo preto curto,
casaco assimétrico laranja, mangas de linho claro, faixa carvão, calça escura
e pernas enfaixadas. O laranja identifica o jogador à primeira olhada.

O núcleo é **um botão, ritmo e impacto**. Não há seleção de direção, mira ou
botão de ataque. Clique esquerdo ativa o gesto de parry e seu som.

## Premissa narrativa

O mestre de Ren foi traído e desarmado pela *Liga dos Mestres Dissidentes*,
uma coalizão que distorceu os caminhos marciais para controlar os setores da
sociedade: o submundo noturno, os latifúndios, as finanças, as artes eruditas
e as arenas nobres. Cada um roubou uma das sete gemas da empunhadura sagrada.
Para restaurar o legado do mestre e impedir o desfecho orquestrado pelo
Mestre Supremo (Boss Final), Ren desafia cada mestre em seu próprio terreno,
quebrando sua postura e provando a pureza da técnica sobre os truques de cada
ambiente.

## A trilha dos sete mestres

| Mestre | Cenário | Perfeito / Bom | Preparações (s) | Fintas | Mecânica |
|---|---|---:|---|---:|---|
| 1. Gorou, Mestre do Dojo Tradicional | Dojo de tatame e bambu | 90 / 220 ms | 1,10 · 0,97 · 1,03 | 0% | Ataques rítmicos e diretos. Tutorial prático. |
| 2. Neon Jax, Campeão da Balada | Rave underground | 80 / 200 ms | 0,90 · 0,80 · 1,10 | 20% | Ataque sincopado: ameaça e atrasa a descida. |
| 3. Cavan, Titã do Campo | Celeiro ao entardecer | 75 / 190 ms | 1,20 · 1,00 · 1,33 | 30% | Finta pesada: partida lenta, golpe atrasado. |
| 4. Vance, Executivo S.A. | Cobertura corporativa | 70 / 180 ms | 0,83 · 1,13 · 0,87 | 40% | Finta dupla: a lâmina falsa força o parry antecipado. |
| 5. Kaelen, Pintor Visionário | Galeria surrealista | 65 / 170 ms | 0,77 · 0,97 · 0,73 | 50% | Cortes cegantes: a silhueta da lâmina se distorce. |
| 6. Eleonor, Grã-Duquesa Esgrimista | Salão de baile com espelhos | 60 / 160 ms | 0,70 · 0,83 · 0,63 | 60% | Finta tripla e estocadas rápidas. |
| 7. Sombra, o Reflexo de Ren | Jardim de Vidro | 50 / 140 ms | 0,60 · 0,73 · 0,57 | 75% | Mímica: finge o próprio parry e ataca no intervalo. |
| 8. Mestre Supremo (provisório) | Salão da Liga | Alta 70 / 180 ms · Baixa 45 / 130 ms | Alta 1,05 · 0,95 · 1,15 · Baixa 0,60 · 0,55 · 0,65 | Alta 30% (dupla) · Baixa 50% (mímica) | Ataques compostos, troca de postura e fases múltiplas. |

As preparações vêm da especificação em quadros a 60 FPS, convertidas para
segundos. Os sete mestres estão em `Assets/Scripts/Core/BossRoster.cs`, na ordem da trilha.

## Experiência de jogo

1. Antes de cada mestre, duas falas curtas (mestre e Ren) aparecem no painel
   inferior. Cada clique avança uma fala; o último abre o duelo.
2. O mestre assume uma preparação visível, de duração variável e previsível
   pelo movimento do corpo e da katana.
3. Um som curto e um clarão âmbar acompanham os últimos 180 ms. O sinal fica
   claro nos últimos 70 ms. O jogador clica perto do contato.
4. A katana responde no clique. O contato resolve um dos três timings.
5. Um perfeito devolve força ao mestre; quatro perfeitos quebram sua postura.
6. Vencer mostra a fala de derrota do mestre e passa ao próximo. Perder mostra
   o placar e repete o mesmo mestre. Depois da Sombra, a tela final avisa que
   o Boss Final será definido na próxima fase e reinicia a trilha.

## Fintas: um estilo por mestre

Uma finta é sorteada a cada preparação com a chance do mestre. Quando ocorre,
a preparação, a partida e o sinal acontecem em um ou mais **instantes falsos**;
o contato real chega depois, com nova partida e sinal no tom normal. O clique
num instante falso é aceito como a única tentativa do golpe e, como o contato
real está a mais de uma janela boa de distância, o resultado é ruim: "Caiu na
finta". Esperar o contato real permite bom ou perfeito normalmente.

| Mestre | Estilo (roteiro) | Como o código faz |
|---|---|---|
| Gorou | Sem fintas | `FeintChance` 0, `FalseCues` 0 |
| Neon Jax | Ataque sincopado | Um instante falso; a preparação sai do compasso em até ±120 ms (`RhythmJitter`) |
| Cavan | Finta pesada | Um instante falso; atraso longo, 350 a 550 ms |
| Vance | Finta dupla | Dois instantes falsos (`FalseCues` 2) espalhados em 400 a 550 ms |
| Kaelen | Cortes cegantes | Um instante falso; sinal com 35% da força (`CueVisibility`): clarão e som mais fracos, losango translúcido |
| Eleonor | Finta tripla e estocadas rápidas | Três instantes falsos em 540 a 720 ms; preparações curtas |
| Sombra | Mímica e cancelamento | A partida falsa mostra o gesto de parry (`MimicParry`); o atraso, 300 a 360 ms, cai no intervalo de cooldown do jogador |

Regras que os testes garantem para todos: os instantes falsos ficam pelo
menos um sinal inteiro (180 ms) afastados entre si e do contato real; o atraso
mínimo é maior que a janela boa; a interface conta o tempo até o **próximo
instante mostrado**, falso ou real, para não entregar a finta. O sorteio usa
um gerador com semente, então a mesma semente reproduz a mesma sequência nos
dois motores.

## Cenários: um por mestre

Cada mestre tem um cenário próprio. No Unity, `ArenaAsset` nomeia a imagem em
`Assets/Resources/Art/Arenas/` (`arena_dojo`, `arena_balada`, `arena_campo`,
`arena_escritorio`, `arena_galeria`, `arena_salao`, `arena_jardim`); enquanto
ela não existe, a arena comum recebe a tonalidade do mestre. No RPG Maker,
cada mestre é um mapa, e o plugin aplica uma tonalidade de tela provisória.
A arte pixel art de cada cenário fica para depois; `docs/ARTE.md` lista os
nomes esperados.

## Boss Final (provisório)

O roteiro deixa o Boss Final "a definir" e nomeia três mecânicas. O código
implementa as três num mestre provisório, o **Mestre Supremo**; nome, cenário
e falas são espaço reservado (`Provisional = true`) até o roteiro decidir.

- **Troca de postura.** Duas posturas com janelas, ritmo e finta próprios:
  *Alta*, lenta e telegrafada, com finta dupla; *Baixa*, rápida, com a mímica
  da Sombra. A troca acontece a cada N golpes, N definido pela fase. O HUD
  mostra a postura ao lado do título e avisa "POSTURA ALTA/BAIXA".
- **Fases múltiplas.** Três fases por vida: 100% a 66%, 66% a 34%, abaixo de
  34%. Cada fase define a cadência da troca (4, 3, 2 golpes), os golpes
  compostos e a velocidade (a última é 10% mais rápida). A fase só muda na
  preparação seguinte e nunca volta. Mestres com fases próprias não usam a
  regra comum da segunda fase.
- **Ataques compostos.** A partir da segunda fase, uma preparação pode abrir
  dois contatos seguidos; na terceira, três. Cada contato tem sua tentativa
  e seu julgamento; entre eles há só uma pausa curta (100 ms) e uma
  preparação curta (500 ms), sem finta. Quebrar a postura interrompe o
  composto e abre a recuperação longa.

Tudo isso está em `BossProfile.Stances`, `BossProfile.Phases`, `ComboWindup`
e `ComboGap`; `CoreSelfTest` cobre fases, posturas e compostos.

## Timings e consequências

O tempo é medido entre o clique aceito e o contato real da lâmina.

| Tipo | Janela | Ren | Mestre | Resposta audiovisual |
|---|---|---|---|---|
| Ruim | Antes da janela boa, ou sem defesa no contato | −25 vida | Sem dano | Pancada grave, reação de dano, partículas vermelhas, tremor, flash vermelho |
| Bom | Entre a janela perfeita e a boa do mestre | Sem dano | Sem dano | Som metálico curto, faíscas discretas, flash amarelo leve, pausa de 45 ms |
| Perfeito | Até a janela perfeita do mestre | Sem dano | −8 vida e −25 postura | Clang com cauda aguda, faíscas claras, anel breve, flash branco, pausa de 90 ms |

Os limites são inclusivos. O input é lido no evento, e o mundo avança até esse
instante antes de avaliar a tentativa. Cliques depois do contato não mudam o
resultado anterior.

## Vida, postura e vitória

| Parâmetro | Inicial |
|---|---:|
| Vida de Ren | 100 |
| Dano por timing ruim | 25 |
| Vida do mestre | 100 |
| Postura do mestre | 100 |
| Dano de vida por perfeito | 8 |
| Redução de postura por perfeito | 25 |
| Dano extra ao quebrar postura | 30 |

Quatro perfeitos causam 32 + 30 = 62 de dano e quebram a postura: o mestre
congela por 160 ms, recupera-se em 1,5 s e prepara os golpes 10% mais rápido
pelo resto do duelo. Mais quatro perfeitos vencem. Quatro erros derrotam Ren.

## Evitar spam e julgamentos confusos

- Uma tentativa por golpe; vale o primeiro clique aceito.
- Intervalo mínimo entre gestos: 320 ms. Clicar fora da preparação só mostra o gesto.
- Segurar o botão e repetição automática de tecla não geram novas tentativas.
- Cada ataque resolve dano uma vez, antes de emitir os eventos visuais.
- Pausa, perda de foco e hitstop congelam o relógio do combate e as animações.
- Um travamento superior a 200 ms pausa o jogo.
- O clique que fecha uma fala ou painel é consumido pela interface e há 200 ms
  de proteção antes de retomar o relógio.

## Animação e impacto

O herói possui `idle`, `parry`, `attack`, `hurt` e `death`. O boss possui
`idle`, `windup`, `attack`, `hurt` e `death`. As poses estão nas pranchas PNG
e os recortes em `Assets/Resources/Art/frames.json`.

Os sete mestres compartilham a prancha do boss, diferenciados por tonalidade
do sprite e do cenário (`Tint` e `ArenaTint`). A **Sombra** usa a prancha do
herói espelhada e escurecida: os dois primeiros frames do corte fazem a
preparação e o contato continua no quarto frame.

Na finta, a sequência de ataque para um frame antes do contato e espera. No
contato bloqueado, a defesa é levada ao frame de choque; o mundo para
brevemente e o som continua. O flash de tela é desenhado pelo HUD e some em
até 300 ms. O tremor máximo é de poucos pixels e pode ser desligado com F.

## Organização para codar (Unity 6)

| Arquivo | Responsabilidade |
|---|---|
| `Assets/Scripts/Core/CombatSettings.cs` | Parâmetros comuns: vida, dano, ritmo, atraso da finta, pausas |
| `Assets/Scripts/Core/BossProfile.cs` + `BossRoster.cs` | Janelas, ritmo, fintas, visual e falas de cada mestre |
| `Assets/Scripts/Core/Campaign.cs` | Posição na trilha |
| `Assets/Scripts/Core/CombatCore.cs` | Relógio, tentativa, finta, classificação, vida, postura e vitória |
| `Assets/Scripts/Core/SpriteSheetData.cs` + `MiniJson.cs` | Recortes e âncoras de `frames.json` |
| `Assets/Scripts/Core/CoreSelfTest.cs` | Regras, mestres, fintas, trilha e prancha; roda no Test Runner e fora do Unity |
| `Assets/Scripts/AparaBootstrap.cs` | Monta o jogo em qualquer cena ao entrar em Play |
| `Assets/Scripts/AparaCombatBridge.cs` | Input, telas, falas, eventos, pausa e ligação com as animações |
| `Assets/Scripts/View/ActorView.cs` | Reprodução, pivô no pé e espelho dos frames |
| `Assets/Scripts/View/FeedbackView.cs` | Sons sintetizados, faíscas e anel de impacto |
| `Assets/Scripts/View/HudView.cs` | Canvas 640 × 360: barras, falas, painéis, flash e resultado |
| `Assets/Scripts/Editor/ArtImportSettings.cs` | Importação em pixel art e menu de testes |
| `Assets/Resources/Art/` | Pranchas, arena e `frames.json` |

O núcleo (`Apara.Core`) não referencia o UnityEngine. A câmera ortográfica
cobre 640 × 360 unidades com um pixel por unidade; as coordenadas do desenho
original (Y para baixo) são convertidas na ponte.

## Passo a passo para desenvolvimento

1. Jogar a trilha inteira e sentir as janelas de cada mestre.
2. Ajustar as janelas em `BossRoster.cs` em passos de 10 ms.
3. Acertar o atraso da finta: curto demais vira golpe normal, longo demais fica óbvio.
4. Refinar os recortes e âncoras se a silhueta deslizar.
5. Criar arte própria por mestre e por cenário; hoje só a cor muda.
6. Redefinir o Boss Final no roteiro: nome, cenário, falas e o ajuste das posturas e fases provisórias.

## Validação

Em 11/09/2026 o núcleo C# compilou com o csc.exe do .NET Framework e passou
474 verificações sem falhas (`unity_project/Tools/RunCoreTests.ps1`); o
núcleo JavaScript do plugin passou 275 em Node 24
(`node rpgmaker_mz/tests/core_test.js`).
**Nem o Unity nem o RPG Maker estavam instalados nesta máquina**: as camadas de
cena, HUD, áudio e input dos dois ainda não foram executadas. O primeiro Play precisa conferir:

- Console sem erros de compilação; pranchas importadas com alfa e sem filtro.
- Pés dos dois na linha do chão; a Sombra escura e espelhada.
- Ler as falas de Gorou e Ren; o clique final abre o duelo.
- Perder quatro golpes: vida zerada, queda, placar e repetição do mestre.
- Acertar oito perfeitos: fala de derrota do mestre e próximo mestre.
- Contra Vance ou depois: clicar no sinal agudo e ver "Caiu na finta".
- Terminar a trilha: tela final e reinício em Gorou.
- Som, sensação das janelas e legibilidade da finta: só jogando.

## Briefing para outra pessoa ou assistente continuar

Continue este projeto Unity 2D de duelo com katana. Preserve Ren de cabelo
curto e roupa predominantemente laranja. Preserve o parry de um botão, os
três julgamentos de timing, a trilha de sete mestres com janelas decrescentes
e fintas crescentes, as falas antes e depois de cada duelo. Use os recursos
existentes; mantenha som e animação ligados ao contato. Não reintroduza
seleção direcional ou um segundo botão obrigatório de ataque. Toda alteração
de regra deve ser refletida neste roteiro, em `BossRoster.cs` ou
`CombatSettings.cs` e em `CoreSelfTest.cs`.
