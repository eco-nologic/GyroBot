#ifndef VIRTUALSTEPPER_H
#define VIRTUALSTEPPER_H

#include "IMotor.h"
#include "Config.h"

// Software-only motor simulation (no hardware drivers)
class VirtualStepper : public IMotor {
private:
    float speedMmS = 0;
    bool direction = true;
    long encoderCount = 0;

public:
    VirtualStepper();
    ~VirtualStepper() = default;

    void begin();
    void setPwm(int pwm) override;
    void setDirection(bool forward) override;
    long getEncoderCount() override;
    void resetEncoder() override;
    bool isHealthy() override { return true; }

    // Simulate motor motion
    void update(float deltaTime);
};

#endif // VIRTUALSTEPPER_H
