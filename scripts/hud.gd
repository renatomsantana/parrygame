extends CanvasLayer
## Interface em português; o combate é controlado por main.gd.

signal primary_pressed
signal theme_selected(index: int)

const GuardIndicator = preload("res://scripts/guard_indicator.gd")

## Distância do centro da janela até o indicador de altura. Ele fica à
## esquerda do jogador: no centro cairia em cima do oponente, bem onde
## está a leitura do golpe.
const INDICATOR_CENTER := -370.0
const ThemeProfile = preload("res://scripts/config/duel_theme.gd")

var health_label: Label
var steady_label: Label
var enemy_label: Label
var count_label: Label
var tell_label: Label
var feedback_label: Label
var ready_label: Label
var cooldown_bar: ProgressBar
var flash: ColorRect
var menu: Control
var menu_title: Label
var menu_description: Label
var menu_button: Button
var guard_indicator: GuardIndicator
var theme_picker: OptionButton
var menu_style: StyleBoxFlat
var eyebrow: Label
var accent := Color("a9e5cf")
var opponent_name: String = "RONIN"


func _ready() -> void:
	var root := Control.new()
	root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)
	flash = ColorRect.new()
	flash.color = Color(0, 0, 0, 0)
	flash.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(flash)
	flash.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var top := HBoxContainer.new()
	top.set_anchors_and_offsets_preset(Control.PRESET_TOP_WIDE)
	top.offset_left = 34
	top.offset_right = -34
	top.offset_top = 26
	root.add_child(top)
	health_label = label("", 22)
	health_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	top.add_child(health_label)
	enemy_label = label("", 22)
	enemy_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	enemy_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	top.add_child(enemy_label)
	count_label = label("", 22)
	count_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	count_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	top.add_child(count_label)
	# A firmeza fica colada na vida, à esquerda: são o mesmo assunto — quanto
	# ainda dá para errar antes de doer.
	steady_label = label("", 15)
	root.add_child(steady_label)
	steady_label.offset_left = 36
	steady_label.offset_top = 58
	steady_label.offset_right = 380
	steady_label.offset_bottom = 80
	tell_label = centered_label(root, 95, "", 24)
	# O indicador fica sobre o jogador, que ocupa o terço esquerdo do palco.
	# No centro ele cairia em cima do oponente, bem onde está a leitura.
	guard_indicator = GuardIndicator.new()
	root.add_child(guard_indicator)
	guard_indicator.set_anchors_and_offsets_preset(Control.PRESET_CENTER)
	guard_indicator.offset_left = INDICATOR_CENTER - 110
	guard_indicator.offset_right = INDICATOR_CENTER + 110
	guard_indicator.offset_top = -110
	guard_indicator.offset_bottom = 110
	feedback_label = centered_label(root, 114, "", 27, true)
	ready_label = centered_label(root, -132, "PARRY PRONTO", 18)
	ready_label.anchor_top = 1
	ready_label.anchor_bottom = 1
	cooldown_bar = ProgressBar.new()
	cooldown_bar.show_percentage = false
	cooldown_bar.max_value = 1.0
	cooldown_bar.value = 1.0
	cooldown_bar.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(cooldown_bar)
	cooldown_bar.anchor_left = 0.5
	cooldown_bar.anchor_right = 0.5
	cooldown_bar.anchor_top = 1
	cooldown_bar.anchor_bottom = 1
	cooldown_bar.offset_left = -100
	cooldown_bar.offset_right = 100
	cooldown_bar.offset_top = -100
	cooldown_bar.offset_bottom = -92
	var controls := centered_label(root, -62,
		"A / D  aproximar e afastar     •     Espaço ou Esquerdo  aparar     •     Direito  contra-atacar", 17)
	controls.anchor_top = 1
	controls.anchor_bottom = 1
	var secondary := centered_label(root, -34, "ESC  pausar     /     R  recomeçar", 14)
	secondary.anchor_top = 1
	secondary.anchor_bottom = 1
	secondary.modulate = Color("9bb1c5")
	_build_menu(root)


func _build_menu(root: Control) -> void:
	menu = Control.new()
	root.add_child(menu)
	menu.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var dim := ColorRect.new()
	dim.color = Color(0.015, 0.025, 0.045, 0.9)
	menu.add_child(dim)
	dim.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var center := CenterContainer.new()
	menu.add_child(center)
	center.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var panel := PanelContainer.new()
	panel.custom_minimum_size = Vector2(540, 0)
	var style := StyleBoxFlat.new()
	menu_style = style
	style.bg_color = Color("182638")
	style.border_color = Color("62e3d6")
	style.border_width_top = 3
	style.content_margin_left = 36
	style.content_margin_right = 36
	style.content_margin_top = 30
	style.content_margin_bottom = 32
	panel.add_theme_stylebox_override("panel", style)
	center.add_child(panel)
	var column := VBoxContainer.new()
	column.add_theme_constant_override("separation", 18)
	panel.add_child(column)
	eyebrow = label("UM DUELO DE TEMPO E PRECISÃO", 14)
	eyebrow.modulate = Color("62e3d6")
	column.add_child(eyebrow)
	menu_title = label("APARA", 52)
	column.add_child(menu_title)
	menu_description = label("", 20)
	menu_description.custom_minimum_size.x = 470
	menu_description.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	column.add_child(menu_description)
	theme_picker = OptionButton.new()
	theme_picker.custom_minimum_size.y = 42
	theme_picker.add_theme_font_size_override("font_size", 18)
	theme_picker.item_selected.connect(func(index: int) -> void: theme_selected.emit(index))
	column.add_child(theme_picker)
	menu_button = Button.new()
	menu_button.custom_minimum_size.y = 50
	menu_button.add_theme_font_size_override("font_size", 22)
	menu_button.pressed.connect(func() -> void: primary_pressed.emit())
	column.add_child(menu_button)


func show_menu(title: String, description: String, button_text: String) -> void:
	menu_title.text = title
	menu_description.text = description
	menu_button.text = button_text
	menu.show()
	menu_button.grab_focus()


func hide_menu() -> void:
	menu.hide()
	menu_button.release_focus()


func refresh(health: int, enemy_health: int, parries: int, readiness: float,
		tell: String, color: Color, active: bool,
		steady: int = 0, steady_max: int = 0) -> void:
	health_label.text = "VOCÊ   %d" % health
	# Cheio e vazio em vez de um número: a firmeza é olhada de canto de olho,
	# no meio do duelo, e uma contagem exige leitura.
	steady_label.text = "FIRMEZA   " + "•".repeat(maxi(steady, 0)) \
		+ "·".repeat(maxi(steady_max - steady, 0))
	steady_label.modulate = Color("ef7670") if steady <= 1 else accent
	enemy_label.text = "%s   %d" % [opponent_name, enemy_health]
	count_label.text = "PARRIES   %02d" % parries
	tell_label.text = tell
	tell_label.modulate = color
	cooldown_bar.value = readiness
	ready_label.text = "APARANDO" if active else (
		"PARRY PRONTO" if readiness >= 1.0 else "RECUPERANDO…")
	ready_label.modulate = accent if active else Color.WHITE


func apply_theme(profile: ThemeProfile) -> void:
	accent = profile.guard_color
	opponent_name = profile.opponent_name
	menu_style.bg_color = profile.panel_color
	menu_style.border_color = profile.attack_color
	eyebrow.text = profile.stage_name.to_upper()
	eyebrow.modulate = profile.attack_color
	menu_title.add_theme_color_override("font_color", profile.text_color)
	menu_description.add_theme_color_override("font_color", profile.text_color)
	guard_indicator.guard_color = profile.guard_color
	guard_indicator.attack_color = profile.attack_color
	var bar_style := StyleBoxFlat.new()
	bar_style.bg_color = accent
	cooldown_bar.add_theme_stylebox_override("fill", bar_style)
	guard_indicator.queue_redraw()


func update_guard(incoming: int, imminent: bool, active: bool) -> void:
	guard_indicator.incoming_direction = incoming
	guard_indicator.imminent = imminent
	guard_indicator.parrying = active
	guard_indicator.queue_redraw()


static func label(text: String, font_size: int) -> Label:
	var result := Label.new()
	result.text = text
	result.add_theme_font_size_override("font_size", font_size)
	result.add_theme_color_override("font_color", Color("e4edf5"))
	# Contorno, não só sombra: a HUD passa por cima de chão claro e de armadura
	# saturada. O preto sobrevive ao modulate colorido dos avisos.
	result.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.85))
	result.add_theme_constant_override("outline_size", 6)
	result.add_theme_color_override("font_shadow_color", Color(0, 0, 0, 0.5))
	result.add_theme_constant_override("shadow_offset_x", 1)
	result.add_theme_constant_override("shadow_offset_y", 2)
	result.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return result


static func centered_label(parent: Control, y: float, text: String,
		font_size: int, middle: bool = false) -> Label:
	var result := label(text, font_size)
	parent.add_child(result)
	result.anchor_right = 1
	if middle:
		result.anchor_top = 0.5
		result.anchor_bottom = 0.5
	result.offset_top = y
	result.offset_bottom = y + font_size + 12
	result.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	return result
