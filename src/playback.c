#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <miniaudio.h>

#include "therapy.h"

#define TINNITUS_TONE_FREQUENCY 10000.0
#define TINNITUS_TONE_AMPLITUDE 0.01

#define DEVICE_FORMAT ma_format_f32
#define DEVICE_CHANNELS 2
#define DEVICE_SAMPLE_RATE 48000

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

int main(int argc, char **argv) {
    srand(time(NULL));

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = DEVICE_FORMAT;
    deviceConfig.playback.channels = DEVICE_CHANNELS;
    deviceConfig.sampleRate = DEVICE_SAMPLE_RATE;
    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = &sineWave;

    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        return -4;
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start playback device.\n");
        ma_device_uninit(&device);
        return -5;
    }

    sineWaveConfig = ma_waveform_config_init(device.playback.format, device.playback.channels,
                                             device.sampleRate, ma_waveform_type_sine,
                                             TINNITUS_TONE_AMPLITUDE, TINNITUS_TONE_FREQUENCY);

    ma_waveform_init(&sineWaveConfig, &sineWave);

    signal(SIGINT, signal_handler);

    set_therapy_tone_amplitude(TINNITUS_TONE_AMPLITUDE);
    play_therapy_tones(&sineWave, TINNITUS_TONE_FREQUENCY, TINNITUS_AND_THERAPY_TONES);

    ma_device_uninit(&device);

    (void)argc;
    (void)argv;
    return 0;
}
