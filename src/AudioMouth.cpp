#include "AudioMouth.h"
#include <iostream>
#include <unistd.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <deque>

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

AudioMouth::AudioMouth(PCA9685* pwmController)
    : pwm(pwmController),
      running(false),
      prevServoPulse(SERVO_MIN_PULSE) {
    pwm->setServoPulse(MOUTH_SERVO_CHANNEL, SERVO_MIN_PULSE);
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
    pwm->setServoPulse(MOUTH_SERVO_CHANNEL, SERVO_MIN_PULSE);
}

void AudioMouth::moveServoBasedOnAmplitude(short* buffer, int size) {
    double sum = 0.0;
    for (int i = 0; i < size; i++)
        sum += std::abs(buffer[i]);

    double avgAmplitude = sum / size;

    // Reduce the threshold for quieter sounds
    if (avgAmplitude < SOUND_MIN_THRESHOLD)
        return;

    // Normalize and scale sensitivity
    double normalized = (avgAmplitude / 32768.0) * 2.0; // double sensitivity
    if (normalized > 1.0) normalized = 1.0;

    uint16_t targetPulse =
        SERVO_MIN_PULSE +
        normalized * (SERVO_MAX_PULSE - SERVO_MIN_PULSE);

    // Clamp target pulse within servo range
    targetPulse = clamp(targetPulse, SERVO_MIN_PULSE, SERVO_MAX_PULSE);

    // Limit how fast the servo can move
    double delta = targetPulse - prevServoPulse;
    delta = clamp(delta, -MAX_SERVO_SPEED, MAX_SERVO_SPEED);

    // Smooth movement
    double smoothed = prevServoPulse + delta * SMOOTHING_FACTOR;
    smoothed = clamp(smoothed, (double)SERVO_MIN_PULSE, (double)SERVO_MAX_PULSE);

    if (std::fabs(smoothed - prevServoPulse) > SERVO_MOVEMENT_THRESHOLD) {
        prevServoPulse = static_cast<uint16_t>(smoothed);
        pwm->setServoPulse(MOUTH_SERVO_CHANNEL, prevServoPulse);
        usleep(SERVO_UPDATE_DELAY_US);
    }
}

// -------- SAFE pitch-up (decimation) --------
// Rubber-band-like effect: a simple variable-speed resampler with a short
// history buffer. Playback speed is driven by the normalized amplitude
// (0..1) and smoothed over time to create a stretchy "rubber band" sound.
static void rubberBandEffect(short* buffer, int frames, double ampNorm) {
    // Single tuning knob: 0 = off, 1 = default, >1 = stronger effect
    static constexpr double EFFECT_AMOUNT = 1.0;

    static std::deque<short> history;
    static double readPos = 0.0;
    static double playSpeed = 1.0;
    static double targetSpeed = 1.0;

    // Scale internal parameters from EFFECT_AMOUNT so you can tune the
    // whole effect by changing one variable above.
    const double MIN_ACTIVE_BASE = 0.02; // normalized
    const double SENS_BASE = 1.5;
    const double MIN_SPEED_BASE = 0.6;
    const double MAX_SPEED_BASE = 1.8;
    const size_t HISTORY_MULT_BASE = 8;

    const double MIN_ACTIVE = MIN_ACTIVE_BASE / std::max(0.0001, EFFECT_AMOUNT);
    const double sensitivity = SENS_BASE * EFFECT_AMOUNT;
    const double min_speed = MIN_SPEED_BASE / std::max(0.1, EFFECT_AMOUNT);
    const double max_speed = 1.0 + (MAX_SPEED_BASE - 1.0) * EFFECT_AMOUNT;
    const size_t MAX_HISTORY = frames * static_cast<size_t>(std::max(1.0, HISTORY_MULT_BASE * EFFECT_AMOUNT));

    // Compute target speed from amplitude
    if (ampNorm < MIN_ACTIVE) {
        targetSpeed = 1.0;
    } else {
        targetSpeed = 1.0 + (ampNorm - MIN_ACTIVE) * sensitivity;
    }
    // clamp
    if (targetSpeed < min_speed) targetSpeed = min_speed;
    if (targetSpeed > max_speed) targetSpeed = max_speed;

    // smooth speed changes
    const double SPEED_SMOOTH = 0.12;
    playSpeed += (targetSpeed - playSpeed) * SPEED_SMOOTH;

    // Append incoming samples to history
    for (int i = 0; i < frames; ++i) history.push_back(buffer[i]);
    // Trim history if it grows too large
    while (history.size() > MAX_HISTORY) history.pop_front();

    // Need enough history to read; otherwise output passthrough
    if (history.size() < static_cast<size_t>(frames) + 2) {
        return;
    }

    // Ensure readPos points into history
    if (readPos < 0.0) readPos = 0.0;
    if (readPos > static_cast<double>(history.size() - 1))
        readPos = static_cast<double>(history.size() - 1 - frames);

    // Resample from history into buffer using linear interpolation
    double rp = readPos;
    for (int i = 0; i < frames; ++i) {
        size_t i0 = static_cast<size_t>(std::floor(rp));
        size_t i1 = (i0 + 1 < history.size()) ? i0 + 1 : i0;
        double frac = rp - static_cast<double>(i0);
        double s0 = static_cast<double>(history[i0]);
        double s1 = static_cast<double>(history[i1]);
        double out = s0 + (s1 - s0) * frac;
        // clamp to int16 range
        if (out > 32767.0) out = 32767.0;
        if (out < -32768.0) out = -32768.0;
        buffer[i] = static_cast<short>(out);
        rp += playSpeed;
        // avoid reading past available history
        if (rp >= static_cast<double>(history.size() - 1)) {
            rp = static_cast<double>(history.size() - 1);
            break;
        }
    }

    // Advance readPos and trim consumed history to keep memory bounded
    readPos = rp;
    while (readPos >= 1.0 && history.size() > static_cast<size_t>(frames) * 2) {
        history.pop_front();
        readPos -= 1.0;
    }
}

void AudioMouth::audioProcessingLoop() {
    std::cerr << "Audio thread started" << std::endl;

    snd_pcm_t* captureHandle = nullptr;
    snd_pcm_t* playbackHandle1 = nullptr;
    snd_pcm_t* playbackHandle2 = nullptr;
    snd_pcm_hw_params_t* hwParams = nullptr;
    short* buffer = nullptr;
    int err;

    // --- Capture ---
    err = snd_pcm_open(&captureHandle, DEVICE_INPUT, SND_PCM_STREAM_CAPTURE, 0);
    std::cerr << "Capture: " << snd_strerror(err) << std::endl;
    if (err < 0) return;
    

    // --- Playback 1 ---
    err = snd_pcm_open(&playbackHandle1, DEVICE_OUTPUT1, SND_PCM_STREAM_PLAYBACK, 0);
    std::cerr << "Playback1: " << snd_strerror(err) << std::endl;
    if (err < 0) { snd_pcm_close(captureHandle); return;}

    // --- Playback 2 ---
    err = snd_pcm_open(&playbackHandle2, DEVICE_OUTPUT2, SND_PCM_STREAM_PLAYBACK, 0);
    std::cerr << "Playback2: " << snd_strerror(err) << std::endl;
    if (err < 0) {
        snd_pcm_close(captureHandle);
        snd_pcm_close(playbackHandle1);
        return;
    }

    std::cerr << "All devices opened, entering loop" << std::endl;

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

        // compute average amplitude for both servo and effect control
        double sumA = 0.0;
        for (int i = 0; i < FRAMES; ++i) sumA += std::abs(buffer[i]);
        double avgA = sumA / FRAMES;
        double norm = avgA / 32768.0;

        moveServoBasedOnAmplitude(buffer, FRAMES);
        rubberBandEffect(buffer, FRAMES, norm);

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
