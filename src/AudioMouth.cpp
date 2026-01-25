#include "AudioMouth.h"
#include <iostream>
#include <unistd.h>
#include <cmath>
#include <cstdlib>

// Define static constexpr members
constexpr int AudioMouth::SAMPLE_RATE;
constexpr int AudioMouth::CHANNELS;
constexpr int AudioMouth::FRAMES;
constexpr uint16_t AudioMouth::SERVO_MIN_PULSE;
constexpr uint16_t AudioMouth::SERVO_MAX_PULSE;
constexpr int AudioMouth::SOUND_MIN_THRESHOLD;
constexpr double AudioMouth::SMOOTHING_FACTOR;
constexpr int AudioMouth::DEAD_ZONE;
constexpr double AudioMouth::SERVO_MOVEMENT_THRESHOLD;
constexpr int AudioMouth::SERVO_UPDATE_DELAY_US;
constexpr double AudioMouth::MAX_SERVO_SPEED;

AudioMouth::AudioMouth(PCA9685* pwmController, uint8_t servoChannel)
    : pwm(pwmController), channel(servoChannel), running(false) {
    prevServoPulse = SERVO_MIN_PULSE;
    // Set servo to closed position initially
    pwm->setServoPulse(channel, SERVO_MIN_PULSE);
}

AudioMouth::~AudioMouth() {
    stop();
}

void AudioMouth::start() {
    if (running) return;
    
    running = true;
    audioThread = std::thread(&AudioMouth::audioProcessingLoop, this);
    std::cout << "Audio mouth control started on channel " << (int)channel << std::endl;
}

void AudioMouth::stop() {
    if (!running) return;
    
    running = false;
    if (audioThread.joinable()) {
        audioThread.join();
    }
    
    // Return to closed position
    pwm->setServoPulse(channel, SERVO_MIN_PULSE);
    std::cout << "Audio mouth control stopped" << std::endl;
}

void AudioMouth::moveServoBasedOnAmplitude(short* buffer, int size) {
    double sum = 0.0;
    for (int i = 0; i < size; i++) {
        sum += abs(buffer[i]);
    }
    double avgAmplitude = sum / size;
    
    // Ignore quiet sounds
    if (avgAmplitude < SOUND_MIN_THRESHOLD + DEAD_ZONE) {
        return;
    }
    
    // Calculate target pulse width based on amplitude
    double normalizedAmplitude = avgAmplitude / 32768.0;
    uint16_t targetPulse = SERVO_MIN_PULSE + 
                          (normalizedAmplitude * (SERVO_MAX_PULSE - SERVO_MIN_PULSE));
    
    // Clamp to safe range
    targetPulse = std::max(std::min(targetPulse, SERVO_MAX_PULSE), SERVO_MIN_PULSE);
    
    // Smooth movement with speed limiting
    double pulseChange = targetPulse - prevServoPulse;
    pulseChange = std::max(std::min(pulseChange, MAX_SERVO_SPEED), -MAX_SERVO_SPEED);
    double smoothedPulse = prevServoPulse + pulseChange * SMOOTHING_FACTOR;
    
    // Final clamp
    smoothedPulse = std::max(std::min(smoothedPulse, (double)SERVO_MAX_PULSE), 
                             (double)SERVO_MIN_PULSE);
    
    // Only move if change is significant
    if (fabs(smoothedPulse - prevServoPulse) > SERVO_MOVEMENT_THRESHOLD) {
        prevServoPulse = smoothedPulse;
        pwm->setServoPulse(channel, (uint16_t)smoothedPulse);
        usleep(SERVO_UPDATE_DELAY_US);
    }
}

void AudioMouth::audioProcessingLoop() {
    int err;
    snd_pcm_t* captureHandle;
    snd_pcm_hw_params_t* hwParams;
    short* buffer;
    
    // Open audio device
    if ((err = snd_pcm_open(&captureHandle, DEVICE_INPUT, 
                            SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        std::cerr << "Cannot open input device " << DEVICE_INPUT 
                  << ": " << snd_strerror(err) << std::endl;
        running = false;
        return;
    }
    
    // Configure audio parameters
    snd_pcm_hw_params_alloca(&hwParams);
    snd_pcm_hw_params_any(captureHandle, hwParams);
    snd_pcm_hw_params_set_access(captureHandle, hwParams, 
                                  SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(captureHandle, hwParams, 
                                  SND_PCM_FORMAT_S16_LE);
    unsigned int rate = SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(captureHandle, hwParams, &rate, 0);
    snd_pcm_hw_params_set_channels(captureHandle, hwParams, CHANNELS);
    snd_pcm_hw_params(captureHandle, hwParams);
    
    buffer = (short*)malloc(FRAMES * CHANNELS * sizeof(short));
    
    std::cout << "Audio capture started. Listening for audio input..." << std::endl;
    
    while (running) {
        if ((err = snd_pcm_readi(captureHandle, buffer, FRAMES)) < 0) {
            std::cerr << "Read error: " << snd_strerror(err) << std::endl;
            snd_pcm_prepare(captureHandle);
            continue;
        }
        moveServoBasedOnAmplitude(buffer, FRAMES);
    }
    
    free(buffer);
    snd_pcm_close(captureHandle);
}
