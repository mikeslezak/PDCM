#ifndef PDCM_TYPES_H
#define PDCM_TYPES_H

#include <stdint.h>

// ============================================================================
// Output Channel IDs — All 47 switched outputs + 1 H-bridge
// ============================================================================

enum class OutputChannel : uint8_t {
    // Tier 1 — PDCM Switched (current through MOSFETs)
    FUEL_PUMP           = 0,
    FAN_1               = 1,
    FAN_2               = 2,
    BLOWER              = 3,
    AC_CLUTCH           = 4,
    LOW_BEAM_L          = 5,
    LOW_BEAM_R          = 6,
    HIGH_BEAM_L         = 7,
    HIGH_BEAM_R         = 8,
    TURN_L              = 9,
    TURN_R              = 10,
    BRAKE_L             = 11,
    BRAKE_R             = 12,
    REVERSE             = 13,
    DRL                 = 14,
    INTERIOR            = 15,
    COURTESY            = 16,
    HORN                = 17,
    WIPER               = 18,
    ACCESSORY           = 19,
    FRONT_AXLE          = 20,
    SEAT_HEATER_L       = 21,
    SEAT_HEATER_R       = 22,
    LIGHT_BAR           = 23,
    // Ch 25 = DRV8876 H-bridge (handled separately by HBridge module)

    // Tier 2 — Enable signals (low-current turn-on)
    AMP_REMOTE_1        = 25,
    AMP_REMOTE_2        = 26,
    AMP_REMOTE_3        = 27,
    AMP_REMOTE_4        = 28,
    HEADUNIT_ENABLE     = 29,

    // Tier 3 — Sub-loads
    CAM_FRONT           = 30,
    CAM_REAR            = 31,
    CAM_SIDE            = 32,
    PARKING_SENSORS     = 33,
    RADAR_BSM           = 34,
    GCM_POWER           = 35,
    GPS_CELL            = 36,
    DASH_CAM            = 37,
    FUTURE_MODULE       = 38,
    ROCK_LIGHTS         = 39,
    BED_LIGHTS          = 40,
    PUDDLE_LIGHTS       = 41,
    FUTURE_EXTERIOR     = 42,
    EXPANSION_1         = 43,
    EXPANSION_2         = 44,
    EXPANSION_3         = 45,
    EXPANSION_4         = 46,

    NUM_CHANNELS        = 47
};

// ============================================================================
// Load Shedding Priority — ADR-006
// ============================================================================

enum class ShedPriority : uint8_t {
    CRITICAL    = 0,    // Never shed (brake lights, fuel pump, CAN, horn)
    SAFETY      = 1,    // Never shed (headlights, turn signals)
    DRIVING     = 2,    // Shed at battery < 10.5V
    COMFORT     = 3,    // Shed at battery < 11.5V
};

// ============================================================================
// Fault Codes — Per-channel and system-level
// ============================================================================

enum class FaultCode : uint8_t {
    NONE                = 0,
    OVERCURRENT         = 1,    // Current exceeds channel threshold
    OPEN_LOAD           = 2,    // Commanded ON, no current flowing
    STUCK_ON            = 3,    // Commanded OFF, current still flowing
    SHORT_CIRCUIT       = 4,    // Current exceeds fuse-level threshold
    CAN_TIMEOUT         = 5,    // ECM heartbeat lost
    BRAKE_DISAGREE      = 6,    // Brake switches disagree
    LOW_VOLTAGE         = 7,    // Battery below shedding threshold
    TC_MOTOR_STALL      = 8,    // Transfer case motor stalled
    TC_POSITION_FAULT   = 9,    // Transfer case position sensor invalid
    WATCHDOG_RESET      = 10,   // Recovered from watchdog reset
    HBRIDGE_FAULT       = 11,   // DRV8876 nFAULT asserted
    THERMAL_SHUTDOWN     = 12,   // DRV8876 thermal shutdown
};

// ============================================================================
// Light Mode — Lighting controller states
// ============================================================================

enum class LightMode : uint8_t {
    OFF             = 0,
    PARKING         = 1,    // Parking lights only
    LOW_BEAM        = 2,    // Low beams + parking
    HIGH_BEAM       = 3,    // High beams + low + parking
    FLASH_TO_PASS   = 4,    // Momentary high beam flash
    DRL             = 5,    // Daytime running lights
    WELCOME         = 6,    // Welcome sequence (approach lighting)
    GOODBYE         = 7,    // Goodbye sequence (leaving lighting)
};

// ============================================================================
// 4WD Mode — Transfer case target modes
// ============================================================================
// Note: TransferCaseMode enum already in VehicleTypes.h (TWO_HI, AUTO_4WD, etc.)
// FourWDState tracks the controller's internal state machine.

enum class FourWDState : uint8_t {
    IDLE            = 0,    // In target mode, motor off
    SHIFTING        = 1,    // Motor running, transitioning
    STALLED         = 2,    // Motor stalled during shift
    FAULT           = 3,    // Position sensor fault or timeout
};

// ============================================================================
// Key Position — Ignition switch states
// ============================================================================

enum class KeyPosition : uint8_t {
    OFF             = 0,
    ACCESSORY       = 1,
    RUN             = 2,
    START           = 3,
};

// ============================================================================
// Wiper Mode
// ============================================================================

enum class WiperMode : uint8_t {
    OFF             = 0,
    INTERMITTENT    = 1,
    SPEED_LOW       = 2,
    SPEED_HIGH      = 3,
    WASH            = 4,
};

// ============================================================================
// Output State — Runtime state for each channel
// ============================================================================

struct OutputState {
    bool     commanded_on;      // Firmware wants this channel ON
    bool     actually_on;       // Gate driver is actually energized
    uint16_t duty_permille;     // PWM duty 0–1000 (0.0–100.0%), 1000 = full ON
    uint16_t current_mA;        // Measured current in milliamps
    uint8_t  fault;             // FaultCode enum value (0 = no fault)
    bool     shed;              // Currently load-shed
};

// ============================================================================
// Channel Configuration — Static config per channel (stored in flash)
// ============================================================================

struct ChannelConfig {
    uint8_t      gate_pin;          // MCU GPIO pin for TC4427A input
    uint8_t      mux_channel;       // ADC MUX channel for current sense (Teensy)
    uint16_t     shunt_mohm;        // Shunt resistance in milliohms
    uint16_t     overcurrent_mA;    // Software overcurrent threshold
    uint16_t     max_inrush_mA;     // Allowed inrush current (for soft-start channels)
    uint8_t      shed_priority;     // ShedPriority enum value
    bool         pwm_capable;       // Channel supports PWM output
    bool         soft_start;        // Channel uses inrush management
};

// ============================================================================
// CAN Fault Bitmask — Matches 0x357 byte 1 layout
// ============================================================================

namespace PDCMFaultBits {
    constexpr uint8_t OVERCURRENT       = 0x80;
    constexpr uint8_t CAN_TIMEOUT       = 0x40;
    constexpr uint8_t BRAKE_DISAGREE    = 0x20;
    constexpr uint8_t TC_FAULT          = 0x10;
    constexpr uint8_t FAN_FAULT         = 0x08;
    constexpr uint8_t FUEL_PUMP_FAULT   = 0x04;
    constexpr uint8_t LIGHT_FAULT       = 0x02;
    constexpr uint8_t LOW_VOLTAGE       = 0x01;
}

// ============================================================================
// Timing Constants
// ============================================================================

namespace PDCMTiming {
    // Main loop task rates (ms)
    constexpr uint32_t RATE_WATCHDOG        = 1;
    constexpr uint32_t RATE_CURRENT_SCAN    = 2;
    constexpr uint32_t RATE_SWITCH_INPUT    = 10;
    constexpr uint32_t RATE_BRAKE_MONITOR   = 10;
    constexpr uint32_t RATE_BRAKE_CAN_TX    = 50;
    constexpr uint32_t RATE_LIGHT_UPDATE    = 100;
    constexpr uint32_t RATE_SWITCH_CAN_TX   = 100;
    constexpr uint32_t RATE_LIGHT_CAN_TX    = 100;
    constexpr uint32_t RATE_POWER_CAN_TX    = 500;
    constexpr uint32_t RATE_AC_CAN_TX       = 500;
    constexpr uint32_t RATE_4WD_CAN_TX      = 500;
    constexpr uint32_t RATE_HEARTBEAT       = 500;
    constexpr uint32_t RATE_FAN_UPDATE      = 500;
    constexpr uint32_t RATE_POWER_UPDATE    = 500;
    constexpr uint32_t RATE_FAULT_CAN_TX    = 1000;
    constexpr uint32_t RATE_LOAD_SHED       = 1000;
    constexpr uint32_t RATE_BATTERY_CHECK   = 1000;

    // Safety timeouts
    constexpr uint32_t ECM_HEARTBEAT_TIMEOUT_MS  = 3000;
    constexpr uint32_t FUEL_PUMP_PRIME_MS         = 2000;
    constexpr uint32_t FUEL_PUMP_CRANK_TIMEOUT_MS = 5000;
    constexpr uint32_t TC_MOTOR_TIMEOUT_MS        = 5000;
    constexpr uint32_t BRAKE_DISAGREE_TIMEOUT_MS  = 100;

    // Turn signal timing
    constexpr uint32_t TURN_SIGNAL_ON_MS    = 500;
    constexpr uint32_t TURN_SIGNAL_OFF_MS   = 500;

    // Wiper intermittent timing
    constexpr uint32_t WIPER_INTERMIT_MS    = 4000;

    // Welcome/goodbye lighting duration
    constexpr uint32_t WELCOME_DURATION_MS  = 5000;
    constexpr uint32_t GOODBYE_DURATION_MS  = 10000;

    // PWM fade step interval
    constexpr uint32_t FADE_STEP_MS         = 20;
}

// ============================================================================
// Voltage Thresholds (mV)
// ============================================================================

namespace PDCMVoltage {
    constexpr uint16_t SHED_COMFORT_MV      = 11500;    // Shed comfort loads
    constexpr uint16_t SHED_DRIVING_MV      = 10500;    // Shed driving loads
    constexpr uint16_t HYSTERESIS_MV        = 500;      // Recovery hysteresis
    constexpr uint16_t CRANKING_DIP_MV      = 8000;     // Ignore shedding during cranking
    constexpr uint16_t OVERVOLTAGE_MV       = 16000;    // Charging system fault
}

#endif // PDCM_TYPES_H
