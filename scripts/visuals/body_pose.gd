extends RefCounted
## Pose do corpo de um lutador: inclinação, giro do tronco, sobe-e-desce e
## inclinação da cabeça. Não substitui um rig — as pernas continuam rígidas —
## mas dá ao golpe a antecipação e o acompanhamento sem os quais ele não lê.
##
## O modelo precisa expor `Body` (tronco) e `Head` (cabeça) como Node3D. As
## rotações que o arquivo da cena já traz são guardadas como repouso e somadas,
## em vez de sobrescritas: a postura de guarda desenhada na arte sobrevive.

## Amortecimento do empurrão de impacto, por segundo.
const IMPULSE_DECAY := 3.2
## A cabeça devolve parte da inclinação do tronco. É o que mantém o rosto
## legível enquanto o corpo se joga para a frente.
const HEAD_COUNTER := 0.6

var root: Node3D
var body: Node3D
var head: Node3D
var body_rest := Vector3.ZERO
var head_rest := Vector3.ZERO

var lean: float = 0.0
var twist: float = 0.0
var bob: float = 0.0
var head_tilt: float = 0.0
var impulse: float = 0.0
## Relógio próprio do passo, para o sobe-e-desce não depender do tempo global.
var stride: float = 0.0


func attach(model: Node3D) -> void:
	root = model
	body = model.get_node_or_null("Body") as Node3D
	head = model.get_node_or_null("Head") as Node3D
	body_rest = body.rotation if body != null else Vector3.ZERO
	head_rest = head.rotation if head != null else Vector3.ZERO
	reset()


func reset() -> void:
	lean = 0.0
	twist = 0.0
	bob = 0.0
	head_tilt = 0.0
	impulse = 0.0
	stride = 0.0
	_apply()


## Empurrão instantâneo do impacto, que decai sozinho. Separado dos alvos
## porque um golpe recebido não é um estado: é um solavanco em cima do que
## o corpo já estava fazendo.
func punch(amount: float) -> void:
	impulse += amount


## Sobe-e-desce de passo. Sempre positivo e depois deslocado, para o corpo
## subir a cada passada em vez de oscilar em torno do meio como um pêndulo.
func step_bob(delta: float, speed: float, height: float) -> float:
	stride += delta * speed
	return absf(sin(stride)) * height - height * 0.35


func drive(delta: float, target_lean: float, target_twist: float,
		target_bob: float, target_head: float, speed: float = 10.0) -> void:
	var t := minf(1.0, delta * speed)
	lean = lerpf(lean, target_lean, t)
	twist = lerpf(twist, target_twist, t)
	bob = lerpf(bob, target_bob, t)
	head_tilt = lerpf(head_tilt, target_head, t)
	impulse = move_toward(impulse, 0.0, delta * IMPULSE_DECAY)
	_apply()


func _apply() -> void:
	if root == null:
		return
	var pitch := lean + impulse
	root.rotation.x = pitch
	root.position.y = bob
	if body != null:
		body.rotation = body_rest + Vector3(0.0, twist, 0.0)
	if head != null:
		head.rotation = head_rest + Vector3(head_tilt - pitch * HEAD_COUNTER,
			-twist * 0.7, 0.0)
