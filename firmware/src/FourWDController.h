#ifndef FOUR_WD_CONTROLLER_H
#define FOUR_WD_CONTROLLER_H

/**
 * FourWDController — NP246 Transfer Case State Machine
 *
 * Controls the NP246 transfer case via DRV8876 H-bridge (encoder motor)
 * and front axle actuator (single MOSFET channel).
 *
 * State machine: IDLE → SHIFTING → (IDLE or STALLED or FAULT)
 *
 * Modes: 2HI / A4WD / 4HI / NEUTRAL / 4LO
 * Position feedback via potentiometer on ADC.
 *
 * Safety:
 *   - Motor timeout: 5s max per shift attempt
 *   - Stall current detection via DRV8876
 *   - 4LO inhibited above 3 mph (from ECM wheel speed)
 */

#include "../shared/PDCMTypes.h"
#include "types/VehicleTypes.h"

namespace FourWDController {

    void init();

    // Main update (call at 500ms rate)
    void update();

    // Request mode change (from HMI command 0x362)
    void request_mode(TransferCaseMode target);

    // Update wheel speed (from ECM, for 4LO inhibit)
    void set_wheel_speed_kph(uint16_t speed_x10);

    // Current state
    TransferCaseMode current_mode();
    TransferCaseMode target_mode();
    FourWDState state();

    // Status flags
    bool is_shifting();
    bool is_engaged();
    bool is_fault();
    bool is_front_axle_engaged();
}

#endif // FOUR_WD_CONTROLLER_H
