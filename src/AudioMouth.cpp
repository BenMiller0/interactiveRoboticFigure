#include "AudioMouth.h"
#include <iostream>
#include <unistd.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>

// Simple C++11 clamp replacement
template <typename T>
T clamp(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

// -------- Pitch control --------
// 1.15–1.35 is usable
static constexpr double PITCH_FACTOR = 1.25;

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
}

void AudioMouth::stop() {
    if (!running) return;
    running = false;
    if (audioThread.joinable())
        audioThread.join();
    pwm->setServoPulse(channel, SERVO_MIN_PULSE);
}

void AudioMouth::moveServoBasedOnAmplitude(short* buffer, int size) {
    double sum = 0.0;
    for (int i = 0; i < size; i++)
        sum += std::abs(buffer[i]);

    double avgAmplitude = sum / size;
    if (avgAmplitude < SOUND_MIN_THRESHOLD + DEAD_ZONE)
        return;

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

// -------- SAFE pitch-up (decimation) --------
static void pitchUpBuffer(short* buffer, int frames) {
    for (int i = 0; i < frames; i++) {
        int src = static_cast<int>(i * PITCH_FACTOR);
        if (src >= frames)
            src = frames - 1;
        buffer[i] = buffer[src];
    }
}

void AudioMouth::audioProcessingLoop() {
    snd_pcm_t* captureHandle = nullptr;
    snd_pcm_t* playbackHandle1 = nullptr;
    snd_pcm_t* playbackHandle2 = nullptr;
    snd_pcm_hw_params_t* hwParams = nullptr;
    short* buffer = nullptr;
    int err;

    // --- Capture ---
    err = snd_pcm_open(&captureHandle, DEVICE_INPUT, SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) return;

    // --- Playback 1 ---
    err = snd_pcm_open(&playbackHandle1, DEVICE_OUTPUT1, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        snd_pcm_close(captureHandle);
        return;
    }

    // --- Playback 2 ---
    err = snd_pcm_open(&playbackHandle2, DEVICE_OUTPUT2, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        snd_pcm_close(captureHandle);
        snd_pcm_close(playbackHandle1);
        return;
    }

    snd_pcm_hw_params_alloca(&hwParams);

    // Configure capture
    snd_pcm_hw_params_any(captureHandle, hwParams);
    snd_pcm_hw_params_set_access(captureHandle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(captureHandle, hwParams, SND_PCM_FORMAT_S16_LE);
    unsigned int rate = SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(captureHandle, hwParams, &rate, nullptr);
    snd_pcm_hw_params_set_channels(captureHandle, hwParams, CHANNELS);
    snd_pcm_hw_params(captureHandle, hwParams);
    snd_pcm_prepare(captureHandle);

    // Configure playback 1
    snd_pcm_hw_params_any(playbackHandle1, hwParams);
    snd_pcm_hw_params_set_access(playbackHandle1, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(playbackHandle1, hwParams, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate_near(playbackHandle1, hwParams, &rate, nullptr);
    snd_pcm_hw_params_set_channels(playbackHandle1, hwParams, CHANNELS);
    snd_pcm_hw_params(playbackHandle1, hwParams);
    snd_pcm_prepare(playbackHandle1);

    // Configure playback 2
    snd_pcm_hw_params_any(playbackHandle2, hwParams);
    snd_pcm_hw_params_set_access(playbackHandle2, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(playbackHandle2, hwParams, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate_near(playbackHandle2, hwParams, &rate, nullptr);
    snd_pcm_hw_params_set_channels(playbackHandle2, hwParams, CHANNELS);
    snd_pcm_hw_params(playbackHandle2, hwParams);
    snd_pcm_prepare(playbackHandle2);

    // Allocate buffer
    buffer = static_cast<short*>(malloc(FRAMES * CHANNELS * sizeof(short)));
    if (!buffer) return;

    while (running) {
        err = snd_pcm_readi(captureHandle, buffer, FRAMES);
        if (err != FRAMES) {
            snd_pcm_prepare(captureHandle);
            continue;
        }

        moveServoBasedOnAmplitude(buffer, FRAMES);
        pitchUpBuffer(buffer, FRAMES);

        // Write to both outputs
        err = snd_pcm_writei(playbackHandle1, buffer, FRAMES);
        if (err == -EPIPE) snd_pcm_prepare(playbackHandle1);

        err = snd_pcm_writei(playbackHandle2, buffer, FRAMES);
        if (err == -EPIPE) snd_pcm_prepare(playbackHandle2);
    }

    free(buffer);
    snd_pcm_close(captureHandle);
    snd_pcm_close(playbackHandle1);
    snd_pcm_close(playbackHandle2);
}
