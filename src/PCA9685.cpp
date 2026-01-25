#include "PCA9685.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cmath>

void PCA9685::writeReg(uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    if (write(file, buffer, 2) != 2) {
        std::cerr << "Failed to write to I2C device" << std::endl;
    }
}

uint8_t PCA9685::readReg(uint8_t reg) {
    if (write(file, &reg, 1) != 1) {
        std::cerr << "Failed to write register address" << std::endl;
        return 0;
    }
    uint8_t value;
    if (read(file, &value, 1) != 1) {
        std::cerr << "Failed to read from I2C device" << std::endl;
        return 0;
    }
    return value;
}

PCA9685::PCA9685(const char* i2c_device, int address) {
    file = open(i2c_device, O_RDWR);
    if (file < 0) {
        std::cerr << "Failed to open I2C device: " << i2c_device << std::endl;
        std::cerr << "Make sure I2C is enabled: sudo raspi-config" << std::endl;
        exit(1);
    }
    
    if (ioctl(file, I2C_SLAVE, address) < 0) {
        std::cerr << "Failed to set I2C address" << std::endl;
        close(file);
        exit(1);
    }
    
    reset();
    setPWMFreq(50); // 50Hz for servos
    
    std::cout << "PCA9685 initialized successfully" << std::endl;
}

PCA9685::~PCA9685() {
    if (file >= 0) {
        close(file);
    }
}

void PCA9685::reset() {
    writeReg(MODE1, 0x00);
    usleep(10000);
}

void PCA9685::setPWMFreq(float freq) {
    // Calculate prescale value
    float prescaleval = 25000000.0;  // 25MHz oscillator
    prescaleval /= 4096.0;            // 12-bit resolution
    prescaleval /= freq;
    prescaleval -= 1.0;
    
    uint8_t prescale = static_cast<uint8_t>(floor(prescaleval + 0.5));
    
    uint8_t oldmode = readReg(MODE1);
    uint8_t newmode = (oldmode & 0x7F) | 0x10; // Sleep mode
    writeReg(MODE1, newmode);
    writeReg(PRESCALE, prescale);
    writeReg(MODE1, oldmode);
    usleep(5000);
    writeReg(MODE1, oldmode | 0xa1); // Auto-increment on
}

void PCA9685::setPWM(uint8_t channel, uint16_t on, uint16_t off) {
    uint8_t reg = LED0_ON_L + 4 * channel;
    writeReg(reg, on & 0xFF);
    writeReg(reg + 1, on >> 8);
    writeReg(reg + 2, off & 0xFF);
    writeReg(reg + 3, off >> 8);
}

void PCA9685::setServoAngle(uint8_t channel, float angle) {
    // Clamp angle to 0-180
    angle = std::max(0.0f, std::min(180.0f, angle));
    
    // Map angle to pulse width
    // For MG996R: 1ms (0°) to 2ms (180°)
    // At 50Hz: one cycle = 20ms, 4096 steps
    // 1ms = 204.8 steps ≈ 205
    // 2ms = 409.6 steps ≈ 410
    uint16_t pulse = static_cast<uint16_t>(205 + (angle / 180.0) * 205);
    
    setPWM(channel, 0, pulse);
}

void PCA9685::setServoPulse(uint8_t channel, uint16_t pulse_us) {
    // Set servo by microseconds (500-2500 typical range)
    // At 50Hz: 20000us per cycle, 4096 steps
    // pulse_steps = (pulse_us / 20000) * 4096
    uint16_t pulse = static_cast<uint16_t>((pulse_us / 20000.0) * 4096);
    setPWM(channel, 0, pulse);
}
