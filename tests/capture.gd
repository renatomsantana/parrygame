extends SceneTree
## Captura quadros do duelo com renderização real, para conferir a imagem:
##   godot --path . --script res://tests/capture.gd -- <pasta_de_saida>
## Sem a pasta, grava em user://.

const OUT_DEFAULT := "user://"

var game: Node3D
var frame: int = 0
var out_dir: String = OUT_DEFAULT
var shots: Array[String] = []


func _initialize() -> void:
	var args := OS.get_cmdline_user_args()
	if args.size() > 0:
		out_dir = args[0].replace("\\", "/")
		if not out_dir.ends_with("/"):
			out_dir += "/"


func _process(_delta: float) -> bool:
	frame += 1
	# O teste não sequestra o cursor do usuário.
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE

	match frame:
		1:
			game = (load("res://main.tscn") as PackedScene).instantiate() as Node3D
			root.add_child(game)
		30:
			_shot("01_menu.png")
		32:
			game._start_duel()
			_engage()
		70:
			_shot("02_guarda.png")
		72:
			game.brain.state = 1     # WINDUP
			game.brain.incoming_guard = 0
		110:
			_shot("03_golpe_chegando.png")
		170:
			_shot("04_iminente.png")
		175:
			game._try_parry()
			game.control.parry_age = 0.0
			game._resolve_enemy_hit()
		195:
			_shot("05_parry.png")
		200:
			# A troca só é permitida fora de uma luta em andamento.
			game.started = false
			game._on_theme_selected(1)
			game._start_duel()
			_engage()
			game.brain.state = 1
			game.brain.incoming_guard = 2
		240:
			_shot("06_medieval.png")
		250:
			for s in shots:
				print(s)
			root.remove_child(game)
			game.free()
			return true

	if game == null:
		return false
	# Mantém o golpe no ar durante as capturas em vez de deixá-lo resolver.
	if ((frame > 72 and frame < 130) or frame > 205) and game.brain.state == 1:
		game.brain.state_left = 1.0
	if frame >= 130 and frame < 175 and game.brain.state == 1:
		game.brain.state_left = 0.12  # entra no clarão branco
	return false


func _engage() -> void:
	game.player.position = Vector3(-1.2, 0.05, 0)
	game.enemy.position = Vector3(1.05, 0.05, 0)
	game.control.aim_at(game.enemy)
	game.enemy.look_at(game.player.global_position, Vector3.UP)


func _shot(name: String) -> void:
	var image := root.get_texture().get_image()
	if image == null:
		shots.append("SEM IMAGEM: " + name)
		return
	var path := out_dir + name
	var err := image.save_png(path)
	shots.append(("gravado " if err == OK else "ERRO %d " % err) + path)
