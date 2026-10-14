#pragma once

/* Playback level, shared by the therapy logic and the front-end's device setup. */
#define TINNITUS_TONE_AMPLITUDE 0.01

typedef enum TherapyMode {
    TINNITUS_TONE_ONLY,
    THERAPY_TONES_ONLY,
    TINNITUS_AND_THERAPY_TONES
} TherapyMode;

void set_therapy_tone_amplitude(double amplitude);
void play_therapy_tones(ma_waveform *sineWave, double frequency, TherapyMode mode);
