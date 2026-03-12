#ifndef CAN_MANAGER_H
#define CAN_MANAGER_H

/**
 * CANManager — Vehicle CAN FD Communication
 *
 * Handles all 9 published messages and 5 consumed messages.
 * Registers CAN callbacks on init, publishes at correct rates.
 *
 * Published: 0x350-0x357, 0x36F
 * Consumed: 0x310, 0x360-0x363
 */

#include <stdint.h>

namespace CANManager {

    // Initialize CAN callbacks for consumed messages
    void init();

    // --- Publish functions (called by main loop at correct rates) ---
    void publish_switch_state();     // 0x350, 100ms
    void publish_light_state();      // 0x351, 100ms
    void publish_power_state();      // 0x352, 500ms
    void publish_cruise_event();     // 0x353, on-event
    void publish_ac_state();         // 0x354, 500ms
    void publish_4wd_state();        // 0x355, 500ms
    void publish_brake_state();      // 0x356, 50ms
    void publish_faults();           // 0x357, 1000ms
    void publish_heartbeat();        // 0x36F, 500ms

    // Check if a cruise event needs to be sent
    bool has_cruise_event();
}

#endif // CAN_MANAGER_H
