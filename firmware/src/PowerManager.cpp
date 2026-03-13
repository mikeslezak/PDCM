/**
 * PowerManager.cpp — Non-Lighting Power Output Implementation
 */

#include "PowerManager.h"
#include "GateDriver.h"
#include "SwitchInput.h"
#include "../hal/HAL.h"
#include "../shared/PDCMTypes.h"

// Fuel pump state machine
static enum {
    FP_OFF,
    FP_PRIMING,
    FP_RUNNING,
    FP_CRANK_TIMEOUT
} fuelPumpState = FP_OFF;
static uint32_t fuelPumpTimer = 0;

// A/C
static bool acClutchReq = false;

// Wiper intermittent timer
static uint32_t wiperIntermitTimer = 0;
static bool wiperIntermitPhase = false;  // true = active stroke

// Blower
static uint16_t blowerPWM = 0;

// Previous key position for edge detection
static KeyPosition prevKeyPos = KeyPosition::OFF;

namespace PowerManager {

void init() {
    fuelPumpState = FP_OFF;
    acClutchReq = false;
    wiperIntermitTimer = 0;
    wiperIntermitPhase = false;
    blowerPWM = 0;
    prevKeyPos = KeyPosition::OFF;
}

void update() {
    uint32_t now = HAL::millis();
    KeyPosition key = SwitchInput::key_position();

    // --- Key position edge detection ---
    if (key == KeyPosition::RUN && prevKeyPos != KeyPosition::RUN) {
        // Key just turned to RUN — prime fuel pump
        fuel_pump_prime();
    }
    prevKeyPos = key;

    // --- Fuel pump state machine ---
    switch (fuelPumpState) {
        case FP_OFF:
            GateDriver::set(OutputChannel::FUEL_PUMP, false);
            break;

        case FP_PRIMING:
            GateDriver::set(OutputChannel::FUEL_PUMP, true);
            if (now - fuelPumpTimer >= PDCMTiming::FUEL_PUMP_PRIME_MS) {
                // Check if engine started (key went to START and back to RUN)
                if (key == KeyPosition::START) {
                    fuelPumpState = FP_RUNNING;
                } else {
                    // Engine didn't start — wait for crank
                    fuelPumpState = FP_CRANK_TIMEOUT;
                    fuelPumpTimer = now;
                }
            }
            break;

        case FP_CRANK_TIMEOUT:
            // Keep pump on briefly waiting for crank
            GateDriver::set(OutputChannel::FUEL_PUMP, true);
            if (key == KeyPosition::START) {
                fuelPumpState = FP_RUNNING;
            } else if (now - fuelPumpTimer >= PDCMTiming::FUEL_PUMP_CRANK_TIMEOUT_MS) {
                fuelPumpState = FP_OFF;
                GateDriver::set(OutputChannel::FUEL_PUMP, false);
            }
            break;

        case FP_RUNNING:
            GateDriver::set(OutputChannel::FUEL_PUMP, true);
            // ECM can turn it off via fuel_pump_run(false)
            if (key == KeyPosition::OFF || key == KeyPosition::ACCESSORY) {
                fuelPumpState = FP_OFF;
            }
            break;
    }

    // --- A/C clutch ---
    bool acOn = acClutchReq && (key == KeyPosition::RUN || key == KeyPosition::START);
    GateDriver::set(OutputChannel::AC_CLUTCH, acOn);

    // --- Horn (direct from switch) ---
    GateDriver::set(OutputChannel::HORN, SwitchInput::horn());

    // --- Wiper ---
    WiperMode wm = SwitchInput::wiper_mode();
    switch (wm) {
        case WiperMode::OFF:
            GateDriver::set(OutputChannel::WIPER, false);
            wiperIntermitPhase = false;
            break;

        case WiperMode::INTERMITTENT:
            if (!wiperIntermitPhase) {
                if (now - wiperIntermitTimer >= PDCMTiming::WIPER_INTERMIT_MS) {
                    wiperIntermitPhase = true;
                    wiperIntermitTimer = now;
                    GateDriver::set(OutputChannel::WIPER, true);
                }
            } else {
                // One wipe cycle (~1 second)
                if (now - wiperIntermitTimer >= 1000) {
                    wiperIntermitPhase = false;
                    wiperIntermitTimer = now;
                    GateDriver::set(OutputChannel::WIPER, false);
                }
            }
            break;

        case WiperMode::SPEED_LOW:
        case WiperMode::SPEED_HIGH:
        case WiperMode::WASH:
            GateDriver::set(OutputChannel::WIPER, true);
            break;
    }

    // --- Blower motor ---
    GateDriver::set_pwm(OutputChannel::BLOWER, blowerPWM);

    // --- Accessory power (key-switched) ---
    bool accOn = (key == KeyPosition::ACCESSORY || key == KeyPosition::RUN ||
                  key == KeyPosition::START);
    GateDriver::set(OutputChannel::ACCESSORY, accOn);

    // --- Amp remotes (on when key is RUN or ACC) ---
    GateDriver::set(OutputChannel::AMP_REMOTE_1, accOn);
    GateDriver::set(OutputChannel::AMP_REMOTE_2, accOn);

    // --- HeadUnit enable (on when key is ACC or RUN) ---
    GateDriver::set(OutputChannel::HEADUNIT_ENABLE, accOn);

    // --- Front axle actuator is handled by FourWDController ---
}

void fuel_pump_prime() {
    fuelPumpState = FP_PRIMING;
    fuelPumpTimer = HAL::millis();
}

void fuel_pump_run(bool on) {
    if (on) {
        fuelPumpState = FP_RUNNING;
    } else if (fuelPumpState == FP_RUNNING) {
        fuelPumpState = FP_OFF;
    }
}

bool fuel_pump_on() {
    return GateDriver::is_on(OutputChannel::FUEL_PUMP);
}

void ac_clutch(bool on) { acClutchReq = on; }
bool ac_clutch_on() { return GateDriver::is_on(OutputChannel::AC_CLUTCH); }
bool ac_request() { return SwitchInput::ac_request(); }

bool horn_on() { return GateDriver::is_on(OutputChannel::HORN); }
bool wiper_on() { return GateDriver::is_on(OutputChannel::WIPER); }

void set_blower_duty(uint16_t duty) {
    if (duty > 1000) duty = 1000;
    blowerPWM = duty;
}
uint16_t blower_duty() { return blowerPWM; }

bool accessory_on() { return GateDriver::is_on(OutputChannel::ACCESSORY); }

void set_seat_heater_left(uint16_t duty) {
    if (duty > 1000) duty = 1000;
    GateDriver::set_pwm(OutputChannel::SEAT_HEATER_L, duty);
}

void set_seat_heater_right(uint16_t duty) {
    if (duty > 1000) duty = 1000;
    GateDriver::set_pwm(OutputChannel::SEAT_HEATER_R, duty);
}

bool amps_on() { return GateDriver::is_on(OutputChannel::AMP_REMOTE_1); }
bool headunit_on() { return GateDriver::is_on(OutputChannel::HEADUNIT_ENABLE); }

} // namespace PowerManager
