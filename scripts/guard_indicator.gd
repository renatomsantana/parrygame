extends Control
## As três alturas do golpe, empilhadas como aparecem no duelo: alto em cima,
## baixo embaixo. O parry não tem direção, então o indicador não mostra guarda
## nenhuma do jogador — mostra de que altura o golpe vem, que é o que diz
## quanto tempo ele tem: alto é lento, baixo chega antes.
##
## Desenhado em blocos alinhados a uma grade, do tamanho do pixel do mundo 3D:
## a interface do duelo acompanha o acabamento da cena em vez de destoar dela.

var incoming_direction: int = -1
var imminent: bool = false
var parrying: bool = false
var guard_color := Color("a9e5cf")
var attack_color := Color("f2bb66")
## Lado do bloco, em pixels da janela. main.gd iguala ao divisor da resolução.
var cell: float = 4.0

## Altura do centro de cada faixa, do topo para a base do widget.
const BAND_HEIGHTS: Array[float] = [-46.0, 0.0, 46.0]
const BAND_HALF_WIDTH := 30.0
const INCOMING_OFFSET := 52.0
const IDLE_COLOR := Color(0.68, 0.78, 0.88, 0.40)


func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE


func _draw() -> void:
	var center := size * 0.5
	for band in range(3):
		var y: float = BAND_HEIGHTS[band]
		# Durante a janela as três acendem: a defesa cobre qualquer altura, e o
		# indicador não pode sugerir que ainda há um lado a escolher.
		var color := guard_color if parrying else IDLE_COLOR
		_bar(center + Vector2(0, y), BAND_HALF_WIDTH, color, 2 if parrying else 1)
		if band != incoming_direction:
			continue
		# A seta do golpe chega pelo lado do oponente, que está sempre à
		# direita do jogador no palco.
		var cue_color := Color.WHITE if imminent else attack_color
		_bar(center + Vector2(0, y), BAND_HALF_WIDTH + cell * 3.0, cue_color, 2)
		_arrow(center + Vector2(INCOMING_OFFSET, y), cue_color)


## Faixa horizontal em blocos, crescendo do centro para os dois lados.
func _bar(middle: Vector2, half_width: float, color: Color, thickness: int) -> void:
	var steps := int(half_width * 2.0 / cell) + 1
	for i in range(steps + 1):
		var x := -half_width + cell * float(i)
		for layer in range(thickness):
			_block(middle + Vector2(x, float(layer) * cell), color)


## Seta de três degraus apontando para a esquerda, na direção do jogador.
func _arrow(tip: Vector2, color: Color) -> void:
	for step in range(3):
		var half := 2 - step
		var x := tip.x + float(step) * cell
		for offset in range(-half, half + 1):
			_block(Vector2(x, tip.y + float(offset) * cell), color)


func _block(point: Vector2, color: Color) -> void:
	var corner := Vector2(floorf(point.x / cell), floorf(point.y / cell)) * cell
	draw_rect(Rect2(corner, Vector2(cell, cell)), color)
