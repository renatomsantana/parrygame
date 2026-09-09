# APARA — base de desenvolvimento 0.2

Projeto de duelo **lateral** para **Godot 4.6 ou mais recente, edição
Standard / GDScript**, com duas direções de arte disponíveis.

Os dois lutadores aparecem de corpo inteiro, de perfil, e a câmera é
ortográfica olhando na horizontal: toda superfície deitada vira uma linha e
a cena lê como elevação 2D, sem perspectiva. O mundo continua sendo
geometria 3D — é o enquadramento e a paleta fechada que dão a leitura.

| Tema | Base visual incluída |
| --- | --- |
| **Samurai — padrão** | Ronin de chapéu e placas em magenta contra um duelista em ciano; portal, bordos e lanternas |
| **Medieval** | Cavaleiro de aço azul contra um duelista em âmbar; muralha, torres, estandartes e tochas |

Os cenários, personagens e armas são cenas nativas editáveis no Godot.
As formas simples servem de base para criarmos a arte final. O combate e seus
ajustes são compartilhados pelos dois temas.

## Começar

1. Extraia **todo** o ZIP em uma pasta nova.
2. Abra o [Godot](https://godotengine.org/download/), clique em **Importar** e
   escolha `project.godot`.
3. Abra `main.tscn`: a prévia samurai já mostra o palco, os dois lutadores e a
   câmera lateral no editor.
4. Pressione **F5**, escolha **Samurai** ou **Medieval** no menu e inicie o duelo.

O tema pode ser trocado antes de iniciar ou na tela de resultado. Durante a
pausa de uma luta, o seletor fica oculto. **R** reinicia com o tema atual.
A escolha no menu vale para essa execução; o tema inicial fica em `main.tscn`,
no campo **Duel Theme** de **Parry2D**. A prévia salva da cena principal é samurai;
o campo Duel Theme determina o tema aplicado ao executar.

## Controles

| Controle | Ação |
| --- | --- |
| **Espaço** ou **clique esquerdo** | Aparar |
| **Clique direito** | Contra-atacar depois de aparar |
| **A** / **D** | Aproximar e afastar no palco |
| **Esc** | Pausar / continuar e liberar / capturar o cursor |
| **R** | Reiniciar o duelo |

O parry não tem direção — um botão só, e a única decisão é **quando**. Mas os
três golpes continuam importando, porque a altura virou relógio: **alto é
lento** e telegrafado, **baixo chega antes** do que a mão espera. A interface
diz as duas coisas juntas, e as três faixas sobre o duelista mostram de onde
vem o golpe.

Cuidado com a **finta**: às vezes o clarão vem e o golpe não. Quem apara nela
gasta a defesa e come o golpe de verdade logo depois. Nada avisa por escrito —
o aviso está no corpo do oponente, que recolhe em vez de comprometer, e no som
que começa e não termina.

A **firmeza** ao lado da vida conta quantas defesas dá para jogar fora. Cada
botão apertado à toa gasta um ponto; zerando, custa vida e volta cheia. Também
não há aviso de parry perfeito: ele se reconhece pelo som e pelo tranco. As três faixas
empilhadas sobre o duelista mostram onde está a sua guarda; a faixa dourada
com a seta mostra em que altura vem o golpe. No clarão branco, clique
esquerdo com a guarda na mesma altura.

É preciso acertar o tempo. Segurar o botão não renova a defesa.
Depois do parry, use o direito durante o atordoamento. Um contra-ataque remove
um ponto de vida. Você e o oponente começam com cinco pontos.

O parry dura 0,24 s, o perfeito aceita os últimos 0,075 s antes do impacto,
e a recuperação entre tentativas é de 0,62 s. Os tempos têm a resolução dos
passos de física. Há faíscas, sons sintetizados, pausa breve no impacto,
recuo e tremor de câmera; os efeitos precisam ser avaliados jogando.

## Onde vamos trabalhar

| Para mudar | Abra |
| --- | --- |
| Dificuldade, sensibilidade, vida e impacto | `config/combat.tres` no Inspetor |
| Regras de parry e do duelo | `scripts/main.gd` |
| Arena samurai | `scenes/themes/samurai/arena.tscn` |
| Ronin e armadura | `scenes/themes/samurai/sentinel.tscn` |
| Katana | `scenes/themes/samurai/weapon.tscn` |
| Arena, cavaleiro e espada medieval | Os três arquivos correspondentes em `scenes/themes/medieval/` |
| Cores, nomes e cenas de cada tema | `config/themes/samurai.tres` ou `medieval.tres` |
| Interface e indicador da guarda | `scripts/hud.gd` e `scripts/guard_indicator.gd` |
| Sons e faíscas | `scripts/feedback.gd` |

**Edite a cena de arte original**, na pasta do tema. Alterações feitas apenas
nos filhos das instâncias de prévia em `main.tscn` são substituídas ao carregar
o perfil do tema. Salve a cena original, pare o jogo e execute novamente.

- [Direção de arte e modelos](docs/DESIGN.md)
- [Guia para programar e testar](docs/DESENVOLVIMENTO.md)
- [Próximas etapas](docs/PROXIMOS_PASSOS.md)
- [Pasta para novos assets](art/LEIA-ME.md)

## Estado desta entrega

O projeto roda no Godot 4.6.3: importação limpa, sintaxe conferida nos sete
scripts, 126 verificações automáticas em `tests/smoke_test.gd` e os dois
temas renderizando — veja `docs/capturas/`. **O que falta é jogar:** a sensação do
parry e a clareza da leitura precisam de mouse e teclado para serem julgadas.

Esta é a base de código e design para continuar o desenvolvimento. Ainda não
inclui modelos finais, rig de personagem, animações esqueléticas, campanha,
salvamento ou executável exportado. As poses atuais movem nós por código.
