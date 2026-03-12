/**
 * PDCM — Power Distribution & Control Module
 *
 * Full solid-state power distribution for 1998 Chevy Silverado.
 * 47 TC4427A-driven MOSFET channels + 1 DRV8876 H-bridge.
 * Per-channel current sensing, software overcurrent protection.
 *
 * Module ID: 0x10
 * CAN Range: 0x350–0x36F
 * Heartbeat: 0x36F @ 500ms
 *
 * Target: Teensy 4.1 (prototype) / NXP S32K358 (production)
 * See ADR-001 through ADR-008 in DECISIONS.md
 */

#include <Arduino.h>

#include "../hal/HAL.h"
#include "../shared/PDCMConfig.h"
#include "../shared/PDCMTypes.h"

// Layer 0 — Hardware Drivers
#include "GateDriver.h"
#include "CurrentSense.h"
#include "HBridge.h"

// Layer 1 — Input Processing
#include "SwitchInput.h"
#include "BrakeMonitor.h"
#include "BatteryMonitor.h"

// Layer 2 — Output Control
#include "LightController.h"
#include "PowerManager.h"
#include "FanController.h"
#include "FourWDController.h"

// Layer 3 — System Services
#include "FaultManager.h"
#include "LoadShedder.h"

// Layer 4 — Communication
#include "CANManager.h"

// ============================================================================
// Timing state
// ============================================================================

static uint32_t lastWatchdog     = 0;
static uint32_t lastCurrentScan  = 0;
static uint32_t lastSwitchInput  = 0;
static uint32_t lastBrakeMonitor = 0;
static uint32_t lastBrakeCAN     = 0;
static uint32_t lastSwitchCAN    = 0;
static uint32_t lastLightCAN     = 0;
static uint32_t lastLightUpdate  = 0;
static uint32_t lastPowerCAN     = 0;
static uint32_t lastACCAN        = 0;
static uint32_t last4WDCAN       = 0;
static uint32_t lastHeartbeat    = 0;
static uint32_t lastPowerUpdate  = 0;
static uint32_t lastFaultCAN     = 0;
static uint32_t lastLoadShed     = 0;
static uint32_t lastBatteryCheck = 0;

// ============================================================================
// Power-On Self-Test
// ============================================================================

static void runPOST() {
    HAL::serial_println("PDCM POST starting...");

    // Check battery voltage is reasonable
    BatteryMonitor::update();
    uint16_t batt = BatteryMonitor::voltage_mv();
    if (batt < 9000 || batt > 16000) {
        HAL::serial_print("POST WARNING: Battery voltage ");
        // Simple integer print
        char buf[8];
        uint16_t v = batt / 1000;
        uint16_t frac = (batt % 1000) / 100;
        buf[0] = '0' + (v / 10);
        buf[1] = '0' + (v % 10);
        buf[2] = '.';
        buf[3] = '0' + frac;
        buf[4] = 'V';
        buf[5] = '\0';
        HAL::serial_println(buf);
    }

    // Verify H-bridge is not in fault state
    if (HBridge::is_fault()) {
        HAL::serial_println("POST WARNING: H-bridge fault detected");
    }

    // Verify all gate drivers are OFF (no stuck-on channels)
    // Brief delay for current sense to stabilize
    HAL::delay_ms(50);
    CurrentSense::scan_step();  // Quick partial scan
    int8_t stuck = CurrentSense::check_stuck_on();
    if (stuck >= 0) {
        HAL::serial_print("POST FAULT: Stuck-on detected on channel ");
        char ch[4] = { (char)('0' + stuck / 10), (char)('0' + stuck % 10), '\0', '\0' };
        HAL::serial_println(ch);
    }

    HAL::serial_println("PDCM POST complete");
}

// ============================================================================
// Arduino entry points
// ============================================================================

void setup() {
    // --- Initialize HAL (serial, SPI, CAN) ---
    HAL::init();
    HAL::serial_println("PDCM initializing...");

    // --- Initialize in dependency order ---

    // Layer 0: Hardware drivers
    GateDriver::init();         // All outputs OFF
    CurrentSense::init();       // MUX + ADC
    HBridge::init();            // DRV8876

    // Layer 1: Input processing
    SwitchInput::init();        // Port expander + ADC
    BrakeMonitor::init();       // Direct GPIO
    BatteryMonitor::init();     // ADC

    // Layer 2: Output control
    LightController::init();
    PowerManager::init();
    FanController::init();
    FourWDController::init();

    // Layer 3: System services
    FaultManager::init();
    LoadShedder::init();

    // Layer 4: Communication
    CANManager::init();         // Register CAN callbacks

    // Watchdog
    HAL::watchdog_init(500);    // 500ms timeout

    // Power-on self-test
    runPOST();

    HAL::serial_println("PDCM initialized — 47ch solid-state power distribution");
}

void loop() {
    // Process incoming CAN messages
    HAL::can_poll();

    uint32_t now = HAL::millis();

    // --- 1ms: Watchdog feed ---
    if (now - lastWatchdog >= PDCMTiming::RATE_WATCHDOG) {
        lastWatchdog = now;
        HAL::watchdog_feed();
    }

    // --- 2ms: Current sense scan (one MUX step per call) ---
    if (now - lastCurrentScan >= PDCMTiming::RATE_CURRENT_SCAN) {
        lastCurrentScan = now;
        CurrentSense::scan_step();
    }

    // --- 10ms: Switch inputs ---
    if (now - lastSwitchInput >= PDCMTiming::RATE_SWITCH_INPUT) {
        lastSwitchInput = now;
        SwitchInput::update();

        // Process switch inputs → lighting mode
        if (SwitchInput::high_beam()) {
            LightController::set_mode(LightMode::HIGH_BEAM);
        } else if (SwitchInput::flash_to_pass()) {
            LightController::set_mode(LightMode::FLASH_TO_PASS);
        }
        // Note: headlight on/off controlled by headlight switch (not stalk)
        // DRL auto-enabled when engine running + headlights off

        LightController::set_turn_left(SwitchInput::turn_left());
        LightController::set_turn_right(SwitchInput::turn_right());
        LightController::set_hazard(SwitchInput::hazard());

        // Blower PWM from HVAC fan speed
        uint16_t blower = ((uint16_t)SwitchInput::hvac_fan_speed() * 1000) / 7;
        PowerManager::set_blower_duty(blower);

        // Cruise events — publish immediately on-event
        if (CANManager::has_cruise_event()) {
            CANManager::publish_cruise_event();
        }
    }

    // --- 10ms: Brake monitor ---
    if (now - lastBrakeMonitor >= PDCMTiming::RATE_BRAKE_MONITOR) {
        lastBrakeMonitor = now;
        BrakeMonitor::update();
    }

    // --- 50ms: Brake state CAN TX (safety-critical, fastest rate) ---
    if (now - lastBrakeCAN >= PDCMTiming::RATE_BRAKE_CAN_TX) {
        lastBrakeCAN = now;
        CANManager::publish_brake_state();
    }

    // --- 100ms: Switch state CAN TX ---
    if (now - lastSwitchCAN >= PDCMTiming::RATE_SWITCH_CAN_TX) {
        lastSwitchCAN = now;
        CANManager::publish_switch_state();
    }

    // --- 100ms: Light controller update + CAN TX ---
    if (now - lastLightUpdate >= PDCMTiming::RATE_LIGHT_UPDATE) {
        lastLightUpdate = now;
        LightController::update();
    }
    if (now - lastLightCAN >= PDCMTiming::RATE_LIGHT_CAN_TX) {
        lastLightCAN = now;
        CANManager::publish_light_state();
    }

    // --- 500ms: Power state CAN TX + updates ---
    if (now - lastPowerUpdate >= PDCMTiming::RATE_POWER_UPDATE) {
        lastPowerUpdate = now;
        PowerManager::update();
        FanController::update();
        FourWDController::update();
    }
    if (now - lastPowerCAN >= PDCMTiming::RATE_POWER_CAN_TX) {
        lastPowerCAN = now;
        CANManager::publish_power_state();
    }
    if (now - lastACCAN >= PDCMTiming::RATE_AC_CAN_TX) {
        lastACCAN = now;
        CANManager::publish_ac_state();
    }
    if (now - last4WDCAN >= PDCMTiming::RATE_4WD_CAN_TX) {
        last4WDCAN = now;
        CANManager::publish_4wd_state();
    }

    // --- 500ms: Heartbeat ---
    if (now - lastHeartbeat >= PDCMTiming::RATE_HEARTBEAT) {
        lastHeartbeat = now;
        CANManager::publish_heartbeat();
    }

    // --- 1000ms: Fault state CAN TX + system checks ---
    if (now - lastFaultCAN >= PDCMTiming::RATE_FAULT_CAN_TX) {
        lastFaultCAN = now;
        FaultManager::update();
        CANManager::publish_faults();
    }
    if (now - lastLoadShed >= PDCMTiming::RATE_LOAD_SHED) {
        lastLoadShed = now;
        LoadShedder::update();
    }
    if (now - lastBatteryCheck >= PDCMTiming::RATE_BATTERY_CHECK) {
        lastBatteryCheck = now;
        BatteryMonitor::update();
    }
}
