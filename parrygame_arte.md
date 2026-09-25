# PARRYGAME: Documento de Arte

## A história em uma linha

Hanzo era o sensei de todos. Oboro, o mais forte dos alunos, se voltou contra ele e o derrotou. Os outros alunos se espalharam, cada um distorcendo o que aprendeu. Kojiro, o 13º aluno, vai enfrentar os 12 um por um pra provar que o estilo de Hanzo ainda vive.

> Observação: aqui o Oboro está contado como o 12º e último vilão. Se ele for um chefe separado dos 12, é só mover ele pra fora da lista e criar mais um aluno.

---

## Regras de combate que a arte precisa mostrar

- **Kojiro tem vida; os vilões, postura.** Errar tira vida de Kojiro; aparar quebra a postura do vilão. A postura quebrada abre a guarda e o próximo golpe mata.
- **Parry bom:** defendeu no tempo, mas não perfeito. Kojiro ainda perde um pouco de vida, o inimigo perde um pouco de postura.
- **Parry perfeito:** defendeu no frame certo. Kojiro não perde nada, o inimigo perde muito.
- **Postura quebrada:** o personagem fica aberto por um tempo curto. Se levar golpe nessa janela, morre.

**A postura dos vilões tem que aparecer no corpo do personagem**, não só na interface. O jogador deve olhar pro sprite e saber quem tá quase caindo.

---

## Especificações gerais do pixel art

| Item | Valor |
|---|---|
| Tamanho da célula (personagens normais) | 128x128 px |
| Altura do personagem dentro da célula | uns 88 a 96 px |
| Tamanho da célula (vilões grandes e Oboro) | 160x160 px |
| FPS base das animações | 12 fps (golpes rápidos podem ter frames de 1 tick) |
| Contorno | 1 px, cor escura da paleta do personagem (não preto puro) |
| Paleta por personagem | 12 a 16 cores, sempre com 1 cor de destaque exclusiva |
| Luz | vindo de cima e da esquerda, igual pra todo mundo |
| Pivô / ponto de origem | pés, no centro da base da célula |

### Organização do spritesheet (pensando no código em C)

- Uma linha por animação, todas com a mesma altura de célula.
- Cada animação acompanhada de um arquivo de dados (JSON ou um `.h` gerado) com os frames de cada fase.

```c
typedef struct {
    int row;            // linha no spritesheet
    int frame_count;
    int windup_end;     // último frame da antecipação
    int active_start;   // primeiro frame que acerta
    int active_end;     // último frame que acerta
    int perfect_frame;  // frame exato do parry perfeito
    int posture_dmg;    // dano de postura se acertar
} AttackAnim;
```

### As 3 fases de todo golpe

1. **Antecipação:** o frame mais legível do golpe. O jogador reage aqui. Pose exagerada, arma bem visível, silhueta clara.
2. **Ativo:** poucos frames, rápido. Pode usar smear (rastro borrado da lâmina) em 1 ou 2 frames.
3. **Recuperação:** a janela pra punir. Pose aberta, corpo levemente desequilibrado.

**Regra de ouro:** cada vilão tem um "tell" visual único que avisa o golpe. Nunca dois vilões com o mesmo tell.

### Golpe que não dá pra defender

Alguns vilões têm golpes que não aceitam parry (tem que esquivar). Pra esses:
- Os olhos do vilão brilham na cor vermelha por 2 frames antes da antecipação.
- A lâmina ganha um contorno vermelho durante o ativo.
- Mesmo símbolo pra todos os vilões, pra o jogador aprender uma vez só.

---

## Postura mostrada no corpo

Todo personagem precisa de variações de idle conforme a postura:

| Postura | Visual |
|---|---|
| 0 a 40% | idle normal, guarda alta, respiração lenta |
| 40 a 75% | ombros sobem e descem mais rápido, guarda um pouco mais baixa |
| 75 a 99% | ofegante, lâmina tremendo, 1 frame de "tropeço" no ciclo do idle |
| Quebrada | animação de guarda quebrada (ver abaixo) |

**Animação de guarda quebrada (todo mundo tem):**
- 1 frame de impacto: arma é jogada pro lado, corpo arqueia pra trás
- 3 a 4 frames cambaleando, braços abertos
- loop de 4 frames "atordoado" enquanto dura a janela de execução
- Efeito: uma rachadura branca atravessa o personagem por 2 frames

**Animação de morte (todo mundo tem):** cada vilão tem a sua (ver fichas), mas a base é: corte, pausa de 3 frames parado, queda.

---

## Efeitos visuais (VFX) do parry

### Parry bom
- Faísca pequena, 5 frames, laranja e amarela
- Hitstop curto (congela uns 3 frames)
- Kojiro desliza uns pixels pra trás

### Parry perfeito
- Faísca grande em forma de estrela, 7 frames, branca no centro e dourada nas pontas
- Flash branco na tela inteira por 1 frame
- Hitstop maior (congela uns 6 frames)
- Vilão recua com o braço jogado pra trás
- Um anel de onda de choque sai do ponto de contato

### Postura quebrando
- Tela escurece nas bordas (vinheta) e fica levemente dessaturada
- Rachadura branca atravessa o sprite
- Folhas ou pétalas param no ar enquanto dura a janela

### Execução
- Tela fica em duas cores só (fundo preto ou vermelho, personagens em silhueta branca) por uns frames
- Um traço de corte atravessa a tela
- Volta ao normal com o vilão caindo

---

## Direção de som (resumo)

| Evento | Som |
|---|---|
| Parry bom | "tink" metálico curto e médio, sem eco |
| Parry perfeito | "clang" agudo e forte, com eco longo e um sino leve por baixo |
| Tomou dano de postura | batida seca de madeira, tipo bokken |
| Postura acima de 75% | respiração ofegante do personagem em loop |
| Postura quebrou | rachadura de vidro/cerâmica + tambor taiko grave + silêncio de meio segundo |
| Execução | corte de lâmina limpo, depois silêncio total, depois o corpo caindo |
| Golpe que não dá pra defender | nota grave de shamisen ou flauta shakuhachi antes do ataque |

Cada vilão também tem um **som de tell** próprio (listado na ficha), que toca junto com o frame de antecipação.

---

## Protagonista: Kojiro

- **Quem é:** o 13º e último aluno de Hanzo. O mais novo, o que menos acreditavam.
- **Arma:** katana simples, bainha com uma fita branca (luto pelo Hanzo)
- **Visual:** kimono azul índigo gasto, hakama cinza, faixa branca na testa, cabelo preso curto, sandálias de palha
- **Cor de destaque:** branco da fita
- **Silhueta:** média, equilibrada, postura baixa e firme. Tem que parecer "o estilo correto" no meio de 12 versões tortas.

### Animações do Kojiro

| Animação | Frames | Obs |
|---|---|---|
| Idle (3 variações de postura) | 6 cada | respiração, mão no cabo |
| Andar | 8 | |
| Correr | 8 | |
| Esquiva pra trás | 5 | |
| Esquiva pro lado | 5 | |
| Pulo | 6 | |
| Guarda (segurando) | 2 em loop | |
| Parry bom | 4 | katana na horizontal, pé recua |
| Parry perfeito | 5 | katana gira no punho, pose mais limpa, fita balança |
| Ataque leve 1, 2, 3 | 5, 5, 7 | combo básico |
| Ataque pesado | 9 | antecipação longa, cabo acima da cabeça |
| Contra-ataque pós parry perfeito | 6 | golpe rápido que só sai depois de um perfeito |
| Levar golpe | 3 | |
| Guarda quebrada | 8 | |
| Execução em vilão | 12 | corta de costas, embainha devagar |
| Morte | 10 | cai de joelhos, katana cravada no chão |

---

## Os 12 alunos (vilões)

Cada um aprendeu o mesmo estilo de Hanzo e deformou num caminho diferente. A arte deve mostrar isso: **roupa parecida com a de Kojiro na base (índigo e cinza da escola), mas corrompida pela personalidade de cada um.**

---

### 1. Jirō, o Muro

- **Arma:** tetsubō (clava de ferro com cravos)
- **Silhueta:** enorme, largo, quase quadrado. Usa célula de 160x160.
- **Visual:** kimono da escola rasgado nas mangas, braços enormes enfaixados, cabeça raspada
- **Cor de destaque:** ferro enferrujado
- **Estilo:** lento, pesado, cada golpe tira muita postura
- **Tell visual:** arrasta a clava no chão (faísca no chão) antes de levantar
- **Som de tell:** ferro arranhando pedra
- **Animações únicas:**
  - Golpe vertical esmagador (antecipação de 8 frames, bem lento)
  - Varredura horizontal que não dá pra defender (tem que pular)
  - Pisão que faz a tela tremer
  - Morte: cai pra frente como uma árvore, levanta poeira

---

### 2. Kaede, a Lua Crescente

- **Arma:** naginata
- **Silhueta:** alta e fina, a arma é maior que ela
- **Visual:** cabelo longo preso alto, hakama branca, mangas amarradas
- **Cor de destaque:** vermelho da fita na haste
- **Estilo:** alcance longo, giros, mantém distância
- **Tell visual:** gira a naginata atrás das costas, a lâmina some por 2 frames e reaparece
- **Som de tell:** assobio do vento
- **Animações únicas:**
  - Giro de 360 com 2 acertos
  - Estocada longa que atravessa meia tela
  - Rasteira com a ponta da haste (não dá pra defender)
  - Morte: a naginata cai antes dela, ela ajoelha apoiada na haste

---

### 3. Ren, as Duas Asas

- **Arma:** duas kodachi (espadas curtas)
- **Silhueta:** pequeno, magro, sempre curvado pra frente
- **Visual:** kimono sem mangas, braços tatuados, bandana cobrindo a boca
- **Cor de destaque:** verde jade nos cabos
- **Estilo:** o mais rápido, combos de 5 a 7 golpes, cada um tira pouco
- **Tell visual:** cruza as lâminas na frente do rosto
- **Som de tell:** lâminas raspando uma na outra
- **Animações únicas:**
  - Combo de 7 cortes (o jogador precisa parry em ritmo)
  - Salto por cima do Kojiro com corte nas costas
  - Rodopio no chão
  - Morte: as duas espadas caem em X no chão, ele cai de lado

---

### 4. Genzō, o Anzol

- **Arma:** kusarigama (foice com corrente e peso)
- **Silhueta:** médio, corrente sempre balançando em volta
- **Visual:** chapéu de palha largo (kasa), capa de palha, rosto escondido
- **Cor de destaque:** amarelo palha
- **Estilo:** ataca de longe, puxa o Kojiro pra perto
- **Tell visual:** gira o peso acima da cabeça, a velocidade do giro indica quando vai soltar
- **Som de tell:** corrente chacoalhando, acelerando
- **Animações únicas:**
  - Arremesso do peso (timing variável, ele finge às vezes)
  - Corrente enrola a katana do Kojiro e puxa (não dá pra defender, tem que esquivar)
  - Corte com a foice depois de puxar
  - Morte: a corrente cai enrolando nele mesmo

---

### 5. Hikari, a Cega

- **Arma:** katana em bengala (shikomizue)
- **Silhueta:** senhora de pé, magra, quase imóvel
- **Visual:** faixa sobre os olhos, kimono cinza simples, cabelo branco solto
- **Cor de destaque:** roxo claro na faixa
- **Estilo:** iaijutsu. Fica parada e saca instantâneo. Tells são mais sonoros que visuais.
- **Tell visual:** mínimo, só o polegar empurra a guarda da espada (1 pixel de lâmina aparecendo)
- **Som de tell:** clique da lâmina saindo da bainha
- **Animações únicas:**
  - Saque instantâneo (ativo de 1 frame, antecipação quase invisível)
  - Idle longo com o ouvido virado pro Kojiro
  - Série de 3 saques em sequência
  - Morte: embainha a espada, senta devagar e fica imóvel

---

### 6. Daisuke, o Trovão

- **Arma:** yari (lança reta)
- **Silhueta:** musculoso, postura larga, lança sempre apontada
- **Visual:** armadura parcial (só ombreiras e peitoral), capacete com chifres pequenos
- **Cor de destaque:** laranja das cordas da armadura
- **Estilo:** investidas de longe, atravessa a arena
- **Tell visual:** recua o pé de trás e raspa o chão 2 vezes, como um touro
- **Som de tell:** grito curto de guerra (kiai)
- **Animações únicas:**
  - Investida atravessando a tela
  - Sequência de 3 estocadas curtas
  - Lança girando pra quebrar guarda (não dá pra defender)
  - Morte: a lança fica cravada no chão e ele cai encostado nela

---

### 7. Sōma, a Lâmina Longa

- **Arma:** ōdachi (espada gigante)
- **Silhueta:** alto, magro demais, a espada é do tamanho dele. Célula 160x160.
- **Visual:** kimono preto, cabelo comprido e bagunçado, olheiras fundas
- **Cor de destaque:** azul gelo na lâmina
- **Estilo:** cortes largos que pegam a arena toda, bem demorados
- **Tell visual:** a lâmina brilha azul da ponta até o cabo (o brilho chegando no cabo é o sinal)
- **Som de tell:** lâmina vibrando, zumbido metálico
- **Animações únicas:**
  - Corte giratório completo
  - Corte descendo do alto que fica cravado no chão (janela grande de punição)
  - Arrastar a espada gerando onda de corte no chão
  - Morte: a espada quebra ao meio, ele cai de costas

---

### 8. Yuki, a Dançarina

- **Arma:** dois tessen (leques de ferro)
- **Silhueta:** leve, sempre em movimento, mangas longas esvoaçantes
- **Visual:** kimono branco com flores de ameixa, maquiagem vermelha nos olhos
- **Cor de destaque:** rosa das flores
- **Estilo:** fintas, dança, os ataques escondem uns aos outros
- **Tell visual:** fecha os leques antes do golpe real (leque aberto = finta, fechado = golpe)
- **Som de tell:** leques estalando ao fechar
- **Animações únicas:**
  - Dança de fintas (3 movimentos, só 1 acerta)
  - Arremesso de leque que volta
  - Pétalas que tapam a visão por uns frames
  - Morte: gira uma última vez e cai com os leques abertos no chão

---

### 9. Kagerō, a Sombra

- **Arma:** tantō (faca) e kunai
- **Silhueta:** agachado, quase no chão, some e aparece
- **Visual:** roupa escura da escola, máscara de raposa quebrada
- **Cor de destaque:** vermelho dos olhos da máscara
- **Estilo:** some em fumaça e ataca de ângulos estranhos, golpes atrasados
- **Tell visual:** a fumaça tem cor diferente no lugar onde ele vai aparecer
- **Som de tell:** estalo seco (bomba de fumaça)
- **Animações únicas:**
  - Sumir e aparecer (6 frames de fumaça cada)
  - Golpe atrasado: antecipação que segura 6 frames a mais que o normal
  - Arremesso de 3 kunai
  - Morte: some em fumaça e só a máscara cai no chão

---

### 10. Bunta, o Bêbado

- **Arma:** katana e cabaça de sakê
- **Silhueta:** barrigudo, torto, sempre cambaleando
- **Visual:** kimono aberto, barba mal feita, bochechas vermelhas
- **Cor de destaque:** vermelho das bochechas e da cabaça
- **Estilo:** timing irregular, parece que vai cair e ataca
- **Tell visual:** toma um gole da cabaça antes de cada combo
- **Som de tell:** soluço
- **Animações únicas:**
  - Tropeção que vira ataque
  - Cuspe de sakê que embaça a tela
  - Combo bêbado com ritmo quebrado
  - Morte: senta, toma um último gole e tomba pro lado roncando (ou morto, fica ambíguo)

---

### 11. Tatsu, o Dragão de Fogo

- **Arma:** katana com óleo na lâmina (pega fogo)
- **Silhueta:** atlético, pose de ataque sempre agressiva
- **Visual:** kimono vermelho escuro, queimaduras nos braços, cabelo curto espetado
- **Cor de destaque:** laranja do fogo
- **Estilo:** agressivo, o fogo aumenta o alcance dos golpes
- **Tell visual:** passa a lâmina na bainha e ela acende
- **Som de tell:** fogo acendendo (whoosh)
- **Animações únicas:**
  - Lâmina acendendo (loop de chamas na lâmina, 4 frames)
  - Corte que deixa rastro de fogo no chão
  - Explosão de fogo (não dá pra defender)
  - Morte: a chama da lâmina apaga, ele cai e sai fumaça

---

### 12. Oboro, o Traidor (Big Boss)

- **Arma:** katana igual à de Kojiro, mas com a lâmina escurecida
- **Silhueta:** do tamanho do Kojiro, espelho dele. Célula 160x160 só na fase 2.
- **Visual:** mesmo uniforme da escola, mas preto e roxo, cabelo longo solto, cicatriz no rosto. Carrega a fita branca de Hanzo amarrada no punho (troféu).
- **Cor de destaque:** roxo da névoa (oboro = lua enevoada)
- **Estilo:** usa o estilo de Hanzo quase perfeito, com os mesmos golpes do Kojiro, só que mais rápidos e com névoa
- **Tell visual:** a névoa se junta na lâmina antes do golpe
- **Som de tell:** sino baixo, igual ao do parry perfeito, só que invertido

**Fase 1 (espelho):**
- Usa as mesmas animações de ataque do Kojiro, redesenhadas com a paleta dele
- Parry perfeito dele mesmo: se o jogador ataca no tempo errado, Oboro faz parry

**Fase 2 (névoa):**
- Idle cercado de névoa roxa (loop de 8 frames)
- Clones de névoa que atacam junto (sprite dele em transparência)
- Técnica final: um golpe que usa um movimento de cada um dos 11 alunos em sequência. Ideal é reaproveitar os frames de ataque dos outros vilões com a paleta do Oboro.

**Morte:**
- A névoa some, a fita branca de Hanzo solta do punho dele e cai no chão
- Última cena: Kojiro pega a fita

---

## Checklist de animação por vilão

Todo vilão precisa ter no mínimo:

- [ ] Entrada na arena (apresentação)
- [ ] Idle em 3 níveis de postura
- [ ] Andar
- [ ] 3 a 5 golpes com tell próprio
- [ ] 1 golpe que não dá pra defender (com o brilho vermelho padrão)
- [ ] Reação a parry bom (recua um pouco)
- [ ] Reação a parry perfeito (braço jogado pra trás, bem aberto)
- [ ] Guarda quebrada + loop atordoado
- [ ] Morte única

## Ordem sugerida de produção

1. Kojiro completo (é ele que o jogador vê o tempo todo)
2. VFX de parry bom, perfeito e postura quebrando
3. Jirō (golpes lentos, bom pra testar timing)
4. Oboro fase 1 (reaproveita a base do Kojiro)
5. Resto dos vilões em ordem de dificuldade
6. Oboro fase 2 por último, porque depende dos frames dos outros
