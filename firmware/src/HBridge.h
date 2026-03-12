#ifndef HBRIDGE_H
#define HBRIDGE_H

/**
 * HBridge — DRV8876 4WD Motor Control
 *
 * Controls the NP246 transfer case encoder motor via TI DRV8876 H-bridge IC.
 * Provides forward/reverse/brake/coast control with current sensing and
 * hardware fault detection.
 *
 * DRV8876 control truth table:
 *   IN1  IN2  Mode
 *   L    L    Coast (Hi-Z)
 *   H    L    Forward
 *   L    H    Reverse
 *   H    H    Brake (low-side recirculation)
 */

#include <stdint.h>

enum class HBridgeDir : uint8_t {
    COAST       = 0,    // Motor free-spinning (Hi-Z outputs)
    FORWARD     = 1,    // Motor forward (2HI → 4LO direction)
    REVERSE     = 2,    // Motor reverse (4LO → 2HI direction)
    BRAKE       = 3,    // Motor braked (low-side short)
};

namespace HBridge {

    // Initialize DRV8876 pins (IN1, IN2, nSLEEP, nFAULT)
    void init();

    // Set motor direction and speed
    // speed: 0–1000 (0.0–100.0%), only used for FORWARD/REVERSE
    void set(HBridgeDir dir, uint16_t speed_permille = 1000);

    // Stop motor (brake mode)
    void stop();

    // Put DRV8876 into sleep mode (low power)
    void sleep();

    // Wake DRV8876 from sleep
    void wake();

    // Check hardware fault (nFAULT pin)
    bool is_fault();

    // Read motor current in mA (DRV8876 IPROPI output)
    uint16_t get_current_mA();

    // Get current direction
    HBridgeDir get_direction();
}

#endif // HBRIDGE_H
