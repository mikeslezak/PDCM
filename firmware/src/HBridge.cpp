/**
 * HBridge.cpp — DRV8876 4WD Motor Control Implementation
 */

#include "HBridge.h"
#include "../hal/HAL.h"
#include "../shared/PDCMConfig.h"

static HBridgeDir currentDir = HBridgeDir::COAST;

namespace HBridge {

void init() {
#ifdef PDCM_TARGET_TEENSY
    HAL::gpio_mode_output(Pin::HBRIDGE_IN1);
    HAL::gpio_mode_output(Pin::HBRIDGE_IN2);
    HAL::gpio_mode_output(Pin::HBRIDGE_NSLEEP);
    HAL::gpio_mode_input(Pin::HBRIDGE_NFAULT, true);  // Active-low, pullup

    // Set PWM frequency for motor control
    HAL::pwm_set_frequency(Pin::HBRIDGE_IN1, 20000);
    HAL::pwm_set_frequency(Pin::HBRIDGE_IN2, 20000);

    // Start in coast mode, awake
    HAL::gpio_write(Pin::HBRIDGE_IN1, false);
    HAL::gpio_write(Pin::HBRIDGE_IN2, false);
    HAL::gpio_write(Pin::HBRIDGE_NSLEEP, true);  // nSLEEP high = active
#endif

    currentDir = HBridgeDir::COAST;
}

void set(HBridgeDir dir, uint16_t speed_permille) {
    if (speed_permille > 1000) speed_permille = 1000;
    currentDir = dir;

#ifdef PDCM_TARGET_TEENSY
    switch (dir) {
        case HBridgeDir::COAST:
            HAL::gpio_write(Pin::HBRIDGE_IN1, false);
            HAL::gpio_write(Pin::HBRIDGE_IN2, false);
            break;

        case HBridgeDir::FORWARD:
            HAL::pwm_write(Pin::HBRIDGE_IN1, speed_permille);
            HAL::gpio_write(Pin::HBRIDGE_IN2, false);
            break;

        case HBridgeDir::REVERSE:
            HAL::gpio_write(Pin::HBRIDGE_IN1, false);
            HAL::pwm_write(Pin::HBRIDGE_IN2, speed_permille);
            break;

        case HBridgeDir::BRAKE:
            HAL::gpio_write(Pin::HBRIDGE_IN1, true);
            HAL::gpio_write(Pin::HBRIDGE_IN2, true);
            break;
    }
#endif
}

void stop() {
    set(HBridgeDir::BRAKE);
}

void sleep() {
#ifdef PDCM_TARGET_TEENSY
    HAL::gpio_write(Pin::HBRIDGE_IN1, false);
    HAL::gpio_write(Pin::HBRIDGE_IN2, false);
    HAL::gpio_write(Pin::HBRIDGE_NSLEEP, false);  // nSLEEP low = sleep
#endif
    currentDir = HBridgeDir::COAST;
}

void wake() {
#ifdef PDCM_TARGET_TEENSY
    HAL::gpio_write(Pin::HBRIDGE_NSLEEP, true);
    HAL::delay_us(100);  // DRV8876 wake-up time
#endif
}

bool is_fault() {
#ifdef PDCM_TARGET_TEENSY
    // nFAULT is active-low: LOW = fault
    return !HAL::gpio_read(Pin::HBRIDGE_NFAULT);
#else
    return false;
#endif
}

uint16_t get_current_mA() {
#ifdef PDCM_TARGET_TEENSY
    // DRV8876 IPROPI output: I_load / 1150 (typical)
    // V_ipropi = I_load × R_ipropi / 1150
    // With 1kΩ sense resistor: V_adc = I_load × 1000 / 1150
    // I_load_mA = V_adc_mV × 1150 / 1000
    uint16_t adc_mv = HAL::adc_read_mv(Pin::ADC_HBRIDGE_CS);
    return (uint16_t)((uint32_t)adc_mv * 1150 / 1000);
#else
    return 0;
#endif
}

HBridgeDir get_direction() {
    return currentDir;
}

} // namespace HBridge
