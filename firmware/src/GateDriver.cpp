/**
 * GateDriver.cpp — TC4427A Output Control Implementation
 */

#include "GateDriver.h"
#include "../hal/HAL.h"
#include "../shared/PDCMConfig.h"

// ============================================================================
// Channel-to-pin mapping table
// ============================================================================

#ifdef PDCM_TARGET_TEENSY

// Direct GPIO pin for each Tier 1-2 channel
static const uint8_t directPins[] = {
    Pin::CH_FUEL_PUMP,      // 0  FUEL_PUMP
    Pin::CH_FAN_1,          // 1  FAN_1
    Pin::CH_FAN_2,          // 2  FAN_2
    Pin::CH_BLOWER,         // 3  BLOWER
    Pin::CH_AC_CLUTCH,      // 4  AC_CLUTCH
    Pin::CH_LOW_BEAM_L,     // 5  LOW_BEAM_L
    Pin::CH_LOW_BEAM_R,     // 6  LOW_BEAM_R
    Pin::CH_HIGH_BEAM_L,    // 7  HIGH_BEAM_L
    Pin::CH_HIGH_BEAM_R,    // 8  HIGH_BEAM_R
    Pin::CH_TURN_L,         // 9  TURN_L
    Pin::CH_TURN_R,         // 10 TURN_R
    Pin::CH_BRAKE_L,        // 11 BRAKE_L
    Pin::CH_BRAKE_R,        // 12 BRAKE_R
    Pin::CH_REVERSE,        // 13 REVERSE
    Pin::CH_DRL,            // 14 DRL
    Pin::CH_INTERIOR,       // 15 INTERIOR
    Pin::CH_COURTESY,       // 16 COURTESY
    Pin::CH_HORN,           // 17 HORN
    Pin::CH_WIPER,          // 18 WIPER
    Pin::CH_ACCESSORY,      // 19 ACCESSORY
    Pin::CH_FRONT_AXLE,     // 20 FRONT_AXLE
    Pin::CH_SEAT_HEATER_L,  // 21 SEAT_HEATER_L
    Pin::CH_SEAT_HEATER_R,  // 22 SEAT_HEATER_R
    Pin::CH_LIGHT_BAR,      // 23 LIGHT_BAR
    0,                      // 24 (H-bridge — not in this table)
    Pin::CH_AMP_REMOTE_1,   // 25 AMP_REMOTE_1
    Pin::CH_AMP_REMOTE_2,   // 26 AMP_REMOTE_2
    Pin::CH_AMP_REMOTE_3,   // 27 AMP_REMOTE_3
    Pin::CH_AMP_REMOTE_4,   // 28 AMP_REMOTE_4
    Pin::CH_HEADUNIT_EN,    // 29 HEADUNIT_ENABLE
};

static constexpr uint8_t NUM_DIRECT_PINS = sizeof(directPins) / sizeof(directPins[0]);

#endif // PDCM_TARGET_TEENSY

// Runtime state
static bool channelState[static_cast<int>(OutputChannel::NUM_CHANNELS)] = {};
static uint16_t channelDuty[static_cast<int>(OutputChannel::NUM_CHANNELS)] = {};

// ============================================================================
// Implementation
// ============================================================================

namespace GateDriver {

void init() {
#ifdef PDCM_TARGET_TEENSY
    // Initialize all direct GPIO pins as outputs (default LOW = OFF)
    for (uint8_t i = 0; i < NUM_DIRECT_PINS; i++) {
        if (i == 24) continue;  // Skip H-bridge slot
        HAL::gpio_mode_output(directPins[i]);
    }

    // Set PWM frequency for PWM-capable channels
    // 25 kHz is above audible range and good for MOSFET switching
    HAL::pwm_set_frequency(Pin::CH_FAN_1, 25000);
    HAL::pwm_set_frequency(Pin::CH_FAN_2, 25000);
    HAL::pwm_set_frequency(Pin::CH_BLOWER, 25000);
    HAL::pwm_set_frequency(Pin::CH_LOW_BEAM_L, 25000);
    HAL::pwm_set_frequency(Pin::CH_LOW_BEAM_R, 25000);
    HAL::pwm_set_frequency(Pin::CH_HIGH_BEAM_L, 25000);
    HAL::pwm_set_frequency(Pin::CH_HIGH_BEAM_R, 25000);
    HAL::pwm_set_frequency(Pin::CH_DRL, 25000);
    HAL::pwm_set_frequency(Pin::CH_INTERIOR, 25000);
    HAL::pwm_set_frequency(Pin::CH_COURTESY, 25000);
    HAL::pwm_set_frequency(Pin::CH_SEAT_HEATER_L, 25000);
    HAL::pwm_set_frequency(Pin::CH_SEAT_HEATER_R, 25000);

    // Initialize MCP23S17 #2 for Tier 3 outputs
    HAL::expander_init(Pin::SPI_CS_EXPANDER2);
    HAL::expander_set_direction(Pin::SPI_CS_EXPANDER2, 0, 0x00);  // Port A all outputs
    HAL::expander_set_direction(Pin::SPI_CS_EXPANDER2, 1, 0x00);  // Port B all outputs
    HAL::expander_write_port(Pin::SPI_CS_EXPANDER2, 0, 0x00);     // All OFF
    HAL::expander_write_port(Pin::SPI_CS_EXPANDER2, 1, 0x00);     // All OFF
#endif

    // Clear state
    for (int i = 0; i < static_cast<int>(OutputChannel::NUM_CHANNELS); i++) {
        channelState[i] = false;
        channelDuty[i] = 0;
    }
}

void set(OutputChannel ch, bool on) {
    uint8_t idx = static_cast<uint8_t>(ch);
    if (idx >= static_cast<uint8_t>(OutputChannel::NUM_CHANNELS)) return;

    channelState[idx] = on;
    channelDuty[idx] = on ? 1000 : 0;

#ifdef PDCM_TARGET_TEENSY
    if (idx < NUM_DIRECT_PINS && idx != 24) {
        // Direct GPIO channel
        HAL::gpio_write(directPins[idx], on);
    } else if (idx >= 30 && idx <= 46) {
        // Tier 3: MCP23S17 #2
        uint8_t exp_idx = idx - 30;
        uint8_t port = (exp_idx < 8) ? 0 : 1;
        uint8_t bit = exp_idx % 8;

        uint8_t current = HAL::expander_read_port(Pin::SPI_CS_EXPANDER2, port);
        if (on) {
            current |= (1 << bit);
        } else {
            current &= ~(1 << bit);
        }
        HAL::expander_write_port(Pin::SPI_CS_EXPANDER2, port, current);
    }
#endif
}

void set_pwm(OutputChannel ch, uint16_t duty_permille) {
    uint8_t idx = static_cast<uint8_t>(ch);
    if (idx >= static_cast<uint8_t>(OutputChannel::NUM_CHANNELS)) return;

    if (duty_permille > 1000) duty_permille = 1000;
    channelDuty[idx] = duty_permille;
    channelState[idx] = (duty_permille > 0);

#ifdef PDCM_TARGET_TEENSY
    if (idx < NUM_DIRECT_PINS && idx != 24) {
        HAL::pwm_write(directPins[idx], duty_permille);
    }
    // Tier 3 channels via expander don't support PWM — on/off only
    // (MCP23S17 GPIO can't do hardware PWM)
#endif
}

bool is_on(OutputChannel ch) {
    uint8_t idx = static_cast<uint8_t>(ch);
    if (idx >= static_cast<uint8_t>(OutputChannel::NUM_CHANNELS)) return false;
    return channelState[idx];
}

uint16_t get_duty(OutputChannel ch) {
    uint8_t idx = static_cast<uint8_t>(ch);
    if (idx >= static_cast<uint8_t>(OutputChannel::NUM_CHANNELS)) return 0;
    return channelDuty[idx];
}

void all_off() {
    for (int i = 0; i < static_cast<int>(OutputChannel::NUM_CHANNELS); i++) {
        set(static_cast<OutputChannel>(i), false);
    }
}

} // namespace GateDriver
