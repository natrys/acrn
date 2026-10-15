#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "therapy.h"
#include "utils.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

/* Radians per cycle, to turn the oscillator phase into an angle. */
#define RADIANS_PER_CYCLE 6.283185307179586476925286766559

/* How long each therapy tone sounds, 1/6th of a second. A batch replays the
 * full tone sequence ROUNDS_PER_BATCH times, which is 12 tones in therapy mode
 * (2s) and 15 in both mode (2.5s). */
#define TONE_SEC (1.0 / 6.0)

/* How many times per batch the full tone sequence is replayed, shuffled anew
 * before each replay. Batches are separated by a silence pause. */
#define ROUNDS_PER_BATCH 3

/* Silent pause after each batch. */
#define SILENCE_SEC 1.5

/* Frames of linear fade at the start and the end of every therapy tone.
 * Long enough to ramp the level down to silence before the tone
 * changes, which is what keeps the frequency change from clicking. */
#define FADE_FRAMES 1000

double _amplitude = TINNITUS_TONE_AMPLITUDE;

void set_therapy_tone_amplitude(double amplitude) { _amplitude = amplitude; }

static int tone_count_for_mode(TherapyMode mode) {
    switch (mode) {
    case TINNITUS_TONE_ONLY:
        return 1;
    case THERAPY_TONES_ONLY:
        return 4;
    case TINNITUS_AND_THERAPY_TONES:
        return 5;
    default:
        return 4;
    }
}

/* The tones of a batch: two derived frequencies above the tinnitus tone and
 * two below, plus the tone itself in both mode. */
static void therapy_tones_init(double freq, double *tones, TherapyMode mode) {
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

/* Amplitude envelope of a tone: it fades in over the first FADE_FRAMES frames
 * of the tone, fades out over the last FADE_FRAMES, and stays flat in between.
 * The ramp starts at 0 and ends exactly at 0, so the tone that follows starts
 * from silence. */
static double fade_gain(size_t position, size_t length) {
    if (position < FADE_FRAMES)
        return (double)position / (double)(FADE_FRAMES - 1);
    if (position + FADE_FRAMES >= length)
        return (double)(length - 1 - position) / (double)(FADE_FRAMES - 1);

    return 1.0;
}

static size_t min_size(size_t a, size_t b) { return a < b ? a : b; }

static void render_silence(float *frames, size_t frame_count, unsigned int channels) {
    size_t total = frame_count * channels;

    for (size_t i = 0; i < total; i++)
        frames[i] = 0.0f;
}

/* Renders `frame_count` frames of the current tone, applying the fade envelope.
 * The oscillator phase carries over between tones, so a tone boundary never
 * restarts the waveform at a non-zero sample. */
static void render_tone(TherapyToneEngine *engine, float *frames, size_t frame_count) {
    unsigned int channels = engine->channels;
    double phase = engine->phase;
    double amplitude = _amplitude;
    double increment = engine->frequency / (double)engine->sample_rate;

    for (size_t i = 0; i < frame_count; i++) {
        double gain = 1.0;
        float sample;

        if (engine->fades)
            gain = fade_gain(engine->tone_frames + i, engine->tone_length);

        sample = (float)(sin(phase * RADIANS_PER_CYCLE) * amplitude * gain);

        // Wrap rather than let the phase grow without bound. floor() keeps
        // this bounded even for frequencies unrepresentable at this rate.
        phase += increment;
        phase -= floor(phase);

        for (unsigned int c = 0; c < channels; c++)
            frames[i * channels + c] = sample;
    }

    engine->phase = phase;
}

/* Moves the engine on to the next tone of the batch, reshuffling the sequence
 * before every replay and falling silent once the batch is played out. */
static void therapy_engine_start_tone(TherapyToneEngine *engine) {
    if (engine->index_in_round >= engine->n_tones) {
        engine->index_in_round = 0;
        engine->rounds_done += 1;

        // Shuffled here rather than after the batch check below, so that the
        // round following a silence pause is not a replay of the one before it.
        fisher_yates_shuffle(engine->sequence, engine->n_tones);

        if (engine->rounds_done >= ROUNDS_PER_BATCH) {
            engine->rounds_done = 0;

            // The tinnitus tone is meant to be heard continuously, so unlike
            // the other modes it gets no silence gap between batches.
            if (engine->mode != TINNITUS_TONE_ONLY) {
                engine->in_silence = 1;
                engine->silence_frames = 0;
                return;
            }
        }
    }

    engine->frequency = engine->tones[engine->sequence[engine->index_in_round]];
    engine->index_in_round += 1;
    engine->tone_frames = 0;
    engine->in_silence = 0;
}

void therapy_engine_init(TherapyToneEngine *engine, double frequency, TherapyMode mode,
                         unsigned int sample_rate, unsigned int channels) {
    engine->mode = mode;
    engine->sample_rate = sample_rate;
    engine->channels = channels;
    engine->tone_length = (size_t)(sample_rate * TONE_SEC);
    engine->silence_length = (size_t)(sample_rate * SILENCE_SEC);

    engine->n_tones = tone_count_for_mode(mode);
    therapy_tones_init(frequency, engine->tones, mode);
    for (int i = 0; i < engine->n_tones; i++)
        engine->sequence[i] = i;

    engine->index_in_round = 0;
    engine->rounds_done = 0;
    engine->frequency = frequency;
    engine->phase = 0.0;

    engine->tone_frames = 0;
    engine->in_silence = 0;
    engine->silence_frames = 0;

    // The tinnitus tone is a single tone played forever, so fading its edges
    // every time a tone boundary rolls over would tremolo it at the batch rate.
    // The other modes switch tones and need the fade to hide the change.
    engine->fades = mode != TINNITUS_TONE_ONLY;

    therapy_engine_start_tone(engine);

    if (VERBOSE) {
        printf("Tones =");
        for (int i = 0; i < engine->n_tones; i++)
            printf(" %f", engine->tones[i]);
        printf("\n");
    }
}

void therapy_engine_render(TherapyToneEngine *engine, float *frames, unsigned int frame_count) {
    /* The schedule is walked in chunks that never straddle a boundary, so the
     * tones stay whole no matter how the device splits the callback. */
    unsigned int channels = engine->channels;
    size_t rendered = 0;

    while (rendered < frame_count) {
        float *out = frames + rendered * channels;
        size_t requested = (size_t)frame_count - rendered;
        size_t chunk;

        if (engine->in_silence) {
            chunk = min_size(requested, engine->silence_length - engine->silence_frames);
            render_silence(out, chunk, channels);
            engine->silence_frames += chunk;
        } else {
            chunk = min_size(requested, engine->tone_length - engine->tone_frames);
            render_tone(engine, out, chunk);
            engine->tone_frames += chunk;
        }

        rendered += chunk;

        if (engine->in_silence) {
            if (engine->silence_frames >= engine->silence_length)
                therapy_engine_start_tone(engine);
        } else if (engine->tone_frames >= engine->tone_length) {
            therapy_engine_start_tone(engine);
        }
    }
}
