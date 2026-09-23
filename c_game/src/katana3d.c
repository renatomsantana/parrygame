/*
 * katana3d.c - carrega o modelo, aplica a textura e desenha com luz simples,
 * cores reduzidas a poucos tons para conversar com a pixel art.
 */
#include "katana3d.h"

#include <math.h>
#include <stdio.h>

#include "raymath.h"
#include "rlgl.h"

#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

/* Medidas do modelo original: eixo longo em Z, cabo em -Z, ponta da lâmina em +Z. */
#define MODEL_BUTT_Z -41.168182f
#define MODEL_TIP_Z 20.825689f

static Model model;
static Shader shader;
static Texture2D color, metal;
static bool ready;
static int locLight, locTint, locLevels, locBlade, locHandle;
static Camera3D camera;

static const char *VS =
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec3 vertexNormal;\n"
    "uniform mat4 mvp; uniform mat4 matModel; uniform mat4 matNormal;\n"
    "out vec2 uv; out vec3 n;\n"
    "void main() {\n"
    "    uv = vertexTexCoord;\n"
    "    n = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *FS =
    "#version 330\n"
    "in vec2 uv; in vec3 n;\n"
    "uniform sampler2D texture0; uniform sampler2D texture1; uniform vec4 colDiffuse;\n"
    "uniform vec3 lightDir; uniform vec4 tint; uniform vec3 bladeTint; uniform vec3 handleTint; uniform float levels;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec3 base = texture(texture0, uv).rgb;\n"
    "    float metal = texture(texture1, uv).r;\n"
    "    base = mix(base, vec3(0.80, 0.82, 0.86), metal * 0.75);\n"
    "    vec3 N = normalize(n);\n"
    "    if (!gl_FrontFacing) N = -N;\n"
    "    vec3 L = normalize(lightDir);\n"
    "    vec3 V = vec3(0.0, 0.0, -1.0);\n"
    "    float diff = max(dot(N, L), 0.0);\n"
    "    float spec = pow(max(dot(N, normalize(L + V)), 0.0), 20.0);\n"
    "    float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0);\n"
    "    vec3 c = base * (0.55 + 0.8 * diff) + vec3(1.0, 0.96, 0.88) * (spec * (0.5 + 1.2 * metal) + rim * 0.3);\n"
    "    c *= tint.rgb * mix(handleTint, bladeTint, metal);\n"
    "    c = floor(c * levels + 0.5) / levels;\n"
    "    finalColor = vec4(c, colDiffuse.a * tint.a);\n"
    "}\n";

bool katana3d_load(const char *dir) {
    char path[512];
    snprintf(path, sizeof path, "%s/katana.glb", dir);
    if (!FileExists(path)) return false;
    model = LoadModel(path);
    if (model.meshCount == 0) return false;
    snprintf(path, sizeof path, "%s/katana_color.png", dir);
    color = LoadTexture(path);
    SetTextureFilter(color, TEXTURE_FILTER_BILINEAR);
    snprintf(path, sizeof path, "%s/katana_metal.png", dir);
    metal = LoadTexture(path);
    SetTextureFilter(metal, TEXTURE_FILTER_BILINEAR);
    shader = LoadShaderFromMemory(VS, FS);
    locLight = GetShaderLocation(shader, "lightDir");
    locTint = GetShaderLocation(shader, "tint");
    locLevels = GetShaderLocation(shader, "levels");
    locBlade = GetShaderLocation(shader, "bladeTint");
    locHandle = GetShaderLocation(shader, "handleTint");
    for (int i = 0; i < model.materialCount; i++) {
        model.materials[i].shader = shader;
        model.materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = color;
        model.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
        model.materials[i].maps[MATERIAL_MAP_METALNESS].texture = metal;
    }
    Vector3 light = Vector3Normalize((Vector3){-0.45f, -0.75f, -0.55f}); /* de cima, à esquerda, para a câmera */
    SetShaderValue(shader, locLight, &light, SHADER_UNIFORM_VEC3);
    float levels = 6;
    SetShaderValue(shader, locLevels, &levels, SHADER_UNIFORM_FLOAT);
    ready = true;
    return true;
}

void katana3d_unload(void) {
    if (!ready) return;
    for (int i = 0; i < model.materialCount; i++) model.materials[i].shader = (Shader){0};
    UnloadShader(shader);
    UnloadTexture(color);
    UnloadTexture(metal);
    UnloadModel(model);
    ready = false;
}

bool katana3d_ready(void) { return ready; }

void katana3d_begin(int width, int height) {
    /* Olhando para +Z com "cima" em -Y: x para a direita e y para baixo, como na tela. */
    camera.position = (Vector3){width / 2.0f, height / 2.0f, -500};
    camera.target = (Vector3){width / 2.0f, height / 2.0f, 0};
    camera.up = (Vector3){0, -1, 0};
    camera.fovy = (float)height;
    camera.projection = CAMERA_ORTHOGRAPHIC;
    rlDrawRenderBatchActive();
    glClear(GL_DEPTH_BUFFER_BIT); /* só a profundidade: o cenário já desenhado fica */
    BeginMode3D(camera);
    rlDisableBackfaceCulling();
}

void katana3d_end(void) {
    rlEnableBackfaceCulling();
    EndMode3D();
}

void katana3d_draw(Vector2 butt, Vector2 tip, float roll, const KatanaStyle *style, Color light) {
    if (!ready) return;
    float dx = tip.x - butt.x, dy = tip.y - butt.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1) return;
    float angle = atan2f(dy, dx);
    float k = len / (MODEL_TIP_Z - MODEL_BUTT_Z);
    /* Pequena na tela, a lâmina engrossa para não virar uma linha pontilhada. */
    float thick = (len < 60 ? 2.2f : 1.5f) * (style ? style->width : 1);
    /* Cabo na origem, lâmina ao longo de +X, giro no próprio eixo, escala, ângulo e posição. */
    Matrix m = MatrixTranslate(0, 0, -MODEL_BUTT_Z);
    m = MatrixMultiply(m, MatrixRotateY(PI / 2));
    m = MatrixMultiply(m, MatrixRotateX(roll * DEG2RAD));
    m = MatrixMultiply(m, MatrixScale(k, k * thick, k * thick));
    m = MatrixMultiply(m, MatrixRotateZ(angle));
    m = MatrixMultiply(m, MatrixTranslate(butt.x, butt.y, -8)); /* na frente da mão de quem segura */
    model.transform = m;
    float t[4] = {light.r / 255.0f, light.g / 255.0f, light.b / 255.0f, light.a / 255.0f};
    Color bc = style ? style->blade : WHITE, hc = style ? style->handle : WHITE;
    float b3[3] = {bc.r / 255.0f, bc.g / 255.0f, bc.b / 255.0f}, h3[3] = {hc.r / 255.0f, hc.g / 255.0f, hc.b / 255.0f};
    SetShaderValue(shader, locTint, t, SHADER_UNIFORM_VEC4);
    SetShaderValue(shader, locBlade, b3, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, locHandle, h3, SHADER_UNIFORM_VEC3);
    DrawModel(model, (Vector3){0, 0, 0}, 1, WHITE);
}
