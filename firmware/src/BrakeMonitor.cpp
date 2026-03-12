/**
 * BrakeMonitor.cpp — Dual Brake Switch + BOP Implementation
 */

#include "BrakeMonitor.h"
#include "../hal/HAL.h"
#include "../shared/PDCMConfig.h"
#include "../shared/PDCMTypes.h"

static bool sw1_state = false;
static bool sw2_state = false;
static bool bop = false;
static bool disagree_fault = false;
static uint32_t disagree_start_ms = 0;

// Debounce counters (3-sample majority)
static uint8_t sw1_history = 0;
static uint8_t sw2_history = 0;

static bool debounce3(uint8_t& history, bool raw) {
    history = ((history << 1) | (raw ? 1 : 0)) & 0x07;
    uint8_t count = 0;
    if (history & 0x01) count++;
    if (history & 0x02) count++;
    if (history & 0x04) count++;
    return (count >= 2);
}

namespace BrakeMonitor {

void init() {
#ifdef PDCM_TARGET_TEENSY
    // Direct GPIO — never behind SPI/I2C for safety
    HAL::gpio_mode_input(Pin::BRAKE_SWITCH_1, true);   // Internal pullup
    HAL::gpio_mode_input(Pin::BRAKE_SWITCH_2, true);   // Internal pullup
#endif

    sw1_state = false;
    sw2_state = false;
    bop = false;
    disagree_fault = false;
    disagree_start_ms = 0;
    sw1_history = 0;
    sw2_history = 0;
}

void update() {
#ifdef PDCM_TARGET_TEENSY
    // Read direct GPIO (active-low: pressed = LOW)
    bool raw1 = !HAL::gpio_read(Pin::BRAKE_SWITCH_1);
    bool raw2 = !HAL::gpio_read(Pin::BRAKE_SWITCH_2);
#else
    bool raw1 = false;
    bool raw2 = false;
#endif

    sw1_state = debounce3(sw1_history, raw1);
    sw2_state = debounce3(sw2_history, raw2);

    // BOP: both switches agree brake is pressed
    bop = (sw1_state && sw2_state);

    // Disagree detection with timeout
    if (sw1_state != sw2_state) {
        if (disagree_start_ms == 0) {
            disagree_start_ms = HAL::millis();
        } else if (HAL::millis() - disagree_start_ms > PDCMTiming::BRAKE_DISAGREE_TIMEOUT_MS) {
            disagree_fault = true;
        }
    } else {
        disagree_start_ms = 0;
        disagree_fault = false;
    }
}

bool switch_1()     { return sw1_state; }
bool switch_2()     { return sw2_state; }
bool bop_active()   { return bop; }
bool disagree()     { return disagree_fault; }
bool brake_pressed(){ return sw1_state || sw2_state; }

} // namespace BrakeMonitor
