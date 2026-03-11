/**
 * PDCM — Power Distribution & Control Module
 *
 * Manages vehicle power distribution, driver switch inputs, lighting,
 * relay control, and 4WD for a 1998 Chevy Silverado.
 *
 * Module ID: 0x10
 * CAN Range: 0x350–0x36F
 * Heartbeat: 0x36F @ 500ms
 */

#include <Arduino.h>
#include <FlexCAN_T4.h>

// Platform headers (from silverado-platform submodule)
#include "can/VehicleCAN.h"
#include "can/VehicleMessages.h"
#include "types/VehicleTypes.h"
#include "types/ModuleIDs.h"

// Module-specific config
#include "PDCMConfig.h"

// Vehicle CAN bus — CAN FD
FlexCAN_T4FD<CAN1, RX_SIZE_256, TX_SIZE_64> vehicleCAN;

// --- Timing ---
static uint32_t lastHeartbeat = 0;
static uint32_t lastSwitchScan = 0;
static uint32_t lastLightState = 0;
static uint32_t lastPowerState = 0;
static uint32_t lastACState = 0;
static uint32_t last4WDState = 0;
static uint32_t lastFaults = 0;
static uint32_t lastBrakeState = 0;

// --- CAN IDs (PDCM publishes) ---
static constexpr uint32_t CAN_PDCM_SWITCH_STATES  = 0x350;
static constexpr uint32_t CAN_PDCM_LIGHT_STATE    = 0x351;
static constexpr uint32_t CAN_PDCM_POWER_STATE    = 0x352;
static constexpr uint32_t CAN_PDCM_CRUISE_BUTTON  = 0x353;
static constexpr uint32_t CAN_PDCM_AC_STATE       = 0x354;
static constexpr uint32_t CAN_PDCM_4WD_STATE      = 0x355;
static constexpr uint32_t CAN_PDCM_BRAKE_STATE    = 0x356;
static constexpr uint32_t CAN_PDCM_FAULTS         = 0x357;
static constexpr uint32_t CAN_PDCM_HEARTBEAT      = 0x36F;

// --- CAN IDs (PDCM consumes) ---
static constexpr uint32_t CAN_ECM_FAN_COMMAND      = 0x360;
static constexpr uint32_t CAN_HMI_LIGHT_COMMAND    = 0x361;
static constexpr uint32_t CAN_HMI_4WD_COMMAND      = 0x362;
static constexpr uint32_t CAN_ECM_RELAY_COMMAND     = 0x363;
static constexpr uint32_t CAN_ECM_DRIVE_MODE        = 0x310;

// --- CAN receive callbacks ---

void onFanCommand(const CANFD_message_t &msg) {
    // TODO: Parse fan speed %, drive cooling fan PWM output
    (void)msg;
}

void onLightCommand(const CANFD_message_t &msg) {
    // TODO: Parse lighting mode overrides from HMI
    (void)msg;
}

void on4WDCommand(const CANFD_message_t &msg) {
    // TODO: Parse 4WD mode request, command transfer case encoder motor
    (void)msg;
}

void onRelayCommand(const CANFD_message_t &msg) {
    // TODO: Parse direct relay control commands from ECM
    (void)msg;
}

void onDriveMode(const CANFD_message_t &msg) {
    // TODO: Parse active drive mode (affects fan strategy)
    (void)msg;
}

void setup() {
    Serial.begin(115200);

    // Initialize vehicle CAN bus
    vehicleCAN.begin();
    CANFD_timings_t timing;
    timing.clock = CLK_24MHz;
    timing.baudrate = 1000000;   // 1 Mbps arbitration
    timing.baudrateFD = 8000000; // 8 Mbps data
    timing.propdelay = 190;
    timing.bus_length = 1;
    timing.sample = 87;
    vehicleCAN.setBaudRate(timing);
    vehicleCAN.enableMBInterrupts();

    // TODO: Set up CAN receive mailbox filters for consumed messages
    // TODO: Register CAN receive callbacks
    // TODO: Initialize switch input pins (stalks, hazards, wipers, key position)
    // TODO: Initialize lighting output pins (headlights, turns, brake, reverse, interior)
    // TODO: Initialize relay output pins (fuel pump, fans, A/C, horn, wipers, blower, accessory)
    // TODO: Initialize 4WD outputs (transfer case encoder motor, front axle actuator)
    // TODO: Initialize brake switch inputs (dual BOP)

    Serial.println("PDCM initialized");
}

void loop() {
    vehicleCAN.events();

    uint32_t now = millis();

    // --- Switch scan (100ms) ---
    if (now - lastSwitchScan >= 100) {
        lastSwitchScan = now;
        // TODO: Read all switch inputs
        // TODO: Detect cruise button events (on-event: send 0x353 immediately)
        // TODO: Pack and send 0x350 Switch States
    }

    // --- Light state (100ms) ---
    if (now - lastLightState >= 100) {
        lastLightState = now;
        // TODO: Pack and send 0x351 Light State
    }

    // --- Brake state (50ms — fast for BOP safety) ---
    if (now - lastBrakeState >= 50) {
        lastBrakeState = now;
        // TODO: Read dual brake switches
        // TODO: Pack and send 0x356 Brake State
    }

    // --- Power state (500ms) ---
    if (now - lastPowerState >= 500) {
        lastPowerState = now;
        // TODO: Read relay states and current sensing
        // TODO: Pack and send 0x352 Power State
    }

    // --- A/C state (500ms) ---
    if (now - lastACState >= 500) {
        lastACState = now;
        // TODO: Pack and send 0x354 A/C State
    }

    // --- 4WD state (500ms) ---
    if (now - last4WDState >= 500) {
        last4WDState = now;
        // TODO: Read transfer case position
        // TODO: Pack and send 0x355 4WD State
    }

    // --- Faults (1000ms) ---
    if (now - lastFaults >= 1000) {
        lastFaults = now;
        // TODO: Pack and send 0x357 Faults
    }

    // --- Heartbeat (500ms) ---
    if (now - lastHeartbeat >= 500) {
        lastHeartbeat = now;
        CANFD_message_t msg;
        msg.id = CAN_PDCM_HEARTBEAT;
        msg.len = 2;
        msg.buf[0] = 0x10; // PDCM module ID
        msg.buf[1] = 0x00; // TODO: status flags
        vehicleCAN.write(msg);
    }
}
