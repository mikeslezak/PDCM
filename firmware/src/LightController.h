#ifndef LIGHT_CONTROLLER_H
#define LIGHT_CONTROLLER_H

/**
 * LightController — Lighting Output Management
 *
 * Controls all lighting outputs with smart features:
 *   - Headlight soft-start (PWM ramp to reduce inrush on halogen bulbs)
 *   - Turn signal flash timer (500ms on / 500ms off)
 *   - Hazard override (turns both signals on, overrides individual turns)
 *   - DRL dimming (reduced PWM when headlights off, engine running)
 *   - Interior/courtesy light fade (PWM ramp up/down)
 *   - Welcome/goodbye lighting sequences
 *   - Brake lights direct from brake switch GPIO (no CAN dependency)
 *
 * Brake lights are driven directly from BrakeMonitor state — never
 * routed through CAN for safety. The MOSFET is the only thing between
 * the brake switch and the brake light.
 */

#include "../shared/PDCMTypes.h"

namespace LightController {

    void init();

    // Main update (call at 100ms rate)
    void update();

    // Set headlight mode (from switch inputs or HMI command)
    void set_mode(LightMode mode);
    LightMode get_mode();

    // Turn signal control (from switch inputs)
    void set_turn_left(bool on);
    void set_turn_right(bool on);
    void set_hazard(bool on);

    // Interior/courtesy (from key position, door switches, HMI)
    void set_interior(bool on);
    void set_courtesy(bool on);

    // Trigger welcome/goodbye sequence
    void trigger_welcome();
    void trigger_goodbye();

    // Get actual output states (for CAN reporting)
    bool is_low_beam_on();
    bool is_high_beam_on();
    bool is_turn_left_on();     // Actual output (includes flash state)
    bool is_turn_right_on();
    bool is_brake_on();
    bool is_reverse_on();
    bool is_drl_on();
    bool is_interior_on();
    bool is_courtesy_on();
}

#endif // LIGHT_CONTROLLER_H
