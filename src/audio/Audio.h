#pragma once
#include <alsa/asoundlib.h>
#include <atomic>
#include <thread>
#include <functional>

class Audio {
public:
    // Callback called with each captured buffer: (buffer, frames)
    using FrameCallback = std::function<void(short*, int)>;

    Audio(FrameCallback callback);
    ~Audio();
    void stop();

private:
    static constexpr int SAMPLE_RATE = 48000;
    static constexpr int CHANNELS    = 1;
    static constexpr int FRAMES      = 1024;

    const char* DEVICE_INPUT   = "plughw:CARD=Device,DEV=0";
    const char* DEVICE_OUTPUT1 = "plughw:CARD=UACDemoV10,DEV=0";
    const char* DEVICE_OUTPUT2 = "plughw:CARD=Device_1,DEV=0";

    std::atomic<bool> running;
    std::thread audioThread;
    FrameCallback frameCallback;

    void loop();
};