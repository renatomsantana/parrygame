extends Node3D
## Faíscas e sons sintetizados. Não usa arquivos de áudio de terceiros.
##
## Cada som é montado em camadas separadas, porque é assim que o ouvido lê um
## impacto: o **ataque** nos primeiros milissegundos diz que bateu, o **corpo**
## diz de que material, o **peso** grave diz o tamanho e a **cauda** diz o
## valor da coisa. Faltando qualquer uma delas o golpe soa barato, mesmo com o
## volume certo.

const Geometry = preload("res://scripts/visuals/geometry.gd")
## Sons de referência, opcionais. Um `<nome>.mp3` ou `.ogg` aqui substitui o
## sintetizado do mesmo nome — serve para ouvir uma referência no lugar dela
## enquanto se ajusta a síntese, sem mexer em código. Tirando o arquivo, o
## som sintetizado volta sozinho.
##
## Arquivos de terceiros ficam **só aqui**: nada do que o jogo sintetiza
## depende deles, e é isso que mantém o projeto distribuível.
const REFERENCE_DIR := "res://art/audio/"
## 44,1 kHz porque metal vive acima de 8 kHz: a 22 kHz o clangor perde o ar e
## sobra um toque de telefone.
const SAMPLE_RATE: int = 44100

const KINDS: Array[String] = ["parry", "perfect", "hit", "hurt", "swing", "cue",
	"armor", "feint"]
const DURATIONS: Dictionary = {
	"parry": 0.70, "perfect": 1.00, "hit": 0.45, "hurt": 0.45,
	"swing": 0.16, "cue": 0.16, "armor": 0.32, "feint": 0.20,
}
## Mistura relativa. O aviso não pode ter o mesmo peso do impacto: se tudo tem
## a mesma importância, nada chama atenção.
const MIX_DB: Dictionary = {
	"parry": 0.0, "perfect": 1.5, "hit": -1.0, "hurt": -2.0,
	"swing": -9.0, "cue": -5.0, "armor": -4.0, "feint": -3.0,
}

var players: Dictionary = {}
var sparks: Array[Dictionary] = []
var random := RandomNumberGenerator.new()
var master_db: float = -10.0


func _ready() -> void:
	random.randomize()
	for kind in KINDS:
		var player := AudioStreamPlayer.new()
		var reference := _reference(kind)
		player.stream = reference if reference != null else _sound(kind)
		player.volume_db = master_db + float(MIX_DB[kind])
		# Aparar dois golpes seguidos não pode cortar a cauda do primeiro.
		player.max_polyphony = 4
		add_child(player)
		players[kind] = player


## Volume geral vindo de combat.tres. A mistura entre os sons é do áudio, não
## da configuração: mexer no volume não deve reordenar o que é importante.
func set_master_volume(db: float) -> void:
	master_db = db
	for kind in players:
		var player: AudioStreamPlayer = players[kind]
		player.volume_db = master_db + float(MIX_DB[kind])


func play(kind: String, pitch: float = 1.0) -> void:
	var player: AudioStreamPlayer = players[kind]
	player.pitch_scale = pitch
	player.play()


func burst(point: Vector3, color: Color, count: int = 18) -> void:
	for i in range(count):
		var spark := Geometry.box(self, Vector3(0.025, 0.025, 0.13), Vector3.ZERO, color, true)
		spark.global_position = point
		var direction := Vector3(random.randf_range(-1, 1),
			random.randf_range(-0.2, 1.2), random.randf_range(-1, 1)).normalized()
		sparks.append({"node": spark, "velocity": direction * random.randf_range(2, 6),
			"life": random.randf_range(0.25, 0.5)})


func tick(delta: float) -> void:
	for i in range(sparks.size() - 1, -1, -1):
		var spark: Dictionary = sparks[i]
		spark["life"] = float(spark["life"]) - delta
		var node: MeshInstance3D = spark["node"]
		if float(spark["life"]) <= 0:
			node.queue_free()
			sparks.remove_at(i)
			continue
		var speed: Vector3 = spark["velocity"]
		speed.y -= delta * 7
		spark["velocity"] = speed
		node.position += speed * delta
		node.scale = Vector3.ONE * minf(1.0, float(spark["life"]) * 5)
		if speed.length_squared() > 0.01:
			node.look_at(node.global_position + speed, Vector3.UP)


func reset() -> void:
	for spark in sparks:
		var node: MeshInstance3D = spark["node"]
		node.queue_free()
	sparks.clear()
	for key in players:
		var player: AudioStreamPlayer = players[key]
		player.stop()


func pause_audio(value: bool) -> void:
	for key in players:
		var player: AudioStreamPlayer = players[key]
		player.stream_paused = value


## Procura um som de referência para este nome. Lê os bytes direto, sem
## passar pela importação: assim basta soltar o arquivo na pasta e rodar.
func _reference(kind: String) -> AudioStream:
	for extension in ["mp3", "ogg"]:
		var path := "%s%s.%s" % [REFERENCE_DIR, kind, extension]
		if not FileAccess.file_exists(path):
			continue
		var bytes := FileAccess.get_file_as_bytes(path)
		if bytes.is_empty():
			continue
		if extension == "ogg":
			return AudioStreamOggVorbis.load_from_buffer(bytes)
		var mp3 := AudioStreamMP3.new()
		mp3.data = bytes
		return mp3
	return null


# --- síntese ---------------------------------------------------------------

## Choque de lâminas. As três camadas obrigatórias mais uma cauda longa.
func _clash(t: float, white: float, perfect: bool) -> float:
	# Ataque: estouro curtíssimo de ruído. São estes 3 ms que soam como "bateu".
	var sample := white * exp(-t * 320.0) * 0.55
	# Corpo: parciais não harmônicos, que é o que diferencia metal de sino. Os
	# agudos morrem antes dos graves, como em metal de verdade.
	sample += sin(TAU * 1180.0 * t) * exp(-t * 9.0) * 0.30
	sample += sin(TAU * 1867.0 * t) * exp(-t * 13.0) * 0.20
	sample += sin(TAU * 2941.0 * t) * exp(-t * 18.0) * 0.12
	sample += sin(TAU * 4310.0 * t) * exp(-t * 26.0) * 0.07
	# Peso: sem grave o golpe não chega ao peito de quem ouve.
	sample += sin(TAU * 88.0 * t) * exp(-t * 30.0) * 0.30
	# Cauda: fica soando muito depois do impacto, baixa. É o que dá tamanho.
	sample += sin(TAU * 2355.0 * t) * exp(-t * 2.6) * 0.09
	if perfect:
		# O parry perfeito ganha um brilho que chega **atrasado**, não mais
		# volume. Um segundo evento logo depois do primeiro lê como recompensa;
		# o mesmo som mais alto lê só como mais alto.
		var late := maxf(t - 0.045, 0.0)
		sample += sin(TAU * 1760.0 * late) * exp(-late * 3.0) * 0.10
		sample += sin(TAU * 3520.0 * late) * exp(-late * 4.0) * 0.16
		sample += sin(TAU * 5280.0 * late) * exp(-late * 6.0) * 0.09
	return sample


## Contra-ataque que acerta: estalo curto por cima, baque grave por baixo.
func _cut(t: float, white: float, rumble: float) -> float:
	var sample := white * exp(-t * 140.0) * 0.45
	# Varredura de grave descendente. A queda de altura é o que dá massa.
	sample += sin(TAU * (150.0 * t - 105.0 * t * t)) * exp(-t * 15.0) * 0.55
	sample += rumble * exp(-t * 30.0) * 0.30
	return sample


## Golpe recebido. Mais grave, mais surdo e de propósito desagradável.
func _wound(t: float, rumble: float) -> float:
	var sample := sin(TAU * (95.0 * t - 55.0 * t * t)) * exp(-t * 11.0) * 0.55
	sample += rumble * exp(-t * 18.0) * 0.28
	# Batimento entre dois graves próximos: soa errado, e é para soar.
	sample += sin(TAU * 58.0 * t) * sin(TAU * 4.5 * t) * exp(-t * 9.0) * 0.20
	return sample


## Lâmina cortando ar: ruído filtrado passando por um pico de volume.
func _whoosh(t: float, duration: float, rumble: float) -> float:
	var shape := sin(PI * clampf(t / duration, 0.0, 1.0))
	return rumble * shape * shape * 0.5


## Aviso do golpe. Duas notas curtas **subindo** — sobe porque anuncia algo que
## vem — e curtas para não cobrir o clangor que chega logo depois.
func _tick(t: float) -> float:
	var sample := sin(TAU * 990.0 * t) * exp(-t * 42.0) * 0.26
	var late := maxf(t - 0.055, 0.0)
	sample += sin(TAU * 1480.0 * late) * exp(-late * 42.0) * 0.24
	return sample


## Bater na armadura: sem cauda nenhuma. O som morre onde bate, e é essa
## ausência que diz "não adiantou" antes de qualquer texto na tela.
func _dull(t: float, white: float) -> float:
	var sample := sin(TAU * 320.0 * t) * exp(-t * 30.0) * 0.32
	sample += sin(TAU * 196.0 * t) * exp(-t * 24.0) * 0.26
	sample += white * exp(-t * 90.0) * 0.18
	return sample


## Finta: começa igual ao aviso e é cortada no meio. O som que não termina é
## o que diz "aquilo ali não valeu" — e diz antes de qualquer texto na tela.
func _choke(t: float) -> float:
	var sample := sin(TAU * 990.0 * t) * exp(-t * 42.0) * 0.26
	# Em vez da segunda nota subindo, uma descendo e um corte seco logo depois.
	var late := maxf(t - 0.055, 0.0)
	sample += sin(TAU * 660.0 * late) * exp(-late * 30.0) * 0.22
	return sample * (1.0 - smoothstep(0.10, 0.13, t))


func _sound(kind: String) -> AudioStreamWAV:
	var duration: float = DURATIONS[kind]
	var samples := int(SAMPLE_RATE * duration)
	var data := PackedByteArray()
	data.resize(samples * 2)
	# Semente fixa: o timbre é reproduzível em toda inicialização.
	var noise := RandomNumberGenerator.new()
	noise.seed = 7301
	# Ruído com memória, por um passa-baixa de um polo. Ruído branco puro soa a
	# chiado de rádio; filtrado, soa a material.
	var rumble := 0.0
	for i in range(samples):
		var t := float(i) / SAMPLE_RATE
		var white := noise.randf_range(-1.0, 1.0)
		rumble = rumble * 0.86 + white * 0.14
		var sample: float = 0.0
		match kind:
			"parry":
				sample = _clash(t, white, false)
			"perfect":
				sample = _clash(t, white, true)
			"hit":
				sample = _cut(t, white, rumble)
			"hurt":
				sample = _wound(t, rumble)
			"swing":
				sample = _whoosh(t, duration, rumble)
			"cue":
				sample = _tick(t)
			"armor":
				sample = _dull(t, white)
			"feint":
				sample = _choke(t)
		# Fade-in de 1 ms e fade-out evitam cliques na borda do buffer.
		sample *= minf(t * 1000.0, 1.0) * minf((duration - t) * 300.0, 1.0)
		# Saturação suave em vez de corte seco: o pico deforma em vez de
		# estalar. Somar quatro camadas passa de 1,0 com facilidade.
		sample = tanh(sample * 1.6) * 0.72
		data.encode_s16(i * 2, int(clampf(sample, -1.0, 1.0) * 32767))
	var stream := AudioStreamWAV.new()
	stream.format = AudioStreamWAV.FORMAT_16_BITS
	stream.mix_rate = SAMPLE_RATE
	stream.stereo = false
	stream.data = data
	return stream
