# APARA — script mestre do duelo lateral

**Versão 0.2 · Godot 4.6+ · GDScript · PC com teclado e mouse**

Este Markdown reúne a proposta e os arquivos-fonte completos para continuarmos
programando e criando os designs. Cada seção de código informa o caminho em
que o conteúdo deve ser salvo. As cenas usam formas nativas do Godot, sem
necessidade de baixar modelos ou texturas. O ZIP do projeto é a opção mais
rápida para abrir tudo de uma vez.

## O jogo

Um duelo **lateral** baseado em leitura de golpe, escolha de guarda e parry
preciso. Os dois lutadores aparecem de corpo inteiro; a câmera é ortográfica e
olha na horizontal, o que dá a leitura 2D. O mouse escolhe a **altura** da
guarda.

| Controle | Resultado |
| --- | --- |
| Movimento curto do mouse para cima ou para baixo | Sobe ou desce a guarda um degrau |
| Clique esquerdo | Dá parry na altura escolhida |
| Clique direito após aparar | Contra-ataca durante o atordoamento |
| A e D | Aproxima e afasta no palco |
| Esc | Pausa e libera o cursor |
| R | Reinicia o duelo |

A altura e o tempo precisam coincidir. Segurar o botão não mantém a defesa.
Faíscas, som metálico, recuo, tremor e uma pausa curta reforçam o impacto.
O parry perfeito recebe som e resposta visual mais fortes.

## Temas de design

| Tema | Direção visual |
| --- | --- |
| Samurai — inicial | Ronin magenta contra pátio roxo quase preto, lanternas em ciano, katana de aço branco |
| Medieval — alternativa no menu | Cavaleiro de tabardo ciano contra muralha azul-noite, tochas em âmbar |

A direção é neo-noir em pixel art: fundo escuro, uma cor forte no oponente e a
complementar no neon do cenário. O mundo 3D é renderizado a 320x180, passa por
gradação e efeitos de tela e cai numa paleta fechada por tema.

Os modelos são bases geométricas editáveis. Podemos substituir as malhas,
refinar materiais e criar animações depois. Os dois temas usam as mesmas
regras de combate.

## Como começar

1. No Godot Standard, importe `project.godot` da pasta extraída do ZIP.
2. Abra `main.tscn` e pressione F5.
3. Escolha o tema no menu e inicie o duelo.
4. Para codar, abra `scripts/main.gd`.
5. Para ajustar a dificuldade e o impacto, edite `config/combat.tres`.
6. Para criar a arte, abra as cenas em `scenes/themes/samurai/` ou `medieval/`.

Se montar o projeto a partir deste Markdown, crie os caminhos indicados e
copie **apenas o conteúdo de cada bloco** para o arquivo correspondente.
Não cole todo este documento em um único script GDScript.

## Estado de verificação

O projeto foi montado a partir deste documento e **executado no Godot 4.6.3**
(Windows, OpenGL Compatibility). Estão confirmados: importação sem erro, checagem
de sintaxe em todos os scripts, 126 verificações automáticas em
`tests/smoke_test.gd` e renderização dos dois temas — as capturas estão em
`docs/capturas/`.

**Falta o julgamento com as mãos:** janela de parry, ritmo do oponente e clareza
da leitura só se avaliam jogando com mouse e teclado. Nenhum teste automático
mede sensação.

## Arquivos do projeto

### `project.godot`

```ini
; Abra este arquivo no Godot 4, versao Standard (GDScript).
config_version=5

[application]
config/name="APARA | Samurai e Medieval"
config/version="0.2.0"
run/main_scene="res://main.tscn"
config/features=PackedStringArray("4.6", "GL Compatibility")
config/description="Base editável de duelo com parry direcional, temas samurai e medieval."

[display]
window/size/viewport_width=1280
window/size/viewport_height=720
window/size/window_width_override=1280
window/size/window_height_override=720
window/stretch/mode="canvas_items"

[rendering]
renderer/rendering_method="gl_compatibility"
renderer/rendering_method.mobile="gl_compatibility"
environment/defaults/default_clear_color=Color(0.025, 0.04, 0.065, 1)
```

### `main.tscn`

```ini
[gd_scene load_steps=11 format=3]

[ext_resource type="Script" path="res://scripts/main.gd" id="main"]
[ext_resource type="Resource" path="res://config/combat.tres" id="combat"]
[ext_resource type="Resource" path="res://config/themes/samurai.tres" id="theme"]
[ext_resource type="PackedScene" path="res://scenes/themes/samurai/arena.tscn" id="arena"]
[ext_resource type="PackedScene" path="res://scenes/themes/samurai/sentinel.tscn" id="enemy"]
[ext_resource type="PackedScene" path="res://scenes/themes/samurai/duelist.tscn" id="duelist"]
[ext_resource type="Shader" path="res://shaders/pixelate.gdshader" id="pixelate"]

[sub_resource type="ShaderMaterial" id="pixel_material"]
shader = ExtResource("pixelate")
shader_parameter/color_levels = 12.0
shader_parameter/dither_strength = 0.85
shader_parameter/exposure = 1.0
shader_parameter/contrast = 1.28
shader_parameter/saturation = 1.12
shader_parameter/bloom_strength = 1.15
shader_parameter/bloom_threshold = 0.63
shader_parameter/bloom_radius = 3.2
shader_parameter/bloom_saturation = 1.7
shader_parameter/aberration = 1.0
shader_parameter/scanline_strength = 0.1
shader_parameter/vignette_strength = 0.45

[sub_resource type="CapsuleShape3D" id="player_shape"]
radius = 0.32
height = 1.7

[sub_resource type="CapsuleShape3D" id="enemy_shape"]
radius = 0.48
height = 2.2

[node name="Parry2D" type="Node3D"]
script = ExtResource("main")
combat = ExtResource("combat")
duel_theme = ExtResource("theme")

[node name="Pixelate" type="SubViewportContainer" parent="."]
texture_filter = 1
material = SubResource("pixel_material")
anchors_preset = 15
anchor_right = 1.0
anchor_bottom = 1.0
mouse_filter = 2
stretch = true
stretch_shrink = 4

[node name="Viewport" type="SubViewport" parent="Pixelate"]
handle_input_locally = false
render_target_update_mode = 4

[node name="Stage" type="Node3D" parent="Pixelate/Viewport"]

[node name="World" parent="Pixelate/Viewport/Stage" instance=ExtResource("arena")]

[node name="Effects" type="Node3D" parent="Pixelate/Viewport"]

[node name="CameraRig" type="Node3D" parent="Pixelate/Viewport"]
position = Vector3(0, 1.35, 0)

[node name="Camera3D" type="Camera3D" parent="Pixelate/Viewport/CameraRig"]
position = Vector3(0, 0, 9)
projection = 1
size = 4.2
current = true
near = 0.05
far = 60.0

[node name="Player" type="CharacterBody3D" parent="Pixelate/Viewport"]
position = Vector3(-2.6, 0.05, 0)
collision_layer = 2
collision_mask = 5

[node name="CollisionShape3D" type="CollisionShape3D" parent="Pixelate/Viewport/Player"]
position = Vector3(0, 0.85, 0)
shape = SubResource("player_shape")

[node name="VisualMount" type="Node3D" parent="Pixelate/Viewport/Player"]
rotation = Vector3(0, -0.5, 0)

[node name="Visual" parent="Pixelate/Viewport/Player/VisualMount" instance=ExtResource("duelist")]

[node name="Sentinel" type="CharacterBody3D" parent="Pixelate/Viewport"]
position = Vector3(3, 0.05, 0)
rotation = Vector3(0, -1.5707963, 0)
collision_layer = 4
collision_mask = 3

[node name="CollisionShape3D" type="CollisionShape3D" parent="Pixelate/Viewport/Sentinel"]
position = Vector3(0, 1.1, 0)
shape = SubResource("enemy_shape")

[node name="VisualMount" type="Node3D" parent="Pixelate/Viewport/Sentinel"]
rotation = Vector3(0, 0.5, 0)

[node name="Visual" parent="Pixelate/Viewport/Sentinel/VisualMount" instance=ExtResource("enemy")]
```

### `shaders/pixelate.gdshader`

Reduz a paleta do mundo 3D já renderizado pequeno, com pontilhado ordenado.
A HUD não passa por este material.

```glsl
shader_type canvas_item;
render_mode unshaded;

// Acabamento do mundo 3D. Aplicado no SubViewportContainer: aqui a textura
// ainda está na resolução baixa, então tudo — aberração, brilho, pontilhado —
// acontece na escala do pixel da arte, não na da janela.
//
// A ordem importa e é esta:
//   1. aberração cromática ao amostrar
//   2. gradação: exposição, contraste, saturação e duotone sombra/luz
//   3. vinheta e linhas de varredura
//   4. paleta fechada do tema, com pontilhado ordenado
//   5. neon somado por cima
// A vinheta e as linhas entram ANTES da paleta de propósito: assim elas
// empurram o pixel para outra cor da lista em vez de inventar cor fora dela.
//
// O neon vem DEPOIS da paleta, e é a única coisa que escapa dela. Quando o
// brilho era calculado antes, a quantização o engolia: o halo caía na mesma
// entrada da paleta que a superfície ao lado e simplesmente desaparecia.
// Somado por cima, ele vira uma camada de luz sobre arte chapada — que é
// como um jogo 2D monta a cena, e é o que costura peças de origens
// diferentes numa imagem só.
//
// A HUD não passa por aqui: fica na resolução da janela para o texto seguir
// legível durante o duelo.

uniform sampler2D palette_tex : filter_nearest, repeat_disable;
uniform int palette_size : hint_range(0, 64) = 0;
uniform float color_levels : hint_range(2.0, 32.0) = 12.0;
uniform float dither_strength : hint_range(0.0, 1.0) = 0.85;

group_uniforms grade;
uniform float exposure : hint_range(0.1, 3.0) = 1.0;
uniform float contrast : hint_range(0.5, 3.0) = 1.0;
uniform float saturation : hint_range(0.0, 2.0) = 1.0;
uniform vec3 shadow_tint : source_color = vec3(0.5);
uniform vec3 highlight_tint : source_color = vec3(0.5);

group_uniforms effects;
uniform float bloom_strength : hint_range(0.0, 2.0) = 0.0;
uniform float bloom_threshold : hint_range(0.0, 1.0) = 0.7;
// Raio do halo em pixels da arte. É o que separa um contorno aceso de uma
// atmosfera acesa; abaixo de 2 o neon não chega a tingir a vizinhança.
uniform float bloom_radius : hint_range(1.0, 8.0) = 3.0;
// Luz neon é colorida. Sem isto o halo cresce branco e lava a cena.
uniform float bloom_saturation : hint_range(0.0, 3.0) = 1.6;
uniform float aberration : hint_range(0.0, 3.0) = 0.0;
uniform float scanline_strength : hint_range(0.0, 1.0) = 0.0;
uniform float vignette_strength : hint_range(0.0, 1.0) = 0.0;

const int MAX_PALETTE = 64;

// Matriz de Bayer 4x4, valores de 0 a 15.
const float BAYER[16] = {
	0.0, 8.0, 2.0, 10.0,
	12.0, 4.0, 14.0, 6.0,
	3.0, 11.0, 1.0, 9.0,
	15.0, 7.0, 13.0, 5.0
};

vec3 palette_color(int index) {
	float u = (float(index) + 0.5) / float(palette_size);
	return texture(palette_tex, vec2(u, 0.5)).rgb;
}

void fragment() {
	vec2 texel = 1.0 / vec2(textureSize(TEXTURE, 0));
	vec2 pixel = floor(UV / texel);
	int cell = int(mod(pixel.y, 4.0)) * 4 + int(mod(pixel.x, 4.0));
	float threshold = BAYER[cell] / 16.0;

	// Aberração cromática: vermelho e azul saem do centro em direções opostas.
	// O deslocamento é medido em pixels da arte, então não muda quando a
	// janela muda de tamanho. Amostrar aqui e não numa função auxiliar é
	// obrigatório: TEXTURE não pode ser passado como argumento.
	vec3 color;
	if (aberration > 0.0) {
		vec2 offset = (UV - vec2(0.5)) * aberration * texel;
		color = vec3(
			texture(TEXTURE, UV + offset).r,
			texture(TEXTURE, UV).g,
			texture(TEXTURE, UV - offset).b);
	} else {
		color = texture(TEXTURE, UV).rgb;
	}

	// Neon: três anéis de oito amostras, só do que passa do limiar. Peso
	// caindo para fora dá um halo que decai em vez de uma auréola dura.
	// Guardado aqui e somado no fim, depois da paleta.
	vec3 glow = vec3(0.0);
	if (bloom_strength > 0.0) {
		float weights[3] = {1.0, 0.55, 0.25};
		float rings[3] = {1.0, 2.2, 3.8};
		float total_weight = 0.0;
		for (int ring = 0; ring < 3; ring++) {
			for (int i = 0; i < 8; i++) {
				float angle = float(i) * (TAU / 8.0) + float(ring) * 0.4;
				vec2 direction = vec2(cos(angle), sin(angle));
				vec2 at = UV + direction * texel * bloom_radius * rings[ring];
				glow += max(texture(TEXTURE, at).rgb - vec3(bloom_threshold),
					vec3(0.0)) * weights[ring];
				total_weight += weights[ring];
			}
		}
		glow /= max(total_weight, 1e-4);
		float glow_luma = dot(glow, vec3(0.2126, 0.7152, 0.0722));
		glow = max(mix(vec3(glow_luma), glow, bloom_saturation), vec3(0.0));
	}

	// Gradação. O duotone é o que tira a cara de plástico: sombra e luz
	// deixam de ser a mesma cor mais escura e mais clara.
	color *= exposure;
	color = (color - vec3(0.5)) * contrast + vec3(0.5);
	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	color = mix(vec3(luma), color, saturation);
	// O duotone é normalizado pela própria luminância: ele desloca o matiz da
	// sombra e o da luz sem mexer no brilho. Sem isso, um tom roxo escuro
	// esmaga a cena inteira para preto — e o oponente some.
	vec3 tint = mix(shadow_tint, highlight_tint, clamp(luma, 0.0, 1.0));
	color *= tint / max(dot(tint, vec3(0.2126, 0.7152, 0.0722)), 1e-4);
	color = clamp(color, vec3(0.0), vec3(1.0));

	if (vignette_strength > 0.0) {
		float edge = length(UV - vec2(0.5)) * 1.4142;
		color *= 1.0 - smoothstep(0.45, 1.0, edge) * vignette_strength;
	}
	if (scanline_strength > 0.0 && mod(pixel.y, 2.0) >= 1.0) {
		color *= 1.0 - scanline_strength;
	}

	vec3 result;
	if (palette_size > 0) {
		int best = 0;
		int second = 0;
		float best_distance = 1e9;
		float second_distance = 1e9;
		for (int i = 0; i < MAX_PALETTE; i++) {
			if (i >= palette_size) {
				break;
			}
			vec3 diff = color - palette_color(i);
			// Peso perceptual: o olho separa verde bem melhor que azul.
			float d = dot(diff * diff, vec3(0.30, 0.59, 0.11));
			if (d < best_distance) {
				second_distance = best_distance;
				second = best;
				best_distance = d;
				best = i;
			} else if (d < second_distance) {
				second_distance = d;
				second = i;
			}
		}
		// Onde a cor cai no segmento entre as duas candidatas: 0 em cima da
		// mais próxima, 1 em cima da segunda. Superfície que bate com uma cor
		// da paleta fica chapada; só a transição entre duas alterna.
		vec3 near = palette_color(best);
		vec3 far = palette_color(second);
		vec3 segment = far - near;
		float position = clamp(dot(color - near, segment)
			/ max(dot(segment, segment), 1e-6), 0.0, 1.0);
		result = position * dither_strength > threshold ? far : near;
	} else {
		float offset = (threshold - 0.46875) * dither_strength / color_levels;
		result = floor((color + offset) * color_levels + 0.5) / color_levels;
	}
	// A camada de luz por cima da arte chapada. É a única cor da imagem que
	// não vem da paleta do tema, e é de propósito: brilho não é superfície.
	COLOR.rgb = clamp(result + glow * bloom_strength, vec3(0.0), vec3(1.0));
	COLOR.a = 1.0;
}
```

### `config/combat.tres`

```ini
[gd_resource type="Resource" script_class="CombatTuning" load_steps=2 format=3]

[ext_resource type="Script" path="res://scripts/config/combat_tuning.gd" id="1_script"]

[resource]
script = ExtResource("1_script")
parry_window = 0.24
perfect_window = 0.075
parry_cooldown = 0.62
enemy_windup = 0.9
feint_hold = 0.55
stun_duration = 1.6
player_max_health = 5
steadiness = 3
enemy_max_health = 5
movement_speed = 4.5
camera_shake = 0.65
parry_hitstop = 0.065
perfect_hitstop = 0.095
screen_flash_enabled = true
sound_volume_db = -10.0
```

### `config/themes/medieval.tres`

```ini
[gd_resource type="Resource" script_class="DuelTheme" load_steps=5 format=3]

[ext_resource type="Script" path="res://scripts/config/duel_theme.gd" id="script"]
[ext_resource type="PackedScene" path="res://scenes/themes/medieval/arena.tscn" id="arena"]
[ext_resource type="PackedScene" path="res://scenes/themes/medieval/sentinel.tscn" id="opponent"]
[ext_resource type="PackedScene" path="res://scenes/themes/medieval/duelist.tscn" id="duelist"]

[resource]
script = ExtResource("script")
theme_name = "Medieval"
stage_name = "Pátio da Coroa"
opponent_name = "CAVALEIRO"
guard_color = Color(0.643137, 0.835294, 0.929412, 1)
attack_color = Color(0.949020, 0.733333, 0.400000, 1)
damage_color = Color(0.937255, 0.462745, 0.439216, 1)
panel_color = Color(0.117647, 0.152941, 0.200000, 1)
text_color = Color(0.937255, 0.901961, 0.835294, 1)
palette = PackedColorArray(0.015686, 0.023529, 0.050980, 1, 0.039216, 0.058824, 0.109804, 1, 0.066667, 0.094118, 0.160784, 1, 0.094118, 0.129412, 0.227451, 1, 0.129412, 0.176471, 0.309804, 1, 0.172549, 0.231373, 0.400000, 1, 0.227451, 0.305882, 0.521569, 1, 0.298039, 0.400000, 0.658824, 1, 0.039216, 0.239216, 0.309804, 1, 0.070588, 0.376471, 0.478431, 1, 0.109804, 0.588235, 0.721569, 1, 0.200000, 0.823529, 0.941176, 1, 0.588235, 0.933333, 1.000000, 1, 0.309804, 0.164706, 0.039216, 1, 0.509804, 0.278431, 0.058824, 1, 0.721569, 0.415686, 0.094118, 1, 0.941176, 0.603922, 0.172549, 1, 1.000000, 0.811765, 0.478431, 1, 0.290196, 0.333333, 0.407843, 1, 0.454902, 0.509804, 0.603922, 1, 0.643137, 0.698039, 0.776471, 1, 0.858824, 0.894118, 0.941176, 1, 1.000000, 1.000000, 1.000000, 1, 0.290000, 0.175000, 0.060000, 1, 0.082000, 0.052000, 0.032000, 1, 0.055000, 0.036000, 0.024000, 1, 1.000000, 0.720000, 0.320000, 1)
shadow_tint = Color(0.180000, 0.260000, 0.520000, 1)
highlight_tint = Color(0.960000, 0.940000, 0.900000, 1)
arena_scene = ExtResource("arena")
opponent_scene = ExtResource("opponent")
duelist_scene = ExtResource("duelist")
weapon_pivot_path = NodePath("WeaponPivot")
attack_cue_path = NodePath("AttackCue")
```

### `config/themes/samurai.tres`

```ini
[gd_resource type="Resource" script_class="DuelTheme" load_steps=5 format=3]

[ext_resource type="Script" path="res://scripts/config/duel_theme.gd" id="script"]
[ext_resource type="PackedScene" path="res://scenes/themes/samurai/arena.tscn" id="arena"]
[ext_resource type="PackedScene" path="res://scenes/themes/samurai/sentinel.tscn" id="opponent"]
[ext_resource type="PackedScene" path="res://scenes/themes/samurai/duelist.tscn" id="duelist"]

[resource]
script = ExtResource("script")
theme_name = "Samurai"
stage_name = "Pátio do Bordo"
opponent_name = "RONIN"
guard_color = Color(0.662745, 0.898039, 0.811765, 1)
attack_color = Color(0.949020, 0.733333, 0.400000, 1)
damage_color = Color(0.937255, 0.462745, 0.439216, 1)
panel_color = Color(0.125490, 0.117647, 0.113725, 1)
text_color = Color(0.937255, 0.901961, 0.835294, 1)
palette = PackedColorArray(0.031373, 0.015686, 0.054902, 1, 0.066667, 0.031373, 0.125490, 1, 0.101961, 0.050980, 0.180392, 1, 0.141176, 0.070588, 0.235294, 1, 0.188235, 0.094118, 0.298039, 1, 0.262745, 0.141176, 0.349020, 1, 0.352941, 0.184314, 0.419608, 1, 0.454902, 0.250980, 0.494118, 1, 0.478431, 0.062745, 0.219608, 1, 0.647059, 0.086275, 0.290196, 1, 0.819608, 0.121569, 0.360784, 1, 1.000000, 0.227451, 0.447059, 1, 1.000000, 0.521569, 0.650980, 1, 0.360784, 0.172549, 0.031373, 1, 0.560784, 0.290196, 0.054902, 1, 0.800000, 0.439216, 0.086275, 1, 1.000000, 0.627451, 0.164706, 1, 1.000000, 0.815686, 0.478431, 1, 0.050980, 0.239216, 0.301961, 1, 0.082353, 0.388235, 0.478431, 1, 0.133333, 0.627451, 0.741176, 1, 0.309804, 0.847059, 0.949020, 1, 0.662745, 0.941176, 1.000000, 1, 0.419608, 0.392157, 0.470588, 1, 0.658824, 0.623529, 0.709804, 1, 0.866667, 0.839216, 0.909804, 1, 1.000000, 1.000000, 1.000000, 1, 0.145000, 0.290000, 0.270000, 1, 0.040000, 0.078000, 0.076000, 1, 0.026000, 0.055000, 0.056000, 1, 0.660000, 0.900000, 0.810000, 1)
shadow_tint = Color(0.300000, 0.180000, 0.520000, 1)
highlight_tint = Color(0.980000, 0.900000, 0.930000, 1)
arena_scene = ExtResource("arena")
opponent_scene = ExtResource("opponent")
duelist_scene = ExtResource("duelist")
weapon_pivot_path = NodePath("WeaponPivot")
attack_cue_path = NodePath("AttackCue")
```

### `scripts/main.gd`

```gdscript
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
```

### `scripts/player_controller.gd`

Corpo, câmera, guarda e tempos do jogador. Não decide o resultado de um golpe.

```gdscript
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
```

### `scripts/enemy_brain.gd`

Máquina de estados do oponente. Devolve eventos; `main.gd` escolhe a consequência.

```gdscript
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
```

### `scripts/config/combat_tuning.gd`

```gdscript
class_name CombatTuning
extends Resource
## Dados de combate compartilhados pelos dois temas. Edite config/combat.tres.

@export_group("Parry e dificuldade")
@export_range(0.12, 0.4, 0.01) var parry_window: float = 0.24
@export_range(0.03, 0.12, 0.005) var perfect_window: float = 0.075
@export_range(0.45, 1.0, 0.01) var parry_cooldown: float = 0.62
@export_range(0.55, 1.5, 0.01) var enemy_windup: float = 0.9
## Quanto o oponente segura o golpe depois de uma finta. Precisa ser maior
## que a recuperação do parry, senão quem cai na finta não tem defesa
## nenhuma; e menor que a soma dela com o tempo de reação, senão a finta
## não cobra nada.
@export_range(0.3, 1.2, 0.01) var feint_hold: float = 0.55
@export_range(0.8, 2.5, 0.05) var stun_duration: float = 1.6
@export_range(1, 10, 1) var player_max_health: int = 5
## Quantas defesas jogadas fora o duelista aguenta antes de a firmeza
## quebrar e custar um ponto de vida. É o preço de apertar à toa.
@export_range(1, 8, 1) var steadiness: int = 3
@export_range(1, 10, 1) var enemy_max_health: int = 5

@export_group("Controles")
@export_range(2.0, 7.0, 0.1) var movement_speed: float = 4.5

@export_group("Sensação do impacto")
@export_range(0.0, 1.0, 0.05) var camera_shake: float = 0.65
@export_range(0.0, 0.15, 0.005) var parry_hitstop: float = 0.065
@export_range(0.0, 0.2, 0.005) var perfect_hitstop: float = 0.095
@export var screen_flash_enabled: bool = true
@export_range(-40.0, 0.0, 1.0) var sound_volume_db: float = -10.0
```

### `scripts/config/duel_theme.gd`

```gdscript
class_name DuelTheme
extends Resource
## Aparência e cenas de um tema. Não contém regras de combate.

@export var theme_name: String = "Samurai"
@export var stage_name: String = "Pátio do Bordo"
@export var opponent_name: String = "RONIN"
@export var arena_scene: PackedScene
@export var opponent_scene: PackedScene
## Modelo do jogador. Na visão lateral ele é um lutador em cena, com a mesma
## estrutura do oponente: WeaponPivot com a arma dentro e Arms opcional.
@export var duelist_scene: PackedScene

@export_group("Cores da interface e do impacto")
@export var guard_color := Color("a9e5cf")
@export var attack_color := Color("f2bb66")
@export var damage_color := Color("ef7670")
@export var panel_color := Color("201e1d")
@export var text_color := Color("efe6d5")

## Paleta fechada do tema. O shader mapeia cada pixel do mundo 3D para a cor
## mais próxima desta lista. Vazia desliga a paleta e volta ao modo de degraus.
@export var palette: PackedColorArray = PackedColorArray()
## Duotone: cor que multiplica as sombras e cor que multiplica as luzes. É o
## que separa a sombra da luz por matiz, não só por brilho.
@export var shadow_tint := Color(0.5, 0.5, 0.5)
@export var highlight_tint := Color(0.5, 0.5, 0.5)

@export_group("Conexões do modelo")
@export var weapon_pivot_path: NodePath = ^"WeaponPivot"
@export var attack_cue_path: NodePath = ^"AttackCue"
## Nó cujos filhos Node3D são virados para o punho a cada quadro. Cada filho é
## um braço que aponta o próprio -Z para a arma. Vazio desliga o recurso.
@export var arms_path: NodePath = ^"Arms"
```

### `scripts/hud.gd`

```gdscript
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
```

### `scripts/guard_indicator.gd`

```gdscript
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
```

### `scripts/feedback.gd`

```gdscript
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
```

### `scripts/visuals/geometry.gd`

```gdscript
extends RefCounted
## Formas transitórias para faíscas. A arte permanente está em scenes/themes/.


static func material(color: Color, emissive: bool = false) -> StandardMaterial3D:
	var result := StandardMaterial3D.new()
	result.albedo_color = color
	result.roughness = 0.7
	result.emission_enabled = emissive
	result.emission = color
	return result


static func box(parent: Node3D, size: Vector3, pos: Vector3, color: Color,
		emissive: bool = false) -> MeshInstance3D:
	var shape := BoxMesh.new()
	shape.size = size
	var instance := MeshInstance3D.new()
	instance.mesh = shape
	instance.material_override = material(color, emissive)
	instance.position = pos
	parent.add_child(instance)
	return instance
```

### `scripts/visuals/arm_rig.gd`

```gdscript
extends RefCounted
## Mira dos braços de um modelo. Cada filho Node3D do nó `Arms` aponta o
## próprio -Z para o punho e estica até alcançá-lo.
##
## Sem isto o lutador carrega a espada com os braços parados: a arma sobe e a
## mão fica para trás, que é o que faz a pose parecer de boneco. O esticamento
## existe porque a distância do ombro ao punho muda muito entre as guardas —
## um braço de comprimento fixo passa longe do cabo nas poses curtas.

## Comprimento usado quando o modelo não declara o seu na meta "reach".
const DEFAULT_REACH := 0.63
const MIN_STRETCH := 0.55
const MAX_STRETCH := 1.45


static func aim(arms: Node3D, grip: Vector3) -> void:
	if arms == null:
		return
	for child in arms.get_children():
		var arm := child as Node3D
		if arm == null:
			continue
		var direction := grip - arm.global_position
		if direction.length_squared() < 0.0001:
			continue
		# Perto da vertical, o vetor de cima precisa sair do eixo do olhar.
		var up := Vector3.UP if absf(direction.normalized().y) < 0.985 else Vector3.BACK
		arm.global_basis = Basis.looking_at(direction, up)
		var reach: float = arm.get_meta("reach", DEFAULT_REACH)
		arm.scale = Vector3(1, 1,
			clampf(direction.length() / reach, MIN_STRETCH, MAX_STRETCH))
```

### `scripts/visuals/body_pose.gd`

```gdscript
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
```

### `scenes/themes/medieval/arena.tscn`

```ini
[gd_scene format=3]

[sub_resource type="Environment" id="Environment_bhjre"]
background_mode = 1
background_color = Color(0.023, 0.035, 0.062, 1)
ambient_light_source = 2
ambient_light_color = Color(0.133, 0.18, 0.341, 1)
ambient_light_energy = 0.32
tonemap_mode = 2
fog_enabled = true
fog_mode = 1
fog_light_color = Color(0.023, 0.035, 0.062, 1)
fog_density = 1.0
fog_depth_curve = 0.9
fog_depth_begin = 10.5
fog_depth_end = 26.0

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_2vv8v"]
albedo_color = Color(0.14209999, 0.1745, 0.24140003, 1)
roughness = 0.85

[sub_resource type="BoxMesh" id="BoxMesh_ss0wn"]
size = Vector3(46, 6, 7)

[sub_resource type="BoxShape3D" id="BoxShape3D_obrti"]
size = Vector3(46, 6, 7)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_expq4"]
albedo_color = Color(1, 0.62, 0.22, 1)
roughness = 0.5
emission_enabled = true
emission = Color(1, 0.62, 0.22, 1)
emission_energy_multiplier = 0.55

[sub_resource type="BoxMesh" id="BoxMesh_mpxti"]
size = Vector3(46, 0.035, 0.03)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_paedi"]
albedo_color = Color(0.09049, 0.11405, 0.16366002, 1)
roughness = 0.85

[sub_resource type="BoxMesh" id="BoxMesh_qt5rg"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_yarxo"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_kkdjx"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_g8yae"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_r2xei"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_3gg6h"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_srxeo"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_kbu7n"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_nd2kw"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_x2gt8"]
size = Vector3(46, 0.04, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_4fb4s"]
size = Vector3(46, 0.04, 0.02)

[sub_resource type="BoxShape3D" id="BoxShape3D_wktfj"]
size = Vector3(1, 6, 8)

[sub_resource type="BoxShape3D" id="BoxShape3D_1ssuq"]
size = Vector3(1, 6, 8)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_c7vp0"]
albedo_color = Color(0.055, 0.055, 0.08, 1)
roughness = 0.9

[sub_resource type="BoxMesh" id="BoxMesh_u1s61"]
size = Vector3(0.14, 3.4, 0.14)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_kyswu"]
albedo_color = Color(0.125, 0.08, 0.028, 1)
metallic = 0.1
roughness = 0.7

[sub_resource type="BoxMesh" id="BoxMesh_sug7o"]
size = Vector3(0.9, 1.9, 0.06)

[sub_resource type="BoxMesh" id="BoxMesh_n5bnh"]
size = Vector3(0.16, 0.26, 0.16)

[sub_resource type="BoxMesh" id="BoxMesh_qkxoh"]
size = Vector3(0.14, 3.4, 0.14)

[sub_resource type="BoxMesh" id="BoxMesh_1jdfj"]
size = Vector3(0.9, 1.9, 0.06)

[sub_resource type="BoxMesh" id="BoxMesh_bn7oy"]
size = Vector3(0.16, 0.26, 0.16)

[sub_resource type="BoxMesh" id="BoxMesh_3q85a"]
size = Vector3(0.16, 1.35, 0.16)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_cg2vd"]
albedo_color = Color(1, 0.62, 0.22, 1)
roughness = 0.5
emission_enabled = true
emission = Color(1, 0.62, 0.22, 1)
emission_energy_multiplier = 1.4

[sub_resource type="BoxMesh" id="BoxMesh_rmd1t"]
size = Vector3(0.34, 0.42, 0.34)

[sub_resource type="BoxMesh" id="BoxMesh_npr6o"]
size = Vector3(0.52, 0.09, 0.52)

[sub_resource type="BoxMesh" id="BoxMesh_7m1xu"]
size = Vector3(0.16, 1.35, 0.16)

[sub_resource type="BoxMesh" id="BoxMesh_4b2tc"]
size = Vector3(0.34, 0.42, 0.34)

[sub_resource type="BoxMesh" id="BoxMesh_c7i5s"]
size = Vector3(0.52, 0.09, 0.52)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_s8yqn"]
albedo_color = Color(0.13934, 0.17989999, 0.25436, 1)
roughness = 0.9

[sub_resource type="BoxMesh" id="BoxMesh_q7snt"]
size = Vector3(32, 1.7, 0.8)

[sub_resource type="BoxMesh" id="BoxMesh_gkwnp"]
size = Vector3(0.8, 0.5, 1)

[sub_resource type="BoxMesh" id="BoxMesh_h4dmj"]
size = Vector3(0.8, 0.5, 1)

[sub_resource type="BoxMesh" id="BoxMesh_j2t7g"]
size = Vector3(0.8, 0.5, 1)

[sub_resource type="BoxMesh" id="BoxMesh_rgi0r"]
size = Vector3(0.8, 0.5, 1)

[sub_resource type="BoxMesh" id="BoxMesh_y1ifg"]
size = Vector3(0.8, 0.5, 1)

[sub_resource type="BoxMesh" id="BoxMesh_ehsr5"]
size = Vector3(0.8, 0.5, 1)

[sub_resource type="BoxMesh" id="BoxMesh_hlb6x"]
size = Vector3(0.8, 0.5, 1)

[sub_resource type="BoxMesh" id="BoxMesh_h38vg"]
size = Vector3(0.8, 0.5, 1)

[sub_resource type="BoxMesh" id="BoxMesh_ttgwk"]
size = Vector3(0.8, 0.5, 1)

[sub_resource type="BoxMesh" id="BoxMesh_vq205"]
size = Vector3(0.8, 0.5, 1)

[sub_resource type="BoxMesh" id="BoxMesh_ex0o3"]
size = Vector3(0.8, 0.5, 1)

[sub_resource type="BoxMesh" id="BoxMesh_j550n"]
size = Vector3(0.8, 0.5, 1)

[sub_resource type="BoxMesh" id="BoxMesh_orapq"]
size = Vector3(0.8, 0.5, 1)

[sub_resource type="BoxMesh" id="BoxMesh_oe3ps"]
size = Vector3(0.8, 0.5, 1)

[sub_resource type="BoxMesh" id="BoxMesh_ct1ya"]
size = Vector3(0.8, 0.5, 1)

[sub_resource type="BoxMesh" id="BoxMesh_1fq7b"]
size = Vector3(0.8, 0.5, 1)

[sub_resource type="BoxMesh" id="BoxMesh_tve6k"]
size = Vector3(0.8, 0.5, 1)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_24oim"]
albedo_color = Color(0.19474001, 0.2489, 0.34596, 1)
roughness = 0.9

[sub_resource type="BoxMesh" id="BoxMesh_vg70r"]
size = Vector3(2.2, 1.5, 0.3)

[sub_resource type="BoxMesh" id="BoxMesh_bmmi5"]
size = Vector3(2.6, 0.22, 0.4)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_toc2h"]
albedo_color = Color(0.089480005, 0.1178, 0.17192, 1)
roughness = 0.9

[sub_resource type="BoxMesh" id="BoxMesh_f7opl"]
size = Vector3(3, 6.4, 1)

[sub_resource type="BoxMesh" id="BoxMesh_wm8yp"]
size = Vector3(3.8, 0.32, 1.2)

[sub_resource type="CylinderMesh" id="CylinderMesh_kiif2"]
top_radius = 0.05
bottom_radius = 1.9
height = 1.8
radial_segments = 6
rings = 1

[sub_resource type="BoxMesh" id="BoxMesh_a6cne"]
size = Vector3(3, 6.4, 1)

[sub_resource type="BoxMesh" id="BoxMesh_l78r1"]
size = Vector3(3.8, 0.32, 1.2)

[sub_resource type="CylinderMesh" id="CylinderMesh_kt1h3"]
top_radius = 0.05
bottom_radius = 1.9
height = 1.8
radial_segments = 6
rings = 1

[sub_resource type="BoxMesh" id="BoxMesh_gx05t"]
size = Vector3(9, 4.2, 1)

[node name="CrownCourtyard" type="Node3D" unique_id=1652150325]

[node name="WorldEnvironment" type="WorldEnvironment" parent="." unique_id=213391863]
environment = SubResource("Environment_bhjre")

[node name="Sun" type="DirectionalLight3D" parent="." unique_id=1760498576]
transform = Transform3D(0.40848747, 0.2697402, -0.8719967, 0, 0.9553365, 0.29552022, 0.91276395, -0.1207163, 0.390243, 0, 0, 0)
light_color = Color(0.94, 0.96, 1, 1)
light_energy = 0.55
shadow_enabled = true

[node name="Ground" type="StaticBody3D" parent="." unique_id=846176652]
collision_mask = 0

[node name="Face" type="MeshInstance3D" parent="Ground" unique_id=824301167]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, -3, -1.6)
material_override = SubResource("StandardMaterial3D_2vv8v")
mesh = SubResource("BoxMesh_ss0wn")

[node name="CollisionShape3D" type="CollisionShape3D" parent="Ground" unique_id=1543684008]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, -3, -1.6)
shape = SubResource("BoxShape3D_obrti")

[node name="GroundLine" type="MeshInstance3D" parent="." unique_id=314829829]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, -0.02, 1.9)
material_override = SubResource("StandardMaterial3D_expq4")
mesh = SubResource("BoxMesh_mpxti")

[node name="Slab0" type="MeshInstance3D" parent="." unique_id=1906004743]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -16, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_paedi")
mesh = SubResource("BoxMesh_qt5rg")

[node name="Slab1" type="MeshInstance3D" parent="." unique_id=1819580187]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -12, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_paedi")
mesh = SubResource("BoxMesh_yarxo")

[node name="Slab2" type="MeshInstance3D" parent="." unique_id=1563448503]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -8, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_paedi")
mesh = SubResource("BoxMesh_kkdjx")

[node name="Slab3" type="MeshInstance3D" parent="." unique_id=1483366825]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -4, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_paedi")
mesh = SubResource("BoxMesh_g8yae")

[node name="Slab4" type="MeshInstance3D" parent="." unique_id=1389258392]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_paedi")
mesh = SubResource("BoxMesh_r2xei")

[node name="Slab5" type="MeshInstance3D" parent="." unique_id=907393077]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 4, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_paedi")
mesh = SubResource("BoxMesh_3gg6h")

[node name="Slab6" type="MeshInstance3D" parent="." unique_id=932833919]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 8, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_paedi")
mesh = SubResource("BoxMesh_srxeo")

[node name="Slab7" type="MeshInstance3D" parent="." unique_id=2116844894]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 12, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_paedi")
mesh = SubResource("BoxMesh_kbu7n")

[node name="Slab8" type="MeshInstance3D" parent="." unique_id=1602886981]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 16, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_paedi")
mesh = SubResource("BoxMesh_nd2kw")

[node name="GroundBand0" type="MeshInstance3D" parent="." unique_id=2061165270]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, -0.55, 1.91)
material_override = SubResource("StandardMaterial3D_paedi")
mesh = SubResource("BoxMesh_x2gt8")

[node name="GroundBand1" type="MeshInstance3D" parent="." unique_id=106946225]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, -1.65, 1.91)
material_override = SubResource("StandardMaterial3D_paedi")
mesh = SubResource("BoxMesh_4fb4s")

[node name="WallL" type="StaticBody3D" parent="." unique_id=767366925]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -7.5, 2, 0)
collision_mask = 0

[node name="CollisionShape3D" type="CollisionShape3D" parent="WallL" unique_id=55751754]
shape = SubResource("BoxShape3D_wktfj")

[node name="WallR" type="StaticBody3D" parent="." unique_id=1078587332]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 7.5, 2, 0)
collision_mask = 0

[node name="CollisionShape3D" type="CollisionShape3D" parent="WallR" unique_id=1121440932]
shape = SubResource("BoxShape3D_1ssuq")

[node name="BannerPoleL" type="MeshInstance3D" parent="." unique_id=252318780]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -3.8, 1.7, -4.2)
material_override = SubResource("StandardMaterial3D_c7vp0")
mesh = SubResource("BoxMesh_u1s61")

[node name="BannerL" type="MeshInstance3D" parent="." unique_id=1102513476]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -3.8, 2.35, -4.1)
material_override = SubResource("StandardMaterial3D_kyswu")
mesh = SubResource("BoxMesh_sug7o")

[node name="BannerTipL" type="MeshInstance3D" parent="." unique_id=68306615]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -3.8, 3.5, -4.2)
material_override = SubResource("StandardMaterial3D_c7vp0")
mesh = SubResource("BoxMesh_n5bnh")

[node name="BannerPoleR" type="MeshInstance3D" parent="." unique_id=1172667516]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 3.8, 1.7, -4.2)
material_override = SubResource("StandardMaterial3D_c7vp0")
mesh = SubResource("BoxMesh_qkxoh")

[node name="BannerR" type="MeshInstance3D" parent="." unique_id=282204088]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 3.8, 2.35, -4.1)
material_override = SubResource("StandardMaterial3D_kyswu")
mesh = SubResource("BoxMesh_1jdfj")

[node name="BannerTipR" type="MeshInstance3D" parent="." unique_id=1924007857]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 3.8, 3.5, -4.2)
material_override = SubResource("StandardMaterial3D_c7vp0")
mesh = SubResource("BoxMesh_bn7oy")

[node name="Torch0Post" type="MeshInstance3D" parent="." unique_id=1322378804]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -6, 0.675, -2.2)
material_override = SubResource("StandardMaterial3D_c7vp0")
mesh = SubResource("BoxMesh_3q85a")

[node name="Torch0Lamp" type="MeshInstance3D" parent="." unique_id=642090995]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -6, 1.59, -2.2)
material_override = SubResource("StandardMaterial3D_cg2vd")
mesh = SubResource("BoxMesh_rmd1t")

[node name="Torch0Cap" type="MeshInstance3D" parent="." unique_id=433821368]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -6, 1.85, -2.2)
material_override = SubResource("StandardMaterial3D_c7vp0")
mesh = SubResource("BoxMesh_npr6o")

[node name="Torch0Light" type="OmniLight3D" parent="." unique_id=1963883613]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -6, 1.59, -1.6)
light_color = Color(1, 0.62, 0.22, 1)
light_energy = 3.0
light_specular = 1.0
omni_range = 5.5

[node name="Torch1Post" type="MeshInstance3D" parent="." unique_id=1103069154]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 6, 0.675, -2.2)
material_override = SubResource("StandardMaterial3D_c7vp0")
mesh = SubResource("BoxMesh_7m1xu")

[node name="Torch1Lamp" type="MeshInstance3D" parent="." unique_id=493662325]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 6, 1.59, -2.2)
material_override = SubResource("StandardMaterial3D_cg2vd")
mesh = SubResource("BoxMesh_4b2tc")

[node name="Torch1Cap" type="MeshInstance3D" parent="." unique_id=1287972232]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 6, 1.85, -2.2)
material_override = SubResource("StandardMaterial3D_c7vp0")
mesh = SubResource("BoxMesh_c7i5s")

[node name="Torch1Light" type="OmniLight3D" parent="." unique_id=1025497586]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 6, 1.59, -1.6)
light_color = Color(1, 0.62, 0.22, 1)
light_energy = 3.0
light_specular = 1.0
omni_range = 5.5

[node name="Wall" type="MeshInstance3D" parent="." unique_id=1095055277]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.85, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_q7snt")

[node name="Merlon0" type="MeshInstance3D" parent="." unique_id=1570070360]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -14.4, 1.95, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_gkwnp")

[node name="Merlon1" type="MeshInstance3D" parent="." unique_id=1341595128]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -12.6, 1.95, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_h4dmj")

[node name="Merlon2" type="MeshInstance3D" parent="." unique_id=1905031594]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -10.8, 1.95, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_j2t7g")

[node name="Merlon3" type="MeshInstance3D" parent="." unique_id=1327665906]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -9, 1.95, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_rgi0r")

[node name="Merlon4" type="MeshInstance3D" parent="." unique_id=1443558017]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -7.2, 1.95, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_y1ifg")

[node name="Merlon5" type="MeshInstance3D" parent="." unique_id=1428976948]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -5.4, 1.95, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_ehsr5")

[node name="Merlon6" type="MeshInstance3D" parent="." unique_id=1437010926]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -3.6, 1.95, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_hlb6x")

[node name="Merlon7" type="MeshInstance3D" parent="." unique_id=212264985]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -1.8, 1.95, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_h38vg")

[node name="Merlon8" type="MeshInstance3D" parent="." unique_id=366974027]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1.95, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_ttgwk")

[node name="Merlon9" type="MeshInstance3D" parent="." unique_id=355992592]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 1.8, 1.95, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_vq205")

[node name="Merlon10" type="MeshInstance3D" parent="." unique_id=1201381016]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 3.6, 1.95, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_ex0o3")

[node name="Merlon11" type="MeshInstance3D" parent="." unique_id=859751589]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 5.4, 1.95, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_j550n")

[node name="Merlon12" type="MeshInstance3D" parent="." unique_id=1762803480]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 7.2, 1.95, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_orapq")

[node name="Merlon13" type="MeshInstance3D" parent="." unique_id=168530132]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 9, 1.95, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_oe3ps")

[node name="Merlon14" type="MeshInstance3D" parent="." unique_id=109523863]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 10.8, 1.95, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_ct1ya")

[node name="Merlon15" type="MeshInstance3D" parent="." unique_id=2066223435]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 12.6, 1.95, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_1fq7b")

[node name="Merlon16" type="MeshInstance3D" parent="." unique_id=1979190932]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 14.4, 1.95, -7)
material_override = SubResource("StandardMaterial3D_s8yqn")
mesh = SubResource("BoxMesh_tve6k")

[node name="Gate" type="MeshInstance3D" parent="." unique_id=326224991]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.75, -6.55)
material_override = SubResource("StandardMaterial3D_24oim")
mesh = SubResource("BoxMesh_vg70r")

[node name="GateArch" type="MeshInstance3D" parent="." unique_id=627429014]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1.6, -6.55)
material_override = SubResource("StandardMaterial3D_24oim")
mesh = SubResource("BoxMesh_bmmi5")

[node name="Tower0" type="MeshInstance3D" parent="." unique_id=1153687933]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -7, 3.2, -14)
material_override = SubResource("StandardMaterial3D_toc2h")
mesh = SubResource("BoxMesh_f7opl")

[node name="TowerRoof0" type="MeshInstance3D" parent="." unique_id=498903877]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -7, 6.55, -14)
material_override = SubResource("StandardMaterial3D_toc2h")
mesh = SubResource("BoxMesh_wm8yp")

[node name="TowerSpire0" type="MeshInstance3D" parent="." unique_id=929253536]
transform = Transform3D(0.8660254, 0, 0.5, 0, 1, 0, -0.5, 0, 0.8660254, -7, 7.6, -14)
material_override = SubResource("StandardMaterial3D_toc2h")
mesh = SubResource("CylinderMesh_kiif2")

[node name="Tower1" type="MeshInstance3D" parent="." unique_id=1757554527]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 7, 3.2, -14)
material_override = SubResource("StandardMaterial3D_toc2h")
mesh = SubResource("BoxMesh_a6cne")

[node name="TowerRoof1" type="MeshInstance3D" parent="." unique_id=832469391]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 7, 6.55, -14)
material_override = SubResource("StandardMaterial3D_toc2h")
mesh = SubResource("BoxMesh_l78r1")

[node name="TowerSpire1" type="MeshInstance3D" parent="." unique_id=1927941083]
transform = Transform3D(0.8660254, 0, 0.5, 0, 1, 0, -0.5, 0, 0.8660254, 7, 7.6, -14)
material_override = SubResource("StandardMaterial3D_toc2h")
mesh = SubResource("CylinderMesh_kt1h3")

[node name="FarKeep" type="MeshInstance3D" parent="." unique_id=860571181]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 1.5, 2.1, -17)
material_override = SubResource("StandardMaterial3D_toc2h")
mesh = SubResource("BoxMesh_gx05t")
```

### `scenes/themes/medieval/sentinel.tscn`

```ini
[gd_scene format=3]

[ext_resource type="PackedScene" path="res://scenes/themes/medieval/weapon.tscn" id="1_jo8r6"]

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_r4cr0"]
albedo_color = Color(0.04, 0.058, 0.086, 1)
roughness = 0.9

[sub_resource type="BoxMesh" id="BoxMesh_jx7jn"]
size = Vector3(0.19, 0.52, 0.21)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_xsqtw"]
albedo_color = Color(0.01, 0.008, 0.018, 1)

[sub_resource type="BoxMesh" id="BoxMesh_1sxn3"]
size = Vector3(0.23, 0.12, 0.38)

[sub_resource type="BoxMesh" id="BoxMesh_4l3fb"]
size = Vector3(0.19, 0.52, 0.21)

[sub_resource type="BoxMesh" id="BoxMesh_041yx"]
size = Vector3(0.23, 0.12, 0.38)

[sub_resource type="CylinderMesh" id="CylinderMesh_qn46u"]
top_radius = 0.3
bottom_radius = 0.58
height = 0.66
radial_segments = 8
rings = 1

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_w3h8f"]
albedo_color = Color(0.24, 0.82, 1, 1)
roughness = 0.5
emission_enabled = true
emission = Color(0.24, 0.82, 1, 1)
emission_energy_multiplier = 0.9

[sub_resource type="CylinderMesh" id="CylinderMesh_kgl0q"]
top_radius = 0.575
bottom_radius = 0.59
height = 0.024
radial_segments = 8
rings = 1

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_ktfxt"]
albedo_color = Color(0.2, 0.24, 0.33, 1)
metallic = 0.85
roughness = 0.3

[sub_resource type="BoxMesh" id="BoxMesh_rr0wl"]
size = Vector3(0.26, 0.3, 0.05)

[sub_resource type="BoxMesh" id="BoxMesh_cnuv5"]
size = Vector3(0.26, 0.3, 0.05)

[sub_resource type="BoxMesh" id="BoxMesh_xf3y0"]
size = Vector3(0.26, 0.3, 0.05)

[sub_resource type="BoxMesh" id="BoxMesh_o05sk"]
size = Vector3(0.26, 0.3, 0.05)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_5bsbf"]
albedo_color = Color(0.062, 0.072, 0.102, 1)
metallic = 0.55
roughness = 0.34

[sub_resource type="BoxMesh" id="BoxMesh_8u5ii"]
size = Vector3(0.26, 0.3, 0.05)

[sub_resource type="BoxMesh" id="BoxMesh_id0rs"]
size = Vector3(0.26, 0.3, 0.05)

[sub_resource type="CylinderMesh" id="CylinderMesh_2pmpu"]
top_radius = 0.315
bottom_radius = 0.335
height = 0.15
radial_segments = 8
rings = 1

[sub_resource type="CylinderMesh" id="CylinderMesh_o8mlo"]
top_radius = 0.345
bottom_radius = 0.345
height = 0.022
radial_segments = 8
rings = 1

[sub_resource type="CylinderMesh" id="CylinderMesh_oqtqs"]
top_radius = 0.4
bottom_radius = 0.27
height = 0.62
radial_segments = 8
rings = 1

[sub_resource type="BoxMesh" id="BoxMesh_65656"]
size = Vector3(0.085, 0.46, 0.05)

[sub_resource type="BoxMesh" id="BoxMesh_y2jdv"]
size = Vector3(0.085, 0.46, 0.05)

[sub_resource type="BoxMesh" id="BoxMesh_fl5fg"]
size = Vector3(0.3, 0.09, 0.11)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_1l33f"]
albedo_color = Color(0.36, 0.43, 0.56, 1)
metallic = 0.9
roughness = 0.26

[sub_resource type="BoxMesh" id="BoxMesh_djv3t"]
size = Vector3(0.21, 0.1, 0.34)

[sub_resource type="BoxMesh" id="BoxMesh_c5dd3"]
size = Vector3(0.236, 0.1, 0.34)

[sub_resource type="BoxMesh" id="BoxMesh_iqw1b"]
size = Vector3(0.262, 0.1, 0.34)

[sub_resource type="BoxMesh" id="BoxMesh_f20ok"]
size = Vector3(0.26, 0.022, 0.03)

[sub_resource type="BoxMesh" id="BoxMesh_rv6gr"]
size = Vector3(0.21, 0.1, 0.34)

[sub_resource type="BoxMesh" id="BoxMesh_6b7yk"]
size = Vector3(0.236, 0.1, 0.34)

[sub_resource type="BoxMesh" id="BoxMesh_khoul"]
size = Vector3(0.262, 0.1, 0.34)

[sub_resource type="BoxMesh" id="BoxMesh_dssg3"]
size = Vector3(0.26, 0.022, 0.03)

[sub_resource type="CylinderMesh" id="CylinderMesh_0rb3w"]
top_radius = 0.105
bottom_radius = 0.115
height = 0.16
radial_segments = 8
rings = 1

[sub_resource type="BoxMesh" id="BoxMesh_3ntsf"]
size = Vector3(0.27, 0.25, 0.26)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_v6uud"]
albedo_color = Color(0.62, 0.95, 1, 1)
roughness = 0.5
emission_enabled = true
emission = Color(0.62, 0.95, 1, 1)
emission_energy_multiplier = 2.0

[sub_resource type="BoxMesh" id="BoxMesh_kvhln"]
size = Vector3(0.22, 0.042, 0.02)

[sub_resource type="CylinderMesh" id="CylinderMesh_tkmsw"]
top_radius = 0.085
bottom_radius = 0.235
height = 0.44
radial_segments = 8
rings = 1

[sub_resource type="CylinderMesh" id="CylinderMesh_2rcju"]
top_radius = 0.238
bottom_radius = 0.245
height = 0.022
radial_segments = 8
rings = 1

[sub_resource type="BoxMesh" id="BoxMesh_1ha44"]
size = Vector3(0.26, 0.22, 0.14)

[sub_resource type="BoxMesh" id="BoxMesh_05e3r"]
size = Vector3(0.026, 0.13, 0.02)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_tqbsr"]
albedo_color = Color(0.52, 0.33, 0.095, 1)
metallic = 0.8
roughness = 0.32

[sub_resource type="BoxMesh" id="BoxMesh_tiygi"]
size = Vector3(0.055, 0.22, 0.38)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_1w45m"]
albedo_color = Color(0.949, 0.733, 0.4, 1)
roughness = 0.5
emission_enabled = true
emission = Color(0.949, 0.733, 0.4, 1)
emission_energy_multiplier = 1.1

[sub_resource type="CylinderMesh" id="CylinderMesh_ksqxa"]
top_radius = 0.425
bottom_radius = 0.425
height = 0.085
radial_segments = 12
rings = 1

[sub_resource type="BoxMesh" id="BoxMesh_f4rov"]
size = Vector3(0.19, 0.1995, 0.3024)

[sub_resource type="BoxMesh" id="BoxMesh_vpl0d"]
size = Vector3(0.2128, 0.2128, 0.0882)

[sub_resource type="BoxMesh" id="BoxMesh_ro00j"]
size = Vector3(0.1634, 0.171, 0.2583)

[sub_resource type="BoxMesh" id="BoxMesh_qb7uj"]
size = Vector3(0.152, 0.1634, 0.1386)

[sub_resource type="BoxMesh" id="BoxMesh_xtat4"]
size = Vector3(0.175, 0.18375, 0.3024)

[sub_resource type="BoxMesh" id="BoxMesh_h16ic"]
size = Vector3(0.196, 0.196, 0.0882)

[sub_resource type="BoxMesh" id="BoxMesh_du8k1"]
size = Vector3(0.1505, 0.1575, 0.2583)

[sub_resource type="BoxMesh" id="BoxMesh_5x4k5"]
size = Vector3(0.14, 0.1505, 0.1386)

[node name="Knight" type="Node3D" unique_id=2101113782]

[node name="Body" type="Node3D" parent="." unique_id=651300898]
transform = Transform3D(0.9800666, 0, 0.19866933, 0, 1, 0, -0.19866933, 0, 0.9800666, 0, 0, 0)

[node name="FrontShin" type="MeshInstance3D" parent="Body" unique_id=1617415574]
transform = Transform3D(1, 0, 0, 0, 0.9950042, -0.09983342, 0, 0.09983342, 0.9950042, -0.25, 0.3, -0.13)
material_override = SubResource("StandardMaterial3D_r4cr0")
mesh = SubResource("BoxMesh_jx7jn")

[node name="FrontFoot" type="MeshInstance3D" parent="Body" unique_id=581739813]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -0.25, 0.06, -0.25)
material_override = SubResource("StandardMaterial3D_xsqtw")
mesh = SubResource("BoxMesh_1sxn3")

[node name="BackShin" type="MeshInstance3D" parent="Body" unique_id=1969571696]
transform = Transform3D(1, 0, 0, 0, 0.990216, 0.13954312, 0, -0.13954312, 0.990216, 0.27, 0.3, 0.13)
material_override = SubResource("StandardMaterial3D_r4cr0")
mesh = SubResource("BoxMesh_4l3fb")

[node name="BackFoot" type="MeshInstance3D" parent="Body" unique_id=2048342355]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0.27, 0.06, 0.21)
material_override = SubResource("StandardMaterial3D_xsqtw")
mesh = SubResource("BoxMesh_041yx")

[node name="Skirt" type="MeshInstance3D" parent="Body" unique_id=534717002]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 0.79, 0)
material_override = SubResource("StandardMaterial3D_r4cr0")
mesh = SubResource("CylinderMesh_qn46u")

[node name="SkirtHem" type="MeshInstance3D" parent="Body" unique_id=1463333180]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 0.475, 0)
material_override = SubResource("StandardMaterial3D_w3h8f")
mesh = SubResource("CylinderMesh_kgl0q")

[node name="HipPlate0" type="MeshInstance3D" parent="Body" unique_id=1388133167]
transform = Transform3D(0.49757108, 0.12104293, -0.8589363, 0, 0.990216, 0.13954312, 0.8674232, -0.069432616, 0.49270284, -0.299261, 0.92, -0.17166202)
material_override = SubResource("StandardMaterial3D_ktfxt")
mesh = SubResource("BoxMesh_rr0wl")

[node name="HipPlate1" type="MeshInstance3D" parent="Body" unique_id=508193537]
transform = Transform3D(0.9393727, 0.04784903, -0.3395429, 0, 0.990216, 0.13954312, 0.3428978, -0.131083, 0.9301819, -0.118299745, 0.92, -0.3240836)
material_override = SubResource("StandardMaterial3D_ktfxt")
mesh = SubResource("BoxMesh_cnuv5")

[node name="HipPlate2" type="MeshInstance3D" parent="Body" unique_id=186880993]
transform = Transform3D(0.9393727, -0.04784903, 0.3395429, 0, 0.990216, 0.13954312, -0.3428978, -0.131083, 0.9301819, 0.118299745, 0.92, -0.3240836)
material_override = SubResource("StandardMaterial3D_ktfxt")
mesh = SubResource("BoxMesh_xf3y0")

[node name="HipPlate3" type="MeshInstance3D" parent="Body" unique_id=1448733939]
transform = Transform3D(0.49757108, -0.12104293, 0.8589363, 0, 0.990216, 0.13954312, -0.8674232, -0.069432616, 0.49270284, 0.299261, 0.92, -0.17166202)
material_override = SubResource("StandardMaterial3D_ktfxt")
mesh = SubResource("BoxMesh_o05sk")

[node name="HipPlate4" type="MeshInstance3D" parent="Body" unique_id=551366803]
transform = Transform3D(-0.17824605, 0.13730846, -0.9743587, 0, 0.990216, 0.13954312, 0.98398596, 0.024873009, -0.1765021, -0.33947515, 0.92, 0.06149489)
material_override = SubResource("StandardMaterial3D_5bsbf")
mesh = SubResource("BoxMesh_8u5ii")

[node name="HipPlate5" type="MeshInstance3D" parent="Body" unique_id=1798651773]
transform = Transform3D(-0.17824605, -0.13730846, 0.9743587, 0, 0.990216, 0.13954312, -0.98398596, 0.024873009, -0.1765021, 0.33947515, 0.92, 0.06149489)
material_override = SubResource("StandardMaterial3D_5bsbf")
mesh = SubResource("BoxMesh_id0rs")

[node name="Belt" type="MeshInstance3D" parent="Body" unique_id=1779979542]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 1.1, 0)
material_override = SubResource("StandardMaterial3D_r4cr0")
mesh = SubResource("CylinderMesh_2pmpu")

[node name="BeltEdge" type="MeshInstance3D" parent="Body" unique_id=198943667]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 1.175, 0)
material_override = SubResource("StandardMaterial3D_w3h8f")
mesh = SubResource("CylinderMesh_o8mlo")

[node name="Torso" type="MeshInstance3D" parent="Body" unique_id=490786666]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 1.48, 0)
material_override = SubResource("StandardMaterial3D_ktfxt")
mesh = SubResource("CylinderMesh_oqtqs")

[node name="StrapL" type="MeshInstance3D" parent="Body" unique_id=1009438227]
transform = Transform3D(1, 0, 0, 0, 0.99820054, 0.059964005, 0, -0.059964005, 0.99820054, -0.135, 1.5, -0.335)
material_override = SubResource("StandardMaterial3D_5bsbf")
mesh = SubResource("BoxMesh_65656")

[node name="StrapR" type="MeshInstance3D" parent="Body" unique_id=1960015385]
transform = Transform3D(1, 0, 0, 0, 0.99820054, 0.059964005, 0, -0.059964005, 0.99820054, 0.135, 1.5, -0.335)
material_override = SubResource("StandardMaterial3D_5bsbf")
mesh = SubResource("BoxMesh_y2jdv")

[node name="Collar" type="MeshInstance3D" parent="Body" unique_id=1837199962]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1.79, -0.175)
material_override = SubResource("StandardMaterial3D_5bsbf")
mesh = SubResource("BoxMesh_fl5fg")

[node name="LeftSode0" type="MeshInstance3D" parent="Body" unique_id=1274669819]
transform = Transform3D(0.9553365, -0.29552022, 0, 0.29552022, 0.9553365, 0, 0, 0, 1, -0.375, 1.64, 0)
material_override = SubResource("StandardMaterial3D_1l33f")
mesh = SubResource("BoxMesh_djv3t")

[node name="LeftSode1" type="MeshInstance3D" parent="Body" unique_id=2002740215]
transform = Transform3D(0.9553365, -0.29552022, 0, 0.29552022, 0.9553365, 0, 0, 0, 1, -0.393, 1.52, 0)
material_override = SubResource("StandardMaterial3D_1l33f")
mesh = SubResource("BoxMesh_c5dd3")

[node name="LeftSode2" type="MeshInstance3D" parent="Body" unique_id=581262094]
transform = Transform3D(0.9553365, -0.29552022, 0, 0.29552022, 0.9553365, 0, 0, 0, 1, -0.411, 1.4, 0)
material_override = SubResource("StandardMaterial3D_1l33f")
mesh = SubResource("BoxMesh_iqw1b")

[node name="LeftSodeEdge" type="MeshInstance3D" parent="Body" unique_id=111992568]
transform = Transform3D(0.9553365, -0.29552022, 0, 0.29552022, 0.9553365, 0, 0, 0, 1, -0.425, 1.385, -0.155)
material_override = SubResource("StandardMaterial3D_w3h8f")
mesh = SubResource("BoxMesh_f20ok")

[node name="RightSode0" type="MeshInstance3D" parent="Body" unique_id=575379926]
transform = Transform3D(0.9553365, 0.29552022, 0, -0.29552022, 0.9553365, 0, 0, 0, 1, 0.375, 1.64, 0)
material_override = SubResource("StandardMaterial3D_1l33f")
mesh = SubResource("BoxMesh_rv6gr")

[node name="RightSode1" type="MeshInstance3D" parent="Body" unique_id=1301932063]
transform = Transform3D(0.9553365, 0.29552022, 0, -0.29552022, 0.9553365, 0, 0, 0, 1, 0.393, 1.52, 0)
material_override = SubResource("StandardMaterial3D_1l33f")
mesh = SubResource("BoxMesh_6b7yk")

[node name="RightSode2" type="MeshInstance3D" parent="Body" unique_id=1851584035]
transform = Transform3D(0.9553365, 0.29552022, 0, -0.29552022, 0.9553365, 0, 0, 0, 1, 0.411, 1.4, 0)
material_override = SubResource("StandardMaterial3D_1l33f")
mesh = SubResource("BoxMesh_khoul")

[node name="RightSodeEdge" type="MeshInstance3D" parent="Body" unique_id=1969599910]
transform = Transform3D(0.9553365, 0.29552022, 0, -0.29552022, 0.9553365, 0, 0, 0, 1, 0.425, 1.385, -0.155)
material_override = SubResource("StandardMaterial3D_w3h8f")
mesh = SubResource("BoxMesh_dssg3")

[node name="Head" type="Node3D" parent="." unique_id=407887255]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1.86, 0)

[node name="Neck" type="MeshInstance3D" parent="Head" unique_id=1935210872]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 0.02, 0)
material_override = SubResource("StandardMaterial3D_xsqtw")
mesh = SubResource("CylinderMesh_0rb3w")

[node name="Skull" type="MeshInstance3D" parent="Head" unique_id=209295373]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.16, 0)
material_override = SubResource("StandardMaterial3D_xsqtw")
mesh = SubResource("BoxMesh_3ntsf")

[node name="EyeSlit" type="MeshInstance3D" parent="Head" unique_id=132426104]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.195, -0.162)
material_override = SubResource("StandardMaterial3D_v6uud")
mesh = SubResource("BoxMesh_kvhln")

[node name="Helm" type="MeshInstance3D" parent="Head" unique_id=880264193]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 0.28, 0)
material_override = SubResource("StandardMaterial3D_1l33f")
mesh = SubResource("CylinderMesh_tkmsw")

[node name="HelmRing" type="MeshInstance3D" parent="Head" unique_id=1280840254]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 0.075, 0)
material_override = SubResource("StandardMaterial3D_w3h8f")
mesh = SubResource("CylinderMesh_2rcju")

[node name="Visor" type="MeshInstance3D" parent="Head" unique_id=854712674]
transform = Transform3D(1, 0, 0, 0, 0.9800666, -0.19866933, 0, 0.19866933, 0.9800666, 0, 0.16, -0.135)
material_override = SubResource("StandardMaterial3D_ktfxt")
mesh = SubResource("BoxMesh_1ha44")

[node name="Breath" type="MeshInstance3D" parent="Head" unique_id=786247755]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.11, -0.198)
material_override = SubResource("StandardMaterial3D_xsqtw")
mesh = SubResource("BoxMesh_05e3r")

[node name="Crest" type="MeshInstance3D" parent="Head" unique_id=218002684]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.56, 0.02)
material_override = SubResource("StandardMaterial3D_tqbsr")
mesh = SubResource("BoxMesh_tiygi")

[node name="AttackCue" type="MeshInstance3D" parent="." unique_id=506676237]
transform = Transform3D(0.9659258, 0, 0.25881904, 0, 1, 0, -0.25881904, 0, 0.9659258, 0, 1.62, 0)
material_override = SubResource("StandardMaterial3D_1w45m")
mesh = SubResource("CylinderMesh_ksqxa")

[node name="Arms" type="Node3D" parent="." unique_id=1590044047]

[node name="SwordArm" type="Node3D" parent="Arms" unique_id=1260808289]
transform = Transform3D(0.6689647, 0.5533361, -0.4962916, 0, 0.66769207, 0.7444375, 0.7432942, -0.4980024, 0.44666243, 0.42, 1.6, -0.02)
metadata/reach = 0.63

[node name="SwordArmUpper" type="MeshInstance3D" parent="Arms/SwordArm" unique_id=210323224]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.1512)
material_override = SubResource("StandardMaterial3D_5bsbf")
mesh = SubResource("BoxMesh_f4rov")

[node name="SwordArmElbow" type="MeshInstance3D" parent="Arms/SwordArm" unique_id=288438442]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.3213)
material_override = SubResource("StandardMaterial3D_ktfxt")
mesh = SubResource("BoxMesh_vpl0d")

[node name="SwordArmLower" type="MeshInstance3D" parent="Arms/SwordArm" unique_id=955546681]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.4473)
material_override = SubResource("StandardMaterial3D_5bsbf")
mesh = SubResource("BoxMesh_ro00j")

[node name="SwordArmHand" type="MeshInstance3D" parent="Arms/SwordArm" unique_id=1201204097]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.63)
material_override = SubResource("StandardMaterial3D_xsqtw")
mesh = SubResource("BoxMesh_qb7uj")

[node name="OffArm" type="Node3D" parent="Arms" unique_id=1337397587]
transform = Transform3D(0.17054145, 0.2694008, -0.94780743, 0, 0.9618988, 0.27340606, 0.98535055, -0.046627067, 0.1640436, -0.42, 1.6, -0.02)
metadata/reach = 0.63

[node name="OffArmUpper" type="MeshInstance3D" parent="Arms/OffArm" unique_id=1252680100]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.1512)
material_override = SubResource("StandardMaterial3D_5bsbf")
mesh = SubResource("BoxMesh_xtat4")

[node name="OffArmElbow" type="MeshInstance3D" parent="Arms/OffArm" unique_id=908567722]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.3213)
material_override = SubResource("StandardMaterial3D_ktfxt")
mesh = SubResource("BoxMesh_h16ic")

[node name="OffArmLower" type="MeshInstance3D" parent="Arms/OffArm" unique_id=947504307]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.4473)
material_override = SubResource("StandardMaterial3D_5bsbf")
mesh = SubResource("BoxMesh_du8k1")

[node name="OffArmHand" type="MeshInstance3D" parent="Arms/OffArm" unique_id=385285016]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.63)
material_override = SubResource("StandardMaterial3D_xsqtw")
mesh = SubResource("BoxMesh_5x4k5")

[node name="RimWarm" type="OmniLight3D" parent="." unique_id=591801016]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -0.58, 1.85, 0.5)
light_color = Color(1, 0.58, 0.2, 1)
light_energy = 3.4
light_specular = 1.0
omni_range = 1.55

[node name="RimCool" type="OmniLight3D" parent="." unique_id=1694203032]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0.58, 1.775, 0.5)
light_color = Color(0.35, 0.8, 1, 1)
light_energy = 2.8
light_specular = 1.0
omni_range = 1.55

[node name="KeyLight" type="SpotLight3D" parent="." unique_id=976977561]
transform = Transform3D(-0.91669595, 0.20244177, -0.34450808, 0, 0.8621638, 0.5066295, 0.3995854, 0.46442524, -0.7903421, -0.85, 2.3, -1.95)
light_color = Color(1, 0.95, 0.98, 1)
light_energy = 3.2
spot_range = 6.0
spot_angle = 36.0
spot_angle_attenuation = 1.4

[node name="WeaponPivot" type="Node3D" parent="." unique_id=520022124]
transform = Transform3D(1, 0, 0, 0, 0.9887711, 0.14943814, 0, -0.14943814, 0.9887711, 0.62, 1.3, -0.2)

[node name="WeaponVisual" type="Node3D" parent="WeaponPivot" unique_id=968405822 instance=ExtResource("1_jo8r6")]
```

### `scenes/themes/medieval/duelist.tscn`

```ini
[gd_scene format=3]

[ext_resource type="PackedScene" path="res://scenes/themes/medieval/weapon.tscn" id="1_c8usm"]

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_t5krx"]
albedo_color = Color(0.055, 0.036, 0.024, 1)
roughness = 0.9

[sub_resource type="BoxMesh" id="BoxMesh_bqduc"]
size = Vector3(0.16, 0.46, 0.18)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_bpjnl"]
albedo_color = Color(0.01, 0.008, 0.018, 1)

[sub_resource type="BoxMesh" id="BoxMesh_bhmdc"]
size = Vector3(0.2, 0.1, 0.32)

[sub_resource type="BoxMesh" id="BoxMesh_nsd25"]
size = Vector3(0.16, 0.46, 0.18)

[sub_resource type="BoxMesh" id="BoxMesh_m5i65"]
size = Vector3(0.2, 0.1, 0.32)

[sub_resource type="CylinderMesh" id="CylinderMesh_bqjgf"]
top_radius = 0.24
bottom_radius = 0.44
height = 0.58
radial_segments = 8
rings = 1

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_u5u2t"]
albedo_color = Color(1, 0.72, 0.32, 1)
roughness = 0.5
emission_enabled = true
emission = Color(1, 0.72, 0.32, 1)

[sub_resource type="CylinderMesh" id="CylinderMesh_f8ot7"]
top_radius = 0.435
bottom_radius = 0.45
height = 0.022
radial_segments = 8
rings = 1

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_5vah8"]
albedo_color = Color(0.29, 0.175, 0.06, 1)
metallic = 0.15
roughness = 0.55

[sub_resource type="CylinderMesh" id="CylinderMesh_6b2pw"]
top_radius = 0.245
bottom_radius = 0.26
height = 0.13
radial_segments = 8
rings = 1

[sub_resource type="CylinderMesh" id="CylinderMesh_s4ni2"]
top_radius = 0.272
bottom_radius = 0.272
height = 0.02
radial_segments = 8
rings = 1

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_l5y5t"]
albedo_color = Color(0.082, 0.052, 0.032, 1)
metallic = 0.25
roughness = 0.45

[sub_resource type="CylinderMesh" id="CylinderMesh_bg8ul"]
top_radius = 0.315
bottom_radius = 0.215
height = 0.52
radial_segments = 8
rings = 1

[sub_resource type="BoxMesh" id="BoxMesh_lcnsg"]
size = Vector3(0.075, 0.44, 0.05)

[sub_resource type="BoxMesh" id="BoxMesh_n1dv6"]
size = Vector3(0.075, 0.44, 0.05)

[sub_resource type="BoxMesh" id="BoxMesh_2dx0o"]
size = Vector3(0.72, 0.11, 0.3)

[sub_resource type="BoxMesh" id="BoxMesh_db0vv"]
size = Vector3(0.74, 0.02, 0.028)

[sub_resource type="CylinderMesh" id="CylinderMesh_ovkrp"]
top_radius = 0.085
bottom_radius = 0.095
height = 0.14
radial_segments = 8
rings = 1

[sub_resource type="BoxMesh" id="BoxMesh_hmuj3"]
size = Vector3(0.28, 0.29, 0.28)

[sub_resource type="BoxMesh" id="BoxMesh_iceru"]
size = Vector3(0.22, 0.17, 0.04)

[sub_resource type="BoxMesh" id="BoxMesh_wlu61"]
size = Vector3(0.3, 0.055, 0.3)

[sub_resource type="CylinderMesh" id="CylinderMesh_w5jns"]
top_radius = 0.15
bottom_radius = 0.17
height = 0.14
radial_segments = 8
rings = 1

[sub_resource type="BoxMesh" id="BoxMesh_o4j7d"]
size = Vector3(0.155, 0.16275, 0.2496)

[sub_resource type="BoxMesh" id="BoxMesh_tatwo"]
size = Vector3(0.1736, 0.1736, 0.0728)

[sub_resource type="BoxMesh" id="BoxMesh_106rm"]
size = Vector3(0.1333, 0.1395, 0.2132)

[sub_resource type="BoxMesh" id="BoxMesh_b13ww"]
size = Vector3(0.124, 0.1333, 0.1144)

[sub_resource type="BoxMesh" id="BoxMesh_d8s0a"]
size = Vector3(0.145, 0.15225, 0.2496)

[sub_resource type="BoxMesh" id="BoxMesh_6rmdg"]
size = Vector3(0.1624, 0.1624, 0.0728)

[sub_resource type="BoxMesh" id="BoxMesh_2jv5e"]
size = Vector3(0.1247, 0.1305, 0.2132)

[sub_resource type="BoxMesh" id="BoxMesh_hr51b"]
size = Vector3(0.116, 0.1247, 0.1144)

[node name="Duelist" type="Node3D" unique_id=2085880227]

[node name="Body" type="Node3D" parent="." unique_id=1700437516]
transform = Transform3D(0.98722726, 0, -0.15931821, 0, 1, 0, 0.15931821, 0, 0.98722726, 0, 0, 0)

[node name="FrontShin" type="MeshInstance3D" parent="Body" unique_id=1822692275]
transform = Transform3D(1, 0, 0, 0, 0.99280864, -0.119712204, 0, 0.119712204, 0.99280864, -0.19, 0.26, -0.14)
material_override = SubResource("StandardMaterial3D_t5krx")
mesh = SubResource("BoxMesh_bqduc")

[node name="FrontFoot" type="MeshInstance3D" parent="Body" unique_id=482715282]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -0.19, 0.05, -0.24)
material_override = SubResource("StandardMaterial3D_bpjnl")
mesh = SubResource("BoxMesh_bhmdc")

[node name="BackShin" type="MeshInstance3D" parent="Body" unique_id=2068798454]
transform = Transform3D(1, 0, 0, 0, 0.98722726, 0.15931821, 0, -0.15931821, 0.98722726, 0.21, 0.26, 0.14)
material_override = SubResource("StandardMaterial3D_t5krx")
mesh = SubResource("BoxMesh_nsd25")

[node name="BackFoot" type="MeshInstance3D" parent="Body" unique_id=31992997]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0.21, 0.05, 0.2)
material_override = SubResource("StandardMaterial3D_bpjnl")
mesh = SubResource("BoxMesh_m5i65")

[node name="Coat" type="MeshInstance3D" parent="Body" unique_id=65736360]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 0.66, 0)
material_override = SubResource("StandardMaterial3D_t5krx")
mesh = SubResource("CylinderMesh_bqjgf")

[node name="CoatHem" type="MeshInstance3D" parent="Body" unique_id=2100589992]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 0.385, 0)
material_override = SubResource("StandardMaterial3D_u5u2t")
mesh = SubResource("CylinderMesh_f8ot7")

[node name="Sash" type="MeshInstance3D" parent="Body" unique_id=1277969252]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 0.96, 0)
material_override = SubResource("StandardMaterial3D_5vah8")
mesh = SubResource("CylinderMesh_6b2pw")

[node name="SashEdge" type="MeshInstance3D" parent="Body" unique_id=683594688]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 1.02, 0)
material_override = SubResource("StandardMaterial3D_u5u2t")
mesh = SubResource("CylinderMesh_s4ni2")

[node name="Torso" type="MeshInstance3D" parent="Body" unique_id=1345280652]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 1.29, 0)
material_override = SubResource("StandardMaterial3D_l5y5t")
mesh = SubResource("CylinderMesh_bg8ul")

[node name="LapelL" type="MeshInstance3D" parent="Body" unique_id=1349921344]
transform = Transform3D(0.97589743, -0.21822962, 0, 0.21822962, 0.97589743, 0, 0, 0, 1, -0.1, 1.32, -0.265)
material_override = SubResource("StandardMaterial3D_5vah8")
mesh = SubResource("BoxMesh_lcnsg")

[node name="LapelR" type="MeshInstance3D" parent="Body" unique_id=657247390]
transform = Transform3D(0.97589743, 0.21822962, 0, -0.21822962, 0.97589743, 0, 0, 0, 1, 0.1, 1.32, -0.265)
material_override = SubResource("StandardMaterial3D_5vah8")
mesh = SubResource("BoxMesh_n1dv6")

[node name="Shoulder" type="MeshInstance3D" parent="Body" unique_id=595133727]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1.5, -0.02)
material_override = SubResource("StandardMaterial3D_l5y5t")
mesh = SubResource("BoxMesh_2dx0o")

[node name="ShoulderEdge" type="MeshInstance3D" parent="Body" unique_id=738645099]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1.555, -0.145)
material_override = SubResource("StandardMaterial3D_u5u2t")
mesh = SubResource("BoxMesh_db0vv")

[node name="Head" type="Node3D" parent="." unique_id=2143373299]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1.56, 0)

[node name="Neck" type="MeshInstance3D" parent="Head" unique_id=525983372]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 0.02, 0)
material_override = SubResource("StandardMaterial3D_bpjnl")
mesh = SubResource("CylinderMesh_ovkrp")

[node name="Skull" type="MeshInstance3D" parent="Head" unique_id=1590571412]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.22, 0)
material_override = SubResource("StandardMaterial3D_l5y5t")
mesh = SubResource("BoxMesh_hmuj3")

[node name="Face" type="MeshInstance3D" parent="Head" unique_id=778026251]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.19, -0.145)
material_override = SubResource("StandardMaterial3D_bpjnl")
mesh = SubResource("BoxMesh_iceru")

[node name="Headband" type="MeshInstance3D" parent="Head" unique_id=1534812202]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.31, 0)
material_override = SubResource("StandardMaterial3D_u5u2t")
mesh = SubResource("BoxMesh_wlu61")

[node name="Coif" type="MeshInstance3D" parent="Head" unique_id=450039697]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 0.4, 0)
material_override = SubResource("StandardMaterial3D_5vah8")
mesh = SubResource("CylinderMesh_w5jns")

[node name="Arms" type="Node3D" parent="." unique_id=2017792483]

[node name="SwordArm" type="Node3D" parent="Arms" unique_id=1240604336]
transform = Transform3D(0.99745864, 0.056185044, -0.04381082, 0, 0.6149138, 0.78859437, 0.07124708, -0.7865903, 0.61335117, 0.32, 1.44, -0.02)
metadata/reach = 0.52

[node name="SwordArmUpper" type="MeshInstance3D" parent="Arms/SwordArm" unique_id=3520476]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.1248)
material_override = SubResource("StandardMaterial3D_l5y5t")
mesh = SubResource("BoxMesh_o4j7d")

[node name="SwordArmElbow" type="MeshInstance3D" parent="Arms/SwordArm" unique_id=653157677]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.2652)
material_override = SubResource("StandardMaterial3D_5vah8")
mesh = SubResource("BoxMesh_tatwo")

[node name="SwordArmLower" type="MeshInstance3D" parent="Arms/SwordArm" unique_id=1080643803]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.3692)
material_override = SubResource("StandardMaterial3D_l5y5t")
mesh = SubResource("BoxMesh_106rm")

[node name="SwordArmHand" type="MeshInstance3D" parent="Arms/SwordArm" unique_id=1471523197]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.52)
material_override = SubResource("StandardMaterial3D_bpjnl")
mesh = SubResource("BoxMesh_b13ww")

[node name="OffArm" type="Node3D" parent="Arms" unique_id=721344683]
transform = Transform3D(0.39054987, 0.4131016, -0.8226894, 0, 0.8936625, 0.44873974, 0.92058176, -0.17525524, 0.34901977, -0.32, 1.44, -0.02)
metadata/reach = 0.52

[node name="OffArmUpper" type="MeshInstance3D" parent="Arms/OffArm" unique_id=971175487]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.1248)
material_override = SubResource("StandardMaterial3D_l5y5t")
mesh = SubResource("BoxMesh_d8s0a")

[node name="OffArmElbow" type="MeshInstance3D" parent="Arms/OffArm" unique_id=1943798721]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.2652)
material_override = SubResource("StandardMaterial3D_5vah8")
mesh = SubResource("BoxMesh_6rmdg")

[node name="OffArmLower" type="MeshInstance3D" parent="Arms/OffArm" unique_id=1275863418]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.3692)
material_override = SubResource("StandardMaterial3D_l5y5t")
mesh = SubResource("BoxMesh_2jv5e")

[node name="OffArmHand" type="MeshInstance3D" parent="Arms/OffArm" unique_id=866715989]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.52)
material_override = SubResource("StandardMaterial3D_bpjnl")
mesh = SubResource("BoxMesh_hr51b")

[node name="RimWarm" type="OmniLight3D" parent="." unique_id=690674661]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -0.58, 1.443, 0.5)
light_color = Color(1, 0.66, 0.26, 1)
light_energy = 3.4
light_specular = 1.0
omni_range = 1.55

[node name="RimCool" type="OmniLight3D" parent="." unique_id=1567663430]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0.58, 1.3845, 0.5)
light_color = Color(1, 0.4, 0.3, 1)
light_energy = 2.8
light_specular = 1.0
omni_range = 1.55

[node name="KeyLight" type="SpotLight3D" parent="." unique_id=84804392]
transform = Transform3D(-0.91669595, 0.16649354, -0.36324704, 0, 0.90905976, 0.4166657, 0.39958543, 0.38195577, -0.8333314, -0.85, 1.794, -1.95)
light_color = Color(1, 0.95, 0.98, 1)
light_energy = 3.2
spot_range = 6.0
spot_angle = 36.0
spot_angle_attenuation = 1.4

[node name="WeaponPivot" type="Node3D" parent="." unique_id=369921237]
transform = Transform3D(1, 0, 0, 0, 0.9887711, 0.14943814, 0, -0.14943814, 0.9887711, 0.34, 1.08, -0.3)

[node name="WeaponVisual" type="Node3D" parent="WeaponPivot" unique_id=2113657674 instance=ExtResource("1_c8usm")]
```

### `scenes/themes/medieval/weapon.tscn`

```ini
[gd_scene load_steps=18 format=3]

[sub_resource type="StandardMaterial3D" id="Steel"]
albedo_color = Color(0.768627, 0.800000, 0.807843, 1)
roughness = 0.72
metallic = 0.75

[sub_resource type="StandardMaterial3D" id="Edge"]
albedo_color = Color(0.933333, 0.945098, 0.905882, 1)
roughness = 0.72
metallic = 0.65

[sub_resource type="StandardMaterial3D" id="Brass"]
albedo_color = Color(0.709804, 0.541176, 0.301961, 1)
roughness = 0.72
metallic = 0.55

[sub_resource type="StandardMaterial3D" id="Grip"]
albedo_color = Color(0.286275, 0.196078, 0.160784, 1)
roughness = 0.72
metallic = 0.0

[sub_resource type="StandardMaterial3D" id="Wrap"]
albedo_color = Color(0.466667, 0.380392, 0.298039, 1)
roughness = 0.72
metallic = 0.0

[sub_resource type="CylinderMesh" id="Root_Handle_mesh"]
top_radius = 0.04
bottom_radius = 0.04
height = 0.3
radial_segments = 12
rings = 1

[sub_resource type="CylinderMesh" id="Root_Wrap0_mesh"]
top_radius = 0.043
bottom_radius = 0.043
height = 0.018
radial_segments = 12
rings = 1

[sub_resource type="CylinderMesh" id="Root_Wrap1_mesh"]
top_radius = 0.043
bottom_radius = 0.043
height = 0.018
radial_segments = 12
rings = 1

[sub_resource type="CylinderMesh" id="Root_Wrap2_mesh"]
top_radius = 0.043
bottom_radius = 0.043
height = 0.018
radial_segments = 12
rings = 1

[sub_resource type="CylinderMesh" id="Root_Wrap3_mesh"]
top_radius = 0.043
bottom_radius = 0.043
height = 0.018
radial_segments = 12
rings = 1

[sub_resource type="CylinderMesh" id="Root_Wrap4_mesh"]
top_radius = 0.043
bottom_radius = 0.043
height = 0.018
radial_segments = 12
rings = 1

[sub_resource type="CylinderMesh" id="Root_Wrap5_mesh"]
top_radius = 0.043
bottom_radius = 0.043
height = 0.018
radial_segments = 12
rings = 1

[sub_resource type="BoxMesh" id="Root_Crossguard_mesh"]
size = Vector3(0.4, 0.055, 0.09)

[sub_resource type="SphereMesh" id="Root_Pommel_mesh"]
radius = 0.062
height = 0.13
radial_segments = 12
rings = 6

[sub_resource type="BoxMesh" id="Root_Blade_mesh"]
size = Vector3(0.115, 1.04, 0.035)

[sub_resource type="BoxMesh" id="Root_Fuller_mesh"]
size = Vector3(0.022, 0.93, 0.039)

[sub_resource type="CylinderMesh" id="Root_Tip_mesh"]
top_radius = 0
bottom_radius = 0.068
height = 0.18
radial_segments = 12
rings = 1

[node name="Longsword" type="Node3D"]


[node name="Handle" type="MeshInstance3D" parent="."]
position = Vector3(0, -0.16, 0)
mesh = SubResource("Root_Handle_mesh")
material_override = SubResource("Grip")

[node name="Wrap0" type="MeshInstance3D" parent="."]
position = Vector3(0, -0.29, 0)
mesh = SubResource("Root_Wrap0_mesh")
material_override = SubResource("Wrap")

[node name="Wrap1" type="MeshInstance3D" parent="."]
position = Vector3(0, -0.24, 0)
mesh = SubResource("Root_Wrap1_mesh")
material_override = SubResource("Wrap")

[node name="Wrap2" type="MeshInstance3D" parent="."]
position = Vector3(0, -0.19, 0)
mesh = SubResource("Root_Wrap2_mesh")
material_override = SubResource("Wrap")

[node name="Wrap3" type="MeshInstance3D" parent="."]
position = Vector3(0, -0.14, 0)
mesh = SubResource("Root_Wrap3_mesh")
material_override = SubResource("Wrap")

[node name="Wrap4" type="MeshInstance3D" parent="."]
position = Vector3(0, -0.09, 0)
mesh = SubResource("Root_Wrap4_mesh")
material_override = SubResource("Wrap")

[node name="Wrap5" type="MeshInstance3D" parent="."]
position = Vector3(0, -0.04, 0)
mesh = SubResource("Root_Wrap5_mesh")
material_override = SubResource("Wrap")

[node name="Crossguard" type="MeshInstance3D" parent="."]
position = Vector3(0, 0, 0)
mesh = SubResource("Root_Crossguard_mesh")
material_override = SubResource("Brass")

[node name="Pommel" type="MeshInstance3D" parent="."]
position = Vector3(0, -0.35, 0)
mesh = SubResource("Root_Pommel_mesh")
material_override = SubResource("Brass")

[node name="Blade" type="MeshInstance3D" parent="."]
position = Vector3(0, 0.55, 0)
mesh = SubResource("Root_Blade_mesh")
material_override = SubResource("Steel")

[node name="Fuller" type="MeshInstance3D" parent="."]
position = Vector3(0, 0.52, 0)
mesh = SubResource("Root_Fuller_mesh")
material_override = SubResource("Edge")

[node name="Tip" type="MeshInstance3D" parent="."]
position = Vector3(0, 1.16, 0)
mesh = SubResource("Root_Tip_mesh")
material_override = SubResource("Steel")
scale = Vector3(1, 1, 0.3)
```

### `scenes/themes/samurai/arena.tscn`

```ini
[gd_scene format=3]

[sub_resource type="Environment" id="Environment_lclij"]
background_mode = 1
background_color = Color(0.043, 0.023, 0.07, 1)
ambient_light_source = 2
ambient_light_color = Color(0.22, 0.153, 0.353, 1)
ambient_light_energy = 0.32
tonemap_mode = 2
fog_enabled = true
fog_mode = 1
fog_light_color = Color(0.043, 0.023, 0.07, 1)
fog_density = 1.0
fog_depth_curve = 0.9
fog_depth_begin = 10.5
fog_depth_end = 26.0

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_dqsm4"]
albedo_color = Color(0.19510001, 0.1601, 0.24700001, 1)
roughness = 0.85

[sub_resource type="BoxMesh" id="BoxMesh_yyfj8"]
size = Vector3(46, 6, 7)

[sub_resource type="BoxShape3D" id="BoxShape3D_j2jl6"]
size = Vector3(46, 6, 7)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_tlf3e"]
albedo_color = Color(0.18, 0.9, 1, 1)
roughness = 0.5
emission_enabled = true
emission = Color(0.18, 0.9, 1, 1)
emission_energy_multiplier = 0.55

[sub_resource type="BoxMesh" id="BoxMesh_56t0o"]
size = Vector3(46, 0.035, 0.03)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_gavuo"]
albedo_color = Color(0.12919001, 0.10069, 0.1703, 1)
roughness = 0.85

[sub_resource type="BoxMesh" id="BoxMesh_iiul7"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_f5hp1"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_rdgw0"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_62a12"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_oan2d"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_jqdbi"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_1sy8q"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_f0xyv"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_33qv2"]
size = Vector3(0.05, 1.5, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_abtvs"]
size = Vector3(46, 0.04, 0.02)

[sub_resource type="BoxMesh" id="BoxMesh_hqsqp"]
size = Vector3(46, 0.04, 0.02)

[sub_resource type="BoxShape3D" id="BoxShape3D_neg80"]
size = Vector3(1, 6, 8)

[sub_resource type="BoxShape3D" id="BoxShape3D_1fsyb"]
size = Vector3(1, 6, 8)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_vmfai"]
albedo_color = Color(0.15, 0.026, 0.062, 1)
metallic = 0.1
roughness = 0.7

[sub_resource type="BoxMesh" id="BoxMesh_i185f"]
size = Vector3(0.3, 3.2, 0.3)

[sub_resource type="BoxMesh" id="BoxMesh_egl4s"]
size = Vector3(0.3, 3.2, 0.3)

[sub_resource type="BoxMesh" id="BoxMesh_xiwjr"]
size = Vector3(8.4, 0.26, 0.4)

[sub_resource type="BoxMesh" id="BoxMesh_o8mhe"]
size = Vector3(9.4, 0.22, 0.52)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_jnvxj"]
albedo_color = Color(0.075, 0.045, 0.075, 1)
roughness = 0.9

[sub_resource type="BoxMesh" id="BoxMesh_n0417"]
size = Vector3(0.7, 0.5, 0.1)

[sub_resource type="BoxMesh" id="BoxMesh_1s81r"]
size = Vector3(0.16, 1.25, 0.16)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_fr4oh"]
albedo_color = Color(0.18, 0.9, 1, 1)
roughness = 0.5
emission_enabled = true
emission = Color(0.18, 0.9, 1, 1)
emission_energy_multiplier = 1.4

[sub_resource type="BoxMesh" id="BoxMesh_wulh8"]
size = Vector3(0.34, 0.42, 0.34)

[sub_resource type="BoxMesh" id="BoxMesh_if8r0"]
size = Vector3(0.52, 0.09, 0.52)

[sub_resource type="BoxMesh" id="BoxMesh_r3lrk"]
size = Vector3(0.16, 1.25, 0.16)

[sub_resource type="BoxMesh" id="BoxMesh_5gmy0"]
size = Vector3(0.34, 0.42, 0.34)

[sub_resource type="BoxMesh" id="BoxMesh_p7yv5"]
size = Vector3(0.52, 0.09, 0.52)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_sa5x4"]
albedo_color = Color(0.19294, 0.15614, 0.259, 1)
roughness = 0.9

[sub_resource type="BoxMesh" id="BoxMesh_4xfq1"]
size = Vector3(0.42, 3.6, 0.42)

[sub_resource type="BoxMesh" id="BoxMesh_2mgce"]
size = Vector3(3.6, 0.75, 1.2)

[sub_resource type="BoxMesh" id="BoxMesh_mn1ki"]
size = Vector3(2.8, 0.75, 1.2)

[sub_resource type="BoxMesh" id="BoxMesh_dg8wh"]
size = Vector3(2, 0.75, 1.2)

[sub_resource type="BoxMesh" id="BoxMesh_s740b"]
size = Vector3(0.42, 3.6, 0.42)

[sub_resource type="BoxMesh" id="BoxMesh_3lrvq"]
size = Vector3(3.6, 0.75, 1.2)

[sub_resource type="BoxMesh" id="BoxMesh_e4osp"]
size = Vector3(2.8, 0.75, 1.2)

[sub_resource type="BoxMesh" id="BoxMesh_j8iq7"]
size = Vector3(2, 0.75, 1.2)

[sub_resource type="BoxMesh" id="BoxMesh_841ho"]
size = Vector3(0.42, 3.6, 0.42)

[sub_resource type="BoxMesh" id="BoxMesh_i4aaa"]
size = Vector3(3.6, 0.75, 1.2)

[sub_resource type="BoxMesh" id="BoxMesh_utxm1"]
size = Vector3(2.8, 0.75, 1.2)

[sub_resource type="BoxMesh" id="BoxMesh_6fhx0"]
size = Vector3(2, 0.75, 1.2)

[sub_resource type="BoxMesh" id="BoxMesh_bsa86"]
size = Vector3(0.42, 3.6, 0.42)

[sub_resource type="BoxMesh" id="BoxMesh_2oigj"]
size = Vector3(3.6, 0.75, 1.2)

[sub_resource type="BoxMesh" id="BoxMesh_joha8"]
size = Vector3(2.8, 0.75, 1.2)

[sub_resource type="BoxMesh" id="BoxMesh_b2gvj"]
size = Vector3(2, 0.75, 1.2)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_kq8jf"]
albedo_color = Color(0.26434, 0.21954, 0.34899998, 1)
roughness = 0.9

[sub_resource type="BoxMesh" id="BoxMesh_g4k32"]
size = Vector3(30, 0.75, 0.6)

[sub_resource type="BoxMesh" id="BoxMesh_aagrq"]
size = Vector3(30, 0.12, 0.9)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_j61qt"]
albedo_color = Color(0.12867999, 0.099080004, 0.178, 1)
roughness = 0.9

[sub_resource type="BoxMesh" id="BoxMesh_xsar7"]
size = Vector3(12, 0.45, 1)

[sub_resource type="BoxMesh" id="BoxMesh_vy0js"]
size = Vector3(10.5, 3.2, 1)

[sub_resource type="BoxMesh" id="BoxMesh_rmcuu"]
size = Vector3(15, 3.6, 1)

[node name="MapleCourtyard" type="Node3D" unique_id=784380594]

[node name="WorldEnvironment" type="WorldEnvironment" parent="." unique_id=524137039]
environment = SubResource("Environment_lclij")

[node name="Sun" type="DirectionalLight3D" parent="." unique_id=1605249915]
transform = Transform3D(0.40848747, 0.2697402, -0.8719967, 0, 0.9553365, 0.29552022, 0.91276395, -0.1207163, 0.390243, 0, 0, 0)
light_color = Color(1, 0.94, 0.96, 1)
light_energy = 0.55
shadow_enabled = true

[node name="Ground" type="StaticBody3D" parent="." unique_id=1937286852]
collision_mask = 0

[node name="Face" type="MeshInstance3D" parent="Ground" unique_id=1003001320]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, -3, -1.6)
material_override = SubResource("StandardMaterial3D_dqsm4")
mesh = SubResource("BoxMesh_yyfj8")

[node name="CollisionShape3D" type="CollisionShape3D" parent="Ground" unique_id=321278831]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, -3, -1.6)
shape = SubResource("BoxShape3D_j2jl6")

[node name="GroundLine" type="MeshInstance3D" parent="." unique_id=70356511]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, -0.02, 1.9)
material_override = SubResource("StandardMaterial3D_tlf3e")
mesh = SubResource("BoxMesh_56t0o")

[node name="Slab0" type="MeshInstance3D" parent="." unique_id=1021502991]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -16, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_gavuo")
mesh = SubResource("BoxMesh_iiul7")

[node name="Slab1" type="MeshInstance3D" parent="." unique_id=1402685153]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -12, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_gavuo")
mesh = SubResource("BoxMesh_f5hp1")

[node name="Slab2" type="MeshInstance3D" parent="." unique_id=478922786]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -8, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_gavuo")
mesh = SubResource("BoxMesh_rdgw0")

[node name="Slab3" type="MeshInstance3D" parent="." unique_id=1073954642]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -4, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_gavuo")
mesh = SubResource("BoxMesh_62a12")

[node name="Slab4" type="MeshInstance3D" parent="." unique_id=1058431021]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_gavuo")
mesh = SubResource("BoxMesh_oan2d")

[node name="Slab5" type="MeshInstance3D" parent="." unique_id=123015569]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 4, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_gavuo")
mesh = SubResource("BoxMesh_jqdbi")

[node name="Slab6" type="MeshInstance3D" parent="." unique_id=1669010466]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 8, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_gavuo")
mesh = SubResource("BoxMesh_1sy8q")

[node name="Slab7" type="MeshInstance3D" parent="." unique_id=690094199]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 12, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_gavuo")
mesh = SubResource("BoxMesh_f0xyv")

[node name="Slab8" type="MeshInstance3D" parent="." unique_id=1068633568]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 16, -0.78, 1.91)
material_override = SubResource("StandardMaterial3D_gavuo")
mesh = SubResource("BoxMesh_33qv2")

[node name="GroundBand0" type="MeshInstance3D" parent="." unique_id=1333744899]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, -0.55, 1.91)
material_override = SubResource("StandardMaterial3D_gavuo")
mesh = SubResource("BoxMesh_abtvs")

[node name="GroundBand1" type="MeshInstance3D" parent="." unique_id=1175127022]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, -1.65, 1.91)
material_override = SubResource("StandardMaterial3D_gavuo")
mesh = SubResource("BoxMesh_hqsqp")

[node name="WallL" type="StaticBody3D" parent="." unique_id=203830335]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -7.5, 2, 0)
collision_mask = 0

[node name="CollisionShape3D" type="CollisionShape3D" parent="WallL" unique_id=1270218704]
shape = SubResource("BoxShape3D_neg80")

[node name="WallR" type="StaticBody3D" parent="." unique_id=2114580827]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 7.5, 2, 0)
collision_mask = 0

[node name="CollisionShape3D" type="CollisionShape3D" parent="WallR" unique_id=4009478]
shape = SubResource("BoxShape3D_1fsyb")

[node name="ToriiLegL" type="MeshInstance3D" parent="." unique_id=1409539646]
transform = Transform3D(0.99955004, -0.0299955, 0, 0.0299955, 0.99955004, 0, 0, 0, 1, -3.3, 1.6, -4.2)
material_override = SubResource("StandardMaterial3D_vmfai")
mesh = SubResource("BoxMesh_i185f")

[node name="ToriiLegR" type="MeshInstance3D" parent="." unique_id=762659087]
transform = Transform3D(0.99955004, 0.0299955, 0, -0.0299955, 0.99955004, 0, 0, 0, 1, 3.3, 1.6, -4.2)
material_override = SubResource("StandardMaterial3D_vmfai")
mesh = SubResource("BoxMesh_egl4s")

[node name="ToriiBeam" type="MeshInstance3D" parent="." unique_id=159586052]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 2.92, -4.2)
material_override = SubResource("StandardMaterial3D_vmfai")
mesh = SubResource("BoxMesh_xiwjr")

[node name="ToriiTop" type="MeshInstance3D" parent="." unique_id=1677322862]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 3.28, -4.2)
material_override = SubResource("StandardMaterial3D_vmfai")
mesh = SubResource("BoxMesh_o8mhe")

[node name="ToriiSign" type="MeshInstance3D" parent="." unique_id=1682836409]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 2.55, -4.1)
material_override = SubResource("StandardMaterial3D_jnvxj")
mesh = SubResource("BoxMesh_n0417")

[node name="Lantern0Post" type="MeshInstance3D" parent="." unique_id=434618999]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -5.6, 0.625, -2.2)
material_override = SubResource("StandardMaterial3D_jnvxj")
mesh = SubResource("BoxMesh_1s81r")

[node name="Lantern0Lamp" type="MeshInstance3D" parent="." unique_id=2137520939]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -5.6, 1.49, -2.2)
material_override = SubResource("StandardMaterial3D_fr4oh")
mesh = SubResource("BoxMesh_wulh8")

[node name="Lantern0Cap" type="MeshInstance3D" parent="." unique_id=1382297200]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -5.6, 1.75, -2.2)
material_override = SubResource("StandardMaterial3D_jnvxj")
mesh = SubResource("BoxMesh_if8r0")

[node name="Lantern0Light" type="OmniLight3D" parent="." unique_id=1504263656]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -5.6, 1.49, -1.6)
light_color = Color(0.18, 0.9, 1, 1)
light_energy = 3.0
light_specular = 1.0
omni_range = 5.5

[node name="Lantern1Post" type="MeshInstance3D" parent="." unique_id=1454122001]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 5.6, 0.625, -2.2)
material_override = SubResource("StandardMaterial3D_jnvxj")
mesh = SubResource("BoxMesh_r3lrk")

[node name="Lantern1Lamp" type="MeshInstance3D" parent="." unique_id=786304620]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 5.6, 1.49, -2.2)
material_override = SubResource("StandardMaterial3D_fr4oh")
mesh = SubResource("BoxMesh_5gmy0")

[node name="Lantern1Cap" type="MeshInstance3D" parent="." unique_id=2102958381]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 5.6, 1.75, -2.2)
material_override = SubResource("StandardMaterial3D_jnvxj")
mesh = SubResource("BoxMesh_p7yv5")

[node name="Lantern1Light" type="OmniLight3D" parent="." unique_id=1754188591]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 5.6, 1.49, -1.6)
light_color = Color(0.18, 0.9, 1, 1)
light_energy = 3.0
light_specular = 1.0
omni_range = 5.5

[node name="Trunk0" type="MeshInstance3D" parent="." unique_id=1867658769]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -8, 1.8, -7)
material_override = SubResource("StandardMaterial3D_sa5x4")
mesh = SubResource("BoxMesh_4xfq1")

[node name="Canopy0_0" type="MeshInstance3D" parent="." unique_id=1322484650]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -8.2, 3.85, -7)
material_override = SubResource("StandardMaterial3D_sa5x4")
mesh = SubResource("BoxMesh_2mgce")

[node name="Canopy0_1" type="MeshInstance3D" parent="." unique_id=1776509796]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -7.7, 4.57, -7)
material_override = SubResource("StandardMaterial3D_sa5x4")
mesh = SubResource("BoxMesh_mn1ki")

[node name="Canopy0_2" type="MeshInstance3D" parent="." unique_id=2128993497]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -8.2, 5.29, -7)
material_override = SubResource("StandardMaterial3D_sa5x4")
mesh = SubResource("BoxMesh_dg8wh")

[node name="Trunk1" type="MeshInstance3D" parent="." unique_id=2134109909]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -4.6, 1.8, -7)
material_override = SubResource("StandardMaterial3D_sa5x4")
mesh = SubResource("BoxMesh_s740b")

[node name="Canopy1_0" type="MeshInstance3D" parent="." unique_id=2119390906]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -4.8, 3.85, -7)
material_override = SubResource("StandardMaterial3D_sa5x4")
mesh = SubResource("BoxMesh_3lrvq")

[node name="Canopy1_1" type="MeshInstance3D" parent="." unique_id=19259080]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -4.3, 4.57, -7)
material_override = SubResource("StandardMaterial3D_sa5x4")
mesh = SubResource("BoxMesh_e4osp")

[node name="Canopy1_2" type="MeshInstance3D" parent="." unique_id=441679001]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -4.8, 5.29, -7)
material_override = SubResource("StandardMaterial3D_sa5x4")
mesh = SubResource("BoxMesh_j8iq7")

[node name="Trunk2" type="MeshInstance3D" parent="." unique_id=802853365]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 4.9, 1.8, -7)
material_override = SubResource("StandardMaterial3D_sa5x4")
mesh = SubResource("BoxMesh_841ho")

[node name="Canopy2_0" type="MeshInstance3D" parent="." unique_id=1426204425]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 4.7, 3.85, -7)
material_override = SubResource("StandardMaterial3D_sa5x4")
mesh = SubResource("BoxMesh_i4aaa")

[node name="Canopy2_1" type="MeshInstance3D" parent="." unique_id=301785962]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 5.2, 4.57, -7)
material_override = SubResource("StandardMaterial3D_sa5x4")
mesh = SubResource("BoxMesh_utxm1")

[node name="Canopy2_2" type="MeshInstance3D" parent="." unique_id=1327794253]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 4.7, 5.29, -7)
material_override = SubResource("StandardMaterial3D_sa5x4")
mesh = SubResource("BoxMesh_6fhx0")

[node name="Trunk3" type="MeshInstance3D" parent="." unique_id=751166120]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 8.4, 1.8, -7)
material_override = SubResource("StandardMaterial3D_sa5x4")
mesh = SubResource("BoxMesh_bsa86")

[node name="Canopy3_0" type="MeshInstance3D" parent="." unique_id=2055716127]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 8.2, 3.85, -7)
material_override = SubResource("StandardMaterial3D_sa5x4")
mesh = SubResource("BoxMesh_2oigj")

[node name="Canopy3_1" type="MeshInstance3D" parent="." unique_id=1170291347]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 8.7, 4.57, -7)
material_override = SubResource("StandardMaterial3D_sa5x4")
mesh = SubResource("BoxMesh_joha8")

[node name="Canopy3_2" type="MeshInstance3D" parent="." unique_id=689276513]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 8.2, 5.29, -7)
material_override = SubResource("StandardMaterial3D_sa5x4")
mesh = SubResource("BoxMesh_b2gvj")

[node name="NearWall" type="MeshInstance3D" parent="." unique_id=748053634]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.375, -6.2)
material_override = SubResource("StandardMaterial3D_kq8jf")
mesh = SubResource("BoxMesh_g4k32")

[node name="NearWallCap" type="MeshInstance3D" parent="." unique_id=997797224]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.81, -6.2)
material_override = SubResource("StandardMaterial3D_kq8jf")
mesh = SubResource("BoxMesh_aagrq")

[node name="FarRoof" type="MeshInstance3D" parent="." unique_id=818084990]
transform = Transform3D(0.99820054, -0.059964005, 0, 0.059964005, 0.99820054, 0, 0, 0, 1, -4.5, 4.6, -14)
material_override = SubResource("StandardMaterial3D_j61qt")
mesh = SubResource("BoxMesh_xsar7")

[node name="FarHall" type="MeshInstance3D" parent="." unique_id=1669290786]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -4.5, 2.9, -14)
material_override = SubResource("StandardMaterial3D_j61qt")
mesh = SubResource("BoxMesh_vy0js")

[node name="FarHill" type="MeshInstance3D" parent="." unique_id=306525480]
transform = Transform3D(0.9950042, 0.09983342, 0, -0.09983342, 0.9950042, 0, 0, 0, 1, 7.5, 1.8, -16)
material_override = SubResource("StandardMaterial3D_j61qt")
mesh = SubResource("BoxMesh_rmcuu")
```

### `scenes/themes/samurai/sentinel.tscn`

```ini
[gd_scene format=3]

[ext_resource type="PackedScene" path="res://scenes/themes/samurai/weapon.tscn" id="1_haloi"]

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_2ewto"]
albedo_color = Color(0.062, 0.042, 0.098, 1)
roughness = 0.9

[sub_resource type="BoxMesh" id="BoxMesh_32s0o"]
size = Vector3(0.19, 0.52, 0.21)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_l8ktk"]
albedo_color = Color(0.01, 0.008, 0.018, 1)

[sub_resource type="BoxMesh" id="BoxMesh_eh2ac"]
size = Vector3(0.23, 0.12, 0.38)

[sub_resource type="BoxMesh" id="BoxMesh_jwv5m"]
size = Vector3(0.19, 0.52, 0.21)

[sub_resource type="BoxMesh" id="BoxMesh_n02jo"]
size = Vector3(0.23, 0.12, 0.38)

[sub_resource type="CylinderMesh" id="CylinderMesh_icl28"]
top_radius = 0.3
bottom_radius = 0.58
height = 0.66
radial_segments = 8
rings = 1

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_tluvo"]
albedo_color = Color(1, 0.16, 0.45, 1)
roughness = 0.5
emission_enabled = true
emission = Color(1, 0.16, 0.45, 1)
emission_energy_multiplier = 0.9

[sub_resource type="CylinderMesh" id="CylinderMesh_vuybh"]
top_radius = 0.575
bottom_radius = 0.59
height = 0.024
radial_segments = 8
rings = 1

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_ecq46"]
albedo_color = Color(0.3, 0.036, 0.116, 1)
metallic = 0.35
roughness = 0.42

[sub_resource type="BoxMesh" id="BoxMesh_0xwsu"]
size = Vector3(0.26, 0.3, 0.05)

[sub_resource type="BoxMesh" id="BoxMesh_y6al1"]
size = Vector3(0.26, 0.3, 0.05)

[sub_resource type="BoxMesh" id="BoxMesh_r5h0f"]
size = Vector3(0.26, 0.3, 0.05)

[sub_resource type="BoxMesh" id="BoxMesh_ghbsq"]
size = Vector3(0.26, 0.3, 0.05)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_4r0oa"]
albedo_color = Color(0.086, 0.034, 0.09, 1)
metallic = 0.55
roughness = 0.34

[sub_resource type="BoxMesh" id="BoxMesh_jhu20"]
size = Vector3(0.26, 0.3, 0.05)

[sub_resource type="BoxMesh" id="BoxMesh_pngmc"]
size = Vector3(0.26, 0.3, 0.05)

[sub_resource type="CylinderMesh" id="CylinderMesh_darxw"]
top_radius = 0.315
bottom_radius = 0.335
height = 0.15
radial_segments = 8
rings = 1

[sub_resource type="CylinderMesh" id="CylinderMesh_e1rgk"]
top_radius = 0.345
bottom_radius = 0.345
height = 0.022
radial_segments = 8
rings = 1

[sub_resource type="CylinderMesh" id="CylinderMesh_vgl5e"]
top_radius = 0.4
bottom_radius = 0.27
height = 0.62
radial_segments = 8
rings = 1

[sub_resource type="BoxMesh" id="BoxMesh_lx00s"]
size = Vector3(0.085, 0.46, 0.05)

[sub_resource type="BoxMesh" id="BoxMesh_hn6yi"]
size = Vector3(0.085, 0.46, 0.05)

[sub_resource type="BoxMesh" id="BoxMesh_mn7k4"]
size = Vector3(0.3, 0.09, 0.11)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_ymv6b"]
albedo_color = Color(0.45, 0.052, 0.172, 1)
metallic = 0.3
roughness = 0.38

[sub_resource type="BoxMesh" id="BoxMesh_51dvr"]
size = Vector3(0.21, 0.1, 0.34)

[sub_resource type="BoxMesh" id="BoxMesh_0xr2k"]
size = Vector3(0.236, 0.1, 0.34)

[sub_resource type="BoxMesh" id="BoxMesh_6d01o"]
size = Vector3(0.262, 0.1, 0.34)

[sub_resource type="BoxMesh" id="BoxMesh_136p0"]
size = Vector3(0.26, 0.022, 0.03)

[sub_resource type="BoxMesh" id="BoxMesh_omkn4"]
size = Vector3(0.21, 0.1, 0.34)

[sub_resource type="BoxMesh" id="BoxMesh_1li24"]
size = Vector3(0.236, 0.1, 0.34)

[sub_resource type="BoxMesh" id="BoxMesh_y2402"]
size = Vector3(0.262, 0.1, 0.34)

[sub_resource type="BoxMesh" id="BoxMesh_sp8be"]
size = Vector3(0.26, 0.022, 0.03)

[sub_resource type="CylinderMesh" id="CylinderMesh_gtqdt"]
top_radius = 0.105
bottom_radius = 0.115
height = 0.16
radial_segments = 8
rings = 1

[sub_resource type="BoxMesh" id="BoxMesh_m5bu5"]
size = Vector3(0.27, 0.25, 0.26)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_m5bo4"]
albedo_color = Color(1, 0.45, 0.68, 1)
roughness = 0.5
emission_enabled = true
emission = Color(1, 0.45, 0.68, 1)
emission_energy_multiplier = 2.2

[sub_resource type="BoxMesh" id="BoxMesh_lrjjx"]
size = Vector3(0.22, 0.042, 0.02)

[sub_resource type="CylinderMesh" id="CylinderMesh_x5rub"]
top_radius = 0.045
bottom_radius = 0.42
height = 0.34
radial_segments = 8
rings = 1

[sub_resource type="CylinderMesh" id="CylinderMesh_h6a3i"]
top_radius = 0.42
bottom_radius = 0.435
height = 0.022
radial_segments = 8
rings = 1

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_1mpi4"]
albedo_color = Color(0.48, 0.31, 0.09, 1)
metallic = 0.8
roughness = 0.32

[sub_resource type="BoxMesh" id="BoxMesh_luoyx"]
size = Vector3(0.05, 0.16, 0.03)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_24vwj"]
albedo_color = Color(0.949, 0.733, 0.4, 1)
roughness = 0.5
emission_enabled = true
emission = Color(0.949, 0.733, 0.4, 1)
emission_energy_multiplier = 1.1

[sub_resource type="CylinderMesh" id="CylinderMesh_mm71n"]
top_radius = 0.425
bottom_radius = 0.425
height = 0.085
radial_segments = 12
rings = 1

[sub_resource type="BoxMesh" id="BoxMesh_41aak"]
size = Vector3(0.19, 0.1995, 0.3024)

[sub_resource type="BoxMesh" id="BoxMesh_ddiu5"]
size = Vector3(0.2128, 0.2128, 0.0882)

[sub_resource type="BoxMesh" id="BoxMesh_oewye"]
size = Vector3(0.1634, 0.171, 0.2583)

[sub_resource type="BoxMesh" id="BoxMesh_d7ryv"]
size = Vector3(0.152, 0.1634, 0.1386)

[sub_resource type="BoxMesh" id="BoxMesh_5sg11"]
size = Vector3(0.175, 0.18375, 0.3024)

[sub_resource type="BoxMesh" id="BoxMesh_dygtr"]
size = Vector3(0.196, 0.196, 0.0882)

[sub_resource type="BoxMesh" id="BoxMesh_cobte"]
size = Vector3(0.1505, 0.1575, 0.2583)

[sub_resource type="BoxMesh" id="BoxMesh_cxfpq"]
size = Vector3(0.14, 0.1505, 0.1386)

[node name="Ronin" type="Node3D" unique_id=1348975407]

[node name="Body" type="Node3D" parent="." unique_id=1924396019]
transform = Transform3D(0.9800666, 0, 0.19866933, 0, 1, 0, -0.19866933, 0, 0.9800666, 0, 0, 0)

[node name="FrontShin" type="MeshInstance3D" parent="Body" unique_id=954762937]
transform = Transform3D(1, 0, 0, 0, 0.9950042, -0.09983342, 0, 0.09983342, 0.9950042, -0.25, 0.3, -0.13)
material_override = SubResource("StandardMaterial3D_2ewto")
mesh = SubResource("BoxMesh_32s0o")

[node name="FrontFoot" type="MeshInstance3D" parent="Body" unique_id=1552798474]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -0.25, 0.06, -0.25)
material_override = SubResource("StandardMaterial3D_l8ktk")
mesh = SubResource("BoxMesh_eh2ac")

[node name="BackShin" type="MeshInstance3D" parent="Body" unique_id=1451286917]
transform = Transform3D(1, 0, 0, 0, 0.990216, 0.13954312, 0, -0.13954312, 0.990216, 0.27, 0.3, 0.13)
material_override = SubResource("StandardMaterial3D_2ewto")
mesh = SubResource("BoxMesh_jwv5m")

[node name="BackFoot" type="MeshInstance3D" parent="Body" unique_id=480726778]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0.27, 0.06, 0.21)
material_override = SubResource("StandardMaterial3D_l8ktk")
mesh = SubResource("BoxMesh_n02jo")

[node name="Skirt" type="MeshInstance3D" parent="Body" unique_id=861571847]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 0.79, 0)
material_override = SubResource("StandardMaterial3D_2ewto")
mesh = SubResource("CylinderMesh_icl28")

[node name="SkirtHem" type="MeshInstance3D" parent="Body" unique_id=947502582]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 0.475, 0)
material_override = SubResource("StandardMaterial3D_tluvo")
mesh = SubResource("CylinderMesh_vuybh")

[node name="HipPlate0" type="MeshInstance3D" parent="Body" unique_id=855652662]
transform = Transform3D(0.49757108, 0.12104293, -0.8589363, 0, 0.990216, 0.13954312, 0.8674232, -0.069432616, 0.49270284, -0.299261, 0.92, -0.17166202)
material_override = SubResource("StandardMaterial3D_ecq46")
mesh = SubResource("BoxMesh_0xwsu")

[node name="HipPlate1" type="MeshInstance3D" parent="Body" unique_id=869339128]
transform = Transform3D(0.9393727, 0.04784903, -0.3395429, 0, 0.990216, 0.13954312, 0.3428978, -0.131083, 0.9301819, -0.118299745, 0.92, -0.3240836)
material_override = SubResource("StandardMaterial3D_ecq46")
mesh = SubResource("BoxMesh_y6al1")

[node name="HipPlate2" type="MeshInstance3D" parent="Body" unique_id=565314634]
transform = Transform3D(0.9393727, -0.04784903, 0.3395429, 0, 0.990216, 0.13954312, -0.3428978, -0.131083, 0.9301819, 0.118299745, 0.92, -0.3240836)
material_override = SubResource("StandardMaterial3D_ecq46")
mesh = SubResource("BoxMesh_r5h0f")

[node name="HipPlate3" type="MeshInstance3D" parent="Body" unique_id=877102614]
transform = Transform3D(0.49757108, -0.12104293, 0.8589363, 0, 0.990216, 0.13954312, -0.8674232, -0.069432616, 0.49270284, 0.299261, 0.92, -0.17166202)
material_override = SubResource("StandardMaterial3D_ecq46")
mesh = SubResource("BoxMesh_ghbsq")

[node name="HipPlate4" type="MeshInstance3D" parent="Body" unique_id=1920608766]
transform = Transform3D(-0.17824605, 0.13730846, -0.9743587, 0, 0.990216, 0.13954312, 0.98398596, 0.024873009, -0.1765021, -0.33947515, 0.92, 0.06149489)
material_override = SubResource("StandardMaterial3D_4r0oa")
mesh = SubResource("BoxMesh_jhu20")

[node name="HipPlate5" type="MeshInstance3D" parent="Body" unique_id=2049409156]
transform = Transform3D(-0.17824605, -0.13730846, 0.9743587, 0, 0.990216, 0.13954312, -0.98398596, 0.024873009, -0.1765021, 0.33947515, 0.92, 0.06149489)
material_override = SubResource("StandardMaterial3D_4r0oa")
mesh = SubResource("BoxMesh_pngmc")

[node name="Belt" type="MeshInstance3D" parent="Body" unique_id=1013497354]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 1.1, 0)
material_override = SubResource("StandardMaterial3D_2ewto")
mesh = SubResource("CylinderMesh_darxw")

[node name="BeltEdge" type="MeshInstance3D" parent="Body" unique_id=1017709804]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 1.175, 0)
material_override = SubResource("StandardMaterial3D_tluvo")
mesh = SubResource("CylinderMesh_e1rgk")

[node name="Torso" type="MeshInstance3D" parent="Body" unique_id=1903903984]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 1.48, 0)
material_override = SubResource("StandardMaterial3D_ecq46")
mesh = SubResource("CylinderMesh_vgl5e")

[node name="StrapL" type="MeshInstance3D" parent="Body" unique_id=978667660]
transform = Transform3D(1, 0, 0, 0, 0.99820054, 0.059964005, 0, -0.059964005, 0.99820054, -0.135, 1.5, -0.335)
material_override = SubResource("StandardMaterial3D_4r0oa")
mesh = SubResource("BoxMesh_lx00s")

[node name="StrapR" type="MeshInstance3D" parent="Body" unique_id=1793069741]
transform = Transform3D(1, 0, 0, 0, 0.99820054, 0.059964005, 0, -0.059964005, 0.99820054, 0.135, 1.5, -0.335)
material_override = SubResource("StandardMaterial3D_4r0oa")
mesh = SubResource("BoxMesh_hn6yi")

[node name="Collar" type="MeshInstance3D" parent="Body" unique_id=728438456]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1.79, -0.175)
material_override = SubResource("StandardMaterial3D_4r0oa")
mesh = SubResource("BoxMesh_mn7k4")

[node name="LeftSode0" type="MeshInstance3D" parent="Body" unique_id=1892141176]
transform = Transform3D(0.9553365, -0.29552022, 0, 0.29552022, 0.9553365, 0, 0, 0, 1, -0.375, 1.64, 0)
material_override = SubResource("StandardMaterial3D_ymv6b")
mesh = SubResource("BoxMesh_51dvr")

[node name="LeftSode1" type="MeshInstance3D" parent="Body" unique_id=2101595783]
transform = Transform3D(0.9553365, -0.29552022, 0, 0.29552022, 0.9553365, 0, 0, 0, 1, -0.393, 1.52, 0)
material_override = SubResource("StandardMaterial3D_ymv6b")
mesh = SubResource("BoxMesh_0xr2k")

[node name="LeftSode2" type="MeshInstance3D" parent="Body" unique_id=2047004008]
transform = Transform3D(0.9553365, -0.29552022, 0, 0.29552022, 0.9553365, 0, 0, 0, 1, -0.411, 1.4, 0)
material_override = SubResource("StandardMaterial3D_ymv6b")
mesh = SubResource("BoxMesh_6d01o")

[node name="LeftSodeEdge" type="MeshInstance3D" parent="Body" unique_id=1816711887]
transform = Transform3D(0.9553365, -0.29552022, 0, 0.29552022, 0.9553365, 0, 0, 0, 1, -0.425, 1.385, -0.155)
material_override = SubResource("StandardMaterial3D_tluvo")
mesh = SubResource("BoxMesh_136p0")

[node name="RightSode0" type="MeshInstance3D" parent="Body" unique_id=1658541262]
transform = Transform3D(0.9553365, 0.29552022, 0, -0.29552022, 0.9553365, 0, 0, 0, 1, 0.375, 1.64, 0)
material_override = SubResource("StandardMaterial3D_ymv6b")
mesh = SubResource("BoxMesh_omkn4")

[node name="RightSode1" type="MeshInstance3D" parent="Body" unique_id=1513939673]
transform = Transform3D(0.9553365, 0.29552022, 0, -0.29552022, 0.9553365, 0, 0, 0, 1, 0.393, 1.52, 0)
material_override = SubResource("StandardMaterial3D_ymv6b")
mesh = SubResource("BoxMesh_1li24")

[node name="RightSode2" type="MeshInstance3D" parent="Body" unique_id=1030574863]
transform = Transform3D(0.9553365, 0.29552022, 0, -0.29552022, 0.9553365, 0, 0, 0, 1, 0.411, 1.4, 0)
material_override = SubResource("StandardMaterial3D_ymv6b")
mesh = SubResource("BoxMesh_y2402")

[node name="RightSodeEdge" type="MeshInstance3D" parent="Body" unique_id=182184934]
transform = Transform3D(0.9553365, 0.29552022, 0, -0.29552022, 0.9553365, 0, 0, 0, 1, 0.425, 1.385, -0.155)
material_override = SubResource("StandardMaterial3D_tluvo")
mesh = SubResource("BoxMesh_sp8be")

[node name="Head" type="Node3D" parent="." unique_id=755427856]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1.86, 0)

[node name="Neck" type="MeshInstance3D" parent="Head" unique_id=1115973024]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 0.02, 0)
material_override = SubResource("StandardMaterial3D_l8ktk")
mesh = SubResource("CylinderMesh_gtqdt")

[node name="Skull" type="MeshInstance3D" parent="Head" unique_id=238652184]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.16, 0)
material_override = SubResource("StandardMaterial3D_l8ktk")
mesh = SubResource("BoxMesh_m5bu5")

[node name="EyeSlit" type="MeshInstance3D" parent="Head" unique_id=1753972798]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.195, -0.162)
material_override = SubResource("StandardMaterial3D_m5bo4")
mesh = SubResource("BoxMesh_lrjjx")

[node name="Kasa" type="MeshInstance3D" parent="Head" unique_id=376460975]
transform = Transform3D(0.9238795, 0.022947233, 0.38199484, 0, 0.99820054, -0.059964005, -0.38268346, 0.055399515, 0.922217, 0, 0.44, 0.02)
material_override = SubResource("StandardMaterial3D_ymv6b")
mesh = SubResource("CylinderMesh_x5rub")

[node name="KasaBrim" type="MeshInstance3D" parent="Head" unique_id=915900678]
transform = Transform3D(0.9238795, 0.022947233, 0.38199484, 0, 0.99820054, -0.059964005, -0.38268346, 0.055399515, 0.922217, 0, 0.295, 0.02)
material_override = SubResource("StandardMaterial3D_tluvo")
mesh = SubResource("CylinderMesh_h6a3i")

[node name="Maedate" type="MeshInstance3D" parent="Head" unique_id=1482635067]
transform = Transform3D(1, 0, 0, 0, 0.9358968, 0.35227424, 0, -0.35227424, 0.9358968, 0, 0.36, -0.26)
material_override = SubResource("StandardMaterial3D_1mpi4")
mesh = SubResource("BoxMesh_luoyx")

[node name="AttackCue" type="MeshInstance3D" parent="." unique_id=646973352]
transform = Transform3D(0.9659258, 0, 0.25881904, 0, 1, 0, -0.25881904, 0, 0.9659258, 0, 1.62, 0)
material_override = SubResource("StandardMaterial3D_24vwj")
mesh = SubResource("CylinderMesh_mm71n")

[node name="Arms" type="Node3D" parent="." unique_id=1783531851]

[node name="SwordArm" type="Node3D" parent="Arms" unique_id=531790596]
transform = Transform3D(0.6689647, 0.5533361, -0.4962916, 0, 0.66769207, 0.7444375, 0.7432942, -0.4980024, 0.44666243, 0.42, 1.6, -0.02)
metadata/reach = 0.63

[node name="SwordArmUpper" type="MeshInstance3D" parent="Arms/SwordArm" unique_id=1888728590]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.1512)
material_override = SubResource("StandardMaterial3D_4r0oa")
mesh = SubResource("BoxMesh_41aak")

[node name="SwordArmElbow" type="MeshInstance3D" parent="Arms/SwordArm" unique_id=1186888765]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.3213)
material_override = SubResource("StandardMaterial3D_ecq46")
mesh = SubResource("BoxMesh_ddiu5")

[node name="SwordArmLower" type="MeshInstance3D" parent="Arms/SwordArm" unique_id=1754785650]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.4473)
material_override = SubResource("StandardMaterial3D_4r0oa")
mesh = SubResource("BoxMesh_oewye")

[node name="SwordArmHand" type="MeshInstance3D" parent="Arms/SwordArm" unique_id=933296912]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.63)
material_override = SubResource("StandardMaterial3D_l8ktk")
mesh = SubResource("BoxMesh_d7ryv")

[node name="OffArm" type="Node3D" parent="Arms" unique_id=2144878428]
transform = Transform3D(0.17054145, 0.2694008, -0.94780743, 0, 0.9618988, 0.27340606, 0.98535055, -0.046627067, 0.1640436, -0.42, 1.6, -0.02)
metadata/reach = 0.63

[node name="OffArmUpper" type="MeshInstance3D" parent="Arms/OffArm" unique_id=200795661]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.1512)
material_override = SubResource("StandardMaterial3D_4r0oa")
mesh = SubResource("BoxMesh_5sg11")

[node name="OffArmElbow" type="MeshInstance3D" parent="Arms/OffArm" unique_id=1964347436]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.3213)
material_override = SubResource("StandardMaterial3D_ecq46")
mesh = SubResource("BoxMesh_dygtr")

[node name="OffArmLower" type="MeshInstance3D" parent="Arms/OffArm" unique_id=994811712]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.4473)
material_override = SubResource("StandardMaterial3D_4r0oa")
mesh = SubResource("BoxMesh_cobte")

[node name="OffArmHand" type="MeshInstance3D" parent="Arms/OffArm" unique_id=1202385800]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.63)
material_override = SubResource("StandardMaterial3D_l8ktk")
mesh = SubResource("BoxMesh_cxfpq")

[node name="RimWarm" type="OmniLight3D" parent="." unique_id=103905975]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -0.58, 1.85, 0.5)
light_color = Color(1, 0.18, 0.48, 1)
light_energy = 3.4
light_specular = 1.0
omni_range = 1.55

[node name="RimCool" type="OmniLight3D" parent="." unique_id=383958250]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0.58, 1.775, 0.5)
light_color = Color(0.3, 0.85, 1, 1)
light_energy = 2.8
light_specular = 1.0
omni_range = 1.55

[node name="KeyLight" type="SpotLight3D" parent="." unique_id=808985377]
transform = Transform3D(-0.91669595, 0.20244177, -0.34450808, 0, 0.8621638, 0.5066295, 0.3995854, 0.46442524, -0.7903421, -0.85, 2.3, -1.95)
light_color = Color(1, 0.95, 0.98, 1)
light_energy = 3.2
spot_range = 6.0
spot_angle = 36.0
spot_angle_attenuation = 1.4

[node name="WeaponPivot" type="Node3D" parent="." unique_id=788613598]
transform = Transform3D(1, 0, 0, 0, 0.9887711, 0.14943814, 0, -0.14943814, 0.9887711, 0.62, 1.3, -0.2)

[node name="WeaponVisual" type="Node3D" parent="WeaponPivot" unique_id=639104529 instance=ExtResource("1_haloi")]
```

### `scenes/themes/samurai/duelist.tscn`

```ini
[gd_scene format=3]

[ext_resource type="PackedScene" path="res://scenes/themes/samurai/weapon.tscn" id="1_km2wj"]

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_at6va"]
albedo_color = Color(0.026, 0.055, 0.056, 1)
roughness = 0.9

[sub_resource type="BoxMesh" id="BoxMesh_puk56"]
size = Vector3(0.16, 0.46, 0.18)

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_pvrlg"]
albedo_color = Color(0.01, 0.008, 0.018, 1)

[sub_resource type="BoxMesh" id="BoxMesh_m7agk"]
size = Vector3(0.2, 0.1, 0.32)

[sub_resource type="BoxMesh" id="BoxMesh_26olo"]
size = Vector3(0.16, 0.46, 0.18)

[sub_resource type="BoxMesh" id="BoxMesh_31j6p"]
size = Vector3(0.2, 0.1, 0.32)

[sub_resource type="CylinderMesh" id="CylinderMesh_ji3tl"]
top_radius = 0.24
bottom_radius = 0.44
height = 0.58
radial_segments = 8
rings = 1

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_5g2hk"]
albedo_color = Color(0.66, 0.9, 0.81, 1)
roughness = 0.5
emission_enabled = true
emission = Color(0.66, 0.9, 0.81, 1)

[sub_resource type="CylinderMesh" id="CylinderMesh_wdreg"]
top_radius = 0.435
bottom_radius = 0.45
height = 0.022
radial_segments = 8
rings = 1

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_f7ulb"]
albedo_color = Color(0.145, 0.29, 0.27, 1)
metallic = 0.15
roughness = 0.55

[sub_resource type="CylinderMesh" id="CylinderMesh_fiqnh"]
top_radius = 0.245
bottom_radius = 0.26
height = 0.13
radial_segments = 8
rings = 1

[sub_resource type="CylinderMesh" id="CylinderMesh_ybt5c"]
top_radius = 0.272
bottom_radius = 0.272
height = 0.02
radial_segments = 8
rings = 1

[sub_resource type="StandardMaterial3D" id="StandardMaterial3D_gwln0"]
albedo_color = Color(0.04, 0.078, 0.076, 1)
metallic = 0.25
roughness = 0.45

[sub_resource type="CylinderMesh" id="CylinderMesh_mwp38"]
top_radius = 0.315
bottom_radius = 0.215
height = 0.52
radial_segments = 8
rings = 1

[sub_resource type="BoxMesh" id="BoxMesh_cffgf"]
size = Vector3(0.075, 0.44, 0.05)

[sub_resource type="BoxMesh" id="BoxMesh_qgnl6"]
size = Vector3(0.075, 0.44, 0.05)

[sub_resource type="BoxMesh" id="BoxMesh_lyb4h"]
size = Vector3(0.72, 0.11, 0.3)

[sub_resource type="BoxMesh" id="BoxMesh_w0lif"]
size = Vector3(0.74, 0.02, 0.028)

[sub_resource type="CylinderMesh" id="CylinderMesh_aa7hh"]
top_radius = 0.085
bottom_radius = 0.095
height = 0.14
radial_segments = 8
rings = 1

[sub_resource type="BoxMesh" id="BoxMesh_2wly7"]
size = Vector3(0.28, 0.29, 0.28)

[sub_resource type="BoxMesh" id="BoxMesh_yv7o7"]
size = Vector3(0.22, 0.17, 0.04)

[sub_resource type="BoxMesh" id="BoxMesh_1ovdl"]
size = Vector3(0.3, 0.055, 0.3)

[sub_resource type="BoxMesh" id="BoxMesh_o868v"]
size = Vector3(0.085, 0.12, 0.24)

[sub_resource type="BoxMesh" id="BoxMesh_ppqjl"]
size = Vector3(0.155, 0.16275, 0.2496)

[sub_resource type="BoxMesh" id="BoxMesh_5u8u4"]
size = Vector3(0.1736, 0.1736, 0.0728)

[sub_resource type="BoxMesh" id="BoxMesh_c7vod"]
size = Vector3(0.1333, 0.1395, 0.2132)

[sub_resource type="BoxMesh" id="BoxMesh_nso1l"]
size = Vector3(0.124, 0.1333, 0.1144)

[sub_resource type="BoxMesh" id="BoxMesh_nu24x"]
size = Vector3(0.145, 0.15225, 0.2496)

[sub_resource type="BoxMesh" id="BoxMesh_fqbna"]
size = Vector3(0.1624, 0.1624, 0.0728)

[sub_resource type="BoxMesh" id="BoxMesh_ehxgx"]
size = Vector3(0.1247, 0.1305, 0.2132)

[sub_resource type="BoxMesh" id="BoxMesh_b0qq6"]
size = Vector3(0.116, 0.1247, 0.1144)

[node name="Duelist" type="Node3D" unique_id=929031875]

[node name="Body" type="Node3D" parent="." unique_id=1336726161]
transform = Transform3D(0.98722726, 0, -0.15931821, 0, 1, 0, 0.15931821, 0, 0.98722726, 0, 0, 0)

[node name="FrontShin" type="MeshInstance3D" parent="Body" unique_id=5284458]
transform = Transform3D(1, 0, 0, 0, 0.99280864, -0.119712204, 0, 0.119712204, 0.99280864, -0.19, 0.26, -0.14)
material_override = SubResource("StandardMaterial3D_at6va")
mesh = SubResource("BoxMesh_puk56")

[node name="FrontFoot" type="MeshInstance3D" parent="Body" unique_id=1595719556]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -0.19, 0.05, -0.24)
material_override = SubResource("StandardMaterial3D_pvrlg")
mesh = SubResource("BoxMesh_m7agk")

[node name="BackShin" type="MeshInstance3D" parent="Body" unique_id=792650043]
transform = Transform3D(1, 0, 0, 0, 0.98722726, 0.15931821, 0, -0.15931821, 0.98722726, 0.21, 0.26, 0.14)
material_override = SubResource("StandardMaterial3D_at6va")
mesh = SubResource("BoxMesh_26olo")

[node name="BackFoot" type="MeshInstance3D" parent="Body" unique_id=655738203]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0.21, 0.05, 0.2)
material_override = SubResource("StandardMaterial3D_pvrlg")
mesh = SubResource("BoxMesh_31j6p")

[node name="Coat" type="MeshInstance3D" parent="Body" unique_id=439058631]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 0.66, 0)
material_override = SubResource("StandardMaterial3D_at6va")
mesh = SubResource("CylinderMesh_ji3tl")

[node name="CoatHem" type="MeshInstance3D" parent="Body" unique_id=1537478766]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 0.385, 0)
material_override = SubResource("StandardMaterial3D_5g2hk")
mesh = SubResource("CylinderMesh_wdreg")

[node name="Sash" type="MeshInstance3D" parent="Body" unique_id=253835486]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 0.96, 0)
material_override = SubResource("StandardMaterial3D_f7ulb")
mesh = SubResource("CylinderMesh_fiqnh")

[node name="SashEdge" type="MeshInstance3D" parent="Body" unique_id=584574967]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 1.02, 0)
material_override = SubResource("StandardMaterial3D_5g2hk")
mesh = SubResource("CylinderMesh_ybt5c")

[node name="Torso" type="MeshInstance3D" parent="Body" unique_id=1940981035]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 1.29, 0)
material_override = SubResource("StandardMaterial3D_gwln0")
mesh = SubResource("CylinderMesh_mwp38")

[node name="LapelL" type="MeshInstance3D" parent="Body" unique_id=788953926]
transform = Transform3D(0.97589743, -0.21822962, 0, 0.21822962, 0.97589743, 0, 0, 0, 1, -0.1, 1.32, -0.265)
material_override = SubResource("StandardMaterial3D_f7ulb")
mesh = SubResource("BoxMesh_cffgf")

[node name="LapelR" type="MeshInstance3D" parent="Body" unique_id=1502447803]
transform = Transform3D(0.97589743, 0.21822962, 0, -0.21822962, 0.97589743, 0, 0, 0, 1, 0.1, 1.32, -0.265)
material_override = SubResource("StandardMaterial3D_f7ulb")
mesh = SubResource("BoxMesh_qgnl6")

[node name="Shoulder" type="MeshInstance3D" parent="Body" unique_id=381943850]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1.5, -0.02)
material_override = SubResource("StandardMaterial3D_gwln0")
mesh = SubResource("BoxMesh_lyb4h")

[node name="ShoulderEdge" type="MeshInstance3D" parent="Body" unique_id=1003655850]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1.555, -0.145)
material_override = SubResource("StandardMaterial3D_5g2hk")
mesh = SubResource("BoxMesh_w0lif")

[node name="Head" type="Node3D" parent="." unique_id=250228848]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1.56, 0)

[node name="Neck" type="MeshInstance3D" parent="Head" unique_id=76612595]
transform = Transform3D(0.9238795, 0, 0.38268346, 0, 1, 0, -0.38268346, 0, 0.9238795, 0, 0.02, 0)
material_override = SubResource("StandardMaterial3D_pvrlg")
mesh = SubResource("CylinderMesh_aa7hh")

[node name="Skull" type="MeshInstance3D" parent="Head" unique_id=551769537]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.22, 0)
material_override = SubResource("StandardMaterial3D_gwln0")
mesh = SubResource("BoxMesh_2wly7")

[node name="Face" type="MeshInstance3D" parent="Head" unique_id=440996304]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.19, -0.145)
material_override = SubResource("StandardMaterial3D_pvrlg")
mesh = SubResource("BoxMesh_yv7o7")

[node name="Headband" type="MeshInstance3D" parent="Head" unique_id=1087983859]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.31, 0)
material_override = SubResource("StandardMaterial3D_5g2hk")
mesh = SubResource("BoxMesh_1ovdl")

[node name="Topknot" type="MeshInstance3D" parent="Head" unique_id=1572029223]
transform = Transform3D(1, 0, 0, 0, 0.87758255, 0.47942555, 0, -0.47942555, 0.87758255, 0, 0.4, 0.11)
material_override = SubResource("StandardMaterial3D_pvrlg")
mesh = SubResource("BoxMesh_o868v")

[node name="Arms" type="Node3D" parent="." unique_id=1836446691]

[node name="SwordArm" type="Node3D" parent="Arms" unique_id=712930578]
transform = Transform3D(0.99745864, 0.056185044, -0.04381082, 0, 0.6149138, 0.78859437, 0.07124708, -0.7865903, 0.61335117, 0.32, 1.44, -0.02)
metadata/reach = 0.52

[node name="SwordArmUpper" type="MeshInstance3D" parent="Arms/SwordArm" unique_id=209140495]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.1248)
material_override = SubResource("StandardMaterial3D_gwln0")
mesh = SubResource("BoxMesh_ppqjl")

[node name="SwordArmElbow" type="MeshInstance3D" parent="Arms/SwordArm" unique_id=1678360862]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.2652)
material_override = SubResource("StandardMaterial3D_f7ulb")
mesh = SubResource("BoxMesh_5u8u4")

[node name="SwordArmLower" type="MeshInstance3D" parent="Arms/SwordArm" unique_id=1800544533]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.3692)
material_override = SubResource("StandardMaterial3D_gwln0")
mesh = SubResource("BoxMesh_c7vod")

[node name="SwordArmHand" type="MeshInstance3D" parent="Arms/SwordArm" unique_id=385307910]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.52)
material_override = SubResource("StandardMaterial3D_pvrlg")
mesh = SubResource("BoxMesh_nso1l")

[node name="OffArm" type="Node3D" parent="Arms" unique_id=1005349676]
transform = Transform3D(0.39054987, 0.4131016, -0.8226894, 0, 0.8936625, 0.44873974, 0.92058176, -0.17525524, 0.34901977, -0.32, 1.44, -0.02)
metadata/reach = 0.52

[node name="OffArmUpper" type="MeshInstance3D" parent="Arms/OffArm" unique_id=1103427597]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.1248)
material_override = SubResource("StandardMaterial3D_gwln0")
mesh = SubResource("BoxMesh_nu24x")

[node name="OffArmElbow" type="MeshInstance3D" parent="Arms/OffArm" unique_id=1978857409]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.2652)
material_override = SubResource("StandardMaterial3D_f7ulb")
mesh = SubResource("BoxMesh_fqbna")

[node name="OffArmLower" type="MeshInstance3D" parent="Arms/OffArm" unique_id=1417259597]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.3692)
material_override = SubResource("StandardMaterial3D_gwln0")
mesh = SubResource("BoxMesh_ehxgx")

[node name="OffArmHand" type="MeshInstance3D" parent="Arms/OffArm" unique_id=870517836]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -0.52)
material_override = SubResource("StandardMaterial3D_pvrlg")
mesh = SubResource("BoxMesh_b0qq6")

[node name="RimWarm" type="OmniLight3D" parent="." unique_id=2045537829]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -0.58, 1.443, 0.5)
light_color = Color(0.35, 1, 0.86, 1)
light_energy = 3.4
light_specular = 1.0
omni_range = 1.55

[node name="RimCool" type="OmniLight3D" parent="." unique_id=737088766]
transform = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0.58, 1.3845, 0.5)
light_color = Color(0.24, 0.7, 1, 1)
light_energy = 2.8
light_specular = 1.0
omni_range = 1.55

[node name="KeyLight" type="SpotLight3D" parent="." unique_id=4260591]
transform = Transform3D(-0.91669595, 0.16649354, -0.36324704, 0, 0.90905976, 0.4166657, 0.39958543, 0.38195577, -0.8333314, -0.85, 1.794, -1.95)
light_color = Color(1, 0.95, 0.98, 1)
light_energy = 3.2
spot_range = 6.0
spot_angle = 36.0
spot_angle_attenuation = 1.4

[node name="WeaponPivot" type="Node3D" parent="." unique_id=1984110944]
transform = Transform3D(1, 0, 0, 0, 0.9887711, 0.14943814, 0, -0.14943814, 0.9887711, 0.34, 1.08, -0.3)

[node name="WeaponVisual" type="Node3D" parent="WeaponPivot" unique_id=710526548 instance=ExtResource("1_km2wj")]
```

### `scenes/themes/samurai/weapon.tscn`

```ini
[gd_scene load_steps=26 format=3]

[sub_resource type="StandardMaterial3D" id="Steel"]
albedo_color = Color(0.768627, 0.800000, 0.807843, 1)
roughness = 0.72
metallic = 0.75

[sub_resource type="StandardMaterial3D" id="Edge"]
albedo_color = Color(0.933333, 0.945098, 0.905882, 1)
roughness = 0.72
metallic = 0.65

[sub_resource type="StandardMaterial3D" id="Brass"]
albedo_color = Color(0.709804, 0.541176, 0.301961, 1)
roughness = 0.72
metallic = 0.55

[sub_resource type="StandardMaterial3D" id="Grip"]
albedo_color = Color(0.156863, 0.149020, 0.145098, 1)
roughness = 0.72
metallic = 0.0

[sub_resource type="StandardMaterial3D" id="Wrap"]
albedo_color = Color(0.654902, 0.470588, 0.333333, 1)
roughness = 0.72
metallic = 0.0

[sub_resource type="CylinderMesh" id="Root_Handle_mesh"]
top_radius = 0.04
bottom_radius = 0.04
height = 0.3
radial_segments = 12
rings = 1

[sub_resource type="CylinderMesh" id="Root_Wrap0_mesh"]
top_radius = 0.043
bottom_radius = 0.043
height = 0.018
radial_segments = 12
rings = 1

[sub_resource type="CylinderMesh" id="Root_Wrap1_mesh"]
top_radius = 0.043
bottom_radius = 0.043
height = 0.018
radial_segments = 12
rings = 1

[sub_resource type="CylinderMesh" id="Root_Wrap2_mesh"]
top_radius = 0.043
bottom_radius = 0.043
height = 0.018
radial_segments = 12
rings = 1

[sub_resource type="CylinderMesh" id="Root_Wrap3_mesh"]
top_radius = 0.043
bottom_radius = 0.043
height = 0.018
radial_segments = 12
rings = 1

[sub_resource type="CylinderMesh" id="Root_Wrap4_mesh"]
top_radius = 0.043
bottom_radius = 0.043
height = 0.018
radial_segments = 12
rings = 1

[sub_resource type="CylinderMesh" id="Root_Wrap5_mesh"]
top_radius = 0.043
bottom_radius = 0.043
height = 0.018
radial_segments = 12
rings = 1

[sub_resource type="CylinderMesh" id="Root_Tsuba_mesh"]
top_radius = 0.125
bottom_radius = 0.125
height = 0.025
radial_segments = 12
rings = 1

[sub_resource type="CylinderMesh" id="Root_Pommel_mesh"]
top_radius = 0.049
bottom_radius = 0.049
height = 0.03
radial_segments = 12
rings = 1

[sub_resource type="BoxMesh" id="Root_Blade0_mesh"]
size = Vector3(0.085, 0.225, 0.025)

[sub_resource type="BoxMesh" id="Root_Edge0_mesh"]
size = Vector3(0.016, 0.225, 0.027)

[sub_resource type="BoxMesh" id="Root_Blade1_mesh"]
size = Vector3(0.085, 0.225, 0.025)

[sub_resource type="BoxMesh" id="Root_Edge1_mesh"]
size = Vector3(0.016, 0.225, 0.027)

[sub_resource type="BoxMesh" id="Root_Blade2_mesh"]
size = Vector3(0.085, 0.225, 0.025)

[sub_resource type="BoxMesh" id="Root_Edge2_mesh"]
size = Vector3(0.016, 0.225, 0.027)

[sub_resource type="BoxMesh" id="Root_Blade3_mesh"]
size = Vector3(0.085, 0.225, 0.025)

[sub_resource type="BoxMesh" id="Root_Edge3_mesh"]
size = Vector3(0.016, 0.225, 0.027)

[sub_resource type="BoxMesh" id="Root_Blade4_mesh"]
size = Vector3(0.085, 0.225, 0.025)

[sub_resource type="BoxMesh" id="Root_Edge4_mesh"]
size = Vector3(0.016, 0.225, 0.027)

[sub_resource type="CylinderMesh" id="Root_Tip_mesh"]
top_radius = 0
bottom_radius = 0.046
height = 0.14
radial_segments = 12
rings = 1

[node name="Katana" type="Node3D"]


[node name="Handle" type="MeshInstance3D" parent="."]
position = Vector3(0, -0.16, 0)
mesh = SubResource("Root_Handle_mesh")
material_override = SubResource("Grip")

[node name="Wrap0" type="MeshInstance3D" parent="."]
position = Vector3(0, -0.29, 0)
mesh = SubResource("Root_Wrap0_mesh")
material_override = SubResource("Wrap")

[node name="Wrap1" type="MeshInstance3D" parent="."]
position = Vector3(0, -0.24, 0)
mesh = SubResource("Root_Wrap1_mesh")
material_override = SubResource("Wrap")

[node name="Wrap2" type="MeshInstance3D" parent="."]
position = Vector3(0, -0.19, 0)
mesh = SubResource("Root_Wrap2_mesh")
material_override = SubResource("Wrap")

[node name="Wrap3" type="MeshInstance3D" parent="."]
position = Vector3(0, -0.14, 0)
mesh = SubResource("Root_Wrap3_mesh")
material_override = SubResource("Wrap")

[node name="Wrap4" type="MeshInstance3D" parent="."]
position = Vector3(0, -0.09, 0)
mesh = SubResource("Root_Wrap4_mesh")
material_override = SubResource("Wrap")

[node name="Wrap5" type="MeshInstance3D" parent="."]
position = Vector3(0, -0.04, 0)
mesh = SubResource("Root_Wrap5_mesh")
material_override = SubResource("Wrap")

[node name="Tsuba" type="MeshInstance3D" parent="."]
position = Vector3(0, 0, 0)
mesh = SubResource("Root_Tsuba_mesh")
material_override = SubResource("Brass")

[node name="Pommel" type="MeshInstance3D" parent="."]
position = Vector3(0, -0.325, 0)
mesh = SubResource("Root_Pommel_mesh")
material_override = SubResource("Brass")

[node name="Blade0" type="MeshInstance3D" parent="."]
position = Vector3(-0, 0.12, 0)
mesh = SubResource("Root_Blade0_mesh")
material_override = SubResource("Steel")
rotation = Vector3(0, 0, 0)

[node name="Edge0" type="MeshInstance3D" parent="."]
position = Vector3(-0.037, 0.12, 0)
mesh = SubResource("Root_Edge0_mesh")
material_override = SubResource("Edge")
rotation = Vector3(0, 0, 0)

[node name="Blade1" type="MeshInstance3D" parent="."]
position = Vector3(-0.006, 0.338, 0)
mesh = SubResource("Root_Blade1_mesh")
material_override = SubResource("Steel")
rotation = Vector3(0, 0, 0.015)

[node name="Edge1" type="MeshInstance3D" parent="."]
position = Vector3(-0.043, 0.338, 0)
mesh = SubResource("Root_Edge1_mesh")
material_override = SubResource("Edge")
rotation = Vector3(0, 0, 0.015)

[node name="Blade2" type="MeshInstance3D" parent="."]
position = Vector3(-0.024, 0.556, 0)
mesh = SubResource("Root_Blade2_mesh")
material_override = SubResource("Steel")
rotation = Vector3(0, 0, 0.03)

[node name="Edge2" type="MeshInstance3D" parent="."]
position = Vector3(-0.061, 0.556, 0)
mesh = SubResource("Root_Edge2_mesh")
material_override = SubResource("Edge")
rotation = Vector3(0, 0, 0.03)

[node name="Blade3" type="MeshInstance3D" parent="."]
position = Vector3(-0.054, 0.774, 0)
mesh = SubResource("Root_Blade3_mesh")
material_override = SubResource("Steel")
rotation = Vector3(0, 0, 0.045)

[node name="Edge3" type="MeshInstance3D" parent="."]
position = Vector3(-0.091, 0.774, 0)
mesh = SubResource("Root_Edge3_mesh")
material_override = SubResource("Edge")
rotation = Vector3(0, 0, 0.045)

[node name="Blade4" type="MeshInstance3D" parent="."]
position = Vector3(-0.096, 0.992, 0)
mesh = SubResource("Root_Blade4_mesh")
material_override = SubResource("Steel")
rotation = Vector3(0, 0, 0.06)

[node name="Edge4" type="MeshInstance3D" parent="."]
position = Vector3(-0.133, 0.992, 0)
mesh = SubResource("Root_Edge4_mesh")
material_override = SubResource("Edge")
rotation = Vector3(0, 0, 0.06)

[node name="Tip" type="MeshInstance3D" parent="."]
position = Vector3(-0.1, 1.09, 0)
mesh = SubResource("Root_Tip_mesh")
material_override = SubResource("Steel")
scale = Vector3(1, 1, 0.3)
```

### `tests/smoke_test.gd`

Teste headless do combate. Roda sem janela:
`godot --headless --path . --script res://tests/smoke_test.gd`
Sai com código 1 se qualquer verificação falhar.

```gdscript
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
```

### `tests/capture.gd`

Captura quadros com renderização real, para conferir a imagem:
`godot --path . --script res://tests/capture.gd -- docs/capturas`
As imagens em `docs/capturas/` vieram desta execução.

```gdscript
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
```

### `LEIA-ME.md`

```markdown
# APARA — base de desenvolvimento 0.2

Projeto de duelo **lateral** para **Godot 4.6 ou mais recente, edição
Standard / GDScript**, com duas direções de arte disponíveis.

Os dois lutadores aparecem de corpo inteiro, de perfil, e a câmera é
ortográfica olhando na horizontal: toda superfície deitada vira uma linha e
a cena lê como elevação 2D, sem perspectiva. O mundo continua sendo
geometria 3D — é o enquadramento e a paleta fechada que dão a leitura.

| Tema | Base visual incluída |
| --- | --- |
| **Samurai — padrão** | Ronin de chapéu e placas em magenta contra um duelista em ciano; portal, bordos e lanternas |
| **Medieval** | Cavaleiro de aço azul contra um duelista em âmbar; muralha, torres, estandartes e tochas |

Os cenários, personagens e armas são cenas nativas editáveis no Godot.
As formas simples servem de base para criarmos a arte final. O combate e seus
ajustes são compartilhados pelos dois temas.

## Começar

1. Extraia **todo** o ZIP em uma pasta nova.
2. Abra o [Godot](https://godotengine.org/download/), clique em **Importar** e
   escolha `project.godot`.
3. Abra `main.tscn`: a prévia samurai já mostra o palco, os dois lutadores e a
   câmera lateral no editor.
4. Pressione **F5**, escolha **Samurai** ou **Medieval** no menu e inicie o duelo.

O tema pode ser trocado antes de iniciar ou na tela de resultado. Durante a
pausa de uma luta, o seletor fica oculto. **R** reinicia com o tema atual.
A escolha no menu vale para essa execução; o tema inicial fica em `main.tscn`,
no campo **Duel Theme** de **Parry2D**. A prévia salva da cena principal é samurai;
o campo Duel Theme determina o tema aplicado ao executar.

## Controles

| Controle | Ação |
| --- | --- |
| **Espaço** ou **clique esquerdo** | Aparar |
| **Clique direito** | Contra-atacar depois de aparar |
| **A** / **D** | Aproximar e afastar no palco |
| **Esc** | Pausar / continuar e liberar / capturar o cursor |
| **R** | Reiniciar o duelo |

O parry não tem direção — um botão só, e a única decisão é **quando**. Mas os
três golpes continuam importando, porque a altura virou relógio: **alto é
lento** e telegrafado, **baixo chega antes** do que a mão espera. A interface
diz as duas coisas juntas, e as três faixas sobre o duelista mostram de onde
vem o golpe.

Cuidado com a **finta**: às vezes o clarão vem e o golpe não. Quem apara nela
gasta a defesa e come o golpe de verdade logo depois. Nada avisa por escrito —
o aviso está no corpo do oponente, que recolhe em vez de comprometer, e no som
que começa e não termina.

A **firmeza** ao lado da vida conta quantas defesas dá para jogar fora. Cada
botão apertado à toa gasta um ponto; zerando, custa vida e volta cheia. Também
não há aviso de parry perfeito: ele se reconhece pelo som e pelo tranco. As três faixas
empilhadas sobre o duelista mostram onde está a sua guarda; a faixa dourada
com a seta mostra em que altura vem o golpe. No clarão branco, clique
esquerdo com a guarda na mesma altura.

É preciso acertar o tempo. Segurar o botão não renova a defesa.
Depois do parry, use o direito durante o atordoamento. Um contra-ataque remove
um ponto de vida. Você e o oponente começam com cinco pontos.

O parry dura 0,24 s, o perfeito aceita os últimos 0,075 s antes do impacto,
e a recuperação entre tentativas é de 0,62 s. Os tempos têm a resolução dos
passos de física. Há faíscas, sons sintetizados, pausa breve no impacto,
recuo e tremor de câmera; os efeitos precisam ser avaliados jogando.

## Onde vamos trabalhar

| Para mudar | Abra |
| --- | --- |
| Dificuldade, sensibilidade, vida e impacto | `config/combat.tres` no Inspetor |
| Regras de parry e do duelo | `scripts/main.gd` |
| Arena samurai | `scenes/themes/samurai/arena.tscn` |
| Ronin e armadura | `scenes/themes/samurai/sentinel.tscn` |
| Katana | `scenes/themes/samurai/weapon.tscn` |
| Arena, cavaleiro e espada medieval | Os três arquivos correspondentes em `scenes/themes/medieval/` |
| Cores, nomes e cenas de cada tema | `config/themes/samurai.tres` ou `medieval.tres` |
| Interface e indicador da guarda | `scripts/hud.gd` e `scripts/guard_indicator.gd` |
| Sons e faíscas | `scripts/feedback.gd` |

**Edite a cena de arte original**, na pasta do tema. Alterações feitas apenas
nos filhos das instâncias de prévia em `main.tscn` são substituídas ao carregar
o perfil do tema. Salve a cena original, pare o jogo e execute novamente.

- [Direção de arte e modelos](docs/DESIGN.md)
- [Guia para programar e testar](docs/DESENVOLVIMENTO.md)
- [Próximas etapas](docs/PROXIMOS_PASSOS.md)
- [Pasta para novos assets](art/LEIA-ME.md)

## Estado desta entrega

O projeto roda no Godot 4.6.3: importação limpa, sintaxe conferida nos sete
scripts, 126 verificações automáticas em `tests/smoke_test.gd` e os dois
temas renderizando — veja `docs/capturas/`. **O que falta é jogar:** a sensação do
parry e a clareza da leitura precisam de mouse e teclado para serem julgadas.

Esta é a base de código e design para continuar o desenvolvimento. Ainda não
inclui modelos finais, rig de personagem, animações esqueléticas, campanha,
salvamento ou executável exportado. As poses atuais movem nós por código.
```

### `docs/DESIGN.md`

```markdown
# Direção de arte — APARA

## Ideia central

Um duelo curto, visto **de lado**, em que o jogador lê a preparação, escolhe a
altura da guarda e responde com um parry preciso. As duas silhuetas, a espada e
o sinal do golpe devem ser fáceis de distinguir do fundo. Cenários e materiais
reforçam o impacto do aço.

**O parry não tem direção: um botão só.** Espaço ou clique esquerdo. Casar a
altura era uma segunda decisão dentro de um instante que dura 240 ms, e ela
chegava sempre tarde. Sobrou a decisão que interessa — **quando**.

### Mas os três golpes ficaram

As três alturas não viraram enfeite: elas viraram **relógio**. Cada uma tem
seu tempo em `ATTACK_TEMPO`, porque um corte alto é um movimento grande e
lento e um corte baixo é curto e chega antes:

| Golpe | Tempo | O que significa |
| --- | --- | --- |
| Alto | ×1,28 | Telegrafado. Dá tempo de ver e decidir |
| Médio | ×1,0 | O ritmo de referência |
| Baixo | ×0,76 | Chega antes do que a mão espera |

Ler a altura continua valendo a pena, só que para saber **quanto tempo se
tem** em vez de para onde apontar. A interface diz as duas coisas juntas:
"GOLPE: ALTO · LENTO".

A altura ainda manda na animação: a lâmina do jogador sobe para aparar um
corte alto e desce para um baixo. Quem escolhe não é o jogador, é o golpe que
veio — sem isso o aparo de um corte alto sairia com a espada na cintura.

### A finta

Um jogo de um botão só, sem armadilha, vira apertar no clarão. A finta é a
armadilha: **um clarão falso**, igual ao verdadeiro, no instante em que o
verdadeiro acenderia — e o golpe não vem. Quem apara por reflexo entra na
recuperação e come o golpe real, que chega logo depois.

Três decisões que a mantêm justa:

1. **O padrão de fintas é fixo** (`FEINT_PATTERN`), não sorteado. O padrão é
   para ser aprendido; um dado escondido transforma leitura em azar.
2. **O corpo entrega.** Depois do clarão falso o oponente recolhe em vez de
   comprometer — é esse puxão para trás que dá para aprender, porque o
   clarão em si é idêntico. Sem a camada de pose do corpo a finta seria
   ilegível, e é por isso que ela veio antes.
3. **`feint_hold` tem de ser maior que a recuperação do parry.** Menor, e
   quem cai na finta não tem defesa nenhuma: vira punição garantida em vez
   de armadilha. Há um teste automático prendendo esse limite.

O som também conta: a finta usa o mesmo começo do aviso e é **cortada no
meio**. O som que não termina diz "aquilo não valeu" sem precisar de texto —
e não há texto: anunciar "FINTA" na tela entregava de graça justamente o que
a finta cobra para ensinar.

### Firmeza

Uma janela que fecha sem aparar nada custa um ponto de **firmeza**. Zerada,
ela quebra: custa um ponto de vida e volta cheia. São três por padrão
(`steadiness` em `combat.tres`).

É o que impede o botão de virar metralhadora. Sem preço para errar, a finta
não cobra nada, a leitura vira enfeite e o duelo inteiro se resolve apertando
sem parar. Com preço, apertar por reflexo passa a ser uma decisão.

Ela aparece como pontos cheios e vazios ao lado da vida, não como número: é
olhada de canto de olho no meio do duelo, e uma contagem exige leitura.

### O que a tela não diz

Não há aviso de finta nem de parry perfeito. O perfeito se reconhece pelo
**som** — é outro clangor —, pelo tranco mais forte e pelas faíscas. Sentir
vale mais do que ler, e ler tira os olhos do oponente bem no momento em que
ele volta a atacar. A tela guarda texto para o que o jogador ainda precisa
decidir: que o contra-ataque está aberto, e que a firmeza quebrou.

As duas bases são fantasia estilizada; não são reconstruções históricas.
Samurai é a direção inicial de trabalho. Medieval permite comparar o clima
e desenvolver outra aparência usando as mesmas regras.

## Comparação dos temas

| Elemento | Samurai — Pátio do Bordo | Medieval — Pátio da Coroa |
| --- | --- | --- |
| Arma | Katana de lâmina levemente curva e guarda circular | Espada longa, guarda em cruz e pomo arredondado |
| Oponente | Ronin com placas sobrepostas, capacete e máscara | Cavaleiro com elmo fechado, ombreiras e tabardo |
| Silhueta | Capacete largo, ombros segmentados, saia de armadura | Ombros arredondados, peito de aço, tabardo vertical |
| Cenário | Pátio de pedra, portal vermelho, bordo e lanternas | Pátio de pedra, muralhas, torres, portão e estandartes |
| Materiais | Laca, tecido, madeira escura, aço e latão | Aço, couro, pedra, tecido azul e latão |
| Luz | Noite roxa, lanternas em ciano | Noite azul, tochas em âmbar |
| Par de cores | Ronin magenta, duelista ciano | Cavaleiro ciano, duelista âmbar |
| Neon do cenário | Ciano nas lanternas | Âmbar nas tochas |
| Movimento a criar | Preparações legíveis e cortes secos | Preparações com peso e recuperação evidente |

## Paleta de referência

A direção é **neo-noir**: quase tudo escuro, uma cor forte no oponente e a
complementar no neon do cenário. Não é decoração — é leitura. Contra um fundo
quase preto, a silhueta do oponente e o clarão do golpe aparecem sozinhos.

| Uso | Samurai | Medieval |
| --- | --- | --- |
| Massa do corpo do oponente | `#160917` | `#101227` |
| Placa na cor do tema | `#4C091D` | `#0A2C40` (tabardo) |
| Placa iluminada (chapéu, ombreiras) | `#730D2C` | `#5C6E8F` |
| Fio aceso das bordas | `#FF2973` | `#3DD1FF` |
| Tecido escuro (hakama / couro) | `#100B19` | `#100E14` |
| Pedra do chão | `#15121C` | `#0F131D` |
| Detalhes de metal | `#7A4F17` | `#855418` |
| Fresta dos olhos | `#FF73AD` | `#9EF2FF` |
| Massa do corpo do jogador | `#0A1413` | `#150D08` |
| Cor do jogador | `#254A45` | `#4A2C0F` |
| Fio aceso do jogador | `#A8E5CF` | `#FFB852` |
| Neon do cenário | `#2EE6FF` (lanternas) | `#FF9A2E` (tochas) |
| Guarda do jogador / parry | `#A9E5CF` | `#A4D5ED` |
| Sinal de ataque / parry perfeito | `#F2BB66` | `#F2BB66` |
| Dano | `#EF7670` | `#EF7670` |
| Painel da interface | `#201E1D` | `#1E2733` |

A armadura **não** é da cor do tema por inteiro. Ela é escura, e a cor do tema
aparece em três lugares: o alto da silhueta, um fio aceso nas bordas e o sinal
do golpe. Foi pintar o corpo todo de magenta que produziu o boneco de plástico.

**Os dois lutadores nunca compartilham família de cor.** Com os dois no quadro
ao mesmo tempo, um corpo escuro roxo ao lado de outro corpo escuro roxo vira
uma mancha só. Cada tema tem duas famílias opostas — magenta contra ciano no
samurai, ciano contra âmbar no medieval — e isso vale também para os refletores
presos a cada modelo: dar um contorno magenta ao jogador o devolve para o time
do oponente.

Três regras que sustentam isso:

1. **A cor vem da superfície, não da luz.** O sol é quase branco de propósito.
   Um sol rosado pinta a katana de rosa junto com a armadura, e aí nada se
   destaca de nada.
2. **A luz ambiente é baixa** (0,32). É ela que abre a sombra e cria silhueta.
   Ambiente alto é exatamente o que dá aparência de brinquedo plástico.
3. **A névoa engole o fundo** entre 4 e 17 unidades, para o cenário virar
   recorte escuro e o duelo ficar sozinho no primeiro plano.
4. **O chão fica mais escuro que o oponente.** Quando os dois têm o mesmo
   valor, o personagem gruda no piso e some — foi o que aconteceu com o
   cavaleiro azul sobre pedra azul.

## A câmera lateral

O duelo corre no eixo **X**, que é o eixo da tela. A câmera é **ortográfica** e
olha na horizontal ao longo de -Z, sem inclinação nenhuma. É essa combinação
que produz a leitura 2D: sem perspectiva, toda superfície deitada projeta uma
linha, e o chão vira uma quina reta em vez de um plano fugindo para o fundo.

| Ajuste | Onde | Valor |
| --- | --- | --- |
| Meia altura visível | `CAMERA_SIZE` em `main.gd` | 5,2 unidades |
| Altura do olho | `CAMERA_HEIGHT` | 1,55 |
| Distância do palco | `Camera3D.position.z` em `main.tscn` | 9 |
| Aproximação no parry | `CAMERA_PUNCH` | 0,22 |

A câmera acompanha o **meio** dos dois lutadores dentro de uma faixa
(`CAMERA_LIMIT`). Sem isso, um empurrão forte tira alguém do quadro.

Como a câmera fica a 9 unidades do duelo, a névoa precisa **começar depois
disso** (10,5 a 26). Herdar a névoa curta da versão em primeira pessoa engolia
os próprios lutadores.

O chão é um bloco cuja **face da frente** é o que se vê; a superfície de cima
some na linha do horizonte do palco, marcada por um fio aceso na quina. O fundo
é montado em três camadas de placas chatas, em z = -4, -7 e -14, com o valor
caindo a cada camada. Tudo fica atrás dos lutadores: uma peça entre eles e a
câmera tapa o duelo.

### Três quartos, não perfil puro

O corpo de cada lutador encara o adversário — é disso que a lógica do golpe
depende. O **volume** gira meio radiano para a câmera, num `VisualMount` que a
lógica não toca, e o sinal desse giro segue quem está de que lado do palco.
De perfil puro somem peito, ombreiras e olhos; some o personagem.

Pela mesma razão o sinal do golpe é um **anel** em volta do peito, e não uma
placa frontal: de lado, a placa ficaria de perfil e sumiria — e é dela que o
duelo inteiro depende.

## A pose diz a mesma coisa que a lâmina

Não há rig nem animação esquelética: as pernas são caixas fixas. O que
existe é uma camada de pose em `scripts/visuals/body_pose.gd`, que move
quatro coisas — inclinação do corpo, giro do tronco, sobe-e-desce e
inclinação da cabeça. É pouco, e resolve a maior parte.

O que ela compra:

**Antecipação e acompanhamento.** Na preparação o oponente se fecha para
trás; ao resolver o golpe, despenca para a frente. Sem isso a espada trocava
de posição sem o corpo participar, e o golpe não tinha peso nenhum.

**Um segundo aviso da altura.** O corpo repete o que o braço diz: sobe no
golpe alto, agacha no baixo (`WINDUP_BOB` em `enemy_brain.gd`, `GUARD_LEAN` e
`GUARD_BOB` em `player_controller.gd`). A 320x180 a espada é um risco fino e
some no fundo; a silhueta inteira não some. Vale para os dois lados — dá para
conferir a própria guarda pela postura sem tirar os olhos do oponente.

**Que o corpo não pareça congelado.** Um sobe-e-desce de respiração parado e
um de passo andando.

O solavanco do impacto é separado dos alvos de estado, porque um golpe
recebido não é um estado: é um empurrão em cima do que o corpo já fazia.
`punch()` soma e decai sozinho.

A cabeça devolve parte da inclinação do tronco (`HEAD_COUNTER`). É o truque
que mantém o rosto legível enquanto o corpo se joga para a frente.

As rotações que o arquivo da cena traz são guardadas como repouso e somadas,
nunca sobrescritas: a postura de guarda desenhada na arte sobrevive.

## Som em camadas

O ouvido lê um impacto em quatro partes, e cada uma diz uma coisa: o **ataque**
nos primeiros milissegundos diz que bateu, o **corpo** diz de que material, o
**peso** grave diz o tamanho e a **cauda** diz o valor da coisa. Faltando
qualquer uma delas o golpe soa barato, mesmo no volume certo. Todos os sons
ficam em `scripts/feedback.gd`, sintetizados — o projeto não usa arquivos de
áudio de terceiros.

Três decisões que sustentam isso:

**O parry perfeito ganha um brilho atrasado, não mais volume.** Um segundo
evento 45 ms depois do primeiro lê como recompensa; o mesmo som mais alto lê só
como mais alto.

**Bater na armadura não tem cauda nenhuma.** O som morre onde bate, e é essa
ausência que diz "não adiantou" antes de qualquer texto na tela.

**A mistura entre os sons é do áudio, não da configuração.** `MIX_DB` fixa a
posição de cada um: o aviso fica abaixo do impacto, o perfeito acima do comum.
`sound_volume_db` em `combat.tres` é só o volume geral — mexer nele não deve
reordenar o que é importante.

Um arquivo em `art/audio/<nome>.mp3` ou `.ogg` substitui o sintetizado do
mesmo nome enquanto estiver na pasta, e some da equação quando sai. É o jeito
de comparar a síntese com uma referência sem tocar em código — e a razão de
nada do que o jogo gera depender de arquivo de terceiros.

A taxa é 44,1 kHz porque metal vive acima de 8 kHz. A 22 kHz o clangor perde o
ar e sobra um toque de telefone. O ruído passa por um filtro de um polo antes
de entrar nas camadas: ruído branco puro soa a chiado de rádio; filtrado, soa a
material. E a soma das camadas passa por saturação suave em vez de corte seco,
senão o pico estala.

## O neon como camada

A referência declarada do projeto é *Katana Zero*. O criador dele conta em
entrevista que o efeito de luz neon "cobria todos os defeitos", e que é ele
que faz arte de dezenas de artistas diferentes parecer uma coisa só. Vale a
pena levar isso a sério: o brilho não é enfeite de acabamento, é o que costura
a imagem.

Por isso o neon é a **última** coisa do shader e a única que escapa da paleta.
Ele é somado por cima do resultado já quantizado, como uma camada de luz sobre
arte chapada — que é como um jogo 2D monta a cena. Enquanto ele era calculado
antes da paleta, a quantização o engolia: o halo caía na mesma entrada da lista
que a superfície ao lado e simplesmente desaparecia.

| Ajuste | Efeito | Valor |
| --- | --- | --- |
| `bloom_threshold` | Acima de quanto uma cor vira fonte de luz | 0,63 |
| `bloom_strength` | Quanto da luz é somado por cima | 1,15 |
| `bloom_radius` | Raio do halo, em pixels da arte | 3,2 |
| `bloom_saturation` | Impede o halo de crescer branco | 1,7 |

O limiar é o ajuste delicado. Baixo demais (0,42 foi tentado) e **toda**
superfície acende: a cena vira névoa colorida e as silhuetas somem. O certo é
que só as fontes acendam — os fios emissivos das bordas, o sinal do golpe, o
fio do chão, as lâmpadas e a lâmina —, com o resto chapado e escuro em volta.
É o contraste entre os dois que produz o efeito, não o brilho sozinho.

## Silhueta e luz do personagem

O mundo é desenhado a 320x180. Nessa escala o oponente ocupa cerca de 50 por 65
pixels: detalhe pequeno vira ruído e o que sobra é **silhueta e valor**. Daí as
três decisões que sustentam os dois modelos.

**Largo, estreito, largo.** O alto da silhueta é a peça mais larga — o chapéu
cônico do ronin, o manto de ombros do cavaleiro. A cintura estreita. A saia
abre de novo. Uma pilha de caixas da mesma largura, que era o desenho anterior,
lê como faixas horizontais empilhadas e não como pessoa.

**Formas facetadas em vez de caixas.** Troncos, chapéu, elmo e saias são
`CylinderMesh` de oito lados com raios diferentes em cima e embaixo, girados um
oitavo de volta para deixar uma face inteira virada para a câmera. A faceta é o
que faz a borda acender quando a luz raspa; a caixa devolve um tom chapado.

**A luz vai presa ao modelo.** Cada oponente carrega o próprio refletor, então
a iluminação não muda quando ele anda pela arena:

| Nó | Papel |
| --- | --- |
| `KeyLight` — SpotLight3D | Chave frontal alta, à esquerda. É ela que dá forma ao corpo; o sol da arena está baixo demais para isso |
| `RimWarm` — OmniLight3D | Contorno atrás do ombro esquerdo, na cor do tema |
| `RimCool` — OmniLight3D | Contorno atrás do ombro direito, na cor complementar |

Os dois contornos ficam com alcance curto (1,55) e acima de 1,75 de altura de
propósito: alcance maior derrama uma poça de luz no chão que rouba a cena.

A postura também é desenho. O tronco fica dentro de um nó `Body` girado cerca
de 0,2 rad e os pés são escalonados, um à frente e outro atrás. A cabeça e o
`AttackCue` ficam **fora** desse giro — a leitura do golpe não pode depender
da pose.

O jogador e o oponente usam a mesma linguagem de construção, mas nunca a mesma
silhueta: o oponente é alto (2,5), de chapéu largo ou elmo e placas; o jogador
é mais baixo (1,95), de cabeça descoberta e casaco. Num relance, quem é quem se
decide pela forma antes de pela cor.

As cores da interface ficam nos perfis em `config/themes/`. Os materiais 3D
ficam dentro das cenas de arte, com nomes como Armor, Cloth, Steel e Stone.
Alterar a paleta da interface não repinta automaticamente a armadura.

## Resolução e paleta

O mundo 3D não é desenhado no tamanho da janela. Ele passa por um SubViewport
renderizado em resolução baixa e ampliado sem suavização, e depois por um
shader que mapeia cada pixel para a **paleta fechada do tema**. É isso que dá o
acabamento pixel art sem redesenhar nada: as formas continuam sendo geometria,
o pixel e a cor aparecem na hora de mostrar.

A paleta vive no perfil do tema, em `config/themes/*.tres`, campo `palette`.
Samurai tem 32 cores, medieval 35. Trocar o tema troca a paleta junto com a
arena, o oponente e a arma.

O shader escolhe as duas cores mais próximas de cada pixel e decide entre elas
pela **posição da cor original no segmento entre as duas**, comparada com uma
matriz de Bayer 4x4. Assim uma superfície que já bate com uma cor da paleta
fica chapada, e só a transição entre duas cores ganha o pontilhado. Uma
fórmula que dithere pelo "empate" entre as candidatas enche a tela de ruído —
foi o primeiro resultado, e não é o que se quer.

| Ajuste | Onde | Efeito |
| --- | --- | --- |
| `palette` | `config/themes/*.tres` | Cores permitidas. Lista vazia volta ao modo de degraus livres |
| `shadow_tint` / `highlight_tint` | idem | Duotone: matiz da sombra e da luz. Normalizado, muda a cor sem mexer no brilho |
| `exposure`, `contrast`, `saturation` | Inspetor do nó raiz | Gradação antes da paleta |
| `bloom_strength`, `bloom_threshold` | idem | Vazamento do neon para os pixels vizinhos |
| `aberration` | idem | Separação de vermelho e azul a partir do centro |
| `scanline_strength`, `vignette_strength` | idem | Linhas de varredura e escurecimento das bordas |
| `pixel_shrink` | Inspetor do nó raiz de `main.tscn` | Divisor da resolução. 4 numa janela de 1280x720 desenha 320x180. 1 desliga |
| `color_levels` | idem | Degraus por canal, usado só quando não há paleta |
| `dither_strength` | idem | Força do pontilhado nas transições. 0 deixa faixas chapadas |

O indicador de guarda é desenhado em blocos do mesmo tamanho do pixel do
mundo, para a interface do duelo não destoar da cena. O **texto** da HUD fica
**fora** desse caminho, na resolução da janela. É escolha deliberada: o duelo
se decide em cerca de 160 ms e a leitura precisa caber nesse tempo. Para levar
a interface inteira para o pixel, o caminho é mudar o modo de esticamento do
projeto para `viewport` e reduzir os tamanhos de fonte em `hud.gd` na mesma
proporção.

Escolher a cor pelos materiais continua valendo: a paleta só aproxima o que a
arte já definiu. Duas cores próximas entre si colapsam na mesma entrada, então
contraste decidido na arte sobrevive melhor. Ao criar uma cor nova para uma
peça, vale acrescentá-la à paleta do tema, senão ela cai na vizinha.

## Como editar a arte agora

1. No painel de arquivos do Godot, abra `scenes/themes/samurai/arena.tscn`.
2. Selecione um objeto na árvore de cena. A aba 3D permite mover e escalar;
   o Inspetor permite editar Mesh e Material Override.
3. Salve com Ctrl+S. Abra `main.tscn` e pressione F5 para conferir no duelo.
4. Repita com `sentinel.tscn` para o ronin e `weapon.tscn` para a katana.
5. Para trabalhar no medieval, use os mesmos nomes na pasta `medieval` e
   selecione esse tema no menu do jogo.

Os materiais são compartilhados por peças da mesma cena. Se uma única peça
precisar de outra cor, torne seu material único antes de editá-lo.

## Trocar os modelos por sprites desenhados

A referência é arte 2D desenhada à mão, não geometria. O caminho existe e a
junta já está pronta: o tema aponta `opponent_scene` e `duelist_scene` para
cenas quaisquer, e o jogo só exige delas o contrato da tabela acima —
`WeaponPivot` com a arma dentro, `AttackCue` com material próprio no oponente
e, opcionalmente, `Arms`. Uma cena com um `AnimatedSprite3D` no lugar das
malhas satisfaz o mesmo contrato e entra sem tocar no combate.

Duas coisas a saber antes de começar:

1. **Assar os modelos atuais em sprites não muda nada na tela.** Sai a mesma
   imagem, só pré-renderizada. O ganho só aparece com quadros desenhados.
2. Um sprite desenhado não recebe os refletores presos ao modelo. A luz que
   hoje vem do rig teria de estar **pintada no próprio quadro** — é assim que
   a referência faz, e é por isso que lá a silhueta é quase preta com duas ou
   três cores. O neon do shader continua valendo por cima.

Referências úteis para esse trabalho estão reunidas em
`docs/REFERENCIAS.md`.

## Trocar as formas simples por modelos

Use `art/models/samurai/` e `art/models/medieval/` para os modelos que criarmos.
Um arquivo `.glb` pode ser importado e instanciado dentro de uma cena de arte.
Mantenha uma cena própria do projeto em volta do modelo para conservar as
conexões descritas abaixo.

**Lutadores:** origem nos pés, eixo Y para cima, frente para -Z. O oponente tem
cerca de 2,5 unidades de altura e o jogador 1,95. As duas cenas seguem o mesmo
contrato — `opponent_scene` e `duelist_scene` no perfil do tema. Cristas e ornamentos podem ultrapassar
o corpo. As colisões pertencem ao personagem em `main.tscn`, não à sua malha.

A raiz da cena do oponente precisa ser Node3D e manter:

| Nó | Função |
| --- | --- |
| `WeaponPivot` — Node3D | Ponto que o código gira para preparar e executar golpes |
| `WeaponPivot/WeaponVisual` | Instância da arma; acompanha o ponto de movimento |
| `AttackCue` — MeshInstance3D | Anel no peito cuja cor muda com o ataque; só o oponente precisa dele |
| `Arms` — Node3D | Opcional. Cada filho Node3D é um braço, virado para o punho a cada quadro |

AttackCue usa **Material Override do tipo StandardMaterial3D**. Se organizar
os nós em outra hierarquia, atualize `weapon_pivot_path`, `attack_cue_path` e
`arms_path` no perfil `.tres` correspondente. O jogo confere as duas primeiras
conexões ao trocar tema; `Arms` é opcional e, se faltar, os braços ficam parados.

Cada braço de `Arms` é montado apontando o próprio **-Z** para longe do ombro,
com a mão na ponta. `enemy_brain.gd` gira o braço para o punho e estica seu
`scale.z` até alcançá-lo, usando a meta `reach` do nó — o comprimento em repouso
entre o ombro e a mão. Sem esse esticamento a mão passa longe do cabo nas
preparações curtas e a pose se desmancha em pedaços soltos.

**Arma:** raiz Node3D com origem na guarda/parte superior do cabo, lâmina
apontando para +Y. O cabo fica abaixo da origem. A cena da arma é instanciada
**dentro** de cada modelo, sob o seu `WeaponPivot`, e é compartilhada pelos
dois lutadores do tema.

Para um modelo novo, duplique a cena de arte, substitua as malhas de protótipo,
preserve os pontos de conexão e aponte o campo de cena no perfil do tema
para a cópia nova. Assim podemos comparar o modelo novo com a base.

## Arena e leitura do combate

O palco é uma faixa no eixo X: chão de 46 unidades e duas paredes invisíveis em
X = ±13. O jogador inicia em X = -2,6; o oponente, em X = 4. Não há profundidade
jogável — o duelo é uma linha, e é isso que dá a leitura de jogo 2D.

Portais, árvores e torres ficam **atrás** dos lutadores, nunca entre eles e a
câmera. Se criar peças acessíveis que
devam bloquear passagem, crie também suas colisões. Os golpes atuais verificam
distância e arco, sem teste de oclusão: obstáculos entre os personagens exigem
uma checagem de linha de visão antes do dano. Evite esses obstáculos nesta base.

O indicador mostra alto, médio e baixo, empilhados como aparecem no duelo, e
fica **sobre o jogador** — no centro cairia em cima do oponente, bem onde está
a leitura. Ele não mostra guarda nenhuma do jogador, porque não há: mostra de
que altura o golpe vem. Durante a janela as três faixas acendem juntas, para
não sugerir que ainda há um lado a escolher. A pose do inimigo deve concordar
com ele: as três posições de preparação em `enemy_brain.gd` são alturas bem
separadas de propósito. O claro final da preparação avisa o
impacto; faíscas e tremor devem permanecer breves para não esconder o golpe seguinte.

## Arte a desenvolver na próxima etapa

A escala da katana dentro da câmera está resolvida: a arma ocupa o canto e
deixa o centro livre. Falta:

1. Criar modelo com rig e animações: repouso, preparação dos três lados,
   golpe, aparado, atordoado, recebendo contra-ataque e derrota. Os braços já
   acompanham a arma por código, mas tronco, quadril e pernas continuam parados.
2. Integrar o rig à lógica, sincronizando o instante de dano com a animação.
3. Levar o cenário medieval para o âmbar que a tabela promete. Hoje a arena é
   azul como o cavaleiro, e só o chão escuro separa os dois.
4. Refinar materiais, som e partículas depois de testar o tempo do parry.

Ainda não há rig, animação esquelética, tecido simulado nem vento. A silhueta,
o valor e a luz de personagem já foram vistos em execução — as capturas em
`docs/capturas/` são a referência do que está no lugar.
```

### `docs/REFERENCIAS.md`

```markdown
# Referências visuais

A direção do APARA é neo-noir, com *Katana Zero* como referência declarada.
Não existe um breakdown técnico oficial daquele jogo; o que existe é isto.

## Para estudar a arte

| Fonte | O que dá para tirar |
| --- | --- |
| [The Spriters Resource — Katana ZERO](https://www.spriters-resource.com/pc_computer/katanazero/) | Os sheets do jogo. É a fonte mais útil: dá para contar as cores por personagem, ver o tamanho real dos quadros e como as poses de golpe são desenhadas |
| [Concept art oficial da Askiisoft](https://blog.askiisoft.com/post/185309489923/the-concept-art-of-katana-zero) | Evolução das silhuetas ao longo dos sete anos de desenvolvimento. É retrospectiva de personagem, não técnica |
| [Entrevista de Justin Stander no MCV](https://mcvuk.com/business-news/askiisoft-katana-zero/) | O achado mais aproveitável: o neon "cobria todos os defeitos" e é o que faz arte de dezenas de artistas parecer uma coisa só |
| [Estudo de Artem Samoilov no ArtStation](https://www.artstation.com/artwork/qA5dKa) | Composição de fundo em camadas |
| [Cena animada na Workshop do Steam](https://steamcommunity.com/sharedfiles/filedetails/?id=2405240007) | Fundo em camadas, com movimento |
| [Thread no fórum do GameMaker sobre as luzes de rua](https://forum.gamemaker.io/index.php?threads/how-to-get-street-lights-effects-like-in-katana-zero.116367/) | Discussão do efeito em engine: superfície de luz, mistura aditiva e desfoque |

## Ouvir uma referência dentro do jogo

`art/audio/<nome>.mp3` (ou `.ogg`) substitui o som sintetizado do mesmo nome
enquanto o arquivo estiver na pasta: `parry`, `perfect`, `hit`, `hurt`,
`swing`, `cue`, `armor`, `feint`. Tirando o arquivo, o sintetizado volta.

Serve para ouvir a referência **no lugar dela**, com o hitstop e o tremor em
volta, que é a única forma de julgar se um som de impacto funciona. O que
aprender ali volta para os números em `feedback.gd`.

Vale o mesmo que para os sheets abaixo: som de jogo comercial é material de
estudo, não de publicação.

## O que já foi trazido para o projeto

O ponto da entrevista virou regra: o neon é a última etapa do shader e a única
que escapa da paleta fechada. Está descrito em `DESIGN.md`, seção
**O neon como camada**.

O que ainda separa o projeto da referência é a técnica: lá a arte é desenhada
quadro a quadro, com a luz pintada dentro do sprite; aqui é geometria iluminada
em tempo real. A junta para trocar uma coisa pela outra está descrita em
`DESIGN.md`, seção **Trocar os modelos por sprites desenhados**.

Os sheets acima são material de referência de terceiros. Servem para estudo de
proporção, contagem de cores e leitura de pose — não para uso no jogo. O mesmo
vale para qualquer áudio colocado em `art/audio/`.
```

### `docs/DESENVOLVIMENTO.md`

```markdown
# Programar e testar

## Abrir o código

Abra `scripts/main.gd` no editor de scripts do próprio Godot. Não é necessário
configurar um editor externo. A cena principal já liga os recursos e os nós.

Os dois lados do duelo moram em arquivos próprios. `main.gd` guarda o fluxo e
a **resolução** dos golpes, que é o único ponto que precisa conhecer os dois ao
mesmo tempo.

| Arquivo / método | Responsabilidade |
| --- | --- |
| `player_controller.gd` | Guarda escolhida, janela, recuperação, movimento e pose da arma |
| `player_controller.gd` → `try_parry()` | Abre a janela e trava o lado; falso quando o clique não conta |
| `player_controller.gd` → `steer_guard()` | Acumula o mouse até o limiar e escolhe o lado |
| `enemy_brain.gd` | Aproximação, preparação, recuperação, atordoamento e pose do oponente |
| `visuals/body_pose.gd` | Inclinação, giro, sobe-e-desce e cabeça dos dois lutadores |
| `visuals/arm_rig.gd` | Vira e estica cada braço até o punho da arma |
| `enemy_brain.gd` → `tick()` | Avança um quadro e devolve os eventos `cue` e `resolve` |
| `enemy_brain.gd` → `start_attack()` | Escolhe o lado no padrão e abre a preparação |
| `main.gd` → `_resolve_enemy_hit()` | Decide esquiva, parry ou dano uma vez por golpe |
| `main.gd` → `_on_parry_success()` | Aplica atordoamento, contagem e efeitos |
| `main.gd` → `_try_counter()` | Resolve o contra-ataque durante o atordoamento |
| `main.gd` → `_apply_theme()` | Instancia as cenas e conecta o visual ao combate |
| `main.gd` → `_apply_pixel_look()` | Liga a resolução baixa e os parâmetros do shader |
| `shaders/pixelate.gdshader` | Mapeia o mundo 3D para a paleta do tema, com pontilhado |
| `main.gd` → `_apply_palette()` | Monta a paleta do tema como textura de uma linha |
| `hud.gd` | Interface, seletor de tema, pausa e resultado |
| `guard_indicator.gd` | Três setores e sinal do golpe |
| `feedback.gd` | Sons sintetizados e partículas transitórias |
| `scripts/config/combat_tuning.gd` | Define os campos editáveis de combate |
| `scripts/config/duel_theme.gd` | Define os campos editáveis de aparência |
| `tests/smoke_test.gd` | 126 verificações de combate e acabamento, sem janela |
| `tests/capture.gd` | Grava quadros do duelo para conferir a imagem |

## Alterar um ajuste

Abra `config/combat.tres` no Inspetor, mude **um ajuste por vez**, salve,
pare o jogo e execute novamente. Esse recurso serve aos dois temas.

Para começar, mexa em `parry_window`, `enemy_windup` ou `feint_hold`. Este
último decide se a finta é armadilha ou punição garantida: precisa segurar o
golpe por mais tempo que a recuperação do parry, senão quem cai nela não tem
defesa nenhuma. Impacto, som, tremor e sobreposição de cor têm seus próprios
campos. Não é preciso procurar números dentro da lógica.

Os valores atuais ficam em `config/combat.tres`; os valores de criação de
um recurso novo ficam nas declarações de `combat_tuning.gd`.

## Contratos entre as partes

As formas físicas ficam em `main.tscn`. O jogador usa camada 2 e máscara 5;
o inimigo, camada 4 e máscara 3; o cenário usa camada 1. Os modelos visuais
podem ser substituídos preservando esses corpos e suas colisões.

Todo o mundo 3D fica dentro de `Pixelate/Viewport`, o SubViewport de resolução
baixa. Os caminhos `Pixelate/Viewport/Stage`, `.../Effects`, `.../Player`,
`.../CameraRig/Camera3D`, `.../Player/VisualMount`, `.../Sentinel` e
`.../Sentinel/VisualMount` são usados em `_bind_scene_nodes()`. Se renomeá-los,
atualize esse método.

Qualquer nó 3D criado por código precisa entrar em `Effects`, dentro do
SubViewport. Fora dele o nó cai num mundo sem câmera e simplesmente não
aparece — é o caso das faíscas de `feedback.gd`. A HUD é o contrário: fica
fora do SubViewport, na resolução da janela.

`PlayerController` e `EnemyBrain` são RefCounted, não nós: não aparecem na
árvore de cena e recebem os corpos por `setup()`. Nenhum dos dois decide o
resultado de um golpe. O oponente devolve eventos e `main.gd` escolhe a
consequência — é isso que mantém **um único instante de resolução por ataque**,
que precisa continuar valendo quando as poses virarem animações.

Os perfis de tema apontam para três PackedScenes: arena, oponente e arma.
Apenas as partes visuais são substituídas. O código duplica o material do
AttackCue antes de mudar sua cor para não alterar o recurso compartilhado.

## Primeira rodada de verificação no Godot

Nenhuma caixa abaixo representa um teste já executado.

- [ ] Importar o projeto no Godot 4.6+ e confirmar ausência de erros de script.
- [ ] Abrir main.tscn e conferir a prévia samurai no editor 3D.
- [ ] F5: iniciar Samurai e conferir cenário, personagem, katana e interface.
- [ ] Escolher as três guardas pelo mouse; para baixo deve manter a guarda atual.
- [ ] Combinar lado e tempo: sem dano, faíscas, som metálico e atordoamento.
- [ ] Aparar no limite final: conferir indicação e efeitos de parry perfeito.
- [ ] Clicar cedo e escolher lado errado: ambos devem permitir receber dano.
- [ ] Segurar o esquerdo: a janela termina e não se renova automaticamente.
- [ ] Usar o direito após aparar: remover apenas um ponto de vida do oponente.
- [ ] Atacar antes de aparar: armadura impede o dano.
- [ ] Recuar além do alcance: golpe do inimigo não causa dano.
- [ ] Pausar no meio do golpe e retomar; não deve avançar durante o menu.
- [ ] Sair da janela: o duelo deve pausar e liberar o cursor.
- [ ] Completar vitória e derrota; reiniciar e conferir vida e contadores.
- [ ] Na tela de resultado, trocar para Medieval e repetir o duelo.
- [ ] Conferir que espada, oponente, arena, nomes e cores mudam juntos.
- [ ] Conferir que o seletor não aparece ao pausar uma luta em andamento.
- [ ] Alterar um material na cena de arte original, salvar e conferir em F5.
- [ ] Reduzir tremor e desligar a sobreposição de cor no recurso de combate.

O projeto já foi importado e executado no Godot 4.6.3. A checagem de sintaxe
passa em todos os scripts e `tests/smoke_test.gd` cobre o jogo com 126 verificações
headless. A lista acima continua valendo para o teste com as mãos: é ela que
julga a sensação, que nenhum teste automático mede.

## Próximas mudanças no código

O controlador do jogador e a máquina de estados do inimigo já saíram de
`main.gd` para `player_controller.gd` e `enemy_brain.gd`, com as verificações
de `tests/smoke_test.gd` passando antes e depois e os quadros de `docs/capturas/`
idênticos. A substituição de poses por animações deve manter um único instante
de resolução de cada ataque.

O próximo passo de código depende de jogar: ajustar `config/combat.tres` com a
sensação na mão, um valor por vez.

Referências: [Resources](https://docs.godotengine.org/en/stable/tutorials/scripting/resources.html),
[PackedScene](https://docs.godotengine.org/en/stable/classes/class_packedscene.html) e
[OptionButton](https://docs.godotengine.org/en/stable/classes/class_optionbutton.html).
```

### `docs/PROXIMOS_PASSOS.md`

```markdown
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
```
godot --headless --path . --script res://tests/smoke_test.gd    # 126 verificações
godot --path . --script res://tests/capture.gd -- docs/capturas # 6 quadros
```

## 1. Jogar

É o único item que continua bloqueado, e bloqueia os outros. Abrir `main.tscn`,
pressionar F5 e disputar duelos curtos. Observar se dá para ler o lado, perceber
a janela, distinguir parry normal de perfeito e contra-atacar com intenção.

Ajustar **um valor por vez** em `config/combat.tres`. Comece por `parry_window`,
`enemy_windup` e `mouse_guard_threshold`. Nenhum teste automático mede sensação.

## 2. Modelar o ronin

A direção de arte agora existe para guiar o modelo, o que antes não era o caso:
silhueta escura recortada contra fundo quase preto, armadura magenta, detalhes
em âmbar, lâmina de aço branco. `docs/DESIGN.md` traz escala, pontos de conexão
e a paleta.

O que ainda entrega a geometria como blocos é a malha: braços e pernas em caixas
separadas. A luz esconde, não resolve. Resolver é modelar com rig.

## 3. Animar

Repouso, preparação dos três lados, golpe, aparado, atordoado, recebendo
contra-ataque e derrota. Ao trocar as poses de código por animações, preservar
**um único instante de resolução por ataque** — é o contrato que `enemy_brain.gd`
sustenta hoje com o evento `resolve`.

## 4. Fechar a paleta

Hoje a paleta aproxima cores que a arte definiu livremente. Com o modelo final,
vale escolher as cores primeiro e pintar dentro delas.

## Decisão pendente

`script_parry_3d.md` duplica os 33 arquivos do projeto. Está sincronizado e
confere byte a byte, mas são duas fontes da mesma verdade. Decidir se ele segue
sendo mestre ou vira só documentação.

O primeiro marco é **um duelo completo com três lados de parry e arte coerente**.
Campanha, múltiplos inimigos, progressão e exportação ficam para depois.
```

### `art/LEIA-ME.md`

```markdown
# Pasta de criação de arte

| Pasta | Destino |
| --- | --- |
| `models/samurai/` | Modelos de ronin, katana e peças de cenário samurai |
| `models/medieval/` | Cavaleiro, espada e peças de castelo |
| `textures/` | Texturas e mapas de materiais que criarmos |
| `audio/` | Sons produzidos, e sons de referência para comparar com os sintetizados |
| `concepts/` | Esboços e referências visuais do projeto |

Um arquivo `audio/<nome>.mp3` ou `.ogg` **substitui** o som sintetizado do
mesmo nome enquanto estiver ali — `parry`, `perfect`, `hit`, `hurt`, `swing`,
`cue`, `armor`, `feint`. Tirando o arquivo, o sintetizado volta sozinho.
Serve para ouvir uma referência no lugar dela ao ajustar a síntese.

Som de jogo comercial é material de estudo, não de publicação: usar para
comparar timbre e tempo é uma coisa, distribuir junto é outra. Por isso nada
do que `feedback.gd` gera depende de arquivo nenhum desta pasta.

Essas pastas são pontos de entrada para assets futuros; não contêm modelos
finais. As bases geométricas existentes estão em `scenes/themes/` e já podem
ser editadas diretamente no Godot. O guia `docs/DESIGN.md` explica escala,
materiais e conexões que os novos modelos devem preservar.
```

