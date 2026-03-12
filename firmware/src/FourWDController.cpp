/**
 * FourWDController.cpp — NP246 Transfer Case State Machine Implementation
 */

#include "FourWDController.h"
#include "HBridge.h"
#include "GateDriver.h"
#include "../hal/HAL.h"
#include "../shared/PDCMConfig.h"
#include "../shared/PDCMTypes.h"

static TransferCaseMode curMode = TransferCaseMode::TWO_HI;
static TransferCaseMode tgtMode = TransferCaseMode::TWO_HI;
static FourWDState curState = FourWDState::IDLE;
static uint32_t shiftStartMs = 0;
static uint16_t wheelSpeedX10 = 0;      // km/h × 10
static bool frontAxleEngaged = false;

// Read current transfer case position from ADC
static TransferCaseMode readPosition() {
#ifdef PDCM_TARGET_TEENSY
    uint16_t mv = HAL::adc_read_mv(Pin::ADC_4WD_POS);

    if (mv < FourWDPosADC::POS_2HI_MV + FourWDPosADC::POS_TOLERANCE_MV)
        return TransferCaseMode::TWO_HI;
    if (mv > FourWDPosADC::POS_A4WD_MV - FourWDPosADC::POS_TOLERANCE_MV &&
        mv < FourWDPosADC::POS_A4WD_MV + FourWDPosADC::POS_TOLERANCE_MV)
        return TransferCaseMode::AUTO_4WD;
    if (mv > FourWDPosADC::POS_4HI_MV - FourWDPosADC::POS_TOLERANCE_MV &&
        mv < FourWDPosADC::POS_4HI_MV + FourWDPosADC::POS_TOLERANCE_MV)
        return TransferCaseMode::FOUR_HI;
    if (mv > FourWDPosADC::POS_NEUTRAL_MV - FourWDPosADC::POS_TOLERANCE_MV &&
        mv < FourWDPosADC::POS_NEUTRAL_MV + FourWDPosADC::POS_TOLERANCE_MV)
        return TransferCaseMode::NEUTRAL;
    if (mv > FourWDPosADC::POS_4LO_MV - FourWDPosADC::POS_TOLERANCE_MV)
        return TransferCaseMode::FOUR_LO;
#endif

    // Position not recognized — return current mode
    return curMode;
}

// Determine motor direction needed to reach target mode
static HBridgeDir getShiftDirection(TransferCaseMode from, TransferCaseMode to) {
    // Position order: 2HI(0V) → A4WD → 4HI → Neutral → 4LO(5V)
    uint8_t fromIdx = static_cast<uint8_t>(from);
    uint8_t toIdx = static_cast<uint8_t>(to);
    if (toIdx > fromIdx) return HBridgeDir::FORWARD;
    if (toIdx < fromIdx) return HBridgeDir::REVERSE;
    return HBridgeDir::BRAKE;
}

namespace FourWDController {

void init() {
    curMode = TransferCaseMode::TWO_HI;
    tgtMode = TransferCaseMode::TWO_HI;
    curState = FourWDState::IDLE;
    shiftStartMs = 0;
    wheelSpeedX10 = 0;
    frontAxleEngaged = false;

    HBridge::init();
}

void update() {
    uint32_t now = HAL::millis();
    TransferCaseMode pos = readPosition();

    switch (curState) {
        case FourWDState::IDLE:
            curMode = pos;
            HBridge::stop();

            // Manage front axle actuator
            // Engage front axle for any 4WD mode
            if (curMode == TransferCaseMode::AUTO_4WD ||
                curMode == TransferCaseMode::FOUR_HI ||
                curMode == TransferCaseMode::FOUR_LO) {
                if (!frontAxleEngaged) {
                    GateDriver::set(OutputChannel::FRONT_AXLE, true);
                    frontAxleEngaged = true;
                }
            } else {
                if (frontAxleEngaged) {
                    GateDriver::set(OutputChannel::FRONT_AXLE, false);
                    frontAxleEngaged = false;
                }
            }

            // Check if a mode change was requested
            if (tgtMode != curMode) {
                // Safety: inhibit 4LO above 3 mph (30 in x10)
                if (tgtMode == TransferCaseMode::FOUR_LO && wheelSpeedX10 > 30) {
                    tgtMode = curMode;  // Reject request
                    break;
                }

                // Start shift
                HBridgeDir dir = getShiftDirection(curMode, tgtMode);
                HBridge::wake();
                HBridge::set(dir, 1000);  // Full speed
                curState = FourWDState::SHIFTING;
                shiftStartMs = now;
            }
            break;

        case FourWDState::SHIFTING:
            // Check if we reached the target position
            if (pos == tgtMode) {
                HBridge::stop();
                curMode = pos;
                curState = FourWDState::IDLE;
                break;
            }

            // Check for motor timeout
            if (now - shiftStartMs > PDCMTiming::TC_MOTOR_TIMEOUT_MS) {
                HBridge::stop();
                curMode = pos;  // Report whatever position we ended at
                curState = FourWDState::STALLED;
                break;
            }

            // Check for hardware fault
            if (HBridge::is_fault()) {
                HBridge::stop();
                curMode = pos;
                curState = FourWDState::FAULT;
                break;
            }

            // Check for motor stall (high current, no position change)
            if (HBridge::get_current_mA() > 3000) {
                HBridge::stop();
                curMode = pos;
                curState = FourWDState::STALLED;
                break;
            }
            break;

        case FourWDState::STALLED:
        case FourWDState::FAULT:
            HBridge::stop();
            curMode = pos;
            // Stay in fault state until new mode request clears it
            if (tgtMode == curMode) {
                curState = FourWDState::IDLE;
            }
            break;
    }
}

void request_mode(TransferCaseMode target) {
    tgtMode = target;
    // If in fault state, requesting the current position clears the fault
    if (curState == FourWDState::STALLED || curState == FourWDState::FAULT) {
        if (target == curMode) {
            curState = FourWDState::IDLE;
        }
    }
}

void set_wheel_speed_kph(uint16_t speed_x10) {
    wheelSpeedX10 = speed_x10;
}

TransferCaseMode current_mode() { return curMode; }
TransferCaseMode target_mode() { return tgtMode; }
FourWDState state() { return curState; }

bool is_shifting() { return curState == FourWDState::SHIFTING; }
bool is_engaged() { return curState == FourWDState::IDLE && curMode == tgtMode; }
bool is_fault() { return curState == FourWDState::FAULT || curState == FourWDState::STALLED; }
bool is_front_axle_engaged() { return frontAxleEngaged; }

} // namespace FourWDController
