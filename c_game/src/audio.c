/*
 * audio.c - todos os sons nascem aqui, sem arquivos.
 * Efeitos: ondas geradas uma vez no início. Trilha: sintetizador simples
 * rodando no callback da raylib (drone + sequenciador de 16 passos por cenário).
 */
#include "audio.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

#define RATE 44100
#define TAU 6.28318530718f

static Sound sounds[SND_COUNT];
/* Vozes extras dos golpes: num combo, um parry não corta a cauda do anterior. */
#define FX_VOICES 4
static Sound voices[SND_COUNT][FX_VOICES];
static int voiceCount[SND_COUNT], voiceNext[SND_COUNT];
static AudioStream stream;
static float master = 0.85f;

/* ------------------------------------------------------------------ */
/* Efeitos                                                             */
/* ------------------------------------------------------------------ */

static float nrand(void) { return (float)rand() / (float)RAND_MAX * 2 - 1; }

typedef float (*SynthFn)(float t, float dur, float *state);

/* Sala pequena (quatro pentes e dois passa-tudo, à Schroeder) para a cauda do metal. */
static void add_room(float *data, int n, float wet) {
    static const int comb[4] = {1116, 1277, 1422, 1557};
    static const int allp[2] = {556, 341};
    float *out = calloc((size_t)n, sizeof(float));
    for (int c = 0; c < 4; c++) {
        float *buf = calloc((size_t)comb[c], sizeof(float));
        float damp = 0;
        for (int i = 0, k = 0; i < n; i++, k = (k + 1) % comb[c]) {
            float y = buf[k];
            damp = y * 0.6f + damp * 0.4f;
            buf[k] = data[i] + damp * 0.74f;
            out[i] += y * 0.25f;
        }
        free(buf);
    }
    for (int a = 0; a < 2; a++) {
        float *buf = calloc((size_t)allp[a], sizeof(float));
        for (int i = 0, k = 0; i < n; i++, k = (k + 1) % allp[a]) {
            float b = buf[k];
            buf[k] = out[i] + b * 0.5f;
            out[i] = b - out[i];
        }
        free(buf);
    }
    for (int i = 0; i < n; i++) data[i] += out[i] * wet;
    free(out);
}

static Sound make_sound_room(float dur, SynthFn fn, float gain, float room) {
    int n = (int)(dur * RATE);
    float *data = malloc(sizeof(float) * (size_t)n);
    float state[8] = {0};
    float peak = 0.0001f;
    for (int i = 0; i < n; i++) data[i] = fn((float)i / RATE, dur, state);
    if (room > 0) add_room(data, n, room);
    for (int i = 0; i < n; i++)
        if (fabsf(data[i]) > peak) peak = fabsf(data[i]);
    /* Normaliza e evita estalo no fim. */
    for (int i = 0; i < n; i++) {
        float fade = fminf(1, (float)(n - i) / (RATE * 0.005f));
        data[i] = data[i] / peak * gain * fade;
    }
    Wave w = {(unsigned int)n, RATE, 32, 1, data};
    Sound s = LoadSoundFromWave(w);
    free(data);
    return s;
}

static Sound make_sound(float dur, SynthFn fn, float gain) { return make_sound_room(dur, fn, gain, 0); }

static float lp(float *st, float x, float a) { *st += (x - *st) * a; return *st; }

static float s_cue(float t, float d, float *st) {
    (void)d; (void)st;
    float env = expf(-t * 18) * fminf(1, t * 400);
    return (sinf(TAU * 880 * t) + 0.35f * sinf(TAU * 1760 * t)) * env;
}
static float s_cue_feint(float t, float d, float *st) {
    (void)d; (void)st;
    float env = expf(-t * 22) * fminf(1, t * 400);
    return (sinf(TAU * 1320 * t) + 0.4f * sinf(TAU * 2640 * t)) * env;
}
/* Choque de duas lâminas. Todas as camadas começam no mesmo instante (o som
   não tem ataque lento: o pico está no primeiro milissegundo):
     estalo   ruído agudo de poucos milissegundos, o "tchk" do contato;
     metal    os modos de vibração de uma barra (1 : 2,76 : 5,40 : 8,93) em duas
              lâminas um pouco desafinadas entre si, o que faz o brilho pulsar;
     baque    um grave que cai de tom, o peso do golpe;
     faíscas  estalinhos soltos que vão rareando;
     nota     (só no perfeito) um agudo longo com vibrato leve, que fica soando. */
static const float BAR[4] = {1.0f, 2.756f, 5.404f, 8.933f};

static float clash(float t, float *st, float fa, float fb, float decay, float thud, float sparks, float ring) {
    float n = nrand();
    float hp = n - lp(&st[0], n, 0.35f);
    float click = hp * expf(-t * 700) * 1.6f;
    static const float k[4] = {1.0f, 1.7f, 3.0f, 4.8f};
    static const float a[4] = {1.0f, 0.7f, 0.45f, 0.3f};
    float metal = 0;
    for (int i = 0; i < 4; i++)
        metal += a[i] * expf(-t * decay * k[i]) * (sinf(TAU * fa * BAR[i] * t) + 0.8f * sinf(TAU * fb * BAR[i] * t + 1.3f));
    float f = 58 + 150 * expf(-t * 40);
    st[1] += TAU * f / RATE;
    float body = sinf(st[1]) * expf(-t * 22) * thud;
    if (t < 0.2f && (float)rand() / (float)RAND_MAX < sparks * expf(-t * 16)) st[2] = 1;
    st[2] *= 0.88f;
    float spark = st[2] * hp * 0.9f;
    float tone = 0;
    if (ring > 0) {
        float vib = 1 + 0.002f * sinf(TAU * 5.5f * t);
        st[3] += TAU * 2637 * vib / RATE;
        tone = sinf(st[3]) * expf(-t * 2.4f) * (1 - expf(-t * 250)) * ring;
    }
    return click + metal * 0.5f + body + spark + tone;
}

/* Perfeito: choque cheio, faíscas e uma nota longa que fica no ar. */
static float s_perfect(float t, float d, float *st) {
    (void)d;
    return clash(t, st, 1180, 1321, 5.5f, 0.9f, 0.004f, 0.4f);
}
/* Bom: o mesmo choque, mais curto e sem a nota longa. */
static float s_good(float t, float d, float *st) {
    (void)d;
    return clash(t, st, 1264, 1418, 15.0f, 0.55f, 0.0015f, 0);
}
/* Levar golpe: batida seca de madeira, como um bokken. */
static float s_bad(float t, float d, float *st) {
    (void)d;
    float wood = sinf(TAU * 520 * t) * expf(-t * 38) + 0.7f * sinf(TAU * 830 * t) * expf(-t * 55);
    float body = sinf(TAU * (90 + 60 * expf(-t * 30)) * t) * expf(-t * 18);
    return wood * 0.8f + body + lp(&st[0], nrand(), 0.4f) * expf(-t * 120);
}
/* Postura quebrando: cerâmica rachando e um taiko grave. */
static float s_break(float t, float d, float *st) {
    (void)d;
    float crack = (nrand() - lp(&st[0], nrand(), 0.3f)) * expf(-t * 18) * (0.6f + 0.4f * sinf(t * 900));
    float taiko = sinf(TAU * (52 + 40 * expf(-t * 10)) * t) * expf(-t * 3.2f);
    return crack * 0.9f + taiko * 1.2f;
}
static float s_swing(float t, float d, float *st) {
    float x = t / d;
    float env = sinf(x * 3.14159f);
    env *= env;
    float a = 0.05f + 0.25f * x;
    return lp(&st[0], nrand(), a) * env;
}
static float s_gesture(float t, float d, float *st) {
    float x = t / d;
    float env = sinf(x * 3.14159f) * expf(-x * 2);
    return (nrand() - lp(&st[0], nrand(), 0.1f)) * env * 0.6f;
}
static float s_ui(float t, float d, float *st) {
    (void)d; (void)st;
    return sinf(TAU * 660 * t) * expf(-t * 60) + 0.4f * sinf(TAU * 1320 * t) * expf(-t * 80);
}
static float s_type(float t, float d, float *st) {
    (void)d; (void)st;
    return sinf(TAU * 520 * t) * expf(-t * 140);
}
static float s_gem(float t, float d, float *st) {
    (void)d; (void)st;
    static const float notes[] = {1046.5f, 1318.5f, 1568, 2093};
    float v = 0;
    for (int i = 0; i < 4; i++) {
        float on = t - i * 0.09f;
        if (on > 0) v += sinf(TAU * notes[i] * on) * expf(-on * 4) * (1 - i * 0.12f);
    }
    return v;
}
static float s_seal(float t, float d, float *st) {
    (void)d;
    float boom = sinf(TAU * (40 + 60 * expf(-t * 6)) * t) * expf(-t * 2.5f);
    float chord = (sinf(TAU * 146.8f * t) + sinf(TAU * 220 * t) + sinf(TAU * 293.7f * t)) * fminf(1, t * 3) * expf(-t * 1.2f) * 0.3f;
    return boom + chord + lp(&st[0], nrand(), 0.05f) * expf(-t * 3) * 2;
}
static float s_drum(float t, float d, float *st) {
    (void)d;
    float body = sinf(TAU * (70 + 80 * expf(-t * 30)) * t) * expf(-t * 8);
    return body + lp(&st[0], nrand(), 0.15f) * expf(-t * 35) * 2;
}
static float s_thunder(float t, float d, float *st) {
    (void)d;
    float crack = nrand() * expf(-t * 20);
    float roll = lp(&st[0], nrand(), 0.01f) * (0.5f + 0.5f * sinf(t * 9)) * expf(-t * 1.2f) * 12;
    return crack * 0.5f + roll;
}

/* Vitória: a lâmina volta à bainha (um "tchk" curto) e três notas de koto subindo, bem suaves. */
static float s_victory(float t, float d, float *st) {
    (void)d;
    float sheath = (nrand() - lp(&st[0], nrand(), 0.3f)) * expf(-t * 60) * 0.5f;
    static const float notes[] = {392.0f, 523.25f, 659.25f};
    float v = 0;
    for (int i = 0; i < 3; i++) {
        float on = t - 0.18f - i * 0.16f;
        if (on <= 0) continue;
        float f = notes[i];
        v += (sinf(TAU * f * on) + 0.35f * sinf(TAU * f * 2 * on) * expf(-on * 6)) * expf(-on * 2.6f) * (0.9f - i * 0.1f);
    }
    return sheath + v * 0.55f;
}

/* Derrota: um baque abafado e uma nota grave que desce devagar, com sopro. */
static float s_defeat(float t, float d, float *st) {
    (void)d;
    float thump = sinf(TAU * (48 + 30 * expf(-t * 12)) * t) * expf(-t * 7);
    float f = 220 * powf(0.72f, fminf(1, t / 1.2f));
    st[1] += TAU * f / RATE;
    float breath = lp(&st[0], nrand(), 0.04f) * 2;
    float tone = (sinf(st[1]) + breath * 0.3f) * fminf(1, t * 4) * expf(-t * 1.6f);
    return thump * 0.9f + tone * 0.45f;
}

/* Espada cravando no chão: baque seco e curto. */
static float s_thud(float t, float d, float *st) {
    (void)d;
    return sinf(TAU * (90 + 120 * expf(-t * 40)) * t) * expf(-t * 22) + lp(&st[0], nrand(), 0.2f) * expf(-t * 50);
}

/* ------------------------------------------------------------------ */
/* Trilha ambiente                                                      */
/* ------------------------------------------------------------------ */

typedef enum { V_OFF, V_TONE, V_KICK, V_NOISE, V_BELL, V_PLUCK } VoiceType;

typedef struct {
    VoiceType type;
    float freq, phase, phase2, amp, decay, t, cutoff, lpState, vib;
} Voice;

#define VOICES 24

static struct {
    volatile int target;
    volatile float intensity, duck;
    int style;
    float gain;                 /* fade da troca de trilha */
    double sample;
    int step;
    float stepPos;
    Voice v[VOICES];
    float dronePhase[4];
    float noiseLp, noiseLp2, windLfo;
    unsigned int seed;
} M;

static float mrand(void) {
    M.seed = M.seed * 1664525u + 1013904223u;
    return (float)(M.seed >> 8) / 16777216.0f;
}

static void voice(VoiceType type, float freq, float amp, float decay, float cutoff) {
    int slot = 0;
    float quiet = 1e9f;
    for (int i = 0; i < VOICES; i++) {
        if (M.v[i].type == V_OFF) { slot = i; break; }
        float level = M.v[i].amp * expf(-M.v[i].t * M.v[i].decay);
        if (level < quiet) { quiet = level; slot = i; }
    }
    Voice *v = &M.v[slot];
    memset(v, 0, sizeof *v);
    v->type = type;
    v->freq = freq;
    v->amp = amp;
    v->decay = decay;
    v->cutoff = cutoff;
    v->vib = type == V_TONE ? 5.5f : 0;
}

static float note(int semitone) { return 220.0f * powf(2, semitone / 12.0f); }

typedef struct {
    float bpm;
    float drone[3];     /* frequências do drone (0 = sem) */
    float droneAmp;
    float noiseAmp, noiseCut; /* vento, água, multidão */
} Style;

static const Style STYLES[MUSIC_SILENCE + 1] = {
    /* DOJO      */ {72, {110, 164.8f, 0}, 0.05f, 0.015f, 0.02f},
    /* SERRA     */ {62, {98, 146.8f, 196}, 0.04f, 0.05f, 0.012f},
    /* CELEIRO   */ {80, {98, 146.8f, 196}, 0.045f, 0.01f, 0.03f},
    /* TELHADOS  */ {90, {73.4f, 110, 138.6f}, 0.05f, 0.03f, 0.015f},
    /* PORTO     */ {100, {82.4f, 123.5f, 0}, 0.04f, 0.03f, 0.03f},
    /* SALAO     */ {96, {130.8f, 196, 261.6f}, 0.035f, 0.0f, 0.02f},
    /* PONTE     */ {84, {61.7f, 92.5f, 0}, 0.04f, 0.06f, 0.05f},
    /* CACHOEIRA */ {60, {65.4f, 98, 0}, 0.04f, 0.16f, 0.12f},
    /* BAMBUZAL  */ {64, {92.5f, 138.6f, 0}, 0.035f, 0.02f, 0.015f},
    /* FORJA     */ {84, {46.2f, 69.3f, 92.5f}, 0.06f, 0.035f, 0.01f},
    /* JARDIM    */ {70, {174.6f, 261.6f, 349.2f}, 0.035f, 0.005f, 0.05f},
    /* CIDADELA  */ {96, {73.4f, 87.3f, 110}, 0.06f, 0.03f, 0.012f},
    /* TEMPLO    */ {66, {87.3f, 130.8f, 174.6f}, 0.04f, 0.02f, 0.03f},
    /* LORE      */ {60, {110, 146.8f, 220}, 0.04f, 0.008f, 0.02f},
    /* TITLE     */ {70, {82.4f, 123.5f, 164.8f}, 0.045f, 0.01f, 0.02f},
    /* SILENCE   */ {60, {0, 0, 0}, 0, 0, 0.02f},
};

/* Escalas: pentatônica menor a partir da raiz de cada trilha. */
static const int PENTA[] = {0, 3, 5, 7, 10, 12, 15, 17};

static void sequencer_step(int style, int step, float intensity) {
    int bar16 = step % 16;
    switch (style) {
        case 0: /* dojo: koto esparso */
            if (bar16 % 4 == 0 && mrand() < 0.45f) voice(V_PLUCK, note(PENTA[(int)(mrand() * 8)]) , 0.10f, 4, 0);
            break;
        case 1: /* serra: shakuhachi esparso e vento */
            if (bar16 == 0 && mrand() < 0.5f) voice(V_TONE, note(PENTA[(int)(mrand() * 6)] + 12), 0.05f, 0.7f, 0);
            if (bar16 == 8 && mrand() < 0.3f) voice(V_PLUCK, note(PENTA[(int)(mrand() * 5)]), 0.07f, 2.5f, 0);
            break;
        case 2: /* celeiro: grilos e violão */
            if (mrand() < 0.35f) voice(V_BELL, 4200 + mrand() * 600, 0.012f, 30, 0);
            if (bar16 % 8 == 0) voice(V_PLUCK, note(PENTA[(int)(mrand() * 5)] - 5), 0.10f, 3, 0);
            break;
        case 3: /* telhados na chuva: pulso grave e o sino do templo ao longe */
            if (bar16 % 8 == 0) voice(V_TONE, 73.4f, 0.09f, 2.5f, 0);
            if (bar16 == 6 && step % 32 == 6) voice(V_BELL, 220, 0.04f, 1.5f, 0);
            break;
        case 4: /* porto: tambores taiko */
            if (bar16 == 0 || bar16 == 6 || bar16 == 10) voice(V_KICK, 70, 0.45f, 6, 0);
            if (bar16 == 12 || bar16 == 14) voice(V_KICK, 95, 0.3f, 9, 0);
            if (bar16 % 4 == 2) voice(V_NOISE, 0, 0.03f, 50, 0.4f);
            break;
        case 5: /* salão do castelo: koto em arpejo e o trovão do taiko */
            if (bar16 % 2 == 0) voice(V_PLUCK, note(PENTA[(step / 2) % 8]), 0.05f, 4, 0);
            if (bar16 == 0) voice(V_KICK, 52, 0.4f, 4, 0);
            if (bar16 == 12 && mrand() < 0.5f) voice(V_NOISE, 0, 0.05f, 4, 0.2f);
            break;
        case 6: /* ponte: rajadas de vento, a corda rangendo e a flauta */
            if (bar16 == 0 || bar16 == 9) voice(V_NOISE, 0, 0.06f, 3, 0.25f);
            if (bar16 == 5 && mrand() < 0.6f) voice(V_TONE, 92.5f + mrand() * 10, 0.05f, 6, 0);
            if (bar16 == 0 && mrand() < 0.4f) voice(V_TONE, note(PENTA[(int)(mrand() * 6)] + 12), 0.05f, 0.8f, 0);
            break;
        case 7: /* cachoeira: quase só água */
            if (bar16 == 0 && mrand() < 0.3f) voice(V_TONE, 196, 0.03f, 1, 0);
            break;
        case 8: /* bambuzal: grilos e flauta */
            if (mrand() < 0.3f) voice(V_BELL, 3800 + mrand() * 800, 0.01f, 35, 0);
            if (bar16 == 0 && mrand() < 0.5f) voice(V_TONE, note(PENTA[(int)(mrand() * 8)] + 12), 0.05f, 1.2f, 0);
            break;
        case 9: /* forja: bigorna */
            if (bar16 == 0 || bar16 == 3) voice(V_BELL, 1650, 0.08f, 9, 0);
            if (bar16 == 8) voice(V_KICK, 45, 0.3f, 5, 0);
            break;
        case 10: /* jardim de pedras: sininhos de vento */
            if (bar16 % 4 == 0 && mrand() < 0.7f) voice(V_BELL, note(PENTA[(int)(mrand() * 8)] + 24), 0.035f, 2, 0);
            break;
        case 11: /* cidadela: tambores que crescem com os selos */
            if (bar16 == 0 || bar16 == 8) voice(V_KICK, 55, 0.5f, 5, 0);
            if (intensity > 0.3f && (bar16 == 4 || bar16 == 12)) voice(V_KICK, 80, 0.35f, 8, 0);
            if (intensity > 0.6f && bar16 % 2 == 1) voice(V_NOISE, 0, 0.04f, 40, 0.7f);
            if (bar16 == 0 && step % 64 == 0) voice(V_TONE, 146.8f, 0.07f, 0.8f, 0);
            break;
        case 12: /* templo do tigre: sino grave e taiko espaçado */
            if (bar16 == 0) voice(V_KICK, 58, 0.4f, 5, 0);
            if (bar16 == 10) voice(V_KICK, 74, 0.25f, 8, 0);
            if (bar16 == 0 && step % 32 == 0) voice(V_BELL, 220, 0.06f, 1.2f, 0);
            if (bar16 % 8 == 4 && mrand() < 0.4f) voice(V_PLUCK, note(PENTA[(int)(mrand() * 6)] + 12), 0.05f, 3, 0);
            break;
        case MUSIC_LORE:
        case MUSIC_TITLE:
            if (bar16 % 8 == 0 && mrand() < 0.7f) voice(V_PLUCK, note(PENTA[(int)(mrand() * 8)]), 0.08f, 2.5f, 0);
            if (bar16 == 0 && mrand() < 0.3f) voice(V_TONE, note(PENTA[(int)(mrand() * 5)] + 12), 0.03f, 0.8f, 0);
            break;
        default: break;
    }
}

static float render_voice(Voice *v) {
    float dt = 1.0f / RATE;
    float env = v->amp * expf(-v->t * v->decay) * fminf(1, v->t * 300);
    float out = 0;
    switch (v->type) {
        case V_TONE: {
            float f = v->freq * (1 + 0.004f * sinf(TAU * v->vib * v->t));
            v->phase += TAU * f * dt;
            out = sinf(v->phase) + 0.25f * sinf(v->phase * 2) + 0.1f * sinf(v->phase * 3);
            env *= fminf(1, v->t * 8);
            break;
        }
        case V_KICK: {
            float f = v->freq * (1 + 2.5f * expf(-v->t * 40));
            v->phase += TAU * f * dt;
            out = sinf(v->phase);
            break;
        }
        case V_NOISE: {
            float n = mrand() * 2 - 1;
            v->lpState += (n - v->lpState) * v->cutoff;
            out = v->cutoff > 0.6f ? n - v->lpState * 0.8f : v->lpState * 2;
            break;
        }
        case V_BELL:
            v->phase += TAU * v->freq * dt;
            v->phase2 += TAU * v->freq * 2.76f * dt;
            out = sinf(v->phase) + 0.4f * sinf(v->phase2);
            break;
        case V_PLUCK:
            v->phase += TAU * v->freq * dt;
            out = sinf(v->phase) + 0.5f * sinf(v->phase * 2) * expf(-v->t * 8) + 0.2f * sinf(v->phase * 3) * expf(-v->t * 12);
            break;
        default: break;
    }
    v->t += dt;
    if (env < 0.0005f && v->t > 0.05f) v->type = V_OFF;
    if (v->phase > 1000) v->phase = fmodf(v->phase, TAU);
    if (v->phase2 > 1000) v->phase2 = fmodf(v->phase2, TAU);
    return out * env;
}

static void music_callback(void *buffer, unsigned int frames) {
    float *out = buffer;
    for (unsigned int i = 0; i < frames; i++) {
        /* Troca de trilha: fade out, troca, fade in. */
        if (M.style != M.target) {
            M.gain -= 1.0f / (RATE * 0.6f);
            if (M.gain <= 0) {
                M.gain = 0;
                M.style = M.target;
                for (int k = 0; k < VOICES; k++) M.v[k].type = V_OFF;
            }
        } else if (M.gain < 1) {
            M.gain += 1.0f / (RATE * 1.2f);
        }
        const Style *st = &STYLES[M.style <= MUSIC_SILENCE ? M.style : MUSIC_SILENCE];
        float intensity = M.intensity;

        M.stepPos += st->bpm * 4 / 60.0f / RATE;
        if (M.stepPos >= 1) {
            M.stepPos -= 1;
            M.step++;
            sequencer_step(M.style, M.step, intensity);
        }

        float s = 0;
        float t = (float)(M.sample / RATE);
        for (int k = 0; k < 3; k++) {
            if (st->drone[k] <= 0) continue;
            M.dronePhase[k] += TAU * st->drone[k] * (1 + 0.002f * sinf(t * (0.3f + k * 0.17f))) / RATE;
            if (M.dronePhase[k] > TAU) M.dronePhase[k] -= TAU;
            s += (sinf(M.dronePhase[k]) + 0.2f * sinf(M.dronePhase[k] * 2)) * st->droneAmp * (0.7f + 0.3f * sinf(t * 0.4f + k));
        }
        if (M.style == 11) s *= 1 + intensity * 0.8f;

        float n = mrand() * 2 - 1;
        M.noiseLp += (n - M.noiseLp) * st->noiseCut;
        M.noiseLp2 += (M.noiseLp - M.noiseLp2) * st->noiseCut;
        M.windLfo = 0.6f + 0.4f * sinf(t * 0.23f) * sinf(t * 0.07f + 1);
        s += M.noiseLp2 * st->noiseAmp * 6 * M.windLfo;

        for (int k = 0; k < VOICES; k++) if (M.v[k].type != V_OFF) s += render_voice(&M.v[k]);

        float duck = 1 - M.duck * 0.7f;
        s *= M.gain * duck * master;
        out[i] = tanhf(s * 1.2f) * 0.8f;
        M.sample += 1;
    }
}

/* ------------------------------------------------------------------ */

void audio_init(void) {
    InitAudioDevice();
    sounds[SND_CUE] = make_sound(0.35f, s_cue, 0.55f);
    sounds[SND_CUE_FEINT] = make_sound(0.3f, s_cue_feint, 0.55f);
    sounds[SND_PERFECT] = make_sound_room(1.6f, s_perfect, 0.95f, 0.22f);
    sounds[SND_GOOD] = make_sound_room(0.5f, s_good, 0.75f, 0.06f);
    sounds[SND_BAD] = make_sound(0.6f, s_bad, 0.95f);
    sounds[SND_BREAK] = make_sound(2.5f, s_break, 0.95f);
    sounds[SND_SWING] = make_sound(0.22f, s_swing, 0.55f);
    sounds[SND_GESTURE] = make_sound(0.16f, s_gesture, 0.4f);
    sounds[SND_UI] = make_sound(0.12f, s_ui, 0.35f);
    sounds[SND_TYPE] = make_sound(0.04f, s_type, 0.12f);
    sounds[SND_GEM] = make_sound(1.4f, s_gem, 0.6f);
    sounds[SND_SEAL] = make_sound(2.8f, s_seal, 0.95f);
    sounds[SND_DRUM] = make_sound(0.6f, s_drum, 0.8f);
    sounds[SND_THUNDER] = make_sound(3.0f, s_thunder, 0.8f);
    sounds[SND_VICTORY] = make_sound(2.2f, s_victory, 0.6f);
    sounds[SND_DEFEAT] = make_sound(2.0f, s_defeat, 0.6f);
    sounds[SND_THUD] = make_sound(0.3f, s_thud, 0.5f);
#if defined(RAYLIB_VERSION_MAJOR) && RAYLIB_VERSION_MAJOR >= 5
    static const SoundId poly[] = {SND_PERFECT, SND_GOOD, SND_BAD, SND_SWING};
    /* sem placa de som o Sound vem vazio, e a raylib não confere isso no alias */
    for (size_t i = 0; i < sizeof poly / sizeof poly[0] && IsAudioDeviceReady(); i++) {
        SoundId id = poly[i];
        if (!sounds[id].stream.buffer) continue;
        voices[id][0] = sounds[id];
        for (int k = 1; k < FX_VOICES; k++) voices[id][k] = LoadSoundAlias(sounds[id]);
        voiceCount[id] = FX_VOICES;
    }
#endif

    memset(&M, 0, sizeof M);
    M.seed = 12345;
    M.target = M.style = MUSIC_SILENCE;
    SetAudioStreamBufferSizeDefault(1024);
    stream = LoadAudioStream(RATE, 32, 1);
    SetAudioStreamCallback(stream, music_callback);
    PlayAudioStream(stream);
}

void audio_shutdown(void) {
    StopAudioStream(stream);
    UnloadAudioStream(stream);
#if defined(RAYLIB_VERSION_MAJOR) && RAYLIB_VERSION_MAJOR >= 5
    for (int i = 0; i < SND_COUNT; i++)
        for (int k = 1; k < voiceCount[i]; k++) UnloadSoundAlias(voices[i][k]);
#endif
    for (int i = 0; i < SND_COUNT; i++) UnloadSound(sounds[i]);
    CloseAudioDevice();
}

void audio_play(SoundId id, float volume, float pitch) {
    if (volume <= 0.001f) return;
    Sound s = sounds[id];
    if (voiceCount[id]) {
        s = voices[id][voiceNext[id]];
        voiceNext[id] = (voiceNext[id] + 1) % voiceCount[id];
    }
    /* o bom varia um pouco de tom para não soar repetido numa sequência */
    if (id == SND_GOOD) pitch *= 0.97f + 0.06f * (float)rand() / (float)RAND_MAX;
    SetSoundVolume(s, volume * master);
    SetSoundPitch(s, pitch);
    PlaySound(s);
}

void audio_music(int style) { M.target = style; }
void audio_music_intensity(float x) { M.intensity = x; }
void audio_music_duck(float x) { M.duck = x < 0 ? 0 : (x > 1 ? 1 : x); }
void audio_set_master(float v) { master = v; }
