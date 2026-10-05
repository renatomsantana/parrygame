#include "../src/arenas.c"
#include "rlgl.h"
#include <string.h>

int main(void) {
    const char *root = getenv("APARA_ARENA_DIR");
    if (!root) return 2;
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN | FLAG_MSAA_4X_HINT);
    InitWindow(320, 180, "teste de cenários");
    if (!IsWindowReady()) return 2;
    char path[1024];
    Image sheet = GenImageColor(640, 180, RED);
    ImageDrawRectangle(&sheet, 320, 0, 320, 180, GREEN);
    snprintf(path, sizeof path, "%s/daichi/back.png", root);
    if (!ExportImage(sheet, path)) return 2;
    UnloadImage(sheet);
    Image bad = GenImageColor(321, 180, BLUE);
    snprintf(path, sizeof path, "%s/daichi/front.png", root);
    if (!ExportImage(bad, path)) return 2;
    UnloadImage(bad);
    Image dojo = GenImageColor(320, 180, BLUE);
    snprintf(path, sizeof path, "%s/raizo/back.png", root);
    if (!ExportImage(dojo, path)) return 2;
    UnloadImage(dojo);
    arena_load_art();
    int failures = 0;
    for (int i = 0; i < ROSTER_SIZE; i++) {
        const MasterProfile *m = roster_get(i);
        if (strcmp(artNames[m->arena], m->name)) failures++;
    }
    if (!arena_has_art(ARENA_CELEIRO) || arena_has_art(ARENA_JARDIM) || art[ARENA_CELEIRO][1].id) failures++;
    if (!arena_has_art(ARENA_DOJO)) failures++;
    for (int i = 0; i < 3; i++) {
        BeginDrawing();
        ClearBackground(BLACK);
        arena_draw_back(ARENA_CELEIRO, &(ArenaCtx){.t = i * AJ_CENARIO_QUADRO});
        rlDrawRenderBatchActive();
        Image got = LoadImageFromScreen();
        Color c = GetImageColor(got, 160, 90), expected = i == 1 ? GREEN : RED;
        if (c.r != expected.r || c.g != expected.g || c.b != expected.b) failures++;
        UnloadImage(got);
        EndDrawing();
    }
    BeginDrawing();
    arena_draw_back(ARENA_DOJO, &(ArenaCtx){0});
    rlDrawRenderBatchActive();
    Image got = LoadImageFromScreen();
    Color c = GetImageColor(got, 160, 90);
    if (c.r != BLUE.r || c.g != BLUE.g || c.b != BLUE.b) failures++;
    UnloadImage(got);
    EndDrawing();
    arena_unload_art();
    if (arena_has_art(ARENA_CELEIRO)) failures++;
    CloseWindow();
    printf("cenários: PNG, dimensão inválida, dois quadros em loop, cores preservadas e liberação: %d falhas\n", failures);
    return failures ? 1 : 0;
}
