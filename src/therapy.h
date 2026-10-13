#pragma once

typedef enum TherapyMode {
  TINNITUS_TONE_ONLY,
  THERAPY_TONES_ONLY,
  TINNITUS_AND_THERAPY_TONES
} TherapyMode;

void set_therapy_tone_amplitude(double amplitude);
void play_therapy_tones(ma_waveform *sineWave, int frequency, TherapyMode mode);
