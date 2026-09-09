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
