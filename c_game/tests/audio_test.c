/* Integração dos arquivos da equipe com o banco de vozes e o streaming real. */
#include "../src/audio.c"

int main(void) {
    SetTraceLogLevel(LOG_WARNING);
    if (!getenv("APARA_AUDIO_DIR")) return 2; /* nunca criar fixtures junto dos arquivos reais */
    static const struct { const char *name; float duration; } files[] = {
        {"sfx/good_01.wav", .25f}, {"sfx/good_02.wav", .35f}, {"sfx/perfect.wav", 2}, {"music/daichi.wav", 1}
    };
    char path[1024];
    for (unsigned i = 0; i < sizeof files / sizeof files[0]; i++) {
        unsigned frames = (unsigned)(RATE * files[i].duration);
        short *data = calloc(frames, sizeof *data);
        if (!data) return 2;
        snprintf(path, sizeof path, "%s/%s", audio_directory(), files[i].name);
        bool ok = ExportWave((Wave){frames, RATE, 16, 1, data}, path);
        free(data);
        if (!ok) return 2;
    }
    snprintf(path, sizeof path, "%s/sfx/swing.wav", audio_directory());
    FILE *bad = fopen(path, "wb");
    if (!bad) return 2;
    fputs("invalid wave", bad);
    if (fclose(bad)) return 2;
    audio_init();
    if (!IsAudioDeviceReady()) {
        fprintf(stderr, "audio_test: dispositivo de áudio indisponível\n");
        audio_shutdown();
        return 2;
    }
    int failures = 0;
#define CHECK(c, m) do { if (!(c)) { fprintf(stderr, "FALHA: %s\n", m); failures++; } } while (0)
    CHECK(variationCount[SND_GOOD] == 1, "as duas variações WAV não foram carregadas");
    CHECK(voiceCount[SND_GOOD] == VOZES_PADRAO, "WAV perdeu a polifonia");
    CHECK(variationVoices[SND_GOOD][0][1].stream.buffer != NULL, "segunda variação não tem vozes extras");
    CHECK(variationCount[SND_PERFECT] == 0 && sounds[SND_PERFECT].stream.sampleRate > 0 &&
          fabs((double)sounds[SND_PERFECT].frameCount / sounds[SND_PERFECT].stream.sampleRate - SOM_PERFEITO_CAUDA) < 0.001,
          "cauda longa substituiu o perfeito embutido");
    CHECK(sounds[SND_SWING].frameCount > 0, "WAV corrompido eliminou o assobio embutido");
    for (int i = 0; i < 10; i++) audio_play(SND_GOOD, 1, 1);
    CHECK(variationNext[SND_GOOD] == 0 && voiceNext[SND_GOOD] == 1 && variationVoiceNext[SND_GOOD][0] == 1,
          "rodízio de variantes cortou o banco de vozes");
    for (int i = 0; i < ROSTER_SIZE; i++) {
        const MasterProfile *m = roster_get(i);
        CHECK(!strcmp(musicNames[m->arena], m->name), "música externa foi associada à arena de outro mestre");
    }
    audio_music(ARENA_CELEIRO);
    CHECK(M.target == MUSIC_SILENCE && trackTarget == ARENA_CELEIRO, "música externa toca junto da trilha sintetizada");
    for (int i = 0; i < 40; i++) audio_update(1.0f / 60);
    CHECK(trackCurrent == ARENA_CELEIRO && tracks[ARENA_CELEIRO].looping && trackGain == 1, "música externa não iniciou com loop e fade");
    audio_music(MUSIC_WIND);
    for (int i = 0; i < 40; i++) audio_update(1.0f / 60);
    CHECK(trackCurrent == -1 && M.target == MUSIC_WIND, "escolha final não voltou ao vento embutido");
    audio_shutdown();
    printf("áudio: WAVs, variações, vozes, arquivos inválidos, streaming e retorno ao vento: %d falhas\n", failures);
    return failures ? 1 : 0;
}
