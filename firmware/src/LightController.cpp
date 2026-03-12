/**
 * LightController.cpp — Lighting Output Implementation
 */

#include "LightController.h"
#include "GateDriver.h"
#include "BrakeMonitor.h"
#include "SwitchInput.h"
#include "../hal/HAL.h"
#include "../shared/PDCMTypes.h"

static LightMode currentMode = LightMode::OFF;

// Turn signal state machine
static bool turnLeftReq = false;
static bool turnRightReq = false;
static bool hazardReq = false;
static bool turnOutput = false;         // Current flash state (on/off)
static uint32_t lastFlashToggle = 0;

// Soft-start ramp
static uint16_t headlightRamp = 0;     // Current ramp level (0-1000)
static constexpr uint16_t RAMP_STEP = 50;   // Step per 100ms update = 500ms to full

// Interior fade
static uint16_t interiorLevel = 0;
static uint16_t interiorTarget = 0;
static uint16_t courtesyLevel = 0;
static uint16_t courtesyTarget = 0;
static constexpr uint16_t FADE_STEP = 40;   // Step per 100ms = ~2.5s fade

// Welcome/goodbye
static bool welcomeActive = false;
static bool goodbyeActive = false;
static uint32_t sequenceStart = 0;

namespace LightController {

void init() {
    currentMode = LightMode::OFF;
    turnLeftReq = false;
    turnRightReq = false;
    hazardReq = false;
    turnOutput = false;
    lastFlashToggle = 0;
    headlightRamp = 0;
    interiorLevel = 0;
    interiorTarget = 0;
    courtesyLevel = 0;
    courtesyTarget = 0;
    welcomeActive = false;
    goodbyeActive = false;
}

void update() {
    uint32_t now = HAL::millis();

    // --- Turn signal flash timer ---
    if (turnLeftReq || turnRightReq || hazardReq) {
        uint32_t period = turnOutput ? PDCMTiming::TURN_SIGNAL_ON_MS
                                     : PDCMTiming::TURN_SIGNAL_OFF_MS;
        if (now - lastFlashToggle >= period) {
            turnOutput = !turnOutput;
            lastFlashToggle = now;
        }
    } else {
        turnOutput = false;
    }

    // Apply turn signal outputs
    bool leftOn  = turnOutput && (turnLeftReq || hazardReq);
    bool rightOn = turnOutput && (turnRightReq || hazardReq);
    GateDriver::set(OutputChannel::TURN_L, leftOn);
    GateDriver::set(OutputChannel::TURN_R, rightOn);

    // --- Headlight control with soft-start ---
    bool wantHeadlights = (currentMode == LightMode::LOW_BEAM ||
                           currentMode == LightMode::HIGH_BEAM);
    bool wantHighBeam = (currentMode == LightMode::HIGH_BEAM ||
                         currentMode == LightMode::FLASH_TO_PASS);

    if (wantHeadlights) {
        if (headlightRamp < 1000) {
            headlightRamp += RAMP_STEP;
            if (headlightRamp > 1000) headlightRamp = 1000;
        }
    } else {
        headlightRamp = 0;  // Instant off
    }

    GateDriver::set_pwm(OutputChannel::LOW_BEAM_L, wantHeadlights ? headlightRamp : 0);
    GateDriver::set_pwm(OutputChannel::LOW_BEAM_R, wantHeadlights ? headlightRamp : 0);
    GateDriver::set_pwm(OutputChannel::HIGH_BEAM_L, wantHighBeam ? headlightRamp : 0);
    GateDriver::set_pwm(OutputChannel::HIGH_BEAM_R, wantHighBeam ? headlightRamp : 0);

    // --- DRL ---
    bool drlOn = (currentMode == LightMode::DRL);
    GateDriver::set_pwm(OutputChannel::DRL, drlOn ? 400 : 0);  // 40% brightness

    // --- Brake lights (direct from brake switch — no CAN dependency) ---
    bool braking = BrakeMonitor::brake_pressed();
    GateDriver::set(OutputChannel::BRAKE_L, braking);
    GateDriver::set(OutputChannel::BRAKE_R, braking);

    // --- Reverse lights ---
    GateDriver::set(OutputChannel::REVERSE, SwitchInput::reverse_switch());

    // --- Interior/courtesy fade ---
    if (interiorLevel < interiorTarget) {
        interiorLevel += FADE_STEP;
        if (interiorLevel > interiorTarget) interiorLevel = interiorTarget;
    } else if (interiorLevel > interiorTarget) {
        if (interiorLevel >= FADE_STEP)
            interiorLevel -= FADE_STEP;
        else
            interiorLevel = 0;
    }
    GateDriver::set_pwm(OutputChannel::INTERIOR, interiorLevel);

    if (courtesyLevel < courtesyTarget) {
        courtesyLevel += FADE_STEP;
        if (courtesyLevel > courtesyTarget) courtesyLevel = courtesyTarget;
    } else if (courtesyLevel > courtesyTarget) {
        if (courtesyLevel >= FADE_STEP)
            courtesyLevel -= FADE_STEP;
        else
            courtesyLevel = 0;
    }
    GateDriver::set_pwm(OutputChannel::COURTESY, courtesyLevel);

    // --- Welcome sequence ---
    if (welcomeActive) {
        uint32_t elapsed = now - sequenceStart;
        if (elapsed < PDCMTiming::WELCOME_DURATION_MS) {
            // Ramp up interior + courtesy + DRL
            interiorTarget = 1000;
            courtesyTarget = 1000;
            GateDriver::set_pwm(OutputChannel::DRL, 400);
        } else {
            welcomeActive = false;
            // Restore normal state (interior off unless requested)
        }
    }

    // --- Goodbye sequence ---
    if (goodbyeActive) {
        uint32_t elapsed = now - sequenceStart;
        if (elapsed < PDCMTiming::GOODBYE_DURATION_MS) {
            // Keep courtesy lights on, fade after delay
            courtesyTarget = 1000;
            if (elapsed > 5000) {
                courtesyTarget = 0;  // Start fading at 5s
            }
        } else {
            goodbyeActive = false;
            courtesyTarget = 0;
        }
    }
}

void set_mode(LightMode mode) { currentMode = mode; }
LightMode get_mode() { return currentMode; }

void set_turn_left(bool on) { turnLeftReq = on; }
void set_turn_right(bool on) { turnRightReq = on; }
void set_hazard(bool on) { hazardReq = on; }

void set_interior(bool on) { interiorTarget = on ? 1000 : 0; }
void set_courtesy(bool on) { courtesyTarget = on ? 1000 : 0; }

void trigger_welcome() {
    welcomeActive = true;
    goodbyeActive = false;
    sequenceStart = HAL::millis();
}

void trigger_goodbye() {
    goodbyeActive = true;
    welcomeActive = false;
    sequenceStart = HAL::millis();
}

bool is_low_beam_on()   { return headlightRamp > 0 &&
                                  (currentMode == LightMode::LOW_BEAM ||
                                   currentMode == LightMode::HIGH_BEAM); }
bool is_high_beam_on()  { return headlightRamp > 0 &&
                                  (currentMode == LightMode::HIGH_BEAM ||
                                   currentMode == LightMode::FLASH_TO_PASS); }
bool is_turn_left_on()  { return GateDriver::is_on(OutputChannel::TURN_L); }
bool is_turn_right_on() { return GateDriver::is_on(OutputChannel::TURN_R); }
bool is_brake_on()      { return BrakeMonitor::brake_pressed(); }
bool is_reverse_on()    { return GateDriver::is_on(OutputChannel::REVERSE); }
bool is_drl_on()        { return currentMode == LightMode::DRL && GateDriver::get_duty(OutputChannel::DRL) > 0; }
bool is_interior_on()   { return interiorLevel > 0; }
bool is_courtesy_on()   { return courtesyLevel > 0; }

} // namespace LightController
