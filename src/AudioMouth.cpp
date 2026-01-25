#include "AudioMouth.h"
#include <iostream>
#include <unistd.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>

template <typename T>
T clamp(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

AudioMouth::AudioMouth(PCA9685* pwmController, uint8_t servoChannel)
    : pwm(pwmController),
      channel(servoChannel),
      running(false),
      prevServoPulse(SERVO_MIN_PULSE) {

    pwm->setServoPulse(channel, SERVO_MIN_PULSE);
}

AudioMouth::~AudioMouth() {
    stop();
}

void AudioMouth::start() {
    if (running) return;

    running = true;
    audioThread = std::thread(&AudioMouth::audioProcessingLoop, this);
    std::cout << "AudioMouth started on channel "
              << static_cast<int>(channel) << std::endl;
}

void AudioMouth::stop() {
    if (!running) return;

    running = false;
    if (audioThread.joinable()) {
        audioThread.join();
    }

    pwm->setServoPulse(channel, SERVO_MIN_PULSE);
    std::cout << "AudioMouth stopped" << std::endl;
}

void AudioMouth::moveServoBasedOnAmplitude(short* buffer, int size) {
    double sum = 0.0;
    for (int i = 0; i < size; i++) {
        sum += std::abs(buffer[i]);
    }

    double avgAmplitude = sum / size;
    if (avgAmplitude < SOUND_MIN_THRESHOLD + DEAD_ZONE) {
        return;
    }

    double normalized = avgAmplitude / 32768.0;
    uint16_t targetPulse =
        SERVO_MIN_PULSE +
        normalized * (SERVO_MAX_PULSE - SERVO_MIN_PULSE);

    targetPulse = clamp(targetPulse,
                             SERVO_MIN_PULSE,
                             SERVO_MAX_PULSE);

    double delta = targetPulse - prevServoPulse;
    delta = clamp(delta, -MAX_SERVO_SPEED, MAX_SERVO_SPEED);

    double smoothed =
        prevServoPulse + delta * SMOOTHING_FACTOR;

    smoothed = clamp(smoothed,
                          (double)SERVO_MIN_PULSE,
                          (double)SERVO_MAX_PULSE);

    if (std::fabs(smoothed - prevServoPulse) >
        SERVO_MOVEMENT_THRESHOLD) {

        prevServoPulse = static_cast<uint16_t>(smoothed);
        pwm->setServoPulse(channel, prevServoPulse);
        usleep(SERVO_UPDATE_DELAY_US);
    }
}

void AudioMouth::audioProcessingLoop() {
    snd_pcm_t* captureHandle = nullptr;
    snd_pcm_t* playbackHandle = nullptr;
    snd_pcm_hw_params_t* hwParams = nullptr;
    short* buffer = nullptr;
    int err;

    /* ---------- Open capture ---------- */
    err = snd_pcm_open(&captureHandle,
                       DEVICE_INPUT,
                       SND_PCM_STREAM_CAPTURE,
                       0);
    if (err < 0) {
        std::cerr << "Cannot open capture device: "
                  << snd_strerror(err) << std::endl;
        running = false;
        return;
    }

    /* ---------- Open playback ---------- */
    err = snd_pcm_open(&playbackHandle,
                       DEVICE_OUTPUT,
                       SND_PCM_STREAM_PLAYBACK,
                       0);
    if (err < 0) {
        std::cerr << "Cannot open playback device: "
                  << snd_strerror(err) << std::endl;
        snd_pcm_close(captureHandle);
        running = false;
        return;
    }

    snd_pcm_hw_params_alloca(&hwParams);

    /* ---------- Configure capture ---------- */
    snd_pcm_hw_params_any(captureHandle, hwParams);
    snd_pcm_hw_params_set_access(
        captureHandle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(
        captureHandle, hwParams, SND_PCM_FORMAT_S16_LE);

    unsigned int rate = SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(
        captureHandle, hwParams, &rate, nullptr);
    snd_pcm_hw_params_set_channels(
        captureHandle, hwParams, CHANNELS);
    snd_pcm_hw_params(captureHandle, hwParams);
    snd_pcm_prepare(captureHandle);

    /* ---------- Configure playback ---------- */
    snd_pcm_hw_params_any(playbackHandle, hwParams);
    snd_pcm_hw_params_set_access(
        playbackHandle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(
        playbackHandle, hwParams, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate_near(
        playbackHandle, hwParams, &rate, nullptr);
    snd_pcm_hw_params_set_channels(
        playbackHandle, hwParams, CHANNELS);
    snd_pcm_hw_params(playbackHandle, hwParams);
    snd_pcm_prepare(playbackHandle);

    buffer = static_cast<short*>(
        malloc(FRAMES * CHANNELS * sizeof(short)));

    if (!buffer) {
        std::cerr << "Audio buffer allocation failed\n";
        snd_pcm_close(captureHandle);
        snd_pcm_close(playbackHandle);
        running = false;
        return;
    }

    std::cout << "Audio capture + playback running\n";

    /* ---------- Main loop ---------- */
    while (running) {
        err = snd_pcm_readi(captureHandle, buffer, FRAMES);
        if (err < 0) {
            snd_pcm_prepare(captureHandle);
            continue;
        }

        moveServoBasedOnAmplitude(buffer, FRAMES);

        err = snd_pcm_writei(playbackHandle, buffer, FRAMES);
        if (err == -EPIPE) {
            snd_pcm_prepare(playbackHandle);
        }
        else if (err < 0) {
            std::cerr << "Playback error: "
                      << snd_strerror(err) << std::endl;
        }
    }

    /* ---------- Cleanup ---------- */
    free(buffer);
    snd_pcm_close(captureHandle);
    snd_pcm_close(playbackHandle);
}
