#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// FIRMWARE & VERSION
// ============================================================================
constexpr char FirmwareVersion[] = "0.1.1";

// ============================================================================
// PHYSICAL ROBOT PARAMETERS
// ============================================================================
constexpr float WheelDiameterMm = 90.0f;      // Drive wheel diameter
constexpr float WheelBaseMm = 83.0f;          // Distance between wheel axles (track width)
constexpr float PenOffsetMm = 130.0f;         // Distance from axle center to pen tip (front)
constexpr int StepsPerRev = 1070;             // Encoder steps per wheel revolution
constexpr float MaxLinearSpeedMmS = 120.0f;   // Max forward/backward speed
constexpr float MaxAngularSpeedRadS = 1.6f;   // Max rotation speed

// ============================================================================
// MOTOR CONTROL - LEFT MOTOR (DC motor with encoder)
// ============================================================================
constexpr int PinMotorLeftIn1 = 17;   // Direction control (H-bridge input 1)
constexpr int PinMotorLeftIn2 = 16;   // Direction control (H-bridge input 2)
constexpr int PinMotorLeftEn  = 4;    // PWM enable (speed control, 0-255)

// ============================================================================
// MOTOR CONTROL - RIGHT MOTOR (DC motor with encoder)
// ============================================================================
constexpr int PinMotorRightIn1 = 19;  // Direction control (H-bridge input 1)
constexpr int PinMotorRightIn2 = 18;  // Direction control (H-bridge input 2)
constexpr int PinMotorRightEn  = 23;  // PWM enable (speed control, 0-255)

// ============================================================================
// ENCODER FEEDBACK - QUADRATURE A/B CHANNELS
// ============================================================================
// Left encoder
constexpr int PinEncoderLeftA = 32;   // Channel A (primary pulse)
constexpr int PinEncoderLeftB = 33;   // Channel B (quadrature, 90° phase shift)

// Right encoder
constexpr int PinEncoderRightA = 27;  // Channel A (primary pulse)
constexpr int PinEncoderRightB = 14;  // Channel B (quadrature, 90° phase shift)

// ============================================================================
// I2C BUS (IMU Communication)
// ============================================================================
constexpr int PinI2cSda = 21;         // Serial data line
constexpr int PinI2cScl = 22;         // Serial clock line
constexpr int I2cFrequency = 400000;  // 400 kHz standard I2C

// IMU address auto-detection
// 0x1E = MPU9250, 0x6B = BNO055
// Set to 0 for auto-detect, or specific address to force
constexpr uint8_t ImuAddress = 0;

// ============================================================================
// BATTERY MONITORING (ADC)
// ============================================================================
constexpr int PinBatteryAdc = 0;           // ADC input pin (GPIO0, may not work on all ESP32s)
constexpr float BatteryDividerRatio = 2.0f; // Voltage divider ratio (3.3V ref scaled by 2x)
constexpr float BatteryLowVoltage = 6.6f;   // Low battery threshold (4S LiPo minimum safe)

// ============================================================================
// PEN CONTROL
// ============================================================================
constexpr int PinPenServo = 25;        // Servo pin for pen lift (PWM)
constexpr int PenDownAngle = 90;       // Servo angle when pen is down (contact)
constexpr int PenUpAngle = 0;          // Servo angle when pen is up (lifted)

// ============================================================================
// WIFI ACCESS POINT
// ============================================================================
constexpr char WifiSsid[] = "RobotWifi";
constexpr char WifiPassword[] = "penbot123";
constexpr char WifiApIp[] = "192.168.4.1";
constexpr uint16_t WebsocketPort = 80;

// ============================================================================
// SYSTEM CONFIGURATION
// ============================================================================
enum class SystemRunMode {
    Sim,   // Virtual motors (software simulation)
    Real   // Physical hardware with actual sensors
};

#ifdef MODE_REAL
constexpr SystemRunMode SYSTEM_MODE = SystemRunMode::Real;
#else
constexpr SystemRunMode SYSTEM_MODE = SystemRunMode::Sim;
#endif

// ============================================================================
// MOTION CONTROL CONSTANTS
// ============================================================================
constexpr float PidKp = 0.5f;          // Proportional gain
constexpr float PidKi = 0.01f;         // Integral gain
constexpr float PidKd = 0.1f;          // Derivative gain

// ============================================================================
// ODOMETRY & SENSOR FUSION
// ============================================================================
constexpr float OdometryUpdateRateHz = 50.0f;  // Update odometry at 50 Hz
constexpr float ImuFusionAlpha = 0.98f;        // IMU blending factor (98% gyro, 2% mag)

// ============================================================================
// LOGGING & DEBUG
// ============================================================================
constexpr bool EnableMotorDebug = true;
constexpr bool EnableImuDebug = true;
constexpr bool EnableOdometryDebug = false;
constexpr bool EnableMotionDebug = false;

#endif // CONFIG_H
