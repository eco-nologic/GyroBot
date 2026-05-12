#ifndef IMOTOR_H
#define IMOTOR_H

#include <Arduino.h>

class IMotor {
public:
    virtual ~IMotor() = default;

    // Initialize motor hardware
    virtual void begin() = 0;

    // Set motor speed in PWM (0-255)
    virtual void setPwm(int pwm) = 0;

    // Set motor direction: true = forward, false = backward
    virtual void setDirection(bool forward) = 0;

    // Get current encoder count
    virtual long getEncoderCount() = 0;

    // Reset encoder count to zero
    virtual void resetEncoder() = 0;

    // Check if motor is healthy (encoder feedback present)
    virtual bool isHealthy() = 0;
};

#endif // IMOTOR_H
