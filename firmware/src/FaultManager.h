#ifndef FAULT_MANAGER_H
#define FAULT_MANAGER_H

/**
 * FaultManager — Per-Channel and System Fault Tracking
 *
 * Monitors all output channels for overcurrent, open-load, and stuck-on faults.
 * Monitors system-level faults: CAN timeout, brake disagree, low voltage.
 * Maintains fault log for diagnostics.
 *
 * Fault severity determines action:
 *   - INFO: Log only
 *   - WARNING: Log + CAN report
 *   - FAULT: Log + CAN report + disable affected channel
 *   - CRITICAL: Log + CAN report + emergency action
 */

#include "../shared/PDCMTypes.h"

namespace FaultManager {

    void init();

    // Check all fault sources (call at 1000ms rate)
    void update();

    // Set/clear specific faults
    void set_fault(FaultCode code, uint8_t channel = 0xFF);
    void clear_fault(FaultCode code);

    // Query
    uint8_t fault_count();
    bool has_fault(FaultCode code);
    uint8_t get_fault_bitmask();        // For 0x357 byte 1

    // Per-channel fault query
    bool channel_has_fault(OutputChannel ch);
    FaultCode channel_fault(OutputChannel ch);

    // ECM heartbeat monitoring
    void ecm_heartbeat_received();
    bool ecm_heartbeat_lost();
}

#endif // FAULT_MANAGER_H
