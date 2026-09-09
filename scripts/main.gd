extends Node3D
## APARA: duelo lateral com guarda em três alturas e parry no clique esquerdo.
## Combate: config/combat.tres. Arte: scenes/themes/. Paleta: config/themes/.
##
## Este arquivo cuida do fluxo do duelo, da troca de tema e da resolução dos
## golpes — o único ponto que conhece os dois lados ao mesmo tempo. O jogador
## está em scripts/player_controller.gd e o oponente em scripts/enemy_brain.gd.

const CombatConfig = preload("res://scripts/config/combat_tuning.gd")
const ThemeProfile = preload("res://scripts/config/duel_theme.gd")
const THEME_PATHS: Array[String] = ["res://config/themes/samurai.tres",
	"res://config/themes/medieval.tres"]
const Hud = preload("res://scripts/hud.gd")
const Feedback = preload("res://scripts/feedback.gd")
const PlayerController = preload("res://scripts/player_controller.gd")
const EnemyBrain = preload("res://scripts/enemy_brain.gd")

var guard_color := Color("a9e5cf")
var attack_color := Color("f2bb66")
var damage_color := Color("ef7670")
# O duelo corre no eixo X, que é o eixo da tela. A câmera olha na horizontal
# ao longo de -Z: assim toda superfície deitada vira linha e a cena lê como
# elevação 2D em vez de perspectiva.
const PLAYER_SPAWN := Vector3(-2.6, 0.05, 0)
const ENEMY_SPAWN := Vector3(4.0, 0.05, 0)
const DIRECTION_NAMES: Array[String] = ["ALTO", "MÉDIO", "BAIXO"]
## O que cada altura custa em tempo, para a interface dizer o que a pose
## já diz. O parry não tem direção: a altura virou relógio.
const TEMPO_NAMES: Array[String] = ["LENTO", "MÉDIO", "RÁPIDO"]
## Meia altura visível da câmera ortográfica, em unidades do mundo.
const CAMERA_SIZE := 4.2
const CAMERA_PUNCH := 0.18
const CAMERA_HEIGHT := 1.35
## Até onde o enquadramento acompanha os lutadores antes de encostar na
## borda. Precisa casar com as paredes da arena: o limite mais a meia
## largura visível tem de alcançar a parede, senão o duelo sai de quadro.
const CAMERA_LIMIT := 4.5
## Três quartos: o corpo encara o adversário, o volume gira para a câmera.
## De perfil puro somem peito, ombreiras e olhos — some o personagem.
const VISUAL_YAW := 0.5
## Alcance de cada teste de acerto. As malhas das armas são visuais.
const ENEMY_REACH := 3.0
const COUNTER_REACH := 3.35

@export var combat: CombatConfig
@export var duel_theme: ThemeProfile

@export_group("Aparência pixel art")
## Divisor da resolução do mundo 3D. 4 num quadro de 1280x720 renderiza a
## 320x180 e amplia sem suavização. 1 desliga o efeito.
@export_range(1, 8, 1) var pixel_shrink: int = 4
## Degraus de cor por canal. Usado só quando o tema não traz paleta.
@export_range(2.0, 32.0, 1.0) var color_levels: float = 12.0
## Pontilhado ordenado nas transições entre duas cores da paleta. 0 desliga.
@export_range(0.0, 1.0, 0.05) var dither_strength: float = 0.85

@export_group("Gradação")
@export_range(0.1, 3.0, 0.05) var exposure: float = 1.0
## Acima de 1 abre a sombra e estoura a luz. É o que separa a silhueta.
@export_range(0.5, 3.0, 0.05) var contrast: float = 1.28
@export_range(0.0, 2.0, 0.05) var saturation: float = 1.3

@export_group("Efeitos de tela")
## Vazamento das áreas mais claras para os vizinhos, no pixel pequeno.
@export_range(0.0, 2.0, 0.05) var bloom_strength: float = 1.15
@export_range(0.0, 1.0, 0.05) var bloom_threshold: float = 0.63
## Raio do halo, em pixels da arte. Separa contorno aceso de ar aceso.
@export_range(1.0, 8.0, 0.1) var bloom_radius: float = 3.2
## Luz neon é colorida; sem isto o halo cresce branco e lava a cena.
@export_range(0.0, 3.0, 0.05) var bloom_saturation: float = 1.7
## Separação de vermelho e azul a partir do centro, em pixels da arte.
@export_range(0.0, 3.0, 0.1) var aberration: float = 1.0
@export_range(0.0, 1.0, 0.05) var scanline_strength: float = 0.1
@export_range(0.0, 1.0, 0.05) var vignette_strength: float = 0.45

var pixel_view: SubViewportContainer
var stage: Node3D
var effects: Node3D
var visual_mount: Node3D
var player_mount: Node3D
var camera_rig: Node3D
var hud: Hud
var feedback: Feedback
var player: CharacterBody3D
var enemy: CharacterBody3D
var camera: Camera3D
var weapon: Node3D
var control: PlayerController
var brain: EnemyBrain

var started: bool = false
var paused: bool = false
var finished: bool = false
var health: int = 5
var enemy_health: int = 5
## Defesas jogadas fora que ainda cabem antes de custar vida.
var steadiness: int = 3
var parries: int = 0
var perfect_parries: int = 0
var combo: int = 0
var best_combo: int = 0
var hitstop_left: float = 0.0
var input_grace: float = 0.0
var message_left: float = 0.0
var end_delay: float = 0.0
var clock: float = 0.0
var shake: float = 0.0
var flash_alpha: float = 0.0
var flash_color := guard_color
var end_menu_shown: bool = false


func _ready() -> void:
	_setup_inputs()
	if combat == null:
		combat = CombatConfig.new()
	_bind_scene_nodes()
	control = PlayerController.new()
	control.setup(player, combat)
	brain = EnemyBrain.new()
	brain.setup(enemy, combat)
	hud = Hud.new()
	add_child(hud)
	_apply_pixel_look()
	feedback = Feedback.new()
	# As faíscas são malhas 3D: precisam nascer dentro do SubViewport, senão
	# ficam num mundo sem câmera e não aparecem.
	effects.add_child(feedback)
	feedback.set_master_volume(combat.sound_volume_db)
	for theme_path in THEME_PATHS:
		var profile := load(theme_path) as ThemeProfile
		hud.theme_picker.add_item("%s · %s" % [profile.theme_name, profile.stage_name])
	hud.primary_pressed.connect(_on_menu_button)
	hud.theme_selected.connect(_on_theme_selected)
	if not _apply_theme(duel_theme):
		hud.menu_button.disabled = true
		return
	health = combat.player_max_health
	enemy_health = combat.enemy_max_health
	steadiness = combat.steadiness
	control.aim_at(enemy)
	_refresh_hud()
	hud.show_menu("APARA", "Um botão só: ESPAÇO ou o clique esquerdo aparam. Não há direção a escolher — só o instante.\n\n"
		+ "A luz dourada diz de que altura vem o golpe, e a altura diz o tempo: alto é lento, baixo chega antes. Apare no clarão branco.\n\n"
		+ "Cuidado com a finta: às vezes o clarão vem e o golpe não. Quem apara nela fica na recuperação e come o golpe de verdade.\n\n"
		+ "Aparou? Clique DIREITO para contra-atacar. A e D aproximam e afastam. ESC pausa.", "COMEÇAR DUELO")
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE


func _setup_inputs() -> void:
	var keys := {"move_left": KEY_A, "move_right": KEY_D,
		"pause_duel": KEY_ESCAPE, "restart_duel": KEY_R}
	for action in keys:
		if not InputMap.has_action(action):
			InputMap.add_action(action)
			var key := InputEventKey.new()
			key.physical_keycode = keys[action]
			InputMap.action_add_event(action, key)
	var buttons := {"parry": MOUSE_BUTTON_LEFT, "counter": MOUSE_BUTTON_RIGHT}
	for action in buttons:
		if not InputMap.has_action(action):
			InputMap.add_action(action)
			var button := InputEventMouseButton.new()
			button.button_index = buttons[action]
			InputMap.action_add_event(action, button)
	# Um botão só para aparar, e o espaço faz o mesmo. O parry deixou de ter
	# direção: sobrou uma decisão, o instante.
	if not InputMap.action_get_events("parry").any(
			func(e): return e is InputEventKey):
		var space := InputEventKey.new()
		space.physical_keycode = KEY_SPACE
		InputMap.action_add_event("parry", space)


func _bind_scene_nodes() -> void:
	# Todo o mundo 3D vive dentro do SubViewport: é ele que é renderizado
	# pequeno. A HUD fica fora, na resolução da janela.
	pixel_view = $Pixelate
	stage = $Pixelate/Viewport/Stage
	effects = $Pixelate/Viewport/Effects
	player = $Pixelate/Viewport/Player
	enemy = $Pixelate/Viewport/Sentinel
	camera_rig = $Pixelate/Viewport/CameraRig
	camera = $Pixelate/Viewport/CameraRig/Camera3D
	player_mount = $Pixelate/Viewport/Player/VisualMount
	visual_mount = $Pixelate/Viewport/Sentinel/VisualMount


func _apply_pixel_look() -> void:
	pixel_view.stretch_shrink = pixel_shrink
	if hud != null:
		# O indicador de guarda usa o mesmo tamanho de bloco do mundo.
		hud.guard_indicator.cell = float(pixel_shrink)
		hud.guard_indicator.queue_redraw()
	var material := pixel_view.material as ShaderMaterial
	if material == null:
		return
	material.set_shader_parameter("color_levels", color_levels)
	material.set_shader_parameter("dither_strength", dither_strength)
	material.set_shader_parameter("exposure", exposure)
	material.set_shader_parameter("contrast", contrast)
	material.set_shader_parameter("saturation", saturation)
	material.set_shader_parameter("bloom_strength", bloom_strength)
	material.set_shader_parameter("bloom_threshold", bloom_threshold)
	material.set_shader_parameter("bloom_radius", bloom_radius)
	material.set_shader_parameter("bloom_saturation", bloom_saturation)
	material.set_shader_parameter("aberration", aberration)
	material.set_shader_parameter("scanline_strength", scanline_strength)
	material.set_shader_parameter("vignette_strength", vignette_strength)


## Monta a paleta do tema como uma textura de uma linha e passa o duotone.
## Lista vazia devolve o shader ao modo de degraus, sem paleta.
func _apply_palette(profile: ThemeProfile) -> void:
	var material := pixel_view.material as ShaderMaterial
	if material == null:
		return
	material.set_shader_parameter("shadow_tint", profile.shadow_tint)
	material.set_shader_parameter("highlight_tint", profile.highlight_tint)
	var colors := profile.palette
	if colors.is_empty():
		material.set_shader_parameter("palette_size", 0)
		return
	var image := Image.create_empty(colors.size(), 1, false, Image.FORMAT_RGB8)
	for i in colors.size():
		image.set_pixel(i, 0, colors[i])
	material.set_shader_parameter("palette_tex", ImageTexture.create_from_image(image))
	material.set_shader_parameter("palette_size", colors.size())


func _apply_theme(next_theme: ThemeProfile) -> bool:
	# Valida os pontos de ligação antes de substituir as instâncias em uso.
	if next_theme == null or next_theme.arena_scene == null or next_theme.opponent_scene == null or next_theme.duelist_scene == null:
		push_error("DuelTheme precisa das cenas de arena, oponente e jogador.")
		return false
	var world := next_theme.arena_scene.instantiate() as Node3D
	var model := next_theme.opponent_scene.instantiate() as Node3D
	var duelist := next_theme.duelist_scene.instantiate() as Node3D
	if world == null or model == null or duelist == null:
		push_error("As cenas do tema precisam de raízes Node3D.")
		for candidate in [world, model, duelist]:
			if candidate != null:
				candidate.free()
		return false
	var pivot := model.get_node_or_null(next_theme.weapon_pivot_path) as Node3D
	var cue := model.get_node_or_null(next_theme.attack_cue_path) as MeshInstance3D
	var player_pivot := duelist.get_node_or_null(next_theme.weapon_pivot_path) as Node3D
	if pivot == null or cue == null or player_pivot == null or not (cue.material_override is StandardMaterial3D):
		push_error("Oponente e jogador precisam de WeaponPivot, e o oponente de um AttackCue com material StandardMaterial3D. Confira os caminhos no tema.")
		world.free()
		model.free()
		duelist.free()
		return false
	for container in [stage, visual_mount, player_mount]:
		for child in container.get_children():
			container.remove_child(child)
			child.queue_free()
	world.name = "World"
	model.name = "Visual"
	duelist.name = "Visual"
	stage.add_child(world)
	visual_mount.add_child(model)
	player_mount.add_child(duelist)
	weapon = player_pivot
	control.attach(player_pivot, duelist.get_node_or_null(next_theme.arms_path) as Node3D,
		duelist)
	# O sinal de impacto não modifica o material compartilhado pelo arquivo da cena.
	brain.attach(model, pivot, cue.material_override.duplicate() as StandardMaterial3D,
		model.get_node_or_null(next_theme.arms_path) as Node3D)
	cue.material_override = brain.cue_material
	duel_theme = next_theme
	guard_color = next_theme.guard_color
	attack_color = next_theme.attack_color
	damage_color = next_theme.damage_color
	brain.apply_palette(guard_color, attack_color)
	_apply_palette(next_theme)
	hud.apply_theme(next_theme)
	var selected := THEME_PATHS.find(next_theme.resource_path)
	hud.theme_picker.select(selected)
	camera.current = true
	feedback.reset()
	return true


func _on_theme_selected(index: int) -> void:
	# A troca de arte acontece entre duelos; não altera uma luta em andamento.
	if started and not finished:
		return
	if index < 0 or index >= THEME_PATHS.size():
		return
	var selected := load(THEME_PATHS[index]) as ThemeProfile
	if _apply_theme(selected):
		hud.menu_button.disabled = false
		control.aim_at(enemy)
		_refresh_hud()


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("restart_duel") and started:
		_start_duel()
		return
	if event.is_action_pressed("pause_duel") and started and not finished:
		_set_paused(not paused)
		return
	if not started or paused or finished or input_grace > 0:
		return


func _notification(what: int) -> void:
	if what == NOTIFICATION_APPLICATION_FOCUS_OUT and started and not finished:
		_set_paused(true)


func _on_menu_button() -> void:
	if not started or finished:
		_start_duel()
	else:
		_set_paused(false)


func _start_duel() -> void:
	started = true
	paused = false
	finished = false
	end_menu_shown = false
	health = combat.player_max_health
	enemy_health = combat.enemy_max_health
	steadiness = combat.steadiness
	parries = 0
	perfect_parries = 0
	combo = 0
	best_combo = 0
	hitstop_left = 0
	shake = 0
	flash_alpha = 0
	input_grace = 0.2
	control.reset(PLAYER_SPAWN)
	brain.reset(ENEMY_SPAWN)
	camera.size = CAMERA_SIZE
	camera.h_offset = 0
	camera.v_offset = 0
	_frame_camera(true)
	feedback.reset()
	feedback.pause_audio(false)
	hud.hide_menu()
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
	control.aim_at(enemy)
	_message("Leia a altura, espere o clarão, apare. Nem todo clarão é golpe.", guard_color, 3.5)
	_refresh_hud()


func _set_paused(value: bool) -> void:
	paused = value
	feedback.pause_audio(value)
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE if value else Input.MOUSE_MODE_CAPTURED
	control.clear_counter()
	if value:
		hud.theme_picker.hide()
		hud.show_menu("PAUSADO", "O duelo está congelado.\n\n"
			+ "Espaço ou esquerdo aparam. Direito contra-ataca.\n\nR recomeça.", "CONTINUAR")
	else:
		# O clique no botão Continuar não vira uma tentativa de parry.
		input_grace = 0.18
		hud.hide_menu()


func _physics_process(delta: float) -> void:
	if not started or paused:
		return
	clock += delta
	_tick_effects(delta)
	_frame_camera()
	if finished:
		hitstop_left = maxf(0, hitstop_left - delta)
		_pose_actors(delta)
		end_delay -= delta
		if end_delay <= 0 and not end_menu_shown:
			_show_result()
		return
	if input_grace > 0:
		# Também congela o golpe ao retomar; o menu não cria um acerto inevitável.
		input_grace = maxf(0, input_grace - delta)
		_refresh_hud()
		return
	if Input.is_action_just_pressed("counter"):
		control.buffer_counter()
	# Pausa só os atores por alguns milissegundos; áudio e faíscas continuam.
	# Nenhum Engine.time_scale global é alterado.
	if hitstop_left > 0:
		hitstop_left = maxf(0, hitstop_left - delta)
		_pose_actors(delta * 0.2)
		_refresh_hud()
		return
	control.tick_timers(delta)
	if control.take_wasted():
		_on_wasted_parry()
	if Input.is_action_just_pressed("parry"):
		_try_parry()
	control.move(delta)
	_tick_enemy(delta)
	if control.counter_buffer > 0 and hitstop_left <= 0 and not finished:
		_try_counter()
	control.aim_at(enemy)
	_pose_actors(delta)
	_refresh_hud()


func _tick_enemy(delta: float) -> void:
	for event in brain.tick(delta, player.global_position):
		match event:
			EnemyBrain.EVENT_CUE:
				feedback.play("cue")
			EnemyBrain.EVENT_FEINT:
				_on_feint()
			EnemyBrain.EVENT_RESOLVE:
				_resolve_enemy_hit()


## O clarão falso. Nada de texto: o aviso é o corpo do oponente recolhendo
## em vez de comprometer, mais o som que começa e não termina. Anunciar
## "FINTA" por escrito entregava de graça o que a finta cobra para ensinar.
func _on_feint() -> void:
	brain.pose_rig.punch(-0.24)
	feedback.play("feint")


## Janela fechada sem aparar nada. Custa firmeza; firmeza no fim custa vida.
## É o que impede o botão de virar metralhadora: sem preço para errar, a
## finta não cobra nada e a leitura vira enfeite.
func _on_wasted_parry() -> void:
	if finished:
		return
	steadiness -= 1
	control.punch(-0.14)
	if steadiness > 0:
		feedback.play("armor", 1.15)
		_refresh_hud()
		return
	steadiness = combat.steadiness
	health = maxi(0, health - 1)
	combo = 0
	_message("FIRMEZA QUEBRADA", damage_color, 1.0)
	_impact(damage_color, 0.04, 0.6, 0.09)
	feedback.play("hurt", 1.1)
	if health == 0:
		_finish(false)
	_refresh_hud()


func _pose_actors(delta: float) -> void:
	control.pose_weapon(delta)
	brain.pose(delta)
	_face_camera(player_mount, player)
	_face_camera(visual_mount, enemy)


## O corpo encara o adversário para a lógica do golpe; o volume gira um pouco
## para a câmera. O sinal segue quem está de que lado, senão quem passa para
## trás do outro aparece de costas.
func _face_camera(mount: Node3D, actor: Node3D) -> void:
	if mount == null:
		return
	mount.rotation.y = -VISUAL_YAW * signf((-actor.global_basis.z).x)


## A câmera acompanha o meio do duelo, dentro de uma faixa. Sem isso os dois
## saem de quadro assim que o oponente é empurrado.
func _frame_camera(snap: bool = false) -> void:
	var middle := (player.global_position.x + enemy.global_position.x) * 0.5
	var target := clampf(middle, -CAMERA_LIMIT, CAMERA_LIMIT)
	camera_rig.position.x = target if snap else lerpf(camera_rig.position.x, target, 0.12)
	camera_rig.position.y = CAMERA_HEIGHT


func _try_parry() -> void:
	if not control.try_parry():
		return
	# A pose de aparo segue o golpe que está vindo. O jogador não escolhe a
	# altura, mas a lâmina dele sobe ou desce junto com a do oponente — sem
	# isso o aparo de um corte alto seria feito com a espada na cintura.
	if brain.is_winding_up():
		control.deflect_height = brain.incoming_guard
	feedback.play("swing", 1.2)


func _resolve_enemy_hit() -> void:
	# Um teste de alcance + arco por golpe. As malhas da espada são visuais.
	if _flat_distance() > ENEMY_REACH or not _facing(enemy, player, 0.3):
		_message("ESQUIVOU", Color("adbdcf"), 0.6)
		feedback.play("swing")
		return
	if control.parries() and _facing(player, enemy, 0.4):
		_on_parry_success(control.parry_age <= minf(combat.perfect_window, combat.parry_window))
		return
	health = maxi(0, health - 1)
	combo = 0
	# Estar na recuperação quando o golpe chega quer dizer que a defesa foi
	# gasta antes da hora — quase sempre numa finta. Dizer isso ensina.
	var too_early := control.cooldown_left > 0.0 and not control.is_parrying()
	_message("CEDO DEMAIS" if too_early else "GOLPE RECEBIDO", damage_color, 1.0)
	control.consume_parry()
	control.punch(-0.38)
	control.knockback = _away_from(enemy, player) * 3.5
	_impact(damage_color, 0.045, 0.7, 0.10)
	feedback.play("hurt")
	if health == 0:
		_finish(false)


func _on_parry_success(perfect: bool) -> void:
	control.consume_parry()  # Uma tentativa só pode aparar um golpe.
	parries += 1
	combo += 1
	best_combo = maxi(best_combo, combo)
	if perfect:
		perfect_parries += 1
	brain.stun(combat.stun_duration)
	brain.pose_rig.punch(-0.30 if perfect else -0.20)
	control.punch(0.16)
	brain.knockback = _away_from(player, enemy) * (3.2 if perfect else 2.2)
	var color := attack_color if perfect else guard_color
	# Sem dizer se foi perfeito. A recompensa está no som — o clangor do
	# perfeito é outro — e no impacto mais forte. Sentir vale mais do que ler,
	# e ler tira os olhos do oponente bem quando ele volta a atacar.
	_message("DIREITO: CONTRA-ATAQUE   ×%d" % combo, guard_color, 1.1)
	_impact(color, combat.perfect_hitstop if perfect else combat.parry_hitstop, 0.8 if perfect else 0.55, 0.065)
	feedback.play("perfect" if perfect else "parry", 1.0 + minf(combo * 0.025, 0.15))
	feedback.burst(_contact_point(), color, 28 if perfect else 18)
	camera.size = CAMERA_SIZE - (CAMERA_PUNCH if perfect else CAMERA_PUNCH * 0.6)


func _try_counter() -> void:
	if control.attack_left > 0 or control.is_parrying():
		return
	control.begin_counter()
	feedback.play("swing", 0.85)
	if _flat_distance() > COUNTER_REACH or not _facing(player, enemy, 0.55):
		_message("CHEGUE MAIS PERTO", Color("adbdcf"), 0.5)
		return
	if not brain.is_open():
		_message("ARMADURA — APARE PRIMEIRO", attack_color, 0.75)
		feedback.play("armor")
		feedback.burst(_contact_point(), Color("c6d0dd"), 6)
		return
	enemy_health = maxi(0, enemy_health - 1)
	brain.stagger(1.0)
	brain.pose_rig.punch(-0.34)
	brain.knockback = _away_from(player, enemy) * 4
	_message("CONTRA-ATAQUE!", guard_color, 0.75)
	_impact(guard_color, 0.06, 0.6, 0.035)
	feedback.play("hit")
	feedback.burst(enemy.global_position + Vector3.UP * 1.4, attack_color, 14)
	if enemy_health == 0:
		_finish(true)


func _flat_distance() -> float:
	var delta := player.global_position - enemy.global_position
	delta.y = 0
	return delta.length()


func _facing(source: Node3D, target: Node3D, minimum_dot: float) -> bool:
	var direction := target.global_position - source.global_position
	direction.y = 0
	return direction.length_squared() > 0.001 and (
		-source.global_basis.z).dot(direction.normalized()) >= minimum_dot


func _away_from(source: Node3D, target: Node3D) -> Vector3:
	var offset := target.global_position - source.global_position
	offset.y = 0
	return offset.normalized()


func _contact_point() -> Vector3:
	return player.global_position.lerp(enemy.global_position, 0.5) + Vector3.UP * 1.35


func _impact(color: Color, stop: float, amount: float, alpha: float) -> void:
	hitstop_left = stop
	shake = maxf(shake, amount)
	flash_color = color
	flash_alpha = alpha if combat.screen_flash_enabled else 0.0


func _message(text: String, color: Color, duration: float) -> void:
	hud.feedback_label.text = text
	hud.feedback_label.modulate = color
	message_left = duration


func _tick_effects(delta: float) -> void:
	feedback.tick(delta * (0.2 if hitstop_left > 0 else 1.0))
	message_left = maxf(0, message_left - delta)
	if message_left <= 0:
		hud.feedback_label.text = ""
	shake = move_toward(shake, 0.0, delta * 3.5)
	camera.h_offset = sin(clock * 173) * shake * 0.12 * combat.camera_shake
	camera.v_offset = cos(clock * 137) * shake * 0.08 * combat.camera_shake
	camera.size = lerpf(camera.size, CAMERA_SIZE, minf(1, delta * 12))
	flash_alpha = move_toward(flash_alpha, 0, delta * 0.7)
	hud.flash.color = Color(flash_color, flash_alpha)


func _refresh_hud() -> void:
	var tell := "LEIA O GOLPE"
	var color := Color("b6cbd7")
	var incoming := -1
	var imminent := false
	if brain.is_winding_up():
		incoming = brain.incoming_guard
		imminent = brain.is_imminent()
		# A altura virou relógio: dizer o tempo junto com o nome é o que
		# transforma a leitura em decisão.
		tell = ("APARE!" if imminent else "GOLPE:  %s · %s" % [
			DIRECTION_NAMES[incoming], TEMPO_NAMES[incoming]])
		color = Color.WHITE if imminent else attack_color
	elif brain.is_open():
		tell = "ABERTO — BOTÃO DIREITO"
		color = guard_color
	elif brain.is_recovering():
		tell = "PREPARE A PRÓXIMA GUARDA"
	if finished:
		tell = "VITÓRIA" if enemy_health == 0 else "FIM DO DUELO"
	var active := control.is_parrying()
	hud.refresh(health, enemy_health, parries, control.readiness(), tell, color,
		active, steadiness, combat.steadiness)
	hud.update_guard(incoming, imminent, active)


func _finish(won: bool) -> void:
	finished = true
	end_delay = 1.1
	control.consume_parry()
	control.clear_counter()
	if won:
		brain.defeat()
		feedback.burst(enemy.global_position + Vector3.UP * 1.5, attack_color, 32)
		_message(duel_theme.opponent_name + " DERROTADO", attack_color, 2.0)
	else:
		_message("LEVANTE. TENTE DE NOVO.", damage_color, 2.0)
	_refresh_hud()


func _show_result() -> void:
	end_menu_shown = true
	hud.theme_picker.show()
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	var stats := "%d parries  •  %d perfeitos  •  sequência máxima %d\n\n" % [
		parries, perfect_parries, best_combo]
	var tip := "Você dominou o ritmo. Experimente acertar mais parries perfeitos."
	if health == 0:
		tip = ("Alto é lento, baixo chega antes. Apare no clarão branco. "
			+ "Segurar o botão não repete o parry.")
	hud.show_menu("VITÓRIA" if enemy_health == 0 else "MAIS UMA?", stats + tip, "JOGAR DE NOVO")
