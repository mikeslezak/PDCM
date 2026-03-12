#ifndef CURRENT_SENSE_H
#define CURRENT_SENSE_H

/**
 * CurrentSense — Per-Channel Shunt Current Measurement
 *
 * Reads shunt voltage on every output channel via analog MUX (Teensy)
 * or direct ADC (S32K358). Converts to milliamps using known shunt values.
 *
 * Teensy: 3× CD74HC4067 (16:1 MUX) → 3 ADC pins
 * S32K358: Direct ADC channels (no MUX needed)
 *
 * INA180 current sense amplifier (gain = 50) between shunt and ADC.
 */

#include "../shared/PDCMTypes.h"

namespace CurrentSense {

    // Initialize MUX select pins and ADC
    void init();

    // Scan one MUX step (call 16 times to complete full scan)
    // Returns true when full scan is complete
    bool scan_step();

    // Get measured current for a channel in milliamps
    uint16_t get_current_mA(OutputChannel ch);

    // Get raw ADC value for a channel (for diagnostics)
    uint16_t get_raw_adc(OutputChannel ch);

    // Get total system current draw (sum of all channels) in mA
    uint32_t get_total_current_mA();

    // Check if any channel exceeds its overcurrent threshold
    // Returns channel index, or -1 if none
    int8_t check_overcurrent();

    // Check for stuck-on fault (current > threshold when channel commanded OFF)
    int8_t check_stuck_on();

    // Check for open-load fault (current < threshold when channel commanded ON)
    int8_t check_open_load();
}

#endif // CURRENT_SENSE_H
