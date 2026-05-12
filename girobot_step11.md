# GiRobot Step 11: Battery Diagnostics & Fix

> **Phase 11**: Diagnose and fix battery ADC reading issue. GPIO 0 may not be available; test alternatives and implement robust voltage measurement.

---

## 📋 Objectives

1. ✅ Diagnose why GPIO 0 ADC not working
2. ✅ Identify alternative GPIO for ADC
3. ✅ Implement voltage divider compensation
4. ✅ Add voltage smoothing/filtering
5. ✅ Verify battery reading accuracy

---

## 🔍 Diagnosis

### Current Configuration

From [include/Config.h](../include/Config.h):

```cpp
constexpr int PinBatteryAdc = 0;    // GPIO 0 = NOT WORKING
constexpr float BatteryDividerRatio = 2.0f;
constexpr float BatteryLowVoltage = 6.6f;
```

### Issue: GPIO 0 on ESP32

GPIO 0 is a **strapping pin** used during boot. It's not reliably available for ADC after startup.

### Alternative GPIO Options for ADC

ESP32 ADC1 available pins (from datasheet):
- GPIO 32 (ADC1_CH4) ← Recommended
- GPIO 33 (ADC1_CH5) ← Recommended
- GPIO 34 (ADC1_CH6) ← Can use if encoders moved
- GPIO 35 (ADC1_CH7)
- GPIO 36 (ADC1_CH0) ← Note: might conflict with GPIO 36 boot mode
- GPIO 39 (ADC1_CH3)

**Best choice**: GPIO 35 (least conflicts)

---

## 🔧 Fix: Update Config.h

In [include/Config.h](../include/Config.h), change:

```cpp
// BEFORE:
constexpr int PinBatteryAdc = 0;

// AFTER (using GPIO 35):
constexpr int PinBatteryAdc = 35;
```

---

## 🔌 Hardware Wiring Check

### Voltage Divider Circuit

Battery → R1 (10k) → ADC pin → R2 (10k) → GND

```
Battery+ (8.4V max)
    |
   R1 (10k)
    |
   ADC (GPIO 35) → should read: 8.4V / 2 = 4.2V
    |
   R2 (10k)
    |
   GND
```

**Measurements**:
- [ ] Battery voltage (multimeter): _____ V
- [ ] Voltage at ADC pin: _____ V (should be ~half battery)
- [ ] Divider ratio: _____ (should be 2.0)

---

## 📖 Enhanced BatteryMonitor.cpp

Add voltage averaging and error checking:

```cpp
// In BatteryMonitor.h:
class BatteryMonitor {
private:
    static constexpr int ADC_SAMPLES = 10;
    float lastVoltage = 0.0f;
    float voltageBuffer[ADC_SAMPLES] = {0};
    int bufferIndex = 0;
    
public:
    void begin() {
        if (Defaults::PinBatteryAdc == 0) {
            Serial.println("[WARN] Battery ADC disabled (GPIO 0)");
            return;
        }
        
        analogSetAttenuation(ADC_11db);  // 0-3.3V range
        pinMode(Defaults::PinBatteryAdc, INPUT);
    }
    
    float getVoltage() {
        if (Defaults::PinBatteryAdc == 0) {
            return 0.0f;  // Disabled
        }
        
        // Read ADC with averaging
        int rawValue = analogRead(Defaults::PinBatteryAdc);
        
        // Convert to voltage: (raw / 4095) * 3.3V
        float adcVoltage = (rawValue / 4095.0f) * 3.3f;
        
        // Apply divider ratio
        float batteryVoltage = adcVoltage * Defaults::BatteryDividerRatio;
        
        // Clamp to reasonable range (0-12V for 2S LiPo)
        if (batteryVoltage < 0) batteryVoltage = 0;
        if (batteryVoltage > 12.0f) batteryVoltage = 12.0f;
        
        // Averaging filter
        voltageBuffer[bufferIndex] = batteryVoltage;
        bufferIndex = (bufferIndex + 1) % ADC_SAMPLES;
        
        // Calculate average
        float sum = 0;
        for (int i = 0; i < ADC_SAMPLES; i++) {
            sum += voltageBuffer[i];
        }
        
        lastVoltage = sum / ADC_SAMPLES;
        
        return lastVoltage;
    }
    
    float getPercentage() {
        float voltage = getVoltage();
        
        // LiPo 2S battery: 6.4V (empty) to 8.4V (full)
        const float MIN_VOLTAGE = 6.4f;
        const float MAX_VOLTAGE = 8.4f;
        
        float percent = (voltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE) * 100.0f;
        
        // Clamp 0-100
        if (percent < 0) percent = 0;
        if (percent > 100) percent = 100;
        
        return percent;
    }
    
    bool isLow() {
        return getVoltage() < Defaults::BatteryLowVoltage;
    }
};
```

---

## 🧪 Testing Procedure

### Test 1: ADC Pin Configuration

```cpp
void testBatteryADC() {
    Serial.println("\n[TEST] Battery ADC setup...");
    
    if (Defaults::PinBatteryAdc == 0) {
        Serial.println("[ERROR] Battery ADC still set to GPIO 0!");
        Serial.println("[FIX] Update Config.h: PinBatteryAdc = 35");
        return;
    }
    
    Serial.printf("[INFO] Battery ADC pin: GPIO %d\n", Defaults::PinBatteryAdc);
    
    // Read raw ADC value
    int rawValue = analogRead(Defaults::PinBatteryAdc);
    Serial.printf("[RAW] ADC reading: %d / 4095\n", rawValue);
    
    if (rawValue == 0 || rawValue == 4095) {
        Serial.println("[WARN] ADC reading at extreme (0 or 4095) - check wiring");
    } else {
        Serial.println("[PASS] ADC reading in valid range");
    }
}
```

### Test 2: Voltage Measurement Accuracy

```cpp
void testVoltageAccuracy() {
    Serial.println("\n[TEST] Battery voltage accuracy...");
    Serial.println("Connect multimeter to battery terminals and compare.");
    
    battery.begin();
    
    for (int i = 0; i < 10; i++) {
        float voltage = battery.getVoltage();
        float percent = battery.getPercentage();
        
        Serial.printf("[MEAS %d] %.2f V (%d%%)\n", i+1, voltage, (int)percent);
        
        delay(500);
    }
    
    // Manual comparison:
    Serial.println("\n[INFO] Manually check:");
    Serial.println("  - Multimeter reading");
    Serial.println("  - ESP32 reading above");
    Serial.println("  - Difference should be < 0.2V");
}
```

### Test 3: Voltage Smoothing

```cpp
void testVoltageSmoothingEffect() {
    Serial.println("\n[TEST] Voltage smoothing filter...");
    
    // Simulate voltage noise by reading rapid succession
    Serial.println("Raw readings (no averaging):");
    for (int i = 0; i < 5; i++) {
        int raw = analogRead(Defaults::PinBatteryAdc);
        float voltage = (raw / 4095.0f) * 3.3f * Defaults::BatteryDividerRatio;
        Serial.printf("  %d: %.2f V\n", i+1, voltage);
    }
    
    Serial.println("Averaged readings:");
    for (int i = 0; i < 5; i++) {
        float voltage = battery.getVoltage();
        Serial.printf("  %d: %.2f V\n", i+1, voltage);
        delay(100);
    }
    
    Serial.println("[INFO] Averaged readings should be more stable");
}
```

### Test 4: Low Battery Warning

```cpp
void testLowBatteryDetection() {
    Serial.println("\n[TEST] Low battery threshold...");
    
    Serial.printf("Low voltage threshold: %.1f V\n", Defaults::BatteryLowVoltage);
    
    for (int i = 0; i < 10; i++) {
        float voltage = battery.getVoltage();
        bool isLow = battery.isLow();
        
        Serial.printf("[CHECK] %.2f V - %s\n", 
                      voltage, 
                      isLow ? "LOW WARNING" : "OK");
        
        delay(500);
    }
}
```

---

## ✅ Verification Checklist: Step 11

- [ ] **Config.h Updated**: PinBatteryAdc changed to 35
- [ ] **ADC Reading Valid**: Not stuck at 0 or 4095
- [ ] **Voltage Accuracy**: ±0.2V vs multimeter
- [ ] **Smoothing Works**: Readings stable, no noise spikes
- [ ] **Low Battery Alert**: Triggers when voltage < 6.6V
- [ ] **Telemetry Display**: Battery shows in web dashboard
- [ ] **No ADC Errors**: Serial shows valid readings

---

## 🐛 If Still Not Working

**Alternative approaches**:
1. Check voltage divider resistors (should be 10k each)
2. Try GPIO 34 or 39 instead of 35
3. Use attenuation setting: `analogSetAttenuation(ADC_11db)`
4. Consider using INA219 current/voltage sensor module (I2C)

---

**Estimated Time**: 1.5 hours  
**Difficulty**: ⭐⭐ Intermediate  
**Next**: [girobot_step12.md](girobot_step12.md)

Last Updated: May 11, 2026
