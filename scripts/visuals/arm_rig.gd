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
