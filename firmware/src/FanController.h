#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H

/**
 * FanController — Cooling Fan PWM Control
 *
 * Receives fan speed command from ECM (0x360).
 * Drives two PWM cooling fan MOSFETs.
 *
 * Safety: If ECM heartbeat is lost (CAN timeout), fans go to 100%.
 * This prevents engine overheat if the CAN bus fails.
 */

#include <stdint.h>

namespace FanController {

    void init();

    // Main update (call at 500ms rate)
    void update();

    // Set fan duty from ECM CAN command (0-255 → mapped to 0-1000)
    void set_duty_from_can(uint8_t duty_byte);

    // Override to full speed (CAN timeout failsafe)
    void force_full_speed();

    // Release full speed override
    void release_override();

    // Get current fan duty (0-1000)
    uint16_t get_duty();

    // Is CAN timeout failsafe active?
    bool is_failsafe();
}

#endif // FAN_CONTROLLER_H
