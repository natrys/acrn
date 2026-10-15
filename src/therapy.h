#pragma once

#include <stddef.h>

/* Playback level of every tone the engine synthesizes. */
#define TINNITUS_TONE_AMPLITUDE 0.01

/* Number of tones a batch can hold, the tinnitus tone included. */
#define THERAPY_TONE_CAPACITY 5

typedef enum TherapyMode {
    TINNITUS_TONE_ONLY,
    THERAPY_TONES_ONLY,
    TINNITUS_AND_THERAPY_TONES
} TherapyMode;

/* State of the tone playback. It is driven by the number of frames rendered
 * rather than by wall clock time, so that everything it produces stays locked
 * to the audio device. It lives for the lifetime of the playback. */
typedef struct TherapyToneEngine {
    TherapyMode mode;
    unsigned int sample_rate;
    unsigned int channels;

    /* Frames of a single tone, of the silence pause between batches, and how
     * far into the current one we are. */
    size_t tone_length;
    size_t tone_frames;
    size_t silence_length;
    size_t silence_frames;

    /* The tones of a batch and the order they are played in, reshuffled before
     * every replay of the sequence. */
    double tones[THERAPY_TONE_CAPACITY];
    int sequence[THERAPY_TONE_CAPACITY];
    int n_tones;
    int index_in_round;
    int rounds_done;

    /* The oscillator, advanced continuously across the whole batch so that a
     * tone boundary never restarts the waveform at a non-zero sample. */
    double frequency;
    double phase;

    int in_silence;
    int fades;
} TherapyToneEngine;

void set_therapy_tone_amplitude(double amplitude);

void therapy_engine_init(TherapyToneEngine *engine, double frequency, TherapyMode mode,
                         unsigned int sample_rate, unsigned int channels);

/* Renders the next `frame_count` interleaved frames of the tone schedule as
 * f32 PCM, which is what the audio device callback asks for. */
void therapy_engine_render(TherapyToneEngine *engine, float *frames, unsigned int frame_count);
