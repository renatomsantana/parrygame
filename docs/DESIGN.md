# Direção de arte — APARA

## Ideia central

Um duelo curto, visto **de lado**, em que o jogador lê a preparação, escolhe a
altura da guarda e responde com um parry preciso. As duas silhuetas, a espada e
o sinal do golpe devem ser fáceis de distinguir do fundo. Cenários e materiais
reforçam o impacto do aço.

**O parry não tem direção: um botão só.** Espaço ou clique esquerdo. Casar a
altura era uma segunda decisão dentro de um instante que dura 240 ms, e ela
chegava sempre tarde. Sobrou a decisão que interessa — **quando**.

### Mas os três golpes ficaram

As três alturas não viraram enfeite: elas viraram **relógio**. Cada uma tem
seu tempo em `ATTACK_TEMPO`, porque um corte alto é um movimento grande e
lento e um corte baixo é curto e chega antes:

| Golpe | Tempo | O que significa |
| --- | --- | --- |
| Alto | ×1,28 | Telegrafado. Dá tempo de ver e decidir |
| Médio | ×1,0 | O ritmo de referência |
| Baixo | ×0,76 | Chega antes do que a mão espera |

Ler a altura continua valendo a pena, só que para saber **quanto tempo se
tem** em vez de para onde apontar. A interface diz as duas coisas juntas:
"GOLPE: ALTO · LENTO".

A altura ainda manda na animação: a lâmina do jogador sobe para aparar um
corte alto e desce para um baixo. Quem escolhe não é o jogador, é o golpe que
veio — sem isso o aparo de um corte alto sairia com a espada na cintura.

### A finta

Um jogo de um botão só, sem armadilha, vira apertar no clarão. A finta é a
armadilha: **um clarão falso**, igual ao verdadeiro, no instante em que o
verdadeiro acenderia — e o golpe não vem. Quem apara por reflexo entra na
recuperação e come o golpe real, que chega logo depois.

Três decisões que a mantêm justa:

1. **O padrão de fintas é fixo** (`FEINT_PATTERN`), não sorteado. O padrão é
   para ser aprendido; um dado escondido transforma leitura em azar.
2. **O corpo entrega.** Depois do clarão falso o oponente recolhe em vez de
   comprometer — é esse puxão para trás que dá para aprender, porque o
   clarão em si é idêntico. Sem a camada de pose do corpo a finta seria
   ilegível, e é por isso que ela veio antes.
3. **`feint_hold` tem de ser maior que a recuperação do parry.** Menor, e
   quem cai na finta não tem defesa nenhuma: vira punição garantida em vez
   de armadilha. Há um teste automático prendendo esse limite.

O som também conta: a finta usa o mesmo começo do aviso e é **cortada no
meio**. O som que não termina diz "aquilo não valeu" sem precisar de texto —
e não há texto: anunciar "FINTA" na tela entregava de graça justamente o que
a finta cobra para ensinar.

### Firmeza

Uma janela que fecha sem aparar nada custa um ponto de **firmeza**. Zerada,
ela quebra: custa um ponto de vida e volta cheia. São três por padrão
(`steadiness` em `combat.tres`).

É o que impede o botão de virar metralhadora. Sem preço para errar, a finta
não cobra nada, a leitura vira enfeite e o duelo inteiro se resolve apertando
sem parar. Com preço, apertar por reflexo passa a ser uma decisão.

Ela aparece como pontos cheios e vazios ao lado da vida, não como número: é
olhada de canto de olho no meio do duelo, e uma contagem exige leitura.

### O que a tela não diz

Não há aviso de finta nem de parry perfeito. O perfeito se reconhece pelo
**som** — é outro clangor —, pelo tranco mais forte e pelas faíscas. Sentir
vale mais do que ler, e ler tira os olhos do oponente bem no momento em que
ele volta a atacar. A tela guarda texto para o que o jogador ainda precisa
decidir: que o contra-ataque está aberto, e que a firmeza quebrou.

As duas bases são fantasia estilizada; não são reconstruções históricas.
Samurai é a direção inicial de trabalho. Medieval permite comparar o clima
e desenvolver outra aparência usando as mesmas regras.

## Comparação dos temas

| Elemento | Samurai — Pátio do Bordo | Medieval — Pátio da Coroa |
| --- | --- | --- |
| Arma | Katana de lâmina levemente curva e guarda circular | Espada longa, guarda em cruz e pomo arredondado |
| Oponente | Ronin com placas sobrepostas, capacete e máscara | Cavaleiro com elmo fechado, ombreiras e tabardo |
| Silhueta | Capacete largo, ombros segmentados, saia de armadura | Ombros arredondados, peito de aço, tabardo vertical |
| Cenário | Pátio de pedra, portal vermelho, bordo e lanternas | Pátio de pedra, muralhas, torres, portão e estandartes |
| Materiais | Laca, tecido, madeira escura, aço e latão | Aço, couro, pedra, tecido azul e latão |
| Luz | Noite roxa, lanternas em ciano | Noite azul, tochas em âmbar |
| Par de cores | Ronin magenta, duelista ciano | Cavaleiro ciano, duelista âmbar |
| Neon do cenário | Ciano nas lanternas | Âmbar nas tochas |
| Movimento a criar | Preparações legíveis e cortes secos | Preparações com peso e recuperação evidente |

## Paleta de referência

A direção é **neo-noir**: quase tudo escuro, uma cor forte no oponente e a
complementar no neon do cenário. Não é decoração — é leitura. Contra um fundo
quase preto, a silhueta do oponente e o clarão do golpe aparecem sozinhos.

| Uso | Samurai | Medieval |
| --- | --- | --- |
| Massa do corpo do oponente | `#160917` | `#101227` |
| Placa na cor do tema | `#4C091D` | `#0A2C40` (tabardo) |
| Placa iluminada (chapéu, ombreiras) | `#730D2C` | `#5C6E8F` |
| Fio aceso das bordas | `#FF2973` | `#3DD1FF` |
| Tecido escuro (hakama / couro) | `#100B19` | `#100E14` |
| Pedra do chão | `#15121C` | `#0F131D` |
| Detalhes de metal | `#7A4F17` | `#855418` |
| Fresta dos olhos | `#FF73AD` | `#9EF2FF` |
| Massa do corpo do jogador | `#0A1413` | `#150D08` |
| Cor do jogador | `#254A45` | `#4A2C0F` |
| Fio aceso do jogador | `#A8E5CF` | `#FFB852` |
| Neon do cenário | `#2EE6FF` (lanternas) | `#FF9A2E` (tochas) |
| Guarda do jogador / parry | `#A9E5CF` | `#A4D5ED` |
| Sinal de ataque / parry perfeito | `#F2BB66` | `#F2BB66` |
| Dano | `#EF7670` | `#EF7670` |
| Painel da interface | `#201E1D` | `#1E2733` |

A armadura **não** é da cor do tema por inteiro. Ela é escura, e a cor do tema
aparece em três lugares: o alto da silhueta, um fio aceso nas bordas e o sinal
do golpe. Foi pintar o corpo todo de magenta que produziu o boneco de plástico.

**Os dois lutadores nunca compartilham família de cor.** Com os dois no quadro
ao mesmo tempo, um corpo escuro roxo ao lado de outro corpo escuro roxo vira
uma mancha só. Cada tema tem duas famílias opostas — magenta contra ciano no
samurai, ciano contra âmbar no medieval — e isso vale também para os refletores
presos a cada modelo: dar um contorno magenta ao jogador o devolve para o time
do oponente.

Três regras que sustentam isso:

1. **A cor vem da superfície, não da luz.** O sol é quase branco de propósito.
   Um sol rosado pinta a katana de rosa junto com a armadura, e aí nada se
   destaca de nada.
2. **A luz ambiente é baixa** (0,32). É ela que abre a sombra e cria silhueta.
   Ambiente alto é exatamente o que dá aparência de brinquedo plástico.
3. **A névoa engole o fundo** entre 4 e 17 unidades, para o cenário virar
   recorte escuro e o duelo ficar sozinho no primeiro plano.
4. **O chão fica mais escuro que o oponente.** Quando os dois têm o mesmo
   valor, o personagem gruda no piso e some — foi o que aconteceu com o
   cavaleiro azul sobre pedra azul.

## A câmera lateral

O duelo corre no eixo **X**, que é o eixo da tela. A câmera é **ortográfica** e
olha na horizontal ao longo de -Z, sem inclinação nenhuma. É essa combinação
que produz a leitura 2D: sem perspectiva, toda superfície deitada projeta uma
linha, e o chão vira uma quina reta em vez de um plano fugindo para o fundo.

| Ajuste | Onde | Valor |
| --- | --- | --- |
| Meia altura visível | `CAMERA_SIZE` em `main.gd` | 5,2 unidades |
| Altura do olho | `CAMERA_HEIGHT` | 1,55 |
| Distância do palco | `Camera3D.position.z` em `main.tscn` | 9 |
| Aproximação no parry | `CAMERA_PUNCH` | 0,22 |

A câmera acompanha o **meio** dos dois lutadores dentro de uma faixa
(`CAMERA_LIMIT`). Sem isso, um empurrão forte tira alguém do quadro.

Como a câmera fica a 9 unidades do duelo, a névoa precisa **começar depois
disso** (10,5 a 26). Herdar a névoa curta da versão em primeira pessoa engolia
os próprios lutadores.

O chão é um bloco cuja **face da frente** é o que se vê; a superfície de cima
some na linha do horizonte do palco, marcada por um fio aceso na quina. O fundo
é montado em três camadas de placas chatas, em z = -4, -7 e -14, com o valor
caindo a cada camada. Tudo fica atrás dos lutadores: uma peça entre eles e a
câmera tapa o duelo.

### Três quartos, não perfil puro

O corpo de cada lutador encara o adversário — é disso que a lógica do golpe
depende. O **volume** gira meio radiano para a câmera, num `VisualMount` que a
lógica não toca, e o sinal desse giro segue quem está de que lado do palco.
De perfil puro somem peito, ombreiras e olhos; some o personagem.

Pela mesma razão o sinal do golpe é um **anel** em volta do peito, e não uma
placa frontal: de lado, a placa ficaria de perfil e sumiria — e é dela que o
duelo inteiro depende.

## A pose diz a mesma coisa que a lâmina

Não há rig nem animação esquelética: as pernas são caixas fixas. O que
existe é uma camada de pose em `scripts/visuals/body_pose.gd`, que move
quatro coisas — inclinação do corpo, giro do tronco, sobe-e-desce e
inclinação da cabeça. É pouco, e resolve a maior parte.

O que ela compra:

**Antecipação e acompanhamento.** Na preparação o oponente se fecha para
trás; ao resolver o golpe, despenca para a frente. Sem isso a espada trocava
de posição sem o corpo participar, e o golpe não tinha peso nenhum.

**Um segundo aviso da altura.** O corpo repete o que o braço diz: sobe no
golpe alto, agacha no baixo (`WINDUP_BOB` em `enemy_brain.gd`, `GUARD_LEAN` e
`GUARD_BOB` em `player_controller.gd`). A 320x180 a espada é um risco fino e
some no fundo; a silhueta inteira não some. Vale para os dois lados — dá para
conferir a própria guarda pela postura sem tirar os olhos do oponente.

**Que o corpo não pareça congelado.** Um sobe-e-desce de respiração parado e
um de passo andando.

O solavanco do impacto é separado dos alvos de estado, porque um golpe
recebido não é um estado: é um empurrão em cima do que o corpo já fazia.
`punch()` soma e decai sozinho.

A cabeça devolve parte da inclinação do tronco (`HEAD_COUNTER`). É o truque
que mantém o rosto legível enquanto o corpo se joga para a frente.

As rotações que o arquivo da cena traz são guardadas como repouso e somadas,
nunca sobrescritas: a postura de guarda desenhada na arte sobrevive.

## Som em camadas

O ouvido lê um impacto em quatro partes, e cada uma diz uma coisa: o **ataque**
nos primeiros milissegundos diz que bateu, o **corpo** diz de que material, o
**peso** grave diz o tamanho e a **cauda** diz o valor da coisa. Faltando
qualquer uma delas o golpe soa barato, mesmo no volume certo. Todos os sons
ficam em `scripts/feedback.gd`, sintetizados — o projeto não usa arquivos de
áudio de terceiros.

Três decisões que sustentam isso:

**O parry perfeito ganha um brilho atrasado, não mais volume.** Um segundo
evento 45 ms depois do primeiro lê como recompensa; o mesmo som mais alto lê só
como mais alto.

**Bater na armadura não tem cauda nenhuma.** O som morre onde bate, e é essa
ausência que diz "não adiantou" antes de qualquer texto na tela.

**A mistura entre os sons é do áudio, não da configuração.** `MIX_DB` fixa a
posição de cada um: o aviso fica abaixo do impacto, o perfeito acima do comum.
`sound_volume_db` em `combat.tres` é só o volume geral — mexer nele não deve
reordenar o que é importante.

Um arquivo em `art/audio/<nome>.mp3` ou `.ogg` substitui o sintetizado do
mesmo nome enquanto estiver na pasta, e some da equação quando sai. É o jeito
de comparar a síntese com uma referência sem tocar em código — e a razão de
nada do que o jogo gera depender de arquivo de terceiros.

A taxa é 44,1 kHz porque metal vive acima de 8 kHz. A 22 kHz o clangor perde o
ar e sobra um toque de telefone. O ruído passa por um filtro de um polo antes
de entrar nas camadas: ruído branco puro soa a chiado de rádio; filtrado, soa a
material. E a soma das camadas passa por saturação suave em vez de corte seco,
senão o pico estala.

## O neon como camada

A referência declarada do projeto é *Katana Zero*. O criador dele conta em
entrevista que o efeito de luz neon "cobria todos os defeitos", e que é ele
que faz arte de dezenas de artistas diferentes parecer uma coisa só. Vale a
pena levar isso a sério: o brilho não é enfeite de acabamento, é o que costura
a imagem.

Por isso o neon é a **última** coisa do shader e a única que escapa da paleta.
Ele é somado por cima do resultado já quantizado, como uma camada de luz sobre
arte chapada — que é como um jogo 2D monta a cena. Enquanto ele era calculado
antes da paleta, a quantização o engolia: o halo caía na mesma entrada da lista
que a superfície ao lado e simplesmente desaparecia.

| Ajuste | Efeito | Valor |
| --- | --- | --- |
| `bloom_threshold` | Acima de quanto uma cor vira fonte de luz | 0,63 |
| `bloom_strength` | Quanto da luz é somado por cima | 1,15 |
| `bloom_radius` | Raio do halo, em pixels da arte | 3,2 |
| `bloom_saturation` | Impede o halo de crescer branco | 1,7 |

O limiar é o ajuste delicado. Baixo demais (0,42 foi tentado) e **toda**
superfície acende: a cena vira névoa colorida e as silhuetas somem. O certo é
que só as fontes acendam — os fios emissivos das bordas, o sinal do golpe, o
fio do chão, as lâmpadas e a lâmina —, com o resto chapado e escuro em volta.
É o contraste entre os dois que produz o efeito, não o brilho sozinho.

## Silhueta e luz do personagem

O mundo é desenhado a 320x180. Nessa escala o oponente ocupa cerca de 50 por 65
pixels: detalhe pequeno vira ruído e o que sobra é **silhueta e valor**. Daí as
três decisões que sustentam os dois modelos.

**Largo, estreito, largo.** O alto da silhueta é a peça mais larga — o chapéu
cônico do ronin, o manto de ombros do cavaleiro. A cintura estreita. A saia
abre de novo. Uma pilha de caixas da mesma largura, que era o desenho anterior,
lê como faixas horizontais empilhadas e não como pessoa.

**Formas facetadas em vez de caixas.** Troncos, chapéu, elmo e saias são
`CylinderMesh` de oito lados com raios diferentes em cima e embaixo, girados um
oitavo de volta para deixar uma face inteira virada para a câmera. A faceta é o
que faz a borda acender quando a luz raspa; a caixa devolve um tom chapado.

**A luz vai presa ao modelo.** Cada oponente carrega o próprio refletor, então
a iluminação não muda quando ele anda pela arena:

| Nó | Papel |
| --- | --- |
| `KeyLight` — SpotLight3D | Chave frontal alta, à esquerda. É ela que dá forma ao corpo; o sol da arena está baixo demais para isso |
| `RimWarm` — OmniLight3D | Contorno atrás do ombro esquerdo, na cor do tema |
| `RimCool` — OmniLight3D | Contorno atrás do ombro direito, na cor complementar |

Os dois contornos ficam com alcance curto (1,55) e acima de 1,75 de altura de
propósito: alcance maior derrama uma poça de luz no chão que rouba a cena.

A postura também é desenho. O tronco fica dentro de um nó `Body` girado cerca
de 0,2 rad e os pés são escalonados, um à frente e outro atrás. A cabeça e o
`AttackCue` ficam **fora** desse giro — a leitura do golpe não pode depender
da pose.

O jogador e o oponente usam a mesma linguagem de construção, mas nunca a mesma
silhueta: o oponente é alto (2,5), de chapéu largo ou elmo e placas; o jogador
é mais baixo (1,95), de cabeça descoberta e casaco. Num relance, quem é quem se
decide pela forma antes de pela cor.

As cores da interface ficam nos perfis em `config/themes/`. Os materiais 3D
ficam dentro das cenas de arte, com nomes como Armor, Cloth, Steel e Stone.
Alterar a paleta da interface não repinta automaticamente a armadura.

## Resolução e paleta

O mundo 3D não é desenhado no tamanho da janela. Ele passa por um SubViewport
renderizado em resolução baixa e ampliado sem suavização, e depois por um
shader que mapeia cada pixel para a **paleta fechada do tema**. É isso que dá o
acabamento pixel art sem redesenhar nada: as formas continuam sendo geometria,
o pixel e a cor aparecem na hora de mostrar.

A paleta vive no perfil do tema, em `config/themes/*.tres`, campo `palette`.
Samurai tem 32 cores, medieval 35. Trocar o tema troca a paleta junto com a
arena, o oponente e a arma.

O shader escolhe as duas cores mais próximas de cada pixel e decide entre elas
pela **posição da cor original no segmento entre as duas**, comparada com uma
matriz de Bayer 4x4. Assim uma superfície que já bate com uma cor da paleta
fica chapada, e só a transição entre duas cores ganha o pontilhado. Uma
fórmula que dithere pelo "empate" entre as candidatas enche a tela de ruído —
foi o primeiro resultado, e não é o que se quer.

| Ajuste | Onde | Efeito |
| --- | --- | --- |
| `palette` | `config/themes/*.tres` | Cores permitidas. Lista vazia volta ao modo de degraus livres |
| `shadow_tint` / `highlight_tint` | idem | Duotone: matiz da sombra e da luz. Normalizado, muda a cor sem mexer no brilho |
| `exposure`, `contrast`, `saturation` | Inspetor do nó raiz | Gradação antes da paleta |
| `bloom_strength`, `bloom_threshold` | idem | Vazamento do neon para os pixels vizinhos |
| `aberration` | idem | Separação de vermelho e azul a partir do centro |
| `scanline_strength`, `vignette_strength` | idem | Linhas de varredura e escurecimento das bordas |
| `pixel_shrink` | Inspetor do nó raiz de `main.tscn` | Divisor da resolução. 4 numa janela de 1280x720 desenha 320x180. 1 desliga |
| `color_levels` | idem | Degraus por canal, usado só quando não há paleta |
| `dither_strength` | idem | Força do pontilhado nas transições. 0 deixa faixas chapadas |

O indicador de guarda é desenhado em blocos do mesmo tamanho do pixel do
mundo, para a interface do duelo não destoar da cena. O **texto** da HUD fica
**fora** desse caminho, na resolução da janela. É escolha deliberada: o duelo
se decide em cerca de 160 ms e a leitura precisa caber nesse tempo. Para levar
a interface inteira para o pixel, o caminho é mudar o modo de esticamento do
projeto para `viewport` e reduzir os tamanhos de fonte em `hud.gd` na mesma
proporção.

Escolher a cor pelos materiais continua valendo: a paleta só aproxima o que a
arte já definiu. Duas cores próximas entre si colapsam na mesma entrada, então
contraste decidido na arte sobrevive melhor. Ao criar uma cor nova para uma
peça, vale acrescentá-la à paleta do tema, senão ela cai na vizinha.

## Como editar a arte agora

1. No painel de arquivos do Godot, abra `scenes/themes/samurai/arena.tscn`.
2. Selecione um objeto na árvore de cena. A aba 3D permite mover e escalar;
   o Inspetor permite editar Mesh e Material Override.
3. Salve com Ctrl+S. Abra `main.tscn` e pressione F5 para conferir no duelo.
4. Repita com `sentinel.tscn` para o ronin e `weapon.tscn` para a katana.
5. Para trabalhar no medieval, use os mesmos nomes na pasta `medieval` e
   selecione esse tema no menu do jogo.

Os materiais são compartilhados por peças da mesma cena. Se uma única peça
precisar de outra cor, torne seu material único antes de editá-lo.

## Trocar os modelos por sprites desenhados

A referência é arte 2D desenhada à mão, não geometria. O caminho existe e a
junta já está pronta: o tema aponta `opponent_scene` e `duelist_scene` para
cenas quaisquer, e o jogo só exige delas o contrato da tabela acima —
`WeaponPivot` com a arma dentro, `AttackCue` com material próprio no oponente
e, opcionalmente, `Arms`. Uma cena com um `AnimatedSprite3D` no lugar das
malhas satisfaz o mesmo contrato e entra sem tocar no combate.

Duas coisas a saber antes de começar:

1. **Assar os modelos atuais em sprites não muda nada na tela.** Sai a mesma
   imagem, só pré-renderizada. O ganho só aparece com quadros desenhados.
2. Um sprite desenhado não recebe os refletores presos ao modelo. A luz que
   hoje vem do rig teria de estar **pintada no próprio quadro** — é assim que
   a referência faz, e é por isso que lá a silhueta é quase preta com duas ou
   três cores. O neon do shader continua valendo por cima.

Referências úteis para esse trabalho estão reunidas em
`docs/REFERENCIAS.md`.

## Trocar as formas simples por modelos

Use `art/models/samurai/` e `art/models/medieval/` para os modelos que criarmos.
Um arquivo `.glb` pode ser importado e instanciado dentro de uma cena de arte.
Mantenha uma cena própria do projeto em volta do modelo para conservar as
conexões descritas abaixo.

**Lutadores:** origem nos pés, eixo Y para cima, frente para -Z. O oponente tem
cerca de 2,5 unidades de altura e o jogador 1,95. As duas cenas seguem o mesmo
contrato — `opponent_scene` e `duelist_scene` no perfil do tema. Cristas e ornamentos podem ultrapassar
o corpo. As colisões pertencem ao personagem em `main.tscn`, não à sua malha.

A raiz da cena do oponente precisa ser Node3D e manter:

| Nó | Função |
| --- | --- |
| `WeaponPivot` — Node3D | Ponto que o código gira para preparar e executar golpes |
| `WeaponPivot/WeaponVisual` | Instância da arma; acompanha o ponto de movimento |
| `AttackCue` — MeshInstance3D | Anel no peito cuja cor muda com o ataque; só o oponente precisa dele |
| `Arms` — Node3D | Opcional. Cada filho Node3D é um braço, virado para o punho a cada quadro |

AttackCue usa **Material Override do tipo StandardMaterial3D**. Se organizar
os nós em outra hierarquia, atualize `weapon_pivot_path`, `attack_cue_path` e
`arms_path` no perfil `.tres` correspondente. O jogo confere as duas primeiras
conexões ao trocar tema; `Arms` é opcional e, se faltar, os braços ficam parados.

Cada braço de `Arms` é montado apontando o próprio **-Z** para longe do ombro,
com a mão na ponta. `enemy_brain.gd` gira o braço para o punho e estica seu
`scale.z` até alcançá-lo, usando a meta `reach` do nó — o comprimento em repouso
entre o ombro e a mão. Sem esse esticamento a mão passa longe do cabo nas
preparações curtas e a pose se desmancha em pedaços soltos.

**Arma:** raiz Node3D com origem na guarda/parte superior do cabo, lâmina
apontando para +Y. O cabo fica abaixo da origem. A cena da arma é instanciada
**dentro** de cada modelo, sob o seu `WeaponPivot`, e é compartilhada pelos
dois lutadores do tema.

Para um modelo novo, duplique a cena de arte, substitua as malhas de protótipo,
preserve os pontos de conexão e aponte o campo de cena no perfil do tema
para a cópia nova. Assim podemos comparar o modelo novo com a base.

## Arena e leitura do combate

O palco é uma faixa no eixo X: chão de 46 unidades e duas paredes invisíveis em
X = ±13. O jogador inicia em X = -2,6; o oponente, em X = 4. Não há profundidade
jogável — o duelo é uma linha, e é isso que dá a leitura de jogo 2D.

Portais, árvores e torres ficam **atrás** dos lutadores, nunca entre eles e a
câmera. Se criar peças acessíveis que
devam bloquear passagem, crie também suas colisões. Os golpes atuais verificam
distância e arco, sem teste de oclusão: obstáculos entre os personagens exigem
uma checagem de linha de visão antes do dano. Evite esses obstáculos nesta base.

O indicador mostra alto, médio e baixo, empilhados como aparecem no duelo, e
fica **sobre o jogador** — no centro cairia em cima do oponente, bem onde está
a leitura. Ele não mostra guarda nenhuma do jogador, porque não há: mostra de
que altura o golpe vem. Durante a janela as três faixas acendem juntas, para
não sugerir que ainda há um lado a escolher. A pose do inimigo deve concordar
com ele: as três posições de preparação em `enemy_brain.gd` são alturas bem
separadas de propósito. O claro final da preparação avisa o
impacto; faíscas e tremor devem permanecer breves para não esconder o golpe seguinte.

## Arte a desenvolver na próxima etapa

A escala da katana dentro da câmera está resolvida: a arma ocupa o canto e
deixa o centro livre. Falta:

1. Criar modelo com rig e animações: repouso, preparação dos três lados,
   golpe, aparado, atordoado, recebendo contra-ataque e derrota. Os braços já
   acompanham a arma por código, mas tronco, quadril e pernas continuam parados.
2. Integrar o rig à lógica, sincronizando o instante de dano com a animação.
3. Levar o cenário medieval para o âmbar que a tabela promete. Hoje a arena é
   azul como o cavaleiro, e só o chão escuro separa os dois.
4. Refinar materiais, som e partículas depois de testar o tempo do parry.

Ainda não há rig, animação esquelética, tecido simulado nem vento. A silhueta,
o valor e a luz de personagem já foram vistos em execução — as capturas em
`docs/capturas/` são a referência do que está no lugar.
