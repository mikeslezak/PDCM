/**
 * BatteryMonitor.cpp — Battery Voltage Monitoring Implementation
 */

#include "BatteryMonitor.h"
#include "../hal/HAL.h"
#include "../shared/PDCMConfig.h"
#include "../shared/PDCMTypes.h"

// Rolling average filter (8 samples)
static uint16_t samples[8] = {};
static uint8_t sampleIdx = 0;
static uint16_t filteredMV = 12600;   // Default to nominal 12.6V

namespace BatteryMonitor {

void init() {
    for (uint8_t i = 0; i < 8; i++) {
        samples[i] = 12600;    // Pre-fill with nominal voltage
    }
    sampleIdx = 0;
    filteredMV = 12600;
}

void update() {
#ifdef PDCM_TARGET_TEENSY
    // Read ADC and convert through voltage divider
    uint16_t adc_mv = HAL::adc_read_mv(Pin::ADC_BATTERY);
    // V_batt = V_adc × (R1+R2)/R2 = V_adc × DIVIDER_MULT_X1000 / 1000
    uint32_t batt_mv = ((uint32_t)adc_mv * BatteryADC::DIVIDER_MULT_X1000) / 1000;
#else
    uint32_t batt_mv = 12600;   // Placeholder
#endif

    // Store in rolling buffer
    samples[sampleIdx] = (uint16_t)batt_mv;
    sampleIdx = (sampleIdx + 1) & 0x07;    // Wrap at 8

    // Compute average
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 8; i++) {
        sum += samples[i];
    }
    filteredMV = (uint16_t)(sum / 8);
}

uint16_t voltage_mv() {
    return filteredMV;
}

bool is_low_comfort() {
    return filteredMV < PDCMVoltage::SHED_COMFORT_MV;
}

bool is_low_driving() {
    return filteredMV < PDCMVoltage::SHED_DRIVING_MV;
}

bool is_cranking_dip() {
    return filteredMV < PDCMVoltage::CRANKING_DIP_MV;
}

bool is_overvoltage() {
    return filteredMV > PDCMVoltage::OVERVOLTAGE_MV;
}

} // namespace BatteryMonitor
