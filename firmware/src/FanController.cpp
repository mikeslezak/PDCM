/**
 * FanController.cpp — Cooling Fan Control Implementation
 */

#include "FanController.h"
#include "GateDriver.h"
#include "../hal/HAL.h"

static uint16_t commandedDuty = 0;     // From ECM CAN (0-1000)
static bool failsafeActive = false;
static bool overrideActive = false;

namespace FanController {

void init() {
    commandedDuty = 0;
    failsafeActive = false;
    overrideActive = false;
}

void update() {
    uint16_t duty = failsafeActive ? 1000 : commandedDuty;

    GateDriver::set_pwm(OutputChannel::FAN_1, duty);
    GateDriver::set_pwm(OutputChannel::FAN_2, duty);
}

void set_duty_from_can(uint8_t duty_byte) {
    // Map 0-255 to 0-1000
    commandedDuty = ((uint32_t)duty_byte * 1000) / 255;
    if (!overrideActive) {
        failsafeActive = false;
    }
}

void force_full_speed() {
    failsafeActive = true;
    overrideActive = true;
}

void release_override() {
    failsafeActive = false;
    overrideActive = false;
}

uint16_t get_duty() {
    return failsafeActive ? 1000 : commandedDuty;
}

bool is_failsafe() {
    return failsafeActive;
}

} // namespace FanController
