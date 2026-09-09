extends RefCounted
## Corpo, defesa e tempos do jogador. Não decide o resultado de um golpe:
## quem conhece os dois lados do duelo é main.gd.
##
## Na visão lateral o jogador é um lutador em cena, não uma câmera: o modelo
## vem do tema e a espada é posada no próprio punho dele.

const CombatConfig = preload("res://scripts/config/combat_tuning.gd")
const ArmRig = preload("res://scripts/visuals/arm_rig.gd")
const BodyPose = preload("res://scripts/visuals/body_pose.gd")

## Alto, médio e baixo. O parry não tem direção — a altura sobrou para a
## **animação**: a lâmina sobe para aparar um corte alto e desce para um baixo,
## seguindo o golpe que veio, não uma escolha do jogador.
enum Height { HIGH, MID, LOW }

# Poses da arma no espaço do modelo do jogador. A frente do modelo é -Z.
const READY_POSITION := Vector3(0.34, 1.30, -0.34)
const READY_ROTATION := Vector3(-1.55, 0, 0)
# Aparo, uma pose por altura do golpe recebido.
const DEFLECT_POSITIONS: Array[Vector3] = [Vector3(0.16, 1.94, -0.22),
	Vector3(0.30, 1.36, -0.48), Vector3(0.28, 0.82, -0.42)]
const DEFLECT_ROTATIONS: Array[Vector3] = [Vector3(-1.05, 0, -0.55),
	Vector3(-1.60, 0, 0.25), Vector3(-2.35, 0, 0.45)]
const ATTACK_POSITION := Vector3(0.10, 1.45, -0.62)
const ATTACK_ROTATION := Vector3(-2.5, 0, -0.35)
const ATTACK_DURATION := 0.38
const COUNTER_BUFFER := 0.18

## O corpo acompanha a altura do aparo. Não é enfeite: com o parry sem direção,
## é a pose que conta ao jogador qual golpe ele acabou de segurar.
const DEFLECT_LEAN: Array[float] = [-0.06, 0.10, 0.20]
const DEFLECT_BOB: Array[float] = [0.03, 0.0, -0.08]
const READY_LEAN := 0.03
const ATTACK_LEAN := 0.34
const STRIDE_SPEED := 10.0
const STRIDE_HEIGHT := 0.035
## Respiração parada: o suficiente para o corpo não parecer congelado.
const BREATH_SPEED := 2.2
const BREATH_HEIGHT := 0.012

var body: CharacterBody3D
var weapon_pivot: Node3D
var arms: Node3D
var combat: CombatConfig

## Altura do golpe sendo aparado. Quem define é main.gd, a partir da preparação
## do oponente: a pose de aparo acompanha o golpe que veio.
var deflect_height: int = Height.MID
var parry_left: float = 0.0
var parry_age: float = 0.0
var cooldown_left: float = 0.0
var attack_left: float = 0.0
var counter_buffer: float = 0.0
var knockback := Vector3.ZERO
## Uma janela aberta que ainda não aparou nada, e o aviso de que fechou
## sem aparar. main.gd consome esse aviso uma vez por janela perdida.
var parry_open: bool = false
var parry_landed: bool = false
var wasted: bool = false
var pose_rig := BodyPose.new()
## Quanto o jogador está andando, de 0 a 1. Só a pose usa.
var travel: float = 0.0


func setup(player_body: CharacterBody3D, tuning: CombatConfig) -> void:
	body = player_body
	combat = tuning


## Solavanco do impacto, por cima do que o corpo já estava fazendo.
func punch(amount: float) -> void:
	pose_rig.punch(amount)


## Liga o controlador às peças do modelo em uso. Chamado a cada troca de tema.
func attach(pivot: Node3D, arm_rig: Node3D = null, model: Node3D = null) -> void:
	weapon_pivot = pivot
	arms = arm_rig
	if model != null:
		pose_rig.attach(model)


func reset(spawn: Vector3) -> void:
	deflect_height = Height.MID
	parry_left = 0.0
	parry_age = 0.0
	cooldown_left = 0.0
	attack_left = 0.0
	counter_buffer = 0.0
	knockback = Vector3.ZERO
	parry_open = false
	parry_landed = false
	wasted = false
	travel = 0.0
	body.position = spawn
	body.velocity = Vector3.ZERO
	pose_rig.reset()


## Recuperação real do parry: nunca menor que a própria janela.
func cooldown_window() -> float:
	return maxf(combat.parry_cooldown, combat.parry_window + 0.12)


func readiness() -> float:
	return 1.0 - cooldown_left / cooldown_window()


func is_parrying() -> bool:
	return parry_left > 0


func tick_timers(delta: float) -> void:
	cooldown_left = maxf(0, cooldown_left - delta)
	parry_left = maxf(0, parry_left - delta)
	parry_age += delta
	attack_left = maxf(0, attack_left - delta)
	counter_buffer = maxf(0, counter_buffer - delta)
	# A janela fechou sem ter aparado nada. É o botão apertado à toa — por
	# pressa, por mania de apertar ou por ter caído numa finta.
	if parry_open and parry_left <= 0.0:
		parry_open = false
		wasted = not parry_landed


func buffer_counter() -> void:
	counter_buffer = COUNTER_BUFFER


func clear_counter() -> void:
	counter_buffer = 0


## Abre a janela. Falso quando o botão não vira tentativa: segurar não renova a
## defesa, e durante a recuperação não há defesa nenhuma. É essa recuperação
## que dá dente à finta — quem apara no clarão falso não tem o que fazer
## quando o golpe de verdade chega logo depois.
func try_parry() -> bool:
	if cooldown_left > 0 or attack_left > 0:
		return false
	parry_left = combat.parry_window
	parry_age = 0
	cooldown_left = cooldown_window()
	parry_open = true
	parry_landed = false
	return true


## O parry não tem direção: dentro da janela, apara. O que separa quem lê o
## duelo de quem chuta é o instante.
func parries() -> bool:
	return parry_left > 0


func consume_parry() -> void:
	parry_left = 0
	parry_landed = true
	parry_open = false


## Devolve, uma vez só, se a última janela fechou sem aparar nada.
func take_wasted() -> bool:
	var value := wasted
	wasted = false
	return value


func begin_counter() -> void:
	counter_buffer = 0
	attack_left = ATTACK_DURATION


## O duelo acontece numa faixa: o jogador anda no eixo do palco, não no eixo
## para onde está virado. Fosse relativo ao corpo, avançar viraria girar.
func move(delta: float) -> void:
	var input := Input.get_axis("move_left", "move_right")
	travel = absf(input)
	var speed := combat.movement_speed * (0.5 if parry_left > 0 else 1.0)
	body.velocity.x = input * speed + knockback.x
	body.velocity.z = knockback.z
	body.velocity.y = -2.0 if body.is_on_floor() else body.velocity.y - 20 * delta
	body.move_and_slide()
	knockback = knockback.move_toward(Vector3.ZERO, delta * 12)


func aim_at(target: Node3D) -> void:
	var flat := target.global_position
	flat.y = body.global_position.y
	if body.global_position.distance_squared_to(flat) > 0.01:
		body.look_at(flat, Vector3.UP)


func pose_weapon(delta: float) -> void:
	if weapon_pivot == null:
		return
	var desired_pos := READY_POSITION
	var desired_rot := READY_ROTATION
	if parry_left > 0:
		desired_pos = DEFLECT_POSITIONS[deflect_height]
		desired_rot = DEFLECT_ROTATIONS[deflect_height]
	if attack_left > 0.15:
		desired_pos = ATTACK_POSITION
		desired_rot = ATTACK_ROTATION
	weapon_pivot.position = weapon_pivot.position.lerp(desired_pos, minf(1, delta * 24))
	weapon_pivot.rotation = weapon_pivot.rotation.lerp(desired_rot, minf(1, delta * 24))
	_pose_body(delta)
	ArmRig.aim(arms, weapon_pivot.global_position)


func _pose_body(delta: float) -> void:
	var lean := READY_LEAN
	var bob := 0.0
	var settle := 12.0
	if travel > 0.0:
		bob += pose_rig.step_bob(delta, STRIDE_SPEED, STRIDE_HEIGHT)
	else:
		bob += pose_rig.step_bob(delta, BREATH_SPEED, BREATH_HEIGHT)
	if parry_left > 0:
		lean = DEFLECT_LEAN[deflect_height]
		bob += DEFLECT_BOB[deflect_height]
		settle = 24.0
	if attack_left > 0.15:
		lean = ATTACK_LEAN
		settle = 24.0
	pose_rig.drive(delta, lean, 0.0, bob, 0.0, settle)
