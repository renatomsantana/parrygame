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
