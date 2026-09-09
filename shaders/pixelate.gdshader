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
