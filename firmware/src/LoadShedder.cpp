/**
 * LoadShedder.cpp — Load Shedding Implementation
 */

#include "LoadShedder.h"
#include "GateDriver.h"
#include "BatteryMonitor.h"
#include "../hal/HAL.h"
#include "../shared/PDCMTypes.h"

// Per-channel shed priority
static const uint8_t channelPriority[] = {
    // Tier 1 channels
    (uint8_t)ShedPriority::CRITICAL,    // 0  Fuel pump
    (uint8_t)ShedPriority::DRIVING,     // 1  Fan 1
    (uint8_t)ShedPriority::DRIVING,     // 2  Fan 2
    (uint8_t)ShedPriority::COMFORT,     // 3  Blower
    (uint8_t)ShedPriority::COMFORT,     // 4  A/C clutch
    (uint8_t)ShedPriority::SAFETY,      // 5  Low beam L
    (uint8_t)ShedPriority::SAFETY,      // 6  Low beam R
    (uint8_t)ShedPriority::SAFETY,      // 7  High beam L
    (uint8_t)ShedPriority::SAFETY,      // 8  High beam R
    (uint8_t)ShedPriority::SAFETY,      // 9  Turn L
    (uint8_t)ShedPriority::SAFETY,      // 10 Turn R
    (uint8_t)ShedPriority::CRITICAL,    // 11 Brake L
    (uint8_t)ShedPriority::CRITICAL,    // 12 Brake R
    (uint8_t)ShedPriority::DRIVING,     // 13 Reverse
    (uint8_t)ShedPriority::DRIVING,     // 14 DRL
    (uint8_t)ShedPriority::COMFORT,     // 15 Interior
    (uint8_t)ShedPriority::COMFORT,     // 16 Courtesy
    (uint8_t)ShedPriority::CRITICAL,    // 17 Horn
    (uint8_t)ShedPriority::DRIVING,     // 18 Wiper
    (uint8_t)ShedPriority::COMFORT,     // 19 Accessory
    (uint8_t)ShedPriority::DRIVING,     // 20 Front axle
    (uint8_t)ShedPriority::COMFORT,     // 21 Seat heater L
    (uint8_t)ShedPriority::COMFORT,     // 22 Seat heater R
    (uint8_t)ShedPriority::DRIVING,     // 23 Light bar
    (uint8_t)ShedPriority::DRIVING,     // 24 (H-bridge)
    // Tier 2 channels
    (uint8_t)ShedPriority::COMFORT,     // 25 Amp remote 1
    (uint8_t)ShedPriority::COMFORT,     // 26 Amp remote 2
    (uint8_t)ShedPriority::COMFORT,     // 27 Amp remote 3
    (uint8_t)ShedPriority::COMFORT,     // 28 Amp remote 4
    (uint8_t)ShedPriority::COMFORT,     // 29 HeadUnit enable
    // Tier 3 channels
    (uint8_t)ShedPriority::COMFORT,     // 30 Front camera
    (uint8_t)ShedPriority::COMFORT,     // 31 Rear camera
    (uint8_t)ShedPriority::COMFORT,     // 32 Side cameras
    (uint8_t)ShedPriority::COMFORT,     // 33 Parking sensors
    (uint8_t)ShedPriority::COMFORT,     // 34 Radar/BSM
    (uint8_t)ShedPriority::COMFORT,     // 35 GCM power
    (uint8_t)ShedPriority::COMFORT,     // 36 GPS/cellular
    (uint8_t)ShedPriority::COMFORT,     // 37 Dash cam
    (uint8_t)ShedPriority::COMFORT,     // 38 Future module
    (uint8_t)ShedPriority::DRIVING,     // 39 Rock lights
    (uint8_t)ShedPriority::COMFORT,     // 40 Bed lights
    (uint8_t)ShedPriority::COMFORT,     // 41 Puddle lights
    (uint8_t)ShedPriority::COMFORT,     // 42 Future exterior
    (uint8_t)ShedPriority::COMFORT,     // 43 Expansion 1
    (uint8_t)ShedPriority::COMFORT,     // 44 Expansion 2
    (uint8_t)ShedPriority::COMFORT,     // 45 Expansion 3
    (uint8_t)ShedPriority::COMFORT,     // 46 Expansion 4
};

static bool shedFlags[static_cast<int>(OutputChannel::NUM_CHANNELS)] = {};
static uint8_t currentShedTier = 0;
static bool comfortShed = false;
static bool drivingShed = false;

namespace LoadShedder {

void init() {
    for (int i = 0; i < static_cast<int>(OutputChannel::NUM_CHANNELS); i++) {
        shedFlags[i] = false;
    }
    currentShedTier = 0;
    comfortShed = false;
    drivingShed = false;
}

void update() {
    uint16_t batt = BatteryMonitor::voltage_mv();

    // Skip shedding during cranking dip
    if (BatteryMonitor::is_cranking_dip()) return;

    // --- Comfort tier: shed at < 11.5V, recover at > 12.0V ---
    if (!comfortShed && batt < PDCMVoltage::SHED_COMFORT_MV) {
        comfortShed = true;
    } else if (comfortShed && batt > PDCMVoltage::SHED_COMFORT_MV + PDCMVoltage::HYSTERESIS_MV) {
        comfortShed = false;
    }

    // --- Driving tier: shed at < 10.5V, recover at > 11.0V ---
    if (!drivingShed && batt < PDCMVoltage::SHED_DRIVING_MV) {
        drivingShed = true;
    } else if (drivingShed && batt > PDCMVoltage::SHED_DRIVING_MV + PDCMVoltage::HYSTERESIS_MV) {
        drivingShed = false;
    }

    // Apply shedding to channels
    for (int i = 0; i < static_cast<int>(OutputChannel::NUM_CHANNELS); i++) {
        bool shouldShed = false;

        if (channelPriority[i] == (uint8_t)ShedPriority::COMFORT && comfortShed) {
            shouldShed = true;
        }
        if (channelPriority[i] == (uint8_t)ShedPriority::DRIVING && drivingShed) {
            shouldShed = true;
        }

        if (shouldShed && !shedFlags[i]) {
            // Shed this channel
            shedFlags[i] = true;
            GateDriver::set(static_cast<OutputChannel>(i), false);
        } else if (!shouldShed && shedFlags[i]) {
            // Restore this channel
            shedFlags[i] = false;
            // Note: channel will be re-enabled by its owning controller
            // on its next update cycle
        }
    }

    // Update tier
    if (drivingShed) currentShedTier = 2;
    else if (comfortShed) currentShedTier = 3;
    else currentShedTier = 0;
}

bool is_shed(uint8_t channel_idx) {
    if (channel_idx >= static_cast<uint8_t>(OutputChannel::NUM_CHANNELS)) return false;
    return shedFlags[channel_idx];
}

bool is_shedding() {
    return comfortShed || drivingShed;
}

uint8_t shed_tier() {
    return currentShedTier;
}

} // namespace LoadShedder
