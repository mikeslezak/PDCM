#ifndef PDCM_CONFIG_H
#define PDCM_CONFIG_H

#include <stdint.h>

// ============================================================================
// Build Target Selection
// ============================================================================

#if defined(PDCM_TARGET_S32K358)
    #include "PDCMConfig_S32K358.h"
#elif !defined(PDCM_TARGET_TEENSY)
    // Default: Teensy 4.1 prototype target
    #define PDCM_TARGET_TEENSY
#endif

// ============================================================================
// Teensy 4.1 Pin Allocation — Prototype Target
// ============================================================================
// Teensy 4.1 has 42 digital pins + 18 analog pins.
// With 47 outputs, we need TC4427A gate drivers on GPIO, SPI port expander
// for switch inputs, and analog MUX for current sense ADC.
//
// Pin budget:
//   Gate driver outputs:  34 GPIO  (Tier 1 ch1-24, Tier 2 ch26-30)
//   H-bridge control:      4 GPIO  (DRV8876: IN1, IN2, nSLEEP, nFAULT)
//   Tier 3 outputs:        13 GPIO (ch31-43, via MCP23S17 #2)
//   Expansion outputs:      4 GPIO (ch44-47, via MCP23S17 #2)
//   Brake switches:         2 GPIO (direct — never behind SPI/I2C)
//   Key position ADC:       1 ADC  (resistor ladder)
//   Battery voltage ADC:    1 ADC  (voltage divider)
//   4WD position ADC:       1 ADC  (potentiometer)
//   CAN FD:                 2 pins (CAN1 TX/RX)
//   SPI:                    4 pins (MOSI/MISO/SCK + CS)
//   MUX select:             4 pins (via MCP23S17 — S0-S3)
//   MCP23S17 CS:            2 pins (CS for each SPI expander)
//   MUX ADC inputs:         3 ADC  (one per CD74HC4067)
//   Serial debug:           1 pin  (TX only needed)
//
// Total direct MCU pins used: ~42 GPIO + 6 ADC + SPI + CAN = fits Teensy 4.1
// ============================================================================

#ifdef PDCM_TARGET_TEENSY

// --- Gate Driver Outputs (TC4427A inputs, directly from Teensy GPIO) ---
// Tier 1 channels 1-24 — these are the most critical, need direct GPIO
namespace Pin {
    // Tier 1 — Heavy loads (fuel pump, fans, blower, A/C)
    constexpr uint8_t CH_FUEL_PUMP      = 2;
    constexpr uint8_t CH_FAN_1          = 3;    // PWM capable
    constexpr uint8_t CH_FAN_2          = 4;    // PWM capable
    constexpr uint8_t CH_BLOWER         = 5;    // PWM capable
    constexpr uint8_t CH_AC_CLUTCH      = 6;

    // Tier 1 — Lighting (PWM capable for soft-start/dimming)
    constexpr uint8_t CH_LOW_BEAM_L     = 7;    // PWM capable
    constexpr uint8_t CH_LOW_BEAM_R     = 8;    // PWM capable
    constexpr uint8_t CH_HIGH_BEAM_L    = 9;    // PWM capable
    constexpr uint8_t CH_HIGH_BEAM_R    = 10;   // PWM capable
    constexpr uint8_t CH_TURN_L         = 11;
    constexpr uint8_t CH_TURN_R         = 12;
    constexpr uint8_t CH_BRAKE_L        = 28;
    constexpr uint8_t CH_BRAKE_R        = 29;
    constexpr uint8_t CH_REVERSE        = 30;
    constexpr uint8_t CH_DRL            = 31;   // PWM capable
    constexpr uint8_t CH_INTERIOR       = 32;   // PWM capable
    constexpr uint8_t CH_COURTESY       = 33;   // PWM capable

    // Tier 1 — Misc
    constexpr uint8_t CH_HORN           = 34;
    constexpr uint8_t CH_WIPER          = 35;
    constexpr uint8_t CH_ACCESSORY      = 36;
    constexpr uint8_t CH_FRONT_AXLE     = 37;

    // Tier 1 — High current
    constexpr uint8_t CH_SEAT_HEATER_L  = 14;   // PWM capable
    constexpr uint8_t CH_SEAT_HEATER_R  = 15;   // PWM capable
    constexpr uint8_t CH_LIGHT_BAR      = 40;

    // Tier 2 — Enable signals (low current, still direct GPIO)
    constexpr uint8_t CH_AMP_REMOTE_1   = 41;
    constexpr uint8_t CH_AMP_REMOTE_2   = 16;
    constexpr uint8_t CH_AMP_REMOTE_3   = 17;
    constexpr uint8_t CH_AMP_REMOTE_4   = 20;
    constexpr uint8_t CH_HEADUNIT_EN    = 21;

    // --- H-Bridge (DRV8876) for 4WD encoder motor ---
    // CAN1 on pins 22/23 — H-bridge uses 24/25 (PWM capable)
    constexpr uint8_t HBRIDGE_IN1       = 24;   // PWM capable
    constexpr uint8_t HBRIDGE_IN2       = 25;   // PWM capable
    constexpr uint8_t HBRIDGE_NSLEEP    = 26;
    constexpr uint8_t HBRIDGE_NFAULT    = 27;   // Input — active low fault

    // --- Safety-Critical Direct Inputs (never behind SPI/I2C) ---
    constexpr uint8_t BRAKE_SWITCH_1    = 38;   // Factory brake switch
    constexpr uint8_t BRAKE_SWITCH_2    = 39;   // Dedicated brake switch

    // --- SPI Bus (for MCP23S17 port expanders + CD74HC4067 MUX) ---
    constexpr uint8_t SPI_MOSI          = 11;   // Teensy SPI0
    constexpr uint8_t SPI_MISO          = 12;   // Teensy SPI0
    constexpr uint8_t SPI_SCK           = 13;   // Teensy SPI0
    constexpr uint8_t SPI_CS_EXPANDER1  = 0;    // MCP23S17 #1 (switch inputs)
    constexpr uint8_t SPI_CS_EXPANDER2  = 1;    // MCP23S17 #2 (Tier 3 outputs)

    // --- CAN FD ---
    // CAN1 TX = pin 22, RX = pin 23 (Teensy 4.1 CAN1 default)
    constexpr uint8_t CAN_TX            = 22;
    constexpr uint8_t CAN_RX            = 23;

    // --- ADC Inputs ---
    // Teensy 4.1 ADC-capable pins: A0(14)..A9(23), A10(24)..A13(27), A14(38)..A17(41)
    // Most analog pins are shared with digital GPIO above.
    // Using pins 18(A4), 19(A5) for dedicated ADC (not used as digital).
    // Using Teensy 4.1 bottom-side pads for MUX ADC (dedicated analog).
    constexpr uint8_t ADC_BATTERY       = 18;   // A4 — Battery voltage divider
    constexpr uint8_t ADC_KEY_POS       = 19;   // A5 — Key position resistor ladder
    constexpr uint8_t ADC_4WD_POS       = 40;   // A16 — Transfer case pot (back pad)
    constexpr uint8_t ADC_MUX_0         = 41;   // A17 — CD74HC4067 #1 (ch 0-15, back pad)
    constexpr uint8_t ADC_MUX_1         = 24;   // A10 — CD74HC4067 #2 (ch 16-31)
    constexpr uint8_t ADC_MUX_2         = 25;   // A11 — CD74HC4067 #3 (ch 32-47)
    constexpr uint8_t ADC_HBRIDGE_CS    = 26;   // A12 — DRV8876 current sense
}

// --- MCP23S17 #1: Switch Inputs (directly-read stalks, buttons, etc.) ---
// Port A: Digital switch inputs
namespace Exp1PortA {
    constexpr uint8_t STALK_LEFT_TURN_L     = 0;
    constexpr uint8_t STALK_LEFT_TURN_R     = 1;
    constexpr uint8_t STALK_LEFT_HIGH_BEAM  = 2;
    constexpr uint8_t STALK_LEFT_FLASH      = 3;
    constexpr uint8_t HAZARD_SW             = 4;
    constexpr uint8_t HORN_SW               = 5;
    constexpr uint8_t REVERSE_SW            = 6;
    constexpr uint8_t AC_REQUEST            = 7;
}

// Port B: More switch inputs + MUX select lines
namespace Exp1PortB {
    constexpr uint8_t WIPER_INT             = 0;   // Wiper intermittent
    constexpr uint8_t WIPER_LOW             = 1;   // Wiper low
    constexpr uint8_t WIPER_HIGH            = 2;   // Wiper high
    constexpr uint8_t WASHER                = 3;   // Washer pump
    constexpr uint8_t MUX_S0                = 4;   // CD74HC4067 select bit 0
    constexpr uint8_t MUX_S1                = 5;   // CD74HC4067 select bit 1
    constexpr uint8_t MUX_S2                = 6;   // CD74HC4067 select bit 2
    constexpr uint8_t MUX_S3                = 7;   // CD74HC4067 select bit 3
}

// --- MCP23S17 #2: Tier 3 Output Channels (cameras, modules, exterior, expansion) ---
// Port A: Channels 31-38
namespace Exp2PortA {
    constexpr uint8_t CH_CAM_FRONT          = 0;   // Ch 31
    constexpr uint8_t CH_CAM_REAR           = 1;   // Ch 32
    constexpr uint8_t CH_CAM_SIDE           = 2;   // Ch 33
    constexpr uint8_t CH_PARKING_SENSORS    = 3;   // Ch 34
    constexpr uint8_t CH_RADAR_BSM          = 4;   // Ch 35
    constexpr uint8_t CH_GCM_POWER          = 5;   // Ch 36
    constexpr uint8_t CH_GPS_CELL           = 6;   // Ch 37
    constexpr uint8_t CH_DASH_CAM           = 7;   // Ch 38
}

// Port B: Channels 39-47
namespace Exp2PortB {
    constexpr uint8_t CH_FUTURE_MODULE      = 0;   // Ch 39
    constexpr uint8_t CH_ROCK_LIGHTS        = 1;   // Ch 40
    constexpr uint8_t CH_BED_LIGHTS         = 2;   // Ch 41
    constexpr uint8_t CH_PUDDLE_LIGHTS      = 3;   // Ch 42
    constexpr uint8_t CH_FUTURE_EXTERIOR    = 4;   // Ch 43
    constexpr uint8_t CH_EXPANSION_1        = 5;   // Ch 44
    constexpr uint8_t CH_EXPANSION_2        = 6;   // Ch 45
    constexpr uint8_t CH_EXPANSION_3        = 7;   // Ch 46
    // Ch 47 (EXPANSION_4) would need a 3rd MCP23S17 or a free Teensy pin
    // For prototype, 46 channels via 2× MCP23S17 is sufficient
}

// --- ADC MUX Channel Mapping ---
// CD74HC4067 is a 16:1 analog MUX. Three MUX ICs cover all 48 shunt channels.
// MUX select lines (S0-S3) shared across all three MUXes via MCP23S17 #1 Port B.
// Each MUX output goes to a separate ADC pin.

// MUX 0 (ADC_MUX_0): Channels 0-15 (Fuel pump through DRL)
// MUX 1 (ADC_MUX_1): Channels 16-31 (Interior through Side cameras)
// MUX 2 (ADC_MUX_2): Channels 32-47 (Parking sensors through Expansion 4)

// --- Current Sense Configuration ---
// Shunt voltage = I × R_shunt
// ADC_mV = shunt_voltage × amplifier_gain (if using op-amp, gain ~50)
// For direct ADC: V_shunt at 10A through 10mΩ = 100mV (within Teensy ADC range)
namespace CurrentSense {
    constexpr uint8_t  NUM_MUX_ICS          = 3;
    constexpr uint8_t  CHANNELS_PER_MUX     = 16;
    constexpr uint16_t ADC_RESOLUTION       = 4096;     // 12-bit
    constexpr uint16_t ADC_REF_MV           = 3300;     // 3.3V reference
    constexpr uint8_t  AMPLIFIER_GAIN       = 50;       // INA180 or similar
}

// --- Battery Voltage Divider ---
// R1 = 10kΩ (to 12V), R2 = 3.3kΩ (to GND)
// V_adc = V_batt × R2 / (R1 + R2) = V_batt × 0.248
// V_batt = V_adc × (R1 + R2) / R2 = V_adc × 4.03
namespace BatteryADC {
    constexpr uint16_t DIVIDER_R1_OHM       = 10000;
    constexpr uint16_t DIVIDER_R2_OHM       = 3300;
    // Multiplier × 1000 for integer math: (R1+R2)/R2 × 1000
    constexpr uint32_t DIVIDER_MULT_X1000   = 4030;
}

// --- Key Position Resistor Ladder ---
// OFF=0V, ACC=~0.8V, RUN=~1.6V, START=~2.5V (via resistor ladder)
namespace KeyPosADC {
    constexpr uint16_t THRESHOLD_OFF_ACC    = 500;   // mV midpoint
    constexpr uint16_t THRESHOLD_ACC_RUN    = 1200;  // mV midpoint
    constexpr uint16_t THRESHOLD_RUN_START  = 2100;  // mV midpoint
}

// --- 4WD Position Potentiometer ---
// NP246 transfer case position sensor: 0V = 2HI, 5V = 4LO
// Through voltage divider to fit 3.3V ADC range
namespace FourWDPosADC {
    constexpr uint16_t POS_2HI_MV           = 300;
    constexpr uint16_t POS_A4WD_MV          = 900;
    constexpr uint16_t POS_4HI_MV           = 1500;
    constexpr uint16_t POS_NEUTRAL_MV       = 2100;
    constexpr uint16_t POS_4LO_MV           = 2700;
    constexpr uint16_t POS_TOLERANCE_MV     = 200;
}

#endif // PDCM_TARGET_TEENSY

// ============================================================================
// Channel Configuration Table — Platform-independent
// ============================================================================
// Defined in PDCMConfig.cpp (or inline in a source file).
// Maps OutputChannel enum to pin, shunt value, thresholds, and behavior.

#endif // PDCM_CONFIG_H
