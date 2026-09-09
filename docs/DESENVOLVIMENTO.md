# Programar e testar

## Abrir o código

Abra `scripts/main.gd` no editor de scripts do próprio Godot. Não é necessário
configurar um editor externo. A cena principal já liga os recursos e os nós.

Os dois lados do duelo moram em arquivos próprios. `main.gd` guarda o fluxo e
a **resolução** dos golpes, que é o único ponto que precisa conhecer os dois ao
mesmo tempo.

| Arquivo / método | Responsabilidade |
| --- | --- |
| `player_controller.gd` | Guarda escolhida, janela, recuperação, movimento e pose da arma |
| `player_controller.gd` → `try_parry()` | Abre a janela e trava o lado; falso quando o clique não conta |
| `player_controller.gd` → `steer_guard()` | Acumula o mouse até o limiar e escolhe o lado |
| `enemy_brain.gd` | Aproximação, preparação, recuperação, atordoamento e pose do oponente |
| `visuals/body_pose.gd` | Inclinação, giro, sobe-e-desce e cabeça dos dois lutadores |
| `visuals/arm_rig.gd` | Vira e estica cada braço até o punho da arma |
| `enemy_brain.gd` → `tick()` | Avança um quadro e devolve os eventos `cue` e `resolve` |
| `enemy_brain.gd` → `start_attack()` | Escolhe o lado no padrão e abre a preparação |
| `main.gd` → `_resolve_enemy_hit()` | Decide esquiva, parry ou dano uma vez por golpe |
| `main.gd` → `_on_parry_success()` | Aplica atordoamento, contagem e efeitos |
| `main.gd` → `_try_counter()` | Resolve o contra-ataque durante o atordoamento |
| `main.gd` → `_apply_theme()` | Instancia as cenas e conecta o visual ao combate |
| `main.gd` → `_apply_pixel_look()` | Liga a resolução baixa e os parâmetros do shader |
| `shaders/pixelate.gdshader` | Mapeia o mundo 3D para a paleta do tema, com pontilhado |
| `main.gd` → `_apply_palette()` | Monta a paleta do tema como textura de uma linha |
| `hud.gd` | Interface, seletor de tema, pausa e resultado |
| `guard_indicator.gd` | Três setores e sinal do golpe |
| `feedback.gd` | Sons sintetizados e partículas transitórias |
| `scripts/config/combat_tuning.gd` | Define os campos editáveis de combate |
| `scripts/config/duel_theme.gd` | Define os campos editáveis de aparência |
| `tests/smoke_test.gd` | 126 verificações de combate e acabamento, sem janela |
| `tests/capture.gd` | Grava quadros do duelo para conferir a imagem |

## Alterar um ajuste

Abra `config/combat.tres` no Inspetor, mude **um ajuste por vez**, salve,
pare o jogo e execute novamente. Esse recurso serve aos dois temas.

Para começar, mexa em `parry_window`, `enemy_windup` ou `feint_hold`. Este
último decide se a finta é armadilha ou punição garantida: precisa segurar o
golpe por mais tempo que a recuperação do parry, senão quem cai nela não tem
defesa nenhuma. Impacto, som, tremor e sobreposição de cor têm seus próprios
campos. Não é preciso procurar números dentro da lógica.

Os valores atuais ficam em `config/combat.tres`; os valores de criação de
um recurso novo ficam nas declarações de `combat_tuning.gd`.

## Contratos entre as partes

As formas físicas ficam em `main.tscn`. O jogador usa camada 2 e máscara 5;
o inimigo, camada 4 e máscara 3; o cenário usa camada 1. Os modelos visuais
podem ser substituídos preservando esses corpos e suas colisões.

Todo o mundo 3D fica dentro de `Pixelate/Viewport`, o SubViewport de resolução
baixa. Os caminhos `Pixelate/Viewport/Stage`, `.../Effects`, `.../Player`,
`.../CameraRig/Camera3D`, `.../Player/VisualMount`, `.../Sentinel` e
`.../Sentinel/VisualMount` são usados em `_bind_scene_nodes()`. Se renomeá-los,
atualize esse método.

Qualquer nó 3D criado por código precisa entrar em `Effects`, dentro do
SubViewport. Fora dele o nó cai num mundo sem câmera e simplesmente não
aparece — é o caso das faíscas de `feedback.gd`. A HUD é o contrário: fica
fora do SubViewport, na resolução da janela.

`PlayerController` e `EnemyBrain` são RefCounted, não nós: não aparecem na
árvore de cena e recebem os corpos por `setup()`. Nenhum dos dois decide o
resultado de um golpe. O oponente devolve eventos e `main.gd` escolhe a
consequência — é isso que mantém **um único instante de resolução por ataque**,
que precisa continuar valendo quando as poses virarem animações.

Os perfis de tema apontam para três PackedScenes: arena, oponente e arma.
Apenas as partes visuais são substituídas. O código duplica o material do
AttackCue antes de mudar sua cor para não alterar o recurso compartilhado.

## Primeira rodada de verificação no Godot

Nenhuma caixa abaixo representa um teste já executado.

- [ ] Importar o projeto no Godot 4.6+ e confirmar ausência de erros de script.
- [ ] Abrir main.tscn e conferir a prévia samurai no editor 3D.
- [ ] F5: iniciar Samurai e conferir cenário, personagem, katana e interface.
- [ ] Escolher as três guardas pelo mouse; para baixo deve manter a guarda atual.
- [ ] Combinar lado e tempo: sem dano, faíscas, som metálico e atordoamento.
- [ ] Aparar no limite final: conferir indicação e efeitos de parry perfeito.
- [ ] Clicar cedo e escolher lado errado: ambos devem permitir receber dano.
- [ ] Segurar o esquerdo: a janela termina e não se renova automaticamente.
- [ ] Usar o direito após aparar: remover apenas um ponto de vida do oponente.
- [ ] Atacar antes de aparar: armadura impede o dano.
- [ ] Recuar além do alcance: golpe do inimigo não causa dano.
- [ ] Pausar no meio do golpe e retomar; não deve avançar durante o menu.
- [ ] Sair da janela: o duelo deve pausar e liberar o cursor.
- [ ] Completar vitória e derrota; reiniciar e conferir vida e contadores.
- [ ] Na tela de resultado, trocar para Medieval e repetir o duelo.
- [ ] Conferir que espada, oponente, arena, nomes e cores mudam juntos.
- [ ] Conferir que o seletor não aparece ao pausar uma luta em andamento.
- [ ] Alterar um material na cena de arte original, salvar e conferir em F5.
- [ ] Reduzir tremor e desligar a sobreposição de cor no recurso de combate.

O projeto já foi importado e executado no Godot 4.6.3. A checagem de sintaxe
passa em todos os scripts e `tests/smoke_test.gd` cobre o jogo com 126 verificações
headless. A lista acima continua valendo para o teste com as mãos: é ela que
julga a sensação, que nenhum teste automático mede.

## Próximas mudanças no código

O controlador do jogador e a máquina de estados do inimigo já saíram de
`main.gd` para `player_controller.gd` e `enemy_brain.gd`, com as verificações
de `tests/smoke_test.gd` passando antes e depois e os quadros de `docs/capturas/`
idênticos. A substituição de poses por animações deve manter um único instante
de resolução de cada ataque.

O próximo passo de código depende de jogar: ajustar `config/combat.tres` com a
sensação na mão, um valor por vez.

Referências: [Resources](https://docs.godotengine.org/en/stable/tutorials/scripting/resources.html),
[PackedScene](https://docs.godotengine.org/en/stable/classes/class_packedscene.html) e
[OptionButton](https://docs.godotengine.org/en/stable/classes/class_optionbutton.html).
