#include "Audio.h"
#include <cstdlib>
#include <cmath>
#include <deque>

static void rubberBandEffect(short* buffer, int frames, double ampNorm) {
    static constexpr double EFFECT_AMOUNT = 1.0;
    static std::deque<short> history;
    static double readPos    = 0.0;
    static double playSpeed  = 1.0;
    static double targetSpeed = 1.0;

    const double MIN_ACTIVE  = 0.02 / std::max(0.0001, EFFECT_AMOUNT);
    const double sensitivity = 1.5  * EFFECT_AMOUNT;
    const double min_speed   = 0.6  / std::max(0.1, EFFECT_AMOUNT);
    const double max_speed   = 1.0  + (0.8 * EFFECT_AMOUNT);
    const size_t MAX_HISTORY = frames * static_cast<size_t>(std::max(1.0, 8.0 * EFFECT_AMOUNT));

    targetSpeed = (ampNorm < MIN_ACTIVE) ? 1.0 : 1.0 + (ampNorm - MIN_ACTIVE) * sensitivity;
    if (targetSpeed < min_speed) targetSpeed = min_speed;
    if (targetSpeed > max_speed) targetSpeed = max_speed;
    playSpeed += (targetSpeed - playSpeed) * 0.12;

    for (int i = 0; i < frames; ++i) history.push_back(buffer[i]);
    while (history.size() > MAX_HISTORY) history.pop_front();
    if (history.size() < static_cast<size_t>(frames) + 2) return;

    if (readPos < 0.0) readPos = 0.0;
    if (readPos > static_cast<double>(history.size() - 1))
        readPos = static_cast<double>(history.size() - 1 - frames);

    double rp = readPos;
    for (int i = 0; i < frames; ++i) {
        size_t i0  = static_cast<size_t>(rp);
        size_t i1  = (i0 + 1 < history.size()) ? i0 + 1 : i0;
        double out = history[i0] + (history[i1] - history[i0]) * (rp - i0);
        if (out >  32767.0) out =  32767.0;
        if (out < -32768.0) out = -32768.0;
        buffer[i] = static_cast<short>(out);
        rp += playSpeed;
        if (rp >= static_cast<double>(history.size() - 1)) { rp = static_cast<double>(history.size() - 1); break; }
    }

    readPos = rp;
    while (readPos >= 1.0 && history.size() > static_cast<size_t>(frames) * 2) {
        history.pop_front();
        readPos -= 1.0;
    }
}

void Audio::pause() {
    if (!running) return;
    running = false;
    if (audioThread.joinable())
        audioThread.join();
}

void Audio::resume() {
    if (running) return;
    running = true;
    audioThread = std::thread(&Audio::loop, this);
}

bool Audio::isPaused() const {
    return !running;
}

Audio::Audio(FrameCallback callback)
    : running(true), frameCallback(callback) {
    audioThread = std::thread(&Audio::loop, this);
}

Audio::~Audio() {
    stop();
}

void Audio::stop() {
    if (!running) return;
    running = false;
    if (audioThread.joinable())
        audioThread.join();
}

void Audio::loop() {
    snd_pcm_t* captureHandle   = nullptr;
    snd_pcm_t* playbackHandle1 = nullptr;
    snd_pcm_t* playbackHandle2 = nullptr;
    snd_pcm_hw_params_t* hwParams = nullptr;
    int err;

    err = snd_pcm_open(&captureHandle,   DEVICE_INPUT,   SND_PCM_STREAM_CAPTURE,  0);
    if (err < 0) return;
    err = snd_pcm_open(&playbackHandle1, DEVICE_OUTPUT1, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) { snd_pcm_close(captureHandle); return; }
    err = snd_pcm_open(&playbackHandle2, DEVICE_OUTPUT2, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) { snd_pcm_close(captureHandle); snd_pcm_close(playbackHandle1); return; }

    snd_pcm_hw_params_alloca(&hwParams);
    unsigned int rate = SAMPLE_RATE;

    auto configure = [&](snd_pcm_t* handle) {
        snd_pcm_hw_params_any(handle, hwParams);
        snd_pcm_hw_params_set_access(handle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(handle, hwParams, SND_PCM_FORMAT_S16_LE);
        snd_pcm_hw_params_set_rate_near(handle, hwParams, &rate, nullptr);
        snd_pcm_hw_params_set_channels(handle, hwParams, CHANNELS);
        snd_pcm_hw_params(handle, hwParams);
        snd_pcm_prepare(handle);
    };

    configure(captureHandle);
    configure(playbackHandle1);
    configure(playbackHandle2);

    short* buffer = static_cast<short*>(malloc(FRAMES * CHANNELS * sizeof(short)));
    if (!buffer) return;

    while (running) {
        err = snd_pcm_readi(captureHandle, buffer, FRAMES);
        if (err != FRAMES) { snd_pcm_prepare(captureHandle); continue; }

        double sumA = 0.0;
        for (int i = 0; i < FRAMES; ++i) sumA += std::abs(buffer[i]);
        double norm = (sumA / FRAMES) / 32768.0;

        frameCallback(buffer, FRAMES);
        rubberBandEffect(buffer, FRAMES, norm);

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