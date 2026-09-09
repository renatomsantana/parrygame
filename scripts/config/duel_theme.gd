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
