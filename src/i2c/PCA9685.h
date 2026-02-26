#pragma once

#include <cstdint>

#define PCA9685_ADDRESS 0x40
#define MODE1 0x00
#define PRESCALE 0xFE
#define LED0_ON_L 0x06

class PCA9685 {
private:
    int file;
    
    void writeReg(uint8_t reg, uint8_t value);
    uint8_t readReg(uint8_t reg);
    
public:
    PCA9685(const char* i2c_device = "/dev/i2c-1", int address = PCA9685_ADDRESS);
    ~PCA9685();
    
    void reset();
    void setPWMFreq(float freq);
    void setPWM(uint8_t channel, uint16_t on, uint16_t off);
    void setServoAngle(uint8_t channel, float angle);
    void setServoPulse(uint8_t channel, uint16_t pulse_us);
};

