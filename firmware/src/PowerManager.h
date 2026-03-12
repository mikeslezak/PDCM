#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

/**
 * PowerManager — Fuel Pump, A/C, Horn, Wiper, Blower, Accessory, Seat Heaters
 *
 * Manages non-lighting power outputs with smart features:
 *   - Fuel pump prime-and-timeout (2s prime on key-to-RUN, off if no start)
 *   - A/C clutch control (from ECM relay command)
 *   - Horn direct from switch
 *   - Wiper intermittent timing
 *   - Blower motor PWM (from HVAC switch)
 *   - Accessory power (key-switched)
 *   - Seat heater PWM temperature control
 *   - Amp remote turn-on (from key position)
 *   - HeadUnit enable (from key position)
 */

#include "../shared/PDCMTypes.h"

namespace PowerManager {

    void init();

    // Main update (call at 500ms rate)
    void update();

    // Fuel pump control
    void fuel_pump_prime();         // Start 2s prime sequence
    void fuel_pump_run(bool on);    // ECM commands fuel pump on/off
    bool fuel_pump_on();

    // A/C clutch (ECM relay command)
    void ac_clutch(bool on);
    bool ac_clutch_on();
    bool ac_request();              // Driver A/C request from switch

    // Horn (direct from switch — update() handles this)
    bool horn_on();

    // Wiper (from switch input — update() handles timing)
    bool wiper_on();

    // Blower motor duty (0-1000)
    void set_blower_duty(uint16_t duty_permille);
    uint16_t blower_duty();

    // Accessory power
    bool accessory_on();

    // Seat heaters (0-1000 PWM each)
    void set_seat_heater_left(uint16_t duty_permille);
    void set_seat_heater_right(uint16_t duty_permille);

    // Amp remotes and HeadUnit
    bool amps_on();
    bool headunit_on();
}

#endif // POWER_MANAGER_H
