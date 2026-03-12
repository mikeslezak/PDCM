/**
 * SwitchInput.cpp — Driver Switch Reading Implementation
 */

#include "SwitchInput.h"
#include "../hal/HAL.h"
#include "../shared/PDCMConfig.h"

// Debounce: 3-sample majority filter
struct DebouncedInput {
    uint8_t history;    // 3-bit shift register
    bool    state;      // Debounced state
};

// Switch states
static DebouncedInput sw_turn_left;
static DebouncedInput sw_turn_right;
static DebouncedInput sw_high_beam;
static DebouncedInput sw_flash;
static DebouncedInput sw_hazard;
static DebouncedInput sw_horn;
static DebouncedInput sw_reverse;
static DebouncedInput sw_ac_request;
static DebouncedInput sw_washer;

// Wiper switches (decoded from multiple inputs)
static WiperMode currentWiperMode = WiperMode::OFF;

// Key position
static KeyPosition currentKeyPos = KeyPosition::OFF;

// Cruise stalk edge detection
static DebouncedInput sw_cruise_set;
static DebouncedInput sw_cruise_resume;
static DebouncedInput sw_cruise_accel;
static DebouncedInput sw_cruise_decel;
static DebouncedInput sw_cruise_cancel;
static DebouncedInput sw_cruise_onoff;

static bool cruise_set_edge = false;
static bool cruise_resume_edge = false;
static bool cruise_accel_edge = false;
static bool cruise_decel_edge = false;
static bool cruise_cancel_edge = false;
static bool cruise_onoff_edge = false;

// HVAC
static uint8_t hvacFan = 0;
static uint8_t hvacMode = 0;

// Debounce helper: shift in new sample, return majority of last 3
static bool debounce(DebouncedInput& input, bool raw) {
    input.history = ((input.history << 1) | (raw ? 1 : 0)) & 0x07;
    // Majority of 3 bits: at least 2 of 3 must be set
    uint8_t count = 0;
    if (input.history & 0x01) count++;
    if (input.history & 0x02) count++;
    if (input.history & 0x04) count++;
    bool prev = input.state;
    input.state = (count >= 2);
    return (input.state && !prev);  // Returns true on rising edge
}

static void clearDebounce(DebouncedInput& input) {
    input.history = 0;
    input.state = false;
}

namespace SwitchInput {

void init() {
#ifdef PDCM_TARGET_TEENSY
    // MCP23S17 #1 already initialized in CurrentSense::init()
    // Port A: all inputs with pullups (switches pull to GND)
    // Port B[0:3]: wiper switch inputs with pullups
    // Port B[4:7]: MUX select outputs (handled by CurrentSense)
#endif

    clearDebounce(sw_turn_left);
    clearDebounce(sw_turn_right);
    clearDebounce(sw_high_beam);
    clearDebounce(sw_flash);
    clearDebounce(sw_hazard);
    clearDebounce(sw_horn);
    clearDebounce(sw_reverse);
    clearDebounce(sw_ac_request);
    clearDebounce(sw_washer);
    clearDebounce(sw_cruise_set);
    clearDebounce(sw_cruise_resume);
    clearDebounce(sw_cruise_accel);
    clearDebounce(sw_cruise_decel);
    clearDebounce(sw_cruise_cancel);
    clearDebounce(sw_cruise_onoff);
}

void update() {
    // Clear edge flags
    cruise_set_edge = false;
    cruise_resume_edge = false;
    cruise_accel_edge = false;
    cruise_decel_edge = false;
    cruise_cancel_edge = false;
    cruise_onoff_edge = false;

#ifdef PDCM_TARGET_TEENSY
    // Read MCP23S17 #1 Port A (stalk + button switches)
    uint8_t portA = HAL::expander_read_port(Pin::SPI_CS_EXPANDER1, 0);

    // Switches are active-low (pulled to GND when pressed)
    bool raw_turn_l     = !(portA & (1 << Exp1PortA::STALK_LEFT_TURN_L));
    bool raw_turn_r     = !(portA & (1 << Exp1PortA::STALK_LEFT_TURN_R));
    bool raw_high_beam  = !(portA & (1 << Exp1PortA::STALK_LEFT_HIGH_BEAM));
    bool raw_flash      = !(portA & (1 << Exp1PortA::STALK_LEFT_FLASH));
    bool raw_hazard     = !(portA & (1 << Exp1PortA::HAZARD_SW));
    bool raw_horn       = !(portA & (1 << Exp1PortA::HORN_SW));
    bool raw_reverse    = !(portA & (1 << Exp1PortA::REVERSE_SW));
    bool raw_ac_req     = !(portA & (1 << Exp1PortA::AC_REQUEST));

    debounce(sw_turn_left, raw_turn_l);
    debounce(sw_turn_right, raw_turn_r);
    debounce(sw_high_beam, raw_high_beam);
    debounce(sw_flash, raw_flash);
    debounce(sw_hazard, raw_hazard);
    debounce(sw_horn, raw_horn);
    debounce(sw_reverse, raw_reverse);
    debounce(sw_ac_request, raw_ac_req);

    // Read Port B lower nibble (wiper switches)
    uint8_t portB = HAL::expander_read_port(Pin::SPI_CS_EXPANDER1, 1);
    bool wiper_int  = !(portB & (1 << Exp1PortB::WIPER_INT));
    bool wiper_low  = !(portB & (1 << Exp1PortB::WIPER_LOW));
    bool wiper_high = !(portB & (1 << Exp1PortB::WIPER_HIGH));
    bool raw_washer = !(portB & (1 << Exp1PortB::WASHER));

    debounce(sw_washer, raw_washer);

    // Decode wiper mode (priority: high > low > intermittent > off)
    if (raw_washer)     currentWiperMode = WiperMode::WASH;
    else if (wiper_high) currentWiperMode = WiperMode::SPEED_HIGH;
    else if (wiper_low)  currentWiperMode = WiperMode::SPEED_LOW;
    else if (wiper_int)  currentWiperMode = WiperMode::INTERMITTENT;
    else                 currentWiperMode = WiperMode::OFF;

    // Key position via ADC resistor ladder
    uint16_t key_mv = HAL::adc_read_mv(Pin::ADC_KEY_POS);
    if (key_mv > KeyPosADC::THRESHOLD_RUN_START)
        currentKeyPos = KeyPosition::START;
    else if (key_mv > KeyPosADC::THRESHOLD_ACC_RUN)
        currentKeyPos = KeyPosition::RUN;
    else if (key_mv > KeyPosADC::THRESHOLD_OFF_ACC)
        currentKeyPos = KeyPosition::ACCESSORY;
    else
        currentKeyPos = KeyPosition::OFF;

    // TODO: Read cruise stalk inputs (right stalk)
    // For now, cruise buttons are on the right multifunction stalk
    // These would be additional MCP23S17 inputs or dedicated GPIO
    // Edge detection for cruise events:
    // cruise_set_edge = debounce(sw_cruise_set, raw_cruise_set);
    // etc.
#endif

    // HVAC fan speed and mode would come from additional ADC or digital inputs
    // TODO: Implement HVAC control reading
}

bool turn_left()        { return sw_turn_left.state; }
bool turn_right()       { return sw_turn_right.state; }
bool high_beam()        { return sw_high_beam.state; }
bool flash_to_pass()    { return sw_flash.state; }
bool hazard()           { return sw_hazard.state; }
bool horn()             { return sw_horn.state; }
WiperMode wiper_mode()  { return currentWiperMode; }
bool washer()           { return sw_washer.state; }
uint8_t hvac_fan_speed(){ return hvacFan; }
uint8_t hvac_mode()     { return hvacMode; }
bool ac_request()       { return sw_ac_request.state; }
KeyPosition key_position() { return currentKeyPos; }
bool reverse_switch()   { return sw_reverse.state; }

bool cruise_set_pressed()    { return cruise_set_edge; }
bool cruise_resume_pressed() { return cruise_resume_edge; }
bool cruise_accel_pressed()  { return cruise_accel_edge; }
bool cruise_decel_pressed()  { return cruise_decel_edge; }
bool cruise_cancel_pressed() { return cruise_cancel_edge; }
bool cruise_onoff_pressed()  { return cruise_onoff_edge; }
bool any_cruise_event() {
    return cruise_set_edge || cruise_resume_edge || cruise_accel_edge ||
           cruise_decel_edge || cruise_cancel_edge || cruise_onoff_edge;
}

} // namespace SwitchInput
