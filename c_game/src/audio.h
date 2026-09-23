/*
 * audio.h - efeitos sintetizados e trilha ambiente gerada em tempo real.
 */
#ifndef APARA_AUDIO_H
#define APARA_AUDIO_H

typedef enum {
    SND_CUE, SND_CUE_FEINT, SND_PERFECT, SND_GOOD, SND_BAD, SND_BREAK, SND_SWING, SND_GESTURE,
    SND_UI, SND_TYPE, SND_GEM, SND_SEAL, SND_DRUM, SND_THUNDER, SND_VICTORY, SND_DEFEAT, SND_THUD, SND_COUNT
} SoundId;

/* Trilhas: uma por cenário (ArenaId) e duas extras. */
#define MUSIC_LORE 13
#define MUSIC_TITLE 14
#define MUSIC_SILENCE 15

void audio_init(void);
void audio_shutdown(void);
void audio_play(SoundId id, float volume, float pitch);
void audio_music(int style);            /* troca com fade */
void audio_music_intensity(float x);    /* 0..1: selos do BIG BOSS, tensão */
void audio_music_duck(float x);         /* 0..1: abafa a trilha (hitstop, falas) */
void audio_set_master(float v);

#endif
