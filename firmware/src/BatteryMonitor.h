#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

/**
 * BatteryMonitor — Battery Voltage Monitoring
 *
 * Reads battery voltage via voltage divider on ADC.
 * Provides voltage in millivolts for load shedding decisions.
 */

#include <stdint.h>

namespace BatteryMonitor {

    void init();

    // Read and filter battery voltage (call at 1000ms rate)
    void update();

    // Battery voltage in millivolts
    uint16_t voltage_mv();

    // Convenience checks
    bool is_low_comfort();      // Below SHED_COMFORT_MV
    bool is_low_driving();      // Below SHED_DRIVING_MV
    bool is_cranking_dip();     // Below CRANKING_DIP_MV (ignore shedding)
    bool is_overvoltage();      // Above OVERVOLTAGE_MV
}

#endif // BATTERY_MONITOR_H
