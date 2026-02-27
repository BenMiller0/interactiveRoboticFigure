#include "../ai/AIVoice.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <cstring>
#include <sstream>

AIVoice::AIVoice()
    : childPid(-1), state(AIState::IDLE), running(false), speakingAmplitude(850) {}

AIVoice::~AIVoice() { stop(); }

void AIVoice::start() {
    pipe(pipeToCpp);
    pipe(pipeToChild);

    childPid = fork();
    if (childPid == 0) {
        dup2(pipeToChild[0], STDIN_FILENO);
        dup2(pipeToCpp[1],   STDOUT_FILENO);

        // Redirect all stderr noise to log file
        int logFd = open("/tmp/taro_ai.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (logFd >= 0) dup2(logFd, STDERR_FILENO);

        close(pipeToChild[1]);
        close(pipeToCpp[0]);

        execl("/usr/bin/python3", "python3",
              "src/ai/taro_ai.py", nullptr);
        _exit(1);
    }

    close(pipeToChild[0]);
    close(pipeToCpp[1]);

    std::cerr << "AIVoice: python process launched, pid=" << childPid << std::endl;

    running = true;
    state   = AIState::IDLE;
    readerThread = std::thread(&AIVoice::readLoop, this);
}

void AIVoice::stop() {
    if (!running) return;
    running = false;
    sendToChild("QUIT\n");
    if (childPid > 0) {
        waitpid(childPid, nullptr, 0);
        childPid = -1;
    }
    close(pipeToChild[1]);
    close(pipeToCpp[0]);
    if (readerThread.joinable())
        readerThread.join();
    state = AIState::IDLE;
}

void AIVoice::triggerListen() {
    if (state == AIState::READY)
        sendToChild("LISTEN\n");
}

void AIVoice::triggerAutoOn()  { sendToChild("AUTO_ON\n");  }
void AIVoice::triggerAutoOff() { sendToChild("AUTO_OFF\n"); }

AIState AIVoice::getState() const  { return state.load(); }
bool AIVoice::isActive() const     { return running && state != AIState::IDLE; }
std::string AIVoice::getLastTranscript() const { return lastTranscript; }
uint16_t AIVoice::getSpeakingAmplitude() const { return speakingAmplitude.load(); }

void AIVoice::sendToChild(const std::string& msg) {
    write(pipeToChild[1], msg.c_str(), msg.size());
}

void AIVoice::readLoop() {
    char buf[512];
    std::string line;
    while (running) {
        ssize_t n = read(pipeToCpp[0], buf, sizeof(buf) - 1);
        if (n <= 0) break;
        buf[n] = '\0';
        line += buf;

        size_t pos;
        while ((pos = line.find('\n')) != std::string::npos) {
            std::string msg = line.substr(0, pos);
            line = line.substr(pos + 1);

            if      (msg == "READY")         { state = AIState::READY; speakingAmplitude = 850; }
            else if (msg == "LISTENING")     { state = AIState::LISTENING; }
            else if (msg == "PROCESSING")    { state = AIState::PROCESSING; }
            else if (msg == "SPEAKING")      { state = AIState::SPEAKING; }
            else if (msg == "DONE_SPEAKING") { state = AIState::READY; speakingAmplitude = 850; }
            else if (msg.substr(0, 11) == "TRANSCRIPT:") {
                lastTranscript = msg.substr(11);
            }
            else if (msg.size() > 4 && msg.substr(0, 4) == "AMP:") {
                int amp = std::stoi(msg.substr(4));
                // Scale 0-32768 to servo range 850-1300
                int pulse = 850 + (amp * 450) / 32768;
                if (pulse > 1300) pulse = 1300;
                speakingAmplitude = static_cast<uint16_t>(pulse);
            }
        }
    }
}