#ifndef BRAKE_MONITOR_H
#define BRAKE_MONITOR_H

/**
 * BrakeMonitor — Dual Brake Switch + BOP Logic
 *
 * Safety-critical module. Reads two independent brake switches on
 * DIRECT MCU GPIO (never behind SPI/I2C — no bus in safety path).
 *
 * BOP (Brake Override Protection): If both switches agree that the
 * brake is pressed, BOP is active. ECM uses this to force throttle closed.
 *
 * Disagree fault: If switches disagree for > 100ms, flag brake_disagree fault.
 * This message publishes at 50ms — fastest of any PDCM message.
 */

#include <stdint.h>

namespace BrakeMonitor {

    // Initialize brake switch GPIO pins
    void init();

    // Read and process brake switches (call at 10ms rate)
    void update();

    // Individual switch states
    bool switch_1();
    bool switch_2();

    // BOP active (both switches pressed)
    bool bop_active();

    // Switches disagree (fault condition)
    bool disagree();

    // Either switch pressed (for brake light control)
    bool brake_pressed();
}

#endif // BRAKE_MONITOR_H
