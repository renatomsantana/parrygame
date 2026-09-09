extends SceneTree
## Teste headless do duelo. Roda sem janela e sem áudio real:
##   godot --headless --path . --script res://tests/smoke_test.gd
## Chama diretamente a lógica do jogo para não depender de tempo de quadro.
## Sai com código 1 se qualquer verificação falhar.

# Espelham os enums de enemy_brain.gd e player_controller.gd. Valores literais
# para não depender de acesso a constantes por instância.
const APPROACH := 0
const WINDUP := 1
const RECOVER := 2
const STUNNED := 3
const DEFEATED := 4
const HIGH := 0
const MID := 1
const LOW := 2

var failures: Array[String] = []
var game: Node3D
var frame: int = 0
var checks: int = 0


func _process(_delta: float) -> bool:
	# A cena só entra depois que a raiz existe: dentro de _initialize o _ready
	# do jogo não dispararia e a HUD viria nula.
	frame += 1
	if frame == 1:
		var packed := load("res://main.tscn") as PackedScene
		if packed == null:
			print("FALHA CRÍTICA: main.tscn não carregou")
			quit(1)
			return true
		game = packed.instantiate() as Node3D
		root.add_child(game)
		return false
	if frame == 2:
		_run_tests()
		return false
	if frame == 8:
		# Alguns quadros entre liberar a cena e sair: é neles que o servidor de
		# áudio solta as reproduções paradas. Sem essa folga o Godot acusa
		# vazamento, de forma intermitente.
		_report()
		return true
	return false


func _run_tests() -> void:
	_section("Estado inicial")
	_check(game.hud != null, "HUD criada")
	_check(game.feedback != null, "Feedback criado")
	_check(game.control != null, "controlador do jogador criado")
	_check(game.brain != null, "máquina de estados do oponente criada")
	_check(game.feedback.players.size() == game.feedback.KINDS.size(),
		"%d sons sintetizados" % game.feedback.KINDS.size())
	_check(game.brain.visual != null, "modelo do oponente instanciado")
	_check(game.brain.weapon_pivot != null, "WeaponPivot localizado")
	_check(game.brain.cue_material != null, "AttackCue com material próprio")
	_check(game.hud.theme_picker.item_count == 2, "seletor com 2 temas")
	_check(game.hud.menu.visible, "menu inicial visível")

	_section("Acabamento pixel art")
	var pixel: SubViewportContainer = game.pixel_view
	var viewport: SubViewport = pixel.get_child(0)
	var material: ShaderMaterial = pixel.material
	_check(material != null, "material do pixel no SubViewportContainer")
	_check(pixel.texture_filter == CanvasItem.TEXTURE_FILTER_NEAREST,
		"ampliação sem suavização")
	_check(pixel.stretch_shrink == game.pixel_shrink, "divisor de resolução aplicado")
	_check(viewport.size * game.pixel_shrink == Vector2i(pixel.size),
		"o mundo é desenhado em %s" % viewport.size)
	# Este é o erro que a mudança para SubViewport criou uma vez: fora dele as
	# faíscas caem num mundo sem câmera e não aparecem.
	_check(game.feedback.get_viewport() == viewport,
		"as faíscas nascem dentro do SubViewport")
	_check(game.hud.get_viewport() != viewport,
		"a HUD fica fora, na resolução da janela")
	_check(game.hud.guard_indicator.cell == float(game.pixel_shrink),
		"indicador de guarda usa o mesmo bloco do mundo")
	_check(game.duel_theme.palette.size() > 0, "o tema samurai traz uma paleta")
	_check(material.get_shader_parameter("palette_size") == game.duel_theme.palette.size(),
		"paleta do tema carregada no shader (%d cores)" % game.duel_theme.palette.size())

	_section("Enquadramento lateral")
	var camera: Camera3D = game.camera
	var limits: Dictionary = game.get_script().get_script_constant_map()
	_check(camera.projection == Camera3D.PROJECTION_ORTHOGONAL,
		"câmera ortográfica")
	# Sem inclinação nenhuma: é o que faz toda superfície deitada virar linha
	# e a cena ler como elevação 2D em vez de perspectiva.
	_check(camera.global_transform.basis.is_equal_approx(Basis.IDENTITY),
		"câmera olha na horizontal, sem inclinação")
	var aspect := float(viewport.size.x) / float(viewport.size.y)
	var half_width: float = camera.size * 0.5 * aspect
	var wall: Node3D = game.stage.get_node_or_null("World/WallR")
	_check(wall != null, "parede direita da arena localizada")
	# O palco não pode ser mais largo do que a câmera alcança: com a câmera
	# presa no limite, um lutador encostado na parede tem de continuar em
	# quadro. Foi um palco de 13 unidades contra um limite de 6,5 que fez os
	# dois sumirem da tela.
	if wall != null:
		_check(limits["CAMERA_LIMIT"] + half_width >= wall.position.x,
			"a câmera alcança a parede do palco")
	# A câmera fica a 9 unidades do duelo. Névoa que comece antes disso
	# engole os próprios lutadores — foi o que aconteceu ao herdar a névoa
	# curta da versão em primeira pessoa.
	var world_env: WorldEnvironment = game.stage.get_node_or_null("World/WorldEnvironment")
	_check(world_env != null, "WorldEnvironment da arena localizado")
	if world_env != null:
		_check(world_env.environment.fog_depth_begin > camera.position.z,
			"a névoa começa depois do duelo")
	_check(game.player.position.x < game.enemy.position.x,
		"jogador à esquerda e oponente à direita no palco")

	_section("Parry sem direção")
	var tempo: Array = _brain_const(game, "ATTACK_TEMPO")
	_check(tempo.size() == 3, "um tempo por altura de golpe")
	# O corte alto é um movimento grande e lento; o baixo é curto e chega
	# antes. É por aqui que as três alturas continuam importando depois que
	# o parry deixou de ter direção.
	_check(float(tempo[HIGH]) > float(tempo[MID])
		and float(tempo[MID]) > float(tempo[LOW]),
		"alto é o golpe mais lento e baixo o mais rápido")
	game.control.cooldown_left = 0.0
	game.control.parry_left = 0.0
	_check(not game.control.parries(), "fora da janela não apara")
	_check(game.control.try_parry(), "o botão abre a janela")
	_check(game.control.parries(), "dentro da janela apara, venha de onde vier")
	_check(not game.control.try_parry(), "segurar o botão não renova a defesa")
	game.control.consume_parry()
	game.control.cooldown_left = 0.0
	_check(InputMap.action_get_events("parry").any(
			func(e): return e is InputEventKey),
		"aparar também pelo teclado, não só pelo clique")

	_section("Firmeza")
	game._start_duel()
	_engage()
	var firm: int = game.steadiness
	_check(firm == game.combat.steadiness, "firmeza cheia no começo do duelo")
	# Janela aberta e fechada sem aparar nada: é o botão apertado à toa.
	game.control.cooldown_left = 0.0
	game.control.try_parry()
	game.control.tick_timers(game.combat.parry_window + 0.01)
	_check(game.control.take_wasted(), "a janela perdida se anuncia uma vez")
	_check(not game.control.take_wasted(), "e só uma vez")
	# Aparar alguma coisa não gasta firmeza.
	game.control.cooldown_left = 0.0
	game.control.try_parry()
	game.control.consume_parry()
	game.control.tick_timers(0.3)
	_check(not game.control.take_wasted(), "janela que aparou não conta como perdida")
	var firm_before: int = game.steadiness
	game._on_wasted_parry()
	_check(game.steadiness == firm_before - 1, "defesa jogada fora custa firmeza")
	# Zerar a firmeza custa vida e recomeça a conta: o duelo não acaba por
	# causa dela, mas errar sai caro.
	var hp_before: int = game.health
	game.steadiness = 1
	game._on_wasted_parry()
	_check(game.health == hp_before - 1, "firmeza zerada custa um ponto de vida")
	_check(game.steadiness == game.combat.steadiness, "a firmeza volta cheia depois de quebrar")

	_section("Finta")
	var feints: Array = _brain_const(game, "FEINT_PATTERN")
	_check(feints.size() == 6 and feints.has(true) and feints.has(false),
		"o padrão de fintas é fixo e mistura golpes com e sem")
	# A finta precisa segurar o golpe além da recuperação do parry: quem cai
	# nela paga, mas quem se recompõe a tempo ainda tem defesa. Sem essa
	# folga a finta vira punição garantida em vez de armadilha.
	_check(game.combat.feint_hold > game.control.cooldown_window() - 0.16,
		"a finta segura o golpe além da recuperação do parry")
	game._start_duel()
	_engage()
	game.brain.state = WINDUP
	game.brain.incoming_guard = MID
	game.brain.feint_ready = true
	game.brain.feint_used = false
	game.brain.cue_played = false
	game.brain.state_left = 0.05
	var events: Array[String] = game.brain.tick(0.0, game.player.global_position)
	var feint_event: String = _brain_const(game, "EVENT_FEINT")
	_check(events.has(feint_event), "o clarão falso avisa a finta")
	_check(not events.has("cue"), "a finta não dispara o aviso verdadeiro")
	_check(game.brain.state == WINDUP, "o golpe continua preparado depois da finta")
	_check(game.brain.state_left > 0.3, "o oponente segura o golpe")
	# Uma finta por ataque: senão o oponente nunca chegaria a golpear.
	game.brain.state_left = 0.05
	events = game.brain.tick(0.0, game.player.global_position)
	_check(not events.has(feint_event), "a finta não se repete no mesmo golpe")
	_check(events.has("cue"), "depois da finta vem o aviso de verdade")

	_section("Som")
	var mix: Dictionary = game.feedback.MIX_DB
	for kind in game.feedback.players:
		var stream: AudioStream = game.feedback.players[kind].stream
		_check(stream != null and stream.get_length() > 0.0,
			"som '%s' com duração" % kind)
		_check(mix.has(kind), "som '%s' tem posição na mistura" % kind)
	# Metal vive acima de 8 kHz. A 22 kHz o clangor perde o ar e sobra um
	# toque de telefone.
	_check(game.feedback.SAMPLE_RATE >= 44100, "áudio sintetizado a 44,1 kHz")
	# O aviso não pode ter o mesmo peso do impacto: se tudo é igualmente
	# importante, nada chama atenção.
	_check(float(mix["cue"]) < float(mix["parry"]),
		"o aviso fica abaixo do impacto na mistura")
	_check(float(mix["perfect"]) > float(mix["parry"]),
		"o parry perfeito se destaca do comum")
	var master: float = game.combat.sound_volume_db
	_check(is_equal_approx(game.feedback.players["parry"].volume_db,
		master + float(mix["parry"])), "volume geral somado à mistura de cada som")
	# Um arquivo em art/audio/ substitui o sintetizado do mesmo nome. Tirando
	# o arquivo, o som sintetizado tem de voltar sozinho — nada do que o jogo
	# gera pode depender de um arquivo de terceiros.
	var synth: AudioStream = game.feedback._sound("parry")
	_check(synth != null and synth.data.size() > 0,
		"a síntese continua de pé mesmo com som de referência na pasta")

	_section("Parry perfeito")
	game._start_duel()
	_check(game.started and not game.hud.menu.visible, "duelo começou e menu sumiu")
	_engage()
	game.brain.incoming_guard = HIGH
	game.brain.state = WINDUP
	game._try_parry()
	_check(game.control.parry_left > 0, "janela de parry aberta")
	game.control.parry_age = 0.0
	game._resolve_enemy_hit()
	_check(game.brain.state == STUNNED, "oponente atordoado")
	_check(game.parries == 1, "1 parry contabilizado")
	_check(game.perfect_parries == 1, "reconhecido como perfeito")
	_check(game.combo == 1, "sequência em 1")
	_check(game.control.parry_left == 0.0, "janela consumida por um golpe só")

	_section("Parry normal")
	game.control.cooldown_left = 0.0
	game.brain.state = WINDUP
	game.brain.incoming_guard = LOW
	game._try_parry()
	game.control.parry_age = game.combat.perfect_window + 0.05
	game._resolve_enemy_hit()
	_check(game.parries == 2, "2 parries")
	_check(game.perfect_parries == 1, "não contou como perfeito")

	_section("Contra-ataque")
	var before: int = game.enemy_health
	game.control.attack_left = 0.0
	game._try_counter()
	_check(game.enemy_health == before - 1, "tira exatamente 1 de vida")

	_section("Armadura")
	game.brain.state = RECOVER
	game.control.attack_left = 0.0
	var armored: int = game.enemy_health
	game._try_counter()
	_check(game.enemy_health == armored, "sem parry, o golpe não passa")

	_section("Lado errado")
	game.control.cooldown_left = 0.0
	game.brain.state = WINDUP
	game.brain.incoming_guard = HIGH
	game._try_parry()
	var hp: int = game.health
	game._resolve_enemy_hit()
	_check(game.health == hp - 1, "golpe fora da janela recebe dano")
	_check(game.combo == 0, "sequência zerada")

	_section("Fora de alcance")
	game.enemy.position = Vector3(11, 0.05, 0)
	game.brain.state = WINDUP
	var safe: int = game.health
	game._resolve_enemy_hit()
	_check(game.health == safe, "golpe distante não causa dano")

	_section("Parry sem cooldown pronto")
	_engage()
	game.control.cooldown_left = 0.4
	var window_before: float = game.control.parry_left
	game._try_parry()
	_check(game.control.parry_left == window_before, "clique durante recuperação é ignorado")

	_section("Padrão de ataque do oponente")
	game.brain.pattern_index = 0
	var sequence: Array[int] = []
	for i in range(6):
		game.brain.start_attack()
		sequence.append(game.brain.incoming_guard)
	_check(sequence == [HIGH, LOW, MID, LOW, HIGH, MID],
		"sequência de lados na ordem definida")
	game.brain.pattern_index = 0
	game.brain.start_attack()
	_check(game.brain.incoming_guard == HIGH, "o padrão reinicia com o índice")

	_section("Limite da janela perfeita")
	_engage()
	game.health = 5
	game.control.cooldown_left = 0.0
	game.control.attack_left = 0.0
	game.brain.state = WINDUP
	game.brain.incoming_guard = MID
	game._try_parry()
	game.control.parry_age = game.combat.perfect_window
	var perfect_before: int = game.perfect_parries
	game._resolve_enemy_hit()
	_check(game.perfect_parries == perfect_before + 1,
		"em cima do limite ainda conta como perfeito")
	game.control.cooldown_left = 0.0
	game.control.attack_left = 0.0
	game.brain.state = WINDUP
	game.brain.incoming_guard = MID
	game._try_parry()
	game.control.parry_age = game.combat.perfect_window + 0.001
	perfect_before = game.perfect_parries
	game._resolve_enemy_hit()
	_check(game.perfect_parries == perfect_before,
		"um milissegundo depois já não é perfeito")

	_section("Sequência e empurrão")
	_check(game.combo >= 2 and game.best_combo >= 2, "sequência acumulou")
	var peak: int = game.best_combo
	game.control.knockback = Vector3.ZERO
	game.control.cooldown_left = 0.0
	game.control.attack_left = 0.0
	game.health = 5
	game.brain.state = WINDUP
	game.brain.incoming_guard = HIGH
	# Sem janela aberta o golpe entra. É assim que se leva dano agora que o
	# parry não tem direção: por tempo, não por lado errado.
	game.control.consume_parry()
	game._resolve_enemy_hit()
	_check(game.combo == 0, "dano zera a sequência atual")
	_check(game.best_combo == peak, "a maior sequência não é perdida")
	var away: Vector3 = game.player.global_position - game.enemy.global_position
	away.y = 0
	_check(game.control.knockback.normalized().dot(away.normalized()) > 0.9,
		"o empurrão do dano afasta o jogador do oponente")

	_section("Barra de recuperação")
	game.control.cooldown_left = 0.0
	game.control.attack_left = 0.0
	_check(is_equal_approx(game.control.readiness(), 1.0), "parada, a barra marca cheia")
	game.control.try_parry()
	_check(game.control.readiness() < 0.01, "logo após aparar, a barra zera")
	game.control.tick_timers(game.control.cooldown_window())
	_check(game.control.readiness() >= 1.0, "ao fim da recuperação, enche de novo")

	_section("Sobreposição de cor")
	game.combat.screen_flash_enabled = false
	game.flash_alpha = 0.0
	game._impact(Color.WHITE, 0.0, 0.0, 0.5)
	_check(game.flash_alpha == 0.0, "desligada no recurso, o impacto não pisca")
	game.combat.screen_flash_enabled = true
	game._impact(Color.WHITE, 0.0, 0.0, 0.5)
	_check(is_equal_approx(game.flash_alpha, 0.5), "ligada, pisca na intensidade pedida")

	_section("Contra-ataque fora de alcance")
	game.enemy.position = Vector3(11, 0.05, 0)
	game.brain.state = STUNNED
	game.control.attack_left = 0.0
	game.control.parry_left = 0.0
	var distant: int = game.enemy_health
	game._try_counter()
	_check(game.enemy_health == distant, "oponente atordoado longe não leva dano")

	_section("Pausa")
	game._start_duel()
	game._set_paused(true)
	_check(game.paused and game.hud.menu.visible, "pausa congela e mostra o menu")
	_check(not game.hud.theme_picker.visible, "seletor de tema some durante a luta")
	game.input_grace = 0.0
	game._set_paused(false)
	_check(not game.paused, "continuar despausa")
	_check(game.input_grace > 0, "o clique em CONTINUAR não vira uma tentativa de parry")

	_section("Vitória")
	_engage()
	game.control.cooldown_left = 0.0
	game.enemy_health = 1
	game.brain.state = STUNNED
	game.control.attack_left = 0.0
	game.control.parry_left = 0.0
	game._try_counter()
	_check(game.enemy_health == 0, "oponente sem vida")
	_check(game.finished, "duelo encerrado")
	_check(game.brain.state == DEFEATED, "estado DERROTADO")
	game._show_result()
	_check(game.hud.menu.visible, "tela de resultado aberta")
	_check(game.hud.theme_picker.visible, "seletor de tema liberado")

	_section("Troca de tema")
	game._on_theme_selected(1)
	_check(game.duel_theme.theme_name == "Medieval", "tema medieval aplicado")
	_check(game.hud.opponent_name == "CAVALEIRO", "nome do oponente atualizado")
	_check(game.stage.get_child_count() == 1, "uma única arena em cena")
	_check(game.stage.get_child(0).name == "World", "arena substituída")
	_check(game.visual_mount.get_child_count() == 1, "um único modelo de oponente")
	_check(game.weapon != null and game.weapon.is_inside_tree(),
		"WeaponPivot do jogador no lugar")
	_check(game.brain.weapon_pivot != null and game.brain.weapon_pivot.is_inside_tree(),
		"WeaponPivot do novo tema válido")
	_check(game.brain.attack_color == game.duel_theme.attack_color,
		"paleta do oponente acompanha o tema")
	_check(game.duel_theme.palette.size() > 0, "o tema medieval traz a sua paleta")
	_check(material.get_shader_parameter("palette_size") == game.duel_theme.palette.size(),
		"a troca de tema recarrega a paleta do shader (%d cores)"
			% game.duel_theme.palette.size())

	_section("Troca bloqueada durante a luta")
	game._start_duel()
	game._on_theme_selected(0)
	_check(game.duel_theme.theme_name == "Medieval", "tema não muda no meio do duelo")

	_section("Derrota")
	_engage()
	game.health = 1
	game.control.cooldown_left = 0.0
	game.brain.state = WINDUP
	game.brain.incoming_guard = MID
	game.control.consume_parry()
	game._resolve_enemy_hit()
	_check(game.health == 0, "vida zerada")
	_check(game.finished, "duelo encerrado na derrota")

	_section("Reinício")
	game._start_duel()
	_check(game.health == game.combat.player_max_health, "vida do jogador restaurada")
	_check(game.enemy_health == game.combat.enemy_max_health, "vida do oponente restaurada")
	_check(game.parries == 0 and game.combo == 0, "contadores zerados")
	_check(not game.finished and game.started, "duelo ativo de novo")
	_check(game.brain.state == APPROACH and game.brain.pattern_index == 0,
		"oponente volta a se aproximar do começo do padrão")
	_check(game.control.deflect_height == MID and game.control.parry_left == 0.0,
		"pose de aparo e janela do jogador reiniciadas")

	# Libera a cena antes de sair para não vazar instâncias no encerramento.
	# Os sons precisam parar primeiro: uma reprodução em curso segura o buffer.
	game.feedback.reset()
	for key in game.feedback.players:
		var sound: AudioStreamPlayer = game.feedback.players[key]
		sound.stream = null
	root.remove_child(game)
	game.free()
	game = null


func _report() -> void:
	print("")
	if failures.is_empty():
		print("TODOS OS TESTES PASSARAM (%d verificações)" % checks)
		quit(0)
	else:
		print("%d FALHA(S) em %d verificações:" % [failures.size(), checks])
		for f in failures:
			print("  - ", f)
		quit(1)


func _engage() -> void:
	# Aproxima os dois e alinha as frentes: os testes de acerto dependem
	# de distância e de ângulo, não de posição de spawn.
	game.player.position = Vector3(-2.0, 0.05, 0)
	game.enemy.position = Vector3(0.0, 0.05, 0)
	game.control.aim_at(game.enemy)
	game.enemy.look_at(game.player.global_position, Vector3.UP)


## Constantes do oponente pelo mapa do script: o teste não guarda cópia
## dos números de design, senão ele passa a testar a si mesmo.
func _brain_const(g: Node3D, name: String) -> Variant:
	return g.brain.get_script().get_script_constant_map()[name]


func _section(title: String) -> void:
	print("\n[", title, "]")


func _check(condition: bool, message: String) -> void:
	checks += 1
	if condition:
		print("  ok    ", message)
	else:
		failures.append(message)
		print("  FALHA ", message)
