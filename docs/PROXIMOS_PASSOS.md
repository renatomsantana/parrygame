# Nossa próxima sessão

## Onde paramos

O projeto roda no Godot 4.6.3. Estão feitos: a base confirmada em execução, o
enquadramento da arma, a legibilidade da HUD, a separação de `main.gd` em
`player_controller.gd` e `enemy_brain.gd`, o acabamento pixel art a 320x180 e a
direção neo-noir com paleta fechada por tema.

O jogo virou **lateral**. A câmera é ortográfica, olha na horizontal e enquadra
os dois lutadores de corpo inteiro; o jogador deixou de ser uma câmera e passou
a ser um duelista em cena, com modelo próprio por tema. A guarda deixou de ser
esquerda/cima/direita e passou a ser **alto / médio / baixo**, escolhida
subindo e descendo o mouse. As arenas foram refeitas em camadas chatas atrás
do palco. `docs/DESIGN.md` traz as regras; `docs/capturas/` traz o resultado.

Antes disso, os dois oponentes já haviam sido redesenhados: silhueta em
ampulheta, formas facetadas, corpo escuro com a cor do tema só no alto e nas
bordas, refletores presos ao modelo e braços que acompanham a arma.

Verificação automática disponível a qualquer momento — 126 verificações sem
janela com `godot --headless --path . --script res://tests/smoke_test.gd`, e
seis quadros renderizados com
`godot --path . --script res://tests/capture.gd -- docs/capturas`.

Nenhum bloco cercado por crases dentro destes documentos: o markdown mestre
guarda cada arquivo dentro de uma cerca, e uma cerca aninhada fecha a de fora.

## 1. Jogar

É o único item que continua bloqueado, e bloqueia os outros. Abrir `main.tscn`,
pressionar F5 e disputar duelos curtos. Observar se dá para ler a **altura**,
perceber a janela, distinguir parry normal de perfeito e contra-atacar com
intenção — e principalmente se a finta engana da primeira vez e deixa de
enganar depois de algumas. Se ela nunca pega, está fraca; se pega sempre,
o aviso no corpo não está legível.

Ajustar **um valor por vez** em `config/combat.tres`. Comece por `parry_window`,
`enemy_windup`, `feint_hold` e `steadiness`. Os dois últimos são os mais
prováveis de estar errados na primeira partida: `feint_hold` decide se a finta
é armadilha ou punição garantida, e `steadiness` decide se apertar à toa dói o
bastante. Nenhum teste automático mede sensação.

## 2. Modelar o ronin

A silhueta já está de pé e serve de referência para o modelo definitivo: chapéu
cônico largo, cintura estreita, hakama que abre, corpo escuro e fio aceso nas
bordas. `docs/DESIGN.md` traz escala, pontos de conexão, paleta e o rig de luz.

O que ainda entrega a geometria como primitiva é a articulação: só os braços se
movem, e por código. Tronco, quadril e pernas continuam rígidos. Resolver é
modelar com rig.

## 3. Animar de verdade

Já existe uma camada de pose por código (`scripts/visuals/body_pose.gd`):
antecipação na preparação, acompanhamento no golpe, postura por altura,
respiração e solavanco no impacto. Ela resolve o peso, não a articulação — as
pernas continuam sendo caixas fixas.

O que falta é rig e animação esquelética: repouso, preparação das três
alturas, golpe, aparado, atordoado, recebendo contra-ataque e derrota. Ao
trocar as poses de código por animações, preservar **um único instante de
resolução por ataque** — é o contrato que `enemy_brain.gd` sustenta hoje com o
evento `resolve`.

## 4. Decidir entre modelo e sprite

A referência declarada é arte 2D desenhada à mão. A junta para trocar geometria
por sprite já existe — o tema aponta para cenas quaisquer, e o contrato é só
`WeaponPivot`, `AttackCue` e `Arms`. O que falta é a decisão, porque as duas
coisas não se somam: um sprite desenhado não recebe os refletores presos ao
modelo, e a luz teria de estar pintada no quadro.

Assar os modelos atuais em sprites **não** adianta: sai a mesma imagem, só
pré-renderizada. As referências estão em `docs/REFERENCIAS.md`.

## 5. Fechar a paleta

As cores do jogador já entraram nas duas listas — sem entrada própria, a cor
nova caía na vizinha. O que falta é o inverso: com o modelo final, escolher as
cores primeiro e pintar dentro delas, em vez de aproximar depois.

## Decisão pendente

`script_parry_3d.md` duplica os 33 arquivos do projeto. Está sincronizado e
confere byte a byte, mas são duas fontes da mesma verdade. Decidir se ele segue
sendo mestre ou vira só documentação.

O primeiro marco é **um duelo completo com três lados de parry e arte coerente**.
Campanha, múltiplos inimigos, progressão e exportação ficam para depois.
