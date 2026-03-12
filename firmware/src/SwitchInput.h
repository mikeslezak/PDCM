#ifndef SWITCH_INPUT_H
#define SWITCH_INPUT_H

/**
 * SwitchInput — Driver Switch Reading + Debounce
 *
 * Reads all switch inputs: stalks, hazard, horn, wiper, HVAC, key position.
 * Digital switches via MCP23S17 #1 (SPI port expander).
 * Key position via ADC resistor ladder.
 * All switches debounced in software (3-sample majority).
 */

#include "../shared/PDCMTypes.h"

namespace SwitchInput {

    // Initialize switch input pins and port expander
    void init();

    // Read all switches (call at 10ms rate)
    void update();

    // --- Turn signal stalk ---
    bool turn_left();
    bool turn_right();
    bool high_beam();
    bool flash_to_pass();

    // --- Hazard / Horn ---
    bool hazard();
    bool horn();

    // --- Wiper ---
    WiperMode wiper_mode();
    bool washer();

    // --- HVAC ---
    uint8_t hvac_fan_speed();       // 0-7
    uint8_t hvac_mode();            // 0=off, 1=vent, 2=floor, 3=defrost
    bool ac_request();

    // --- Key position ---
    KeyPosition key_position();

    // --- Reverse ---
    bool reverse_switch();

    // --- Cruise stalk events ---
    // Returns true once per button press (edge-detected)
    bool cruise_set_pressed();
    bool cruise_resume_pressed();
    bool cruise_accel_pressed();
    bool cruise_decel_pressed();
    bool cruise_cancel_pressed();
    bool cruise_onoff_pressed();
    bool any_cruise_event();        // True if any cruise button pressed this cycle
}

#endif // SWITCH_INPUT_H
