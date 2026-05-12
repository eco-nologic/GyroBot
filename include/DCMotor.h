#ifndef DCMOTOR_H
#define DCMOTOR_H

#include "IMotor.h"
#include "Config.h"
#include <Arduino.h>

class DCMotor : public IMotor {
private:
    int enablePin;
    int in1Pin;
    int in2Pin;
    int encoderAPin;
    int encoderBPin;

    volatile long encoderCount = 0;
    volatile int lastEncoderA = 0;
    volatile int lastEncoderB = 0;
    long lastReportedCount = 0;
    unsigned long lastHealthCheck = 0;
    bool motorHealthy = true;

    static void handleEncoderISR(void* arg);

public:
    DCMotor(int en, int in1, int in2, int encA, int encB);
    ~DCMotor() = default;

    void begin();
    void setPwm(int pwm) override;
    void setDirection(bool forward) override;
    long getEncoderCount() override;
    void resetEncoder() override;
    bool isHealthy() override;

    // Helper to process encoder interrupts
    void processEncoderChange();
};

#endif // DCMOTOR_H
