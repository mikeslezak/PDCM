/**
 * CANManager.cpp — Vehicle CAN FD Communication Implementation
 */

#include "CANManager.h"
#include "SwitchInput.h"
#include "LightController.h"
#include "PowerManager.h"
#include "FanController.h"
#include "FourWDController.h"
#include "BrakeMonitor.h"
#include "BatteryMonitor.h"
#include "CurrentSense.h"
#include "FaultManager.h"
#include "../hal/HAL.h"

// Platform headers
#include "can/VehicleCAN.h"
#include "can/VehicleMessages.h"
#include "types/VehicleTypes.h"
#include "types/ModuleIDs.h"

// ============================================================================
// CAN Receive Callbacks
// ============================================================================

static void onFanCommand(const HAL_CANMessage& msg) {
    if (msg.len >= 1) {
        FanController::set_duty_from_can(msg.data[0]);
        // Bit 7 of byte 1: force full speed
        if (msg.len >= 2 && (msg.data[1] & 0x80)) {
            FanController::force_full_speed();
        }
    }
    FaultManager::ecm_heartbeat_received();  // Fan command implies ECM alive
}

static void onLightCommand(const HAL_CANMessage& msg) {
    if (msg.len >= 1) {
        // Parse lighting mode from HMI
        LightMode mode = static_cast<LightMode>(msg.data[0]);
        LightController::set_mode(mode);
    }
}

static void on4WDCommand(const HAL_CANMessage& msg) {
    if (msg.len >= 1) {
        TransferCaseMode target = static_cast<TransferCaseMode>(msg.data[0]);
        FourWDController::request_mode(target);
    }
}

static void onRelayCommand(const HAL_CANMessage& msg) {
    if (msg.len >= 1) {
        // Byte 0 bit 7: fuel pump
        PowerManager::fuel_pump_run(msg.data[0] & 0x80);
        // Byte 0 bit 6: A/C clutch
        PowerManager::ac_clutch(msg.data[0] & 0x40);
    }
    FaultManager::ecm_heartbeat_received();  // Relay command implies ECM alive
}

static void onDriveMode(const HAL_CANMessage& msg) {
    if (msg.len >= sizeof(VehMsgDriveMode)) {
        // Drive mode affects fan strategy and other behavior
        // For now, just note that ECM is alive
    }
    FaultManager::ecm_heartbeat_received();
}

static void onECMHeartbeat(const HAL_CANMessage& msg) {
    (void)msg;
    FaultManager::ecm_heartbeat_received();
}

// ============================================================================
// CANManager Implementation
// ============================================================================

namespace CANManager {

void init() {
    HAL::can_set_callback(CAN_VEH_PDCM_FAN_CMD, onFanCommand);
    HAL::can_set_callback(CAN_VEH_PDCM_LIGHT_CMD, onLightCommand);
    HAL::can_set_callback(CAN_VEH_PDCM_4WD_CMD, on4WDCommand);
    HAL::can_set_callback(CAN_VEH_PDCM_RELAY_CMD, onRelayCommand);
    HAL::can_set_callback(CAN_VEH_ECM_DRIVE_MODE, onDriveMode);
    HAL::can_set_callback(CAN_VEH_ECM_HEARTBEAT, onECMHeartbeat);
}

void publish_switch_state() {
    VehMsgSwitchState msg = {};

    // Pack left stalk
    msg.stalk_left = 0;
    if (SwitchInput::turn_left())       msg.stalk_left |= 0x01;
    if (SwitchInput::turn_right())      msg.stalk_left |= 0x02;
    if (SwitchInput::high_beam())       msg.stalk_left |= 0x04;
    if (SwitchInput::flash_to_pass())   msg.stalk_left |= 0x08;

    // Pack right stalk (cruise events sent separately via 0x353)
    msg.stalk_right = 0;

    // Pack hazards/horn
    msg.hazards = 0;
    if (SwitchInput::hazard())  msg.hazards |= 0x01;
    if (SwitchInput::horn())    msg.hazards |= 0x02;

    // Wipers
    msg.wipers = static_cast<uint8_t>(SwitchInput::wiper_mode());

    // HVAC
    msg.hvac_fan = SwitchInput::hvac_fan_speed();
    msg.hvac_mode = SwitchInput::hvac_mode();

    // Key position
    msg.key_position = static_cast<uint8_t>(SwitchInput::key_position());

    HAL::can_send(CAN_VEH_PDCM_SWITCH_STATE, (const uint8_t*)&msg, sizeof(msg));
}

void publish_light_state() {
    uint8_t data[4] = {};

    // Byte 0: headlights and signals
    if (LightController::is_low_beam_on())  { data[0] |= 0x80; data[0] |= 0x40; }  // L+R
    if (LightController::is_high_beam_on()) { data[0] |= 0x20; data[0] |= 0x10; }  // L+R
    if (LightController::is_turn_left_on())  data[0] |= 0x08;
    if (LightController::is_turn_right_on()) data[0] |= 0x04;
    if (LightController::is_brake_on())     { data[0] |= 0x02; data[0] |= 0x01; }  // L+R

    // Byte 1: other lights
    if (LightController::is_reverse_on())    data[1] |= 0x80;
    if (LightController::is_interior_on())   data[1] |= 0x40;
    if (LightController::is_courtesy_on())   data[1] |= 0x20;
    if (LightController::is_drl_on())        data[1] |= 0x10;

    HAL::can_send(CAN_VEH_PDCM_LIGHT_STATE, data, 4);
}

void publish_power_state() {
    uint8_t data[8] = {};

    // Byte 0: relay states
    if (PowerManager::fuel_pump_on())   data[0] |= 0x80;
    if (FanController::get_duty() > 0)  data[0] |= 0x40;  // Fan 1
    // Fan 2 same as fan 1 for now
    if (PowerManager::ac_clutch_on())   data[0] |= 0x10;
    if (PowerManager::horn_on())        data[0] |= 0x08;
    if (PowerManager::wiper_on())       data[0] |= 0x04;
    if (PowerManager::blower_duty()>0)  data[0] |= 0x02;
    if (PowerManager::accessory_on())   data[0] |= 0x01;

    // Byte 1: fan duty (0-255)
    data[1] = (uint8_t)((FanController::get_duty() * 255) / 1000);

    // Bytes 2-3: total current (mA, uint16_t LE)
    uint32_t totalMA = CurrentSense::get_total_current_mA();
    if (totalMA > 65535) totalMA = 65535;
    data[2] = (uint8_t)(totalMA & 0xFF);
    data[3] = (uint8_t)((totalMA >> 8) & 0xFF);

    // Bytes 4-5: battery voltage (mV, uint16_t LE)
    uint16_t batt = BatteryMonitor::voltage_mv();
    data[4] = (uint8_t)(batt & 0xFF);
    data[5] = (uint8_t)((batt >> 8) & 0xFF);

    HAL::can_send(CAN_VEH_PDCM_POWER_STATE, data, 8);
}

void publish_cruise_event() {
    VehMsgCruiseBtn msg = {};

    if (SwitchInput::cruise_set_pressed())       msg.event = 0;
    else if (SwitchInput::cruise_resume_pressed()) msg.event = 1;
    else if (SwitchInput::cruise_accel_pressed())  msg.event = 2;
    else if (SwitchInput::cruise_decel_pressed())  msg.event = 3;
    else if (SwitchInput::cruise_cancel_pressed()) msg.event = 4;
    else if (SwitchInput::cruise_onoff_pressed())  msg.event = 5;
    else return;  // No event

    HAL::can_send(CAN_VEH_PDCM_CRUISE_BTN, (const uint8_t*)&msg, sizeof(msg));
}

void publish_ac_state() {
    uint8_t data[4] = {};

    if (PowerManager::ac_clutch_on())   data[0] |= 0x80;
    if (PowerManager::ac_request())     data[0] |= 0x40;
    // Low pressure cutout — future sensor input
    // data[0] |= 0x20;

    HAL::can_send(CAN_VEH_PDCM_AC_STATE, data, 4);
}

void publish_4wd_state() {
    uint8_t data[4] = {};

    // Byte 0: mode (lower nibble)
    data[0] = static_cast<uint8_t>(FourWDController::current_mode()) & 0x0F;

    // Byte 1: status flags
    if (FourWDController::is_engaged())             data[1] |= 0x80;
    if (FourWDController::is_shifting())            data[1] |= 0x40;
    if (FourWDController::is_fault())               data[1] |= 0x20;
    if (FourWDController::is_front_axle_engaged())  data[1] |= 0x10;

    HAL::can_send(CAN_VEH_PDCM_4WD_STATE, data, 4);
}

void publish_brake_state() {
    uint8_t data[4] = {};

    if (BrakeMonitor::switch_1())   data[0] |= 0x80;
    if (BrakeMonitor::switch_2())   data[0] |= 0x40;
    if (BrakeMonitor::bop_active()) data[0] |= 0x20;
    if (BrakeMonitor::disagree())   data[0] |= 0x10;

    HAL::can_send(CAN_VEH_PDCM_BRAKE_STATE, data, 4);
}

void publish_faults() {
    uint8_t data[8] = {};

    data[0] = FaultManager::fault_count();
    data[1] = FaultManager::get_fault_bitmask();

    HAL::can_send(CAN_VEH_PDCM_FAULTS, data, 8);
}

void publish_heartbeat() {
    VehMsgHeartbeat msg = {};
    msg.module_id = ModuleID::PDCM;
    msg.status = FaultManager::fault_count() > 0 ?
        static_cast<uint8_t>(HeartbeatStatus::WARNING) :
        static_cast<uint8_t>(HeartbeatStatus::OK);

    HAL::can_send(CAN_VEH_PDCM_HEARTBEAT, (const uint8_t*)&msg, sizeof(msg));
}

bool has_cruise_event() {
    return SwitchInput::any_cruise_event();
}

} // namespace CANManager
