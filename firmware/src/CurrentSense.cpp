/**
 * CurrentSense.cpp — Per-Channel Current Sensing Implementation
 */

#include "CurrentSense.h"
#include "GateDriver.h"
#include "../hal/HAL.h"
#include "../shared/PDCMConfig.h"

static constexpr uint8_t TOTAL_CHANNELS = static_cast<uint8_t>(OutputChannel::NUM_CHANNELS);

// Per-channel shunt resistance in milliohms (ADR-010: two sizes only)
// 10mΩ × 36 channels (heavy, 1–16A) + 50mΩ × 10 channels (light, 0.3–3.3A)
static const uint16_t shuntMohm[TOTAL_CHANNELS] = {
    10,  10,  10,  10,  10,     // Ch 0-4:   Fuel pump, Fan1, Fan2, Blower, A/C
    10,  10,  10,  10,          // Ch 5-8:   Low beams, High beams
    10,  10,                    // Ch 9-10:  Turn signals
    10,  10,                    // Ch 11-12: Brake lights
    10,  10,  10,  10,          // Ch 13-16: Reverse, DRL, Interior, Courtesy
    10,  10,  10,               // Ch 17-19: Horn, Wiper, Accessory
    10,                         // Ch 20:    Front axle
    10,  10,                    // Ch 21-22: Seat heaters
    10,                         // Ch 23:    Light bar
    0,                          // Ch 24:    H-bridge (sensed separately)
    50,  50,  50,               // Ch 25-27: Amp remotes, HeadUnit (enable signals)
    10,  10,  10,  10,  10,     // Ch 28-32: Cameras, parking, radar
    10,  10,  10,  10,          // Ch 33-36: GCM, GPS, dash cam, future module
    10,  10,  10,               // Ch 37-39: Rock lights, bed lights, puddle
    50,                         // Ch 40:    Future exterior
    50,  50,  50,  50,  50,  50,// Ch 41-46: Expansion 1-6
};

// Per-channel overcurrent thresholds in mA (independent of shunt value)
static const uint16_t overcurrentMA[TOTAL_CHANNELS] = {
    15000, 15000, 15000, 12000, 8000,    // Ch 0-4:   Fuel pump, fans, blower, A/C
    8000,  8000,  8000,  8000,           // Ch 5-8:   Low beams, High beams
    3000,  3000,                          // Ch 9-10:  Turn signals
    3000,  3000,                          // Ch 11-12: Brake lights
    3000,  2000,  2000,  2000,           // Ch 13-16: Reverse, DRL, Interior, Courtesy
    8000,  8000,  8000,                  // Ch 17-19: Horn, Wiper, Accessory
    5000,                                 // Ch 20:    Front axle
    10000, 10000,                         // Ch 21-22: Seat heaters
    20000,                                // Ch 23:    Light bar
    0,                                    // Ch 24:    H-bridge
    500,   500,   500,                   // Ch 25-27: Amp remotes, HeadUnit
    500,   1000,                          // Ch 28-29: Cameras (front, rear)
    2000,  2000,  2000,                  // Ch 30-32: Side cams, parking, radar
    2000,  2000,  2000,  2000,           // Ch 33-36: GCM, GPS, dash cam, future module
    2000,  2000,  5000,                  // Ch 37-39: Rock lights, bed lights, puddle
    2000,  2000,  2000,                  // Ch 40-42: Future exterior, expansion 1-2
    5000,  5000,  5000,  5000,           // Ch 43-46: Expansion 3-6
};

// Measured raw ADC values (updated during scan)
static uint16_t rawADC[TOTAL_CHANNELS] = {};
// Converted current in mA
static uint16_t currentMA[TOTAL_CHANNELS] = {};
// MUX scan position
static uint8_t muxStep = 0;

namespace CurrentSense {

void init() {
#ifdef PDCM_TARGET_TEENSY
    // Initialize MCP23S17 #1 for switch inputs + MUX select
    HAL::expander_init(Pin::SPI_CS_EXPANDER1);
    // Port A: all inputs (switch reading)
    HAL::expander_set_direction(Pin::SPI_CS_EXPANDER1, 0, 0xFF);
    HAL::expander_set_pullups(Pin::SPI_CS_EXPANDER1, 0, 0xFF);
    // Port B: lower 4 bits = inputs (wiper switches), upper 4 bits = outputs (MUX select)
    HAL::expander_set_direction(Pin::SPI_CS_EXPANDER1, 1, 0x0F);
    HAL::expander_set_pullups(Pin::SPI_CS_EXPANDER1, 1, 0x0F);
#endif

    muxStep = 0;
    for (uint8_t i = 0; i < TOTAL_CHANNELS; i++) {
        rawADC[i] = 0;
        currentMA[i] = 0;
    }
}

bool scan_step() {
#ifdef PDCM_TARGET_TEENSY
    // Set MUX channel
    HAL::mux_select(muxStep);

    // Small delay for MUX settling (analog switch ~100ns, but ADC sample time dominates)
    HAL::delay_us(5);

    // Read all 3 MUX ADC outputs simultaneously
    uint16_t adc0 = HAL::adc_read(Pin::ADC_MUX_0);  // Channels 0-15
    uint16_t adc1 = HAL::adc_read(Pin::ADC_MUX_1);  // Channels 16-31
    uint16_t adc2 = HAL::adc_read(Pin::ADC_MUX_2);  // Channels 32-47

    // Store raw ADC for the current MUX step
    uint8_t ch0 = muxStep;         // MUX 0: channels 0-15
    uint8_t ch1 = muxStep + 16;    // MUX 1: channels 16-31
    uint8_t ch2 = muxStep + 32;    // MUX 2: channels 32-47

    if (ch0 < TOTAL_CHANNELS) rawADC[ch0] = adc0;
    if (ch1 < TOTAL_CHANNELS) rawADC[ch1] = adc1;
    if (ch2 < TOTAL_CHANNELS) rawADC[ch2] = adc2;

    // Convert to mA: V_adc = I × R_shunt × AMP_GAIN
    // I_mA = (adc_raw × 3300 / 4096) / (R_shunt_mohm × AMP_GAIN / 1000)
    // Simplified: I_mA = (adc_raw × 3300 × 1000) / (4096 × R_shunt_mohm × AMP_GAIN)
    auto convertToMA = [](uint16_t adc_raw, uint16_t shunt_mohm) -> uint16_t {
        if (shunt_mohm == 0) return 0;
        uint32_t numerator = (uint32_t)adc_raw * 3300UL * 1000UL;
        uint32_t denominator = 4096UL * (uint32_t)shunt_mohm * ::CurrentSense::AMPLIFIER_GAIN;
        return (uint16_t)(numerator / denominator);
    };

    if (ch0 < TOTAL_CHANNELS) currentMA[ch0] = convertToMA(adc0, shuntMohm[ch0]);
    if (ch1 < TOTAL_CHANNELS) currentMA[ch1] = convertToMA(adc1, shuntMohm[ch1]);
    if (ch2 < TOTAL_CHANNELS) currentMA[ch2] = convertToMA(adc2, shuntMohm[ch2]);
#endif

    muxStep++;
    if (muxStep >= 16) {
        muxStep = 0;
        return true;    // Full scan complete
    }
    return false;
}

uint16_t get_current_mA(OutputChannel ch) {
    uint8_t idx = static_cast<uint8_t>(ch);
    if (idx >= TOTAL_CHANNELS) return 0;
    return currentMA[idx];
}

uint16_t get_raw_adc(OutputChannel ch) {
    uint8_t idx = static_cast<uint8_t>(ch);
    if (idx >= TOTAL_CHANNELS) return 0;
    return rawADC[idx];
}

uint32_t get_total_current_mA() {
    uint32_t total = 0;
    for (uint8_t i = 0; i < TOTAL_CHANNELS; i++) {
        total += currentMA[i];
    }
    return total;
}

int8_t check_overcurrent() {
    for (uint8_t i = 0; i < TOTAL_CHANNELS; i++) {
        if (overcurrentMA[i] > 0 && currentMA[i] > overcurrentMA[i]) {
            return (int8_t)i;
        }
    }
    return -1;
}

int8_t check_stuck_on() {
    for (uint8_t i = 0; i < TOTAL_CHANNELS; i++) {
        if (!GateDriver::is_on(static_cast<OutputChannel>(i)) &&
            currentMA[i] > 200) {   // >200mA when OFF = stuck on
            return (int8_t)i;
        }
    }
    return -1;
}

int8_t check_open_load() {
    for (uint8_t i = 0; i < TOTAL_CHANNELS; i++) {
        if (GateDriver::is_on(static_cast<OutputChannel>(i)) &&
            overcurrentMA[i] > 0 &&  // Skip channels with no threshold
            currentMA[i] < 50) {      // <50mA when ON = open load
            return (int8_t)i;
        }
    }
    return -1;
}

} // namespace CurrentSense
