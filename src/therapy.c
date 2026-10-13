#include <math.h>
#include <stdio.h>

#include <miniaudio.h>

#include "therapy.h"
#include "utils.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

double _amplitude = 0.01;

void set_therapy_tone_amplitude(double amplitude) { _amplitude = amplitude; }

void therapy_tones_init(double freq, double *tones, TherapyMode mode) {
    if (mode == TINNITUS_TONE_ONLY) {
        tones[0] = freq;
        return;
    }

    tones[0] = floor(freq * 0.773 - 44.5);
    tones[1] = floor(freq * 0.903 - 21.5);
    tones[2] = floor(freq * 1.09 + 52);
    tones[3] = floor(freq * 1.395 + 26.5);

    if (mode == TINNITUS_AND_THERAPY_TONES)
        tones[4] = freq;
}

void play_therapy_tones(ma_waveform *sineWave, double frequency, TherapyMode mode) {
    int n_tones;
    switch (mode) {
    case TINNITUS_TONE_ONLY:
        n_tones = 1;
        break;
    case THERAPY_TONES_ONLY:
        n_tones = 4;
        break;
    case TINNITUS_AND_THERAPY_TONES:
        n_tones = 5;
        break;
    default:
        n_tones = 4;
    }

    double frequencies[5] = {0.0};
    therapy_tones_init(frequency, frequencies, mode);

    if (VERBOSE) {
        printf("Tones =");
        for (int i = 0; i < n_tones; i++)
            printf(" %f", frequencies[i]);
        printf("\n");
    }

    int sequence[5] = {0, 1, 2, 3, 4};
    struct timespec ts;

    while (1) {
        ts.tv_sec = 0;
        ts.tv_nsec = 200000000;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < n_tones; j++) {
                ma_waveform_set_frequency(sineWave, frequencies[sequence[j]]);
                nanosleep(&ts, NULL);
            }
            fisher_yates_shuffle(sequence, n_tones);
        }
        // The tinnitus tone is meant to be heard continuously, so it gets no
        // silence gap between batches.
        if (mode != TINNITUS_TONE_ONLY) {
            ts.tv_sec = 1;
            ts.tv_nsec = 0;
            ma_waveform_set_amplitude(sineWave, 0.0);
            ma_waveform_set_frequency(sineWave, 0.0);
            nanosleep(&ts, NULL);
            ma_waveform_set_amplitude(sineWave, _amplitude);
        }
    }
}
