#pragma once
#include <string>
#include <thread>
#include <atomic>

enum class AIState {
    IDLE,
    READY,
    LISTENING,
    PROCESSING,
    SPEAKING
};

class AIVoice {
public:
    AIVoice();
    ~AIVoice();

    void start();
    void stop();
    void triggerListen();
    void triggerAutoOn();
    void triggerAutoOff();

    AIState getState() const;
    bool isActive() const;
    std::string getLastTranscript() const;
    uint16_t getSpeakingAmplitude() const;

private:
    int pipeToCpp[2];
    int pipeToChild[2];
    pid_t childPid;

    std::atomic<AIState> state;
    std::atomic<bool> running;
    std::atomic<uint16_t> speakingAmplitude;
    std::string lastTranscript;
    std::thread readerThread;

    void readLoop();
    void sendToChild(const std::string& msg);
};