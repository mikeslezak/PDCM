#ifndef LOAD_SHEDDER_H
#define LOAD_SHEDDER_H

/**
 * LoadShedder — Battery Voltage Protection via Load Priority
 *
 * Four-tier priority shedding (ADR-006):
 *   CRITICAL — never shed (brakes, fuel pump, CAN, horn)
 *   SAFETY   — never shed (headlights, turn signals)
 *   DRIVING  — shed at battery < 10.5V (fans, DRL, reverse, wiper, light bar)
 *   COMFORT  — shed at battery < 11.5V (blower, interior, A/C, amps, etc.)
 *
 * Recovery with 0.5V hysteresis to prevent oscillation.
 */

#include <stdint.h>

namespace LoadShedder {

    void init();

    // Evaluate shedding (call at 1000ms rate)
    void update();

    // Is a given channel currently shed?
    bool is_shed(uint8_t channel_idx);

    // Are any loads currently shed?
    bool is_shedding();

    // Current shedding tier (0 = none, 3 = comfort shed, 2 = driving shed)
    uint8_t shed_tier();
}

#endif // LOAD_SHEDDER_H
