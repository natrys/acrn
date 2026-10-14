#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <miniaudio.h>

#include "therapy.h"

#define DEVICE_FORMAT ma_format_f32
#define DEVICE_CHANNELS 2
#define DEVICE_SAMPLE_RATE 48000

#define MIN_AUDIBLE_FREQUENCY 20.0
#define MAX_AUDIBLE_FREQUENCY 20000.0

ma_waveform sineWave;
ma_waveform_config sineWaveConfig;

ma_device_config deviceConfig;
ma_device device;

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) {
    ma_waveform *pSineWave;
    pSineWave = (ma_waveform *)pDevice->pUserData;
    ma_waveform_read_pcm_frames(pSineWave, pOutput, frameCount, NULL);
    (void)pInput;
}

void signal_handler(int sig) {
    ma_device_uninit(&device);
    printf("Exiting..\n");
    exit(0);
    (void)sig;
}

static void usage(FILE *stream, const char *prog) {
    fprintf(stream,
            "Usage: %s [options] <frequency>\n"
            "\n"
            "Plays the tinnitus tone at <frequency> Hz together with the therapy tones\n"
            "derived from it, in randomized batches followed by silence.\n"
            "\n"
            "Options:\n"
            "  -m, --mode <mode>  tones to play: tinnitus, therapy or both (default: both)\n"
            "  -h, --help         show this help and exit\n",
            prog);
}

static int parse_mode(const char *name, TherapyMode *mode) {
    if (strcmp(name, "tinnitus") == 0) {
        *mode = TINNITUS_TONE_ONLY;
    } else if (strcmp(name, "therapy") == 0) {
        *mode = THERAPY_TONES_ONLY;
    } else if (strcmp(name, "both") == 0) {
        *mode = TINNITUS_AND_THERAPY_TONES;
    } else {
        return -1;
    }
    return 0;
}

static int parse_frequency(const char *arg, double *frequency) {
    char *end = NULL;
    double value = strtod(arg, &end);
    if (end == arg || *end != '\0' || !isfinite(value) || value <= 0.0)
        return -1;
    *frequency = value;
    return 0;
}

int main(int argc, char **argv) {
    const char *prog = argv[0];
    double frequency = 0.0;
    TherapyMode mode = TINNITUS_AND_THERAPY_TONES;
    int have_frequency = 0;

    srand(time(NULL));

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(stdout, prog);
            return 0;
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--mode") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "%s: option %s requires an argument\n", prog, argv[i]);
                return -1;
            }
            if (parse_mode(argv[++i], &mode) != 0) {
                fprintf(stderr, "%s: invalid mode '%s' (expected tinnitus, therapy or both)\n",
                        prog, argv[i]);
                return -1;
            }
        } else if (argv[i][0] == '-' && argv[i][1] != '\0') {
            fprintf(stderr, "%s: unknown option '%s'\n", prog, argv[i]);
            usage(stderr, prog);
            return -1;
        } else if (parse_frequency(argv[i], &frequency) != 0) {
            fprintf(stderr, "%s: invalid frequency '%s'\n", prog, argv[i]);
            return -1;
        } else {
            have_frequency = 1;
        }
    }

    if (!have_frequency) {
        fprintf(stderr, "%s: missing tinnitus frequency\n", prog);
        usage(stderr, prog);
        return -1;
    }

    if (frequency < MIN_AUDIBLE_FREQUENCY || frequency > MAX_AUDIBLE_FREQUENCY)
        fprintf(stderr, "warning: %g Hz is outside the audible range (%g-%g Hz)\n", frequency,
                MIN_AUDIBLE_FREQUENCY, MAX_AUDIBLE_FREQUENCY);

    sineWaveConfig =
        ma_waveform_config_init(DEVICE_FORMAT, DEVICE_CHANNELS, DEVICE_SAMPLE_RATE,
                                ma_waveform_type_sine, TINNITUS_TONE_AMPLITUDE, frequency);
    if (ma_waveform_init(&sineWaveConfig, &sineWave) != MA_SUCCESS) {
        fprintf(stderr, "Failed to initialize waveform.\n");
        return -3;
    }

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = DEVICE_FORMAT;
    deviceConfig.playback.channels = DEVICE_CHANNELS;
    deviceConfig.sampleRate = DEVICE_SAMPLE_RATE;
    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = &sineWave;

    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
        fprintf(stderr, "Failed to open playback device.\n");
        return -4;
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        fprintf(stderr, "Failed to start playback device.\n");
        ma_device_uninit(&device);
        return -5;
    }

    signal(SIGINT, signal_handler);

    play_therapy_tones(&sineWave, frequency, mode);

    ma_device_uninit(&device);

    return 0;
}
