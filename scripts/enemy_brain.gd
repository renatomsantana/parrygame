extends RefCounted
## Aproximação, preparação e recuperação do oponente. A máquina de estados
## avisa o que aconteceu no quadro; quem resolve dano e parry é main.gd.
## Cada ataque tem um único instante de resolução — preserve isso ao trocar
## as poses por animações.

const CombatConfig = preload("res://scripts/config/combat_tuning.gd")
const ArmRig = preload("res://scripts/visuals/arm_rig.gd")
const BodyPose = preload("res://scripts/visuals/body_pose.gd")

enum State { APPROACH, WINDUP, RECOVER, STUNNED, DEFEATED }
## Na visão lateral o golpe se lê pela altura, não pelo lado: alto, médio e
## baixo. Os valores 0, 1 e 2 são índices das tabelas de pose abaixo.
enum Guard { HIGH, MID, LOW }

## Eventos devolvidos por tick(), na ordem em que ocorreram.
const EVENT_CUE := "cue"
const EVENT_RESOLVE := "resolve"
const EVENT_FEINT := "feint"

const ATTACK_PATTERN: Array[int] = [Guard.HIGH, Guard.LOW, Guard.MID,
	Guard.LOW, Guard.HIGH, Guard.MID]
## Ritmo levemente variado, sempre com o mesmo sinal de impacto.
const WINDUP_VARIATION: Array[float] = [0.0, 0.14, -0.06]
## Tempo de cada altura. É por aqui que os três golpes continuam
## importando depois que o parry deixou de ter direção: o corte alto é um
## movimento grande e lento, o baixo é curto e chega antes. Quem lê a
## altura sabe quanto tempo tem.
const ATTACK_TEMPO: Array[float] = [1.28, 1.0, 0.76]
## Quais ataques do padrão trazem finta, na mesma ordem de ATTACK_PATTERN.
## Fixo, não sorteado: o padrão é para ser aprendido, e um dado escondido
## transforma leitura em azar.
const FEINT_PATTERN: Array[bool] = [false, true, false, false, true, false]
## Antecedência do clarão branco e do som de aviso.
const CUE_LEAD := 0.16
## O último instante do golpe não acompanha a esquiva do jogador.
const AIM_LOCK := 0.18
const ATTACK_RANGE := 2.35
const APPROACH_SPEED := 2.25
const RECOVER_AFTER_ATTACK := 0.85

const IDLE_POSITION := Vector3(0.62, 1.30, -0.20)
const IDLE_ROTATION := Vector3(-0.15, 0, 0)
# Uma altura por guarda, bem separadas: de perfil é a altura do braço que
# entrega o golpe, e ela precisa ser legível em 320x180.
const WINDUP_POSITIONS: Array[Vector3] = [Vector3(0.10, 2.45, 0.20),
	Vector3(0.80, 1.55, 0.35), Vector3(0.60, 0.62, 0.25)]
const WINDUP_ROTATIONS: Array[Vector3] = [Vector3(1.30, 0, 0),
	Vector3(-0.15, 0, -1.45), Vector3(-3.0, 0, 0.35)]
const STUNNED_ROTATION := Vector3(0.5, 0, -1.5)
const STUNNED_TILT := -0.19
const RECOVER_POSITION := Vector3(0.05, 1.05, -0.70)
const RECOVER_ROTATION := Vector3(-2.55, 0, 0.25)
## A pose de recuperação só aparece na primeira metade dela.
const RECOVER_POSE_UNTIL := 0.60
const DEFEATED_TILT := -1.3
const DEFEATED_COLOR := Color("596a7c")

# Postura por altura do golpe. O corpo repete o que o braço diz: sobe para
# o golpe alto, agacha para o baixo. É um segundo aviso da mesma coisa, e
# é ele que se lê de longe, quando a espada ainda é um risco fino.
const WINDUP_BOB: Array[float] = [0.06, 0.0, -0.11]
const WINDUP_LEAN := -0.15
## Um pouco mais fechado no último instante, junto com o clarão.
const IMMINENT_LEAN := -0.25
const WINDUP_TWIST := -0.11
## Acompanhamento do golpe: o corpo despenca para a frente ao resolver.
const FOLLOW_LEAN := 0.42
const APPROACH_LEAN := 0.07
const STRIDE_SPEED := 9.0
const STRIDE_HEIGHT := 0.045

var body: CharacterBody3D
var combat: CombatConfig
var visual: Node3D
var weapon_pivot: Node3D
var arms: Node3D
var pose_rig := BodyPose.new()
var cue_material: StandardMaterial3D

var state: int = State.APPROACH
var state_left: float = 0.0
var incoming_guard: int = Guard.HIGH
var pattern_index: int = 0
var knockback := Vector3.ZERO
var cue_played: bool = false
## Este ataque traz finta, e se ela já foi gasta.
var feint_ready: bool = false
var feint_used: bool = false

var guard_color := Color("a9e5cf")
var attack_color := Color("f2bb66")


func setup(enemy_body: CharacterBody3D, tuning: CombatConfig) -> void:
	body = enemy_body
	combat = tuning


## Liga a máquina de estados às peças visuais do tema em uso. O material do
## sinal já chega duplicado: mudar sua cor não altera o arquivo da cena.
func attach(model: Node3D, pivot: Node3D, cue: StandardMaterial3D,
		arm_rig: Node3D = null) -> void:
	visual = model
	weapon_pivot = pivot
	arms = arm_rig
	cue_material = cue
	pose_rig.attach(model)


func apply_palette(guard: Color, attack: Color) -> void:
	guard_color = guard
	attack_color = attack


func reset(spawn: Vector3) -> void:
	state = State.APPROACH
	state_left = 0.0
	pattern_index = 0
	knockback = Vector3.ZERO
	cue_played = false
	feint_ready = false
	feint_used = false
	body.position = spawn
	body.velocity = Vector3.ZERO
	pose_rig.reset()


func is_open() -> bool:
	return state == State.STUNNED


func is_winding_up() -> bool:
	return state == State.WINDUP


func is_recovering() -> bool:
	return state == State.RECOVER


func is_imminent() -> bool:
	return state == State.WINDUP and state_left <= CUE_LEAD


func stun(duration: float) -> void:
	state = State.STUNNED
	state_left = duration


func stagger(duration: float) -> void:
	state = State.RECOVER
	state_left = duration


func defeat() -> void:
	state = State.DEFEATED


func start_attack() -> void:
	incoming_guard = ATTACK_PATTERN[pattern_index % ATTACK_PATTERN.size()]
	state_left = (combat.enemy_windup
		+ WINDUP_VARIATION[pattern_index % WINDUP_VARIATION.size()]) \
		* ATTACK_TEMPO[incoming_guard]
	feint_ready = FEINT_PATTERN[pattern_index % FEINT_PATTERN.size()]
	pattern_index += 1
	state = State.WINDUP
	cue_played = false
	feint_used = false


func tick(delta: float, target: Vector3) -> Array[String]:
	var events: Array[String] = []
	state_left -= delta
	var offset := target - body.global_position
	offset.y = 0
	var distance := offset.length()
	if distance > 0.01 and (state == State.APPROACH
			or (state == State.WINDUP and state_left > AIM_LOCK)):
		body.look_at(body.global_position + offset, Vector3.UP)
	var movement := Vector3.ZERO
	match state:
		State.APPROACH:
			if distance > ATTACK_RANGE:
				movement = offset.normalized() * APPROACH_SPEED
			else:
				start_attack()
		State.WINDUP:
			# A finta é um clarão falso: acende no instante em que o de verdade
			# acenderia e o golpe não vem. Quem apara pelo reflexo entra na
			# recuperação e come o golpe real, que chega logo depois.
			if feint_ready and not feint_used and state_left <= CUE_LEAD:
				feint_used = true
				state_left = combat.feint_hold
				events.append(EVENT_FEINT)
			elif state_left <= CUE_LEAD and not cue_played:
				cue_played = true
				events.append(EVENT_CUE)
			if state_left <= 0:
				state = State.RECOVER
				state_left = RECOVER_AFTER_ATTACK
				events.append(EVENT_RESOLVE)
		State.RECOVER, State.STUNNED:
			if state_left <= 0:
				state = State.APPROACH
		State.DEFEATED:
			pass
	body.velocity.x = movement.x + knockback.x
	body.velocity.z = movement.z + knockback.z
	body.velocity.y = -2.0 if body.is_on_floor() else body.velocity.y - 20 * delta
	body.move_and_slide()
	knockback = knockback.move_toward(Vector3.ZERO, delta * 14)
	return events


func pose(delta: float) -> void:
	var desired_pos := IDLE_POSITION
	var desired_rot := IDLE_ROTATION
	var color := attack_color
	var lean := 0.0
	var twist := 0.0
	var bob := 0.0
	var head := 0.0
	var settle := 10.0
	match state:
		State.APPROACH:
			lean = APPROACH_LEAN
			bob = pose_rig.step_bob(delta, STRIDE_SPEED, STRIDE_HEIGHT)
		State.WINDUP:
			desired_pos = WINDUP_POSITIONS[incoming_guard]
			desired_rot = WINDUP_ROTATIONS[incoming_guard]
			lean = IMMINENT_LEAN if state_left <= CUE_LEAD else WINDUP_LEAN
			# Depois da finta o corpo fica mais recolhido do que já estava. É o
			# aviso de que aquele clarão não valeu, e é ele que dá para aprender.
			if feint_used and not cue_played:
				lean -= 0.09
			twist = WINDUP_TWIST
			bob = WINDUP_BOB[incoming_guard]
			head = 0.06
			if state_left <= CUE_LEAD:
				color = Color.WHITE
		State.RECOVER:
			# A primeira metade da recuperação é o acompanhamento do corte.
			if state_left > RECOVER_POSE_UNTIL:
				desired_pos = RECOVER_POSITION
				desired_rot = RECOVER_ROTATION
				lean = FOLLOW_LEAN
				twist = 0.10
				bob = -0.05
				settle = 18.0
		State.STUNNED:
			color = guard_color
			desired_rot = STUNNED_ROTATION
			lean = STUNNED_TILT
			twist = 0.22
			head = 0.16
		State.DEFEATED:
			lean = DEFEATED_TILT
			color = DEFEATED_COLOR
			settle = 4.0
	weapon_pivot.position = weapon_pivot.position.lerp(desired_pos, minf(1, delta * 14))
	weapon_pivot.rotation = weapon_pivot.rotation.lerp(desired_rot, minf(1, delta * 18))
	pose_rig.drive(delta, lean, twist, bob, head, settle)
	ArmRig.aim(arms, weapon_pivot.global_position)
	cue_material.albedo_color = color
	cue_material.emission = color
