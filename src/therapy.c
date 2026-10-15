#include <math.h>
#include <stdio.h>

#include <miniaudio.h>

#include "therapy.h"
#include "utils.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

/* How long each therapy tone sounds, 1/6th of a second. A batch replays the
 * full tone sequence ROUNDS_PER_BATCH times, which is 12 tones in therapy mode
 * (2s) and 15 in both mode (2.5s). */
#define TONE_DURATION_SEC 0
#define TONE_DURATION_NS 166666667 /* round(1e9 / 6) */

/* How many times per batch the full tone sequence is replayed, shuffled anew
 * before each replay. Batches are separated by a silence pause. */
#define ROUNDS_PER_BATCH 3

/* Silent pause after each batch. */
#define SILENCE_SEC 1
#define SILENCE_NS 500000000

double _amplitude = TINNITUS_TONE_AMPLITUDE;

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
        ts.tv_sec = TONE_DURATION_SEC;
        ts.tv_nsec = TONE_DURATION_NS;
        for (int i = 0; i < ROUNDS_PER_BATCH; i++) {
            for (int j = 0; j < n_tones; j++) {
                ma_waveform_set_frequency(sineWave, frequencies[sequence[j]]);
                nanosleep(&ts, NULL);
            }
            fisher_yates_shuffle(sequence, n_tones);
        }
        // The tinnitus tone is meant to be heard continuously, so it gets no
        // silence gap between batches.
        if (mode != TINNITUS_TONE_ONLY) {
            ts.tv_sec = SILENCE_SEC;
            ts.tv_nsec = SILENCE_NS;
            ma_waveform_set_amplitude(sineWave, 0.0);
            nanosleep(&ts, NULL);
            // Frequency must come back before amplitude: at frequency 0 the
            // phase is frozen, so raising amplitude first emits a DC blip.
            ma_waveform_set_frequency(sineWave, frequencies[sequence[0]]);
            ma_waveform_set_amplitude(sineWave, _amplitude);
        }
    }
}
