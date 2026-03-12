#ifndef GATE_DRIVER_H
#define GATE_DRIVER_H

/**
 * GateDriver — TC4427A Output Control
 *
 * Controls all 47 TC4427A-driven MOSFET channels.
 * Supports on/off switching and PWM for channels that need it.
 *
 * Circuit: MCU GPIO → TC4427A input → TC4427A output → 100Ω → MOSFET gate
 *          10kΩ pulldown on MOSFET gate ensures OFF during boot.
 *
 * Tier 1-2 channels (0-29): Direct MCU GPIO pins
 * Tier 3 channels (30-46): Via MCP23S17 #2 SPI port expander
 */

#include "../shared/PDCMTypes.h"

namespace GateDriver {

    // Initialize all gate driver output pins
    void init();

    // Set channel on/off (digital, no PWM)
    void set(OutputChannel ch, bool on);

    // Set channel PWM duty (0-1000 = 0.0-100.0%)
    // Only works on channels flagged as pwm_capable in ChannelConfig
    void set_pwm(OutputChannel ch, uint16_t duty_permille);

    // Get current commanded state
    bool is_on(OutputChannel ch);
    uint16_t get_duty(OutputChannel ch);

    // Emergency: all channels OFF
    void all_off();
}

#endif // GATE_DRIVER_H
