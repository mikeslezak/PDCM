/**
 * FaultManager.cpp — Fault Detection and Reporting
 */

#include "FaultManager.h"
#include "CurrentSense.h"
#include "BrakeMonitor.h"
#include "BatteryMonitor.h"
#include "FanController.h"
#include "FourWDController.h"
#include "HBridge.h"
#include "../hal/HAL.h"
#include "../shared/PDCMTypes.h"

static constexpr uint8_t MAX_FAULTS = 32;

struct FaultEntry {
    FaultCode code;
    uint8_t   channel;      // 0xFF = system-level
    uint32_t  timestamp_ms;
    bool      active;
};

static FaultEntry faults[MAX_FAULTS] = {};
static uint8_t faultBitmask = 0;
static uint32_t lastECMHeartbeat = 0;
static bool ecmLost = false;

// Per-channel fault tracking
static uint8_t channelFaults[static_cast<int>(OutputChannel::NUM_CHANNELS)] = {};

namespace FaultManager {

void init() {
    for (uint8_t i = 0; i < MAX_FAULTS; i++) {
        faults[i].active = false;
    }
    for (int i = 0; i < static_cast<int>(OutputChannel::NUM_CHANNELS); i++) {
        channelFaults[i] = static_cast<uint8_t>(FaultCode::NONE);
    }
    faultBitmask = 0;
    lastECMHeartbeat = HAL::millis();
    ecmLost = false;
}

void update() {
    uint32_t now = HAL::millis();

    // --- ECM heartbeat monitoring ---
    if (now - lastECMHeartbeat > PDCMTiming::ECM_HEARTBEAT_TIMEOUT_MS) {
        if (!ecmLost) {
            ecmLost = true;
            set_fault(FaultCode::CAN_TIMEOUT);
            // Failsafe: fans to 100%
            FanController::force_full_speed();
        }
    }

    // --- Per-channel current faults ---
    int8_t oc = CurrentSense::check_overcurrent();
    if (oc >= 0) {
        set_fault(FaultCode::OVERCURRENT, (uint8_t)oc);
        channelFaults[oc] = static_cast<uint8_t>(FaultCode::OVERCURRENT);
    }

    int8_t stuck = CurrentSense::check_stuck_on();
    if (stuck >= 0) {
        set_fault(FaultCode::STUCK_ON, (uint8_t)stuck);
        channelFaults[stuck] = static_cast<uint8_t>(FaultCode::STUCK_ON);
    }

    int8_t open = CurrentSense::check_open_load();
    if (open >= 0) {
        set_fault(FaultCode::OPEN_LOAD, (uint8_t)open);
        channelFaults[open] = static_cast<uint8_t>(FaultCode::OPEN_LOAD);
    }

    // --- Brake disagree ---
    if (BrakeMonitor::disagree()) {
        set_fault(FaultCode::BRAKE_DISAGREE);
    } else {
        clear_fault(FaultCode::BRAKE_DISAGREE);
    }

    // --- Low voltage ---
    if (BatteryMonitor::is_low_comfort() && !BatteryMonitor::is_cranking_dip()) {
        set_fault(FaultCode::LOW_VOLTAGE);
    } else if (!BatteryMonitor::is_low_comfort()) {
        clear_fault(FaultCode::LOW_VOLTAGE);
    }

    // --- Transfer case fault ---
    if (FourWDController::is_fault()) {
        set_fault(FaultCode::TC_MOTOR_STALL);
    }

    // --- H-bridge fault ---
    if (HBridge::is_fault()) {
        set_fault(FaultCode::HBRIDGE_FAULT);
    }

    // --- Rebuild bitmask ---
    faultBitmask = 0;
    if (has_fault(FaultCode::OVERCURRENT))      faultBitmask |= PDCMFaultBits::OVERCURRENT;
    if (has_fault(FaultCode::CAN_TIMEOUT))       faultBitmask |= PDCMFaultBits::CAN_TIMEOUT;
    if (has_fault(FaultCode::BRAKE_DISAGREE))    faultBitmask |= PDCMFaultBits::BRAKE_DISAGREE;
    if (has_fault(FaultCode::TC_MOTOR_STALL) ||
        has_fault(FaultCode::TC_POSITION_FAULT)) faultBitmask |= PDCMFaultBits::TC_FAULT;
    if (has_fault(FaultCode::LOW_VOLTAGE))       faultBitmask |= PDCMFaultBits::LOW_VOLTAGE;

    // Check for specific channel group faults
    for (uint8_t ch = static_cast<uint8_t>(OutputChannel::FAN_1);
         ch <= static_cast<uint8_t>(OutputChannel::FAN_2); ch++) {
        if (channelFaults[ch] != 0) faultBitmask |= PDCMFaultBits::FAN_FAULT;
    }
    if (channelFaults[static_cast<uint8_t>(OutputChannel::FUEL_PUMP)] != 0)
        faultBitmask |= PDCMFaultBits::FUEL_PUMP_FAULT;
    for (uint8_t ch = static_cast<uint8_t>(OutputChannel::LOW_BEAM_L);
         ch <= static_cast<uint8_t>(OutputChannel::COURTESY); ch++) {
        if (channelFaults[ch] != 0) faultBitmask |= PDCMFaultBits::LIGHT_FAULT;
    }
}

void set_fault(FaultCode code, uint8_t channel) {
    // Check if fault already exists
    for (uint8_t i = 0; i < MAX_FAULTS; i++) {
        if (faults[i].active && faults[i].code == code && faults[i].channel == channel) {
            return;  // Already logged
        }
    }
    // Find empty slot
    for (uint8_t i = 0; i < MAX_FAULTS; i++) {
        if (!faults[i].active) {
            faults[i].code = code;
            faults[i].channel = channel;
            faults[i].timestamp_ms = HAL::millis();
            faults[i].active = true;
            return;
        }
    }
    // Fault buffer full — overwrite oldest
    uint32_t oldest_time = 0xFFFFFFFF;
    uint8_t oldest_idx = 0;
    for (uint8_t i = 0; i < MAX_FAULTS; i++) {
        if (faults[i].timestamp_ms < oldest_time) {
            oldest_time = faults[i].timestamp_ms;
            oldest_idx = i;
        }
    }
    faults[oldest_idx].code = code;
    faults[oldest_idx].channel = channel;
    faults[oldest_idx].timestamp_ms = HAL::millis();
    faults[oldest_idx].active = true;
}

void clear_fault(FaultCode code) {
    for (uint8_t i = 0; i < MAX_FAULTS; i++) {
        if (faults[i].active && faults[i].code == code) {
            faults[i].active = false;
        }
    }
}

uint8_t fault_count() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_FAULTS; i++) {
        if (faults[i].active) count++;
    }
    return count;
}

bool has_fault(FaultCode code) {
    for (uint8_t i = 0; i < MAX_FAULTS; i++) {
        if (faults[i].active && faults[i].code == code) return true;
    }
    return false;
}

uint8_t get_fault_bitmask() {
    return faultBitmask;
}

bool channel_has_fault(OutputChannel ch) {
    return channelFaults[static_cast<uint8_t>(ch)] != 0;
}

FaultCode channel_fault(OutputChannel ch) {
    return static_cast<FaultCode>(channelFaults[static_cast<uint8_t>(ch)]);
}

void ecm_heartbeat_received() {
    lastECMHeartbeat = HAL::millis();
    if (ecmLost) {
        ecmLost = false;
        clear_fault(FaultCode::CAN_TIMEOUT);
        FanController::release_override();
    }
}

bool ecm_heartbeat_lost() {
    return ecmLost;
}

} // namespace FaultManager
