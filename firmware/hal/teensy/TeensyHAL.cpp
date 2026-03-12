/**
 * TeensyHAL.cpp — Teensy 4.1 HAL Implementation
 *
 * Arduino framework + FlexCAN_T4 for prototype development.
 * Maps HAL interface to Teensy-specific APIs.
 */

#ifdef PDCM_TARGET_TEENSY
// This is the default — no define needed unless S32K358 is explicitly selected

#include "../HAL.h"
#include "../../shared/PDCMConfig.h"
#include <Arduino.h>
#include <FlexCAN_T4.h>
#include <SPI.h>

// ============================================================================
// CAN FD Instance
// ============================================================================

static FlexCAN_T4FD<CAN1, RX_SIZE_256, TX_SIZE_64> canBus;

// CAN callback registry — up to 16 filtered message IDs
struct CANCallbackEntry {
    uint32_t id;
    HAL_CANCallback callback;
};
static CANCallbackEntry canCallbacks[16];
static uint8_t numCallbacks = 0;

// Internal CAN receive handler
static void canISR(const CANFD_message_t& msg) {
    for (uint8_t i = 0; i < numCallbacks; i++) {
        if (canCallbacks[i].id == msg.id && canCallbacks[i].callback) {
            HAL_CANMessage halMsg;
            halMsg.id = msg.id;
            halMsg.len = msg.len;
            memcpy(halMsg.data, msg.buf, msg.len);
            canCallbacks[i].callback(halMsg);
            return;
        }
    }
}

// ============================================================================
// MCP23S17 Register Addresses
// ============================================================================

namespace MCP23S17 {
    constexpr uint8_t IODIRA   = 0x00;  // Port A direction
    constexpr uint8_t IODIRB   = 0x01;  // Port B direction
    constexpr uint8_t GPPUA    = 0x0C;  // Port A pullups
    constexpr uint8_t GPPUB    = 0x0D;  // Port B pullups
    constexpr uint8_t GPIOA    = 0x12;  // Port A data
    constexpr uint8_t GPIOB    = 0x13;  // Port B data
    constexpr uint8_t OLATA    = 0x14;  // Port A output latch
    constexpr uint8_t OLATB    = 0x15;  // Port B output latch
    constexpr uint8_t OPCODE_W = 0x40;  // Write opcode (A2=A1=A0=0)
    constexpr uint8_t OPCODE_R = 0x41;  // Read opcode
}

// ============================================================================
// HAL Implementation
// ============================================================================

namespace HAL {

void init() {
    serial_init(115200);
    spi_init();
    can_init();
}

// --- GPIO ---

void gpio_mode_output(uint8_t pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);     // Default OFF for safety
}

void gpio_mode_input(uint8_t pin, bool pullup) {
    pinMode(pin, pullup ? INPUT_PULLUP : INPUT);
}

void gpio_write(uint8_t pin, bool state) {
    digitalWrite(pin, state ? HIGH : LOW);
}

bool gpio_read(uint8_t pin) {
    return digitalRead(pin) == HIGH;
}

// --- PWM ---

void pwm_write(uint8_t pin, uint16_t duty_permille) {
    // Teensy PWM resolution: 12-bit (0–4095)
    // Map 0–1000 permille to 0–4095
    uint32_t pwm_val = ((uint32_t)duty_permille * 4095) / 1000;
    analogWrite(pin, pwm_val);
}

void pwm_set_frequency(uint8_t pin, uint32_t freq_hz) {
    analogWriteFrequency(pin, freq_hz);
}

// --- ADC ---

uint16_t adc_read(uint8_t pin) {
    analogReadResolution(12);
    return analogRead(pin);
}

uint16_t adc_read_mv(uint8_t pin) {
    uint16_t raw = adc_read(pin);
    // 3.3V reference, 12-bit: mV = raw × 3300 / 4096
    return (uint16_t)(((uint32_t)raw * 3300) / 4096);
}

// --- CAN FD ---

void can_init() {
    canBus.begin();
    CANFD_timings_t timing;
    timing.clock = CLK_24MHz;
    timing.baudrate = 1000000;      // 1 Mbps arbitration
    timing.baudrateFD = 8000000;    // 8 Mbps data
    timing.propdelay = 190;
    timing.bus_length = 1;
    timing.sample = 87;
    canBus.setBaudRate(timing);
    canBus.enableMBInterrupts();
    canBus.onReceive(canISR);

    numCallbacks = 0;
}

bool can_send(uint32_t id, const uint8_t* data, uint8_t len) {
    CANFD_message_t msg;
    msg.id = id;
    msg.len = len;
    msg.flags.extended = 0;
    memcpy(msg.buf, data, len);
    return canBus.write(msg) > 0;
}

void can_set_callback(uint32_t id, HAL_CANCallback callback) {
    if (numCallbacks < 16) {
        canCallbacks[numCallbacks].id = id;
        canCallbacks[numCallbacks].callback = callback;
        numCallbacks++;
    }
}

void can_poll() {
    canBus.events();
}

// --- SPI ---

void spi_init() {
    SPI.begin();
}

void spi_transfer(uint8_t cs_pin, const uint8_t* tx, uint8_t* rx, size_t len) {
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    digitalWrite(cs_pin, LOW);
    for (size_t i = 0; i < len; i++) {
        uint8_t result = SPI.transfer(tx ? tx[i] : 0x00);
        if (rx) rx[i] = result;
    }
    digitalWrite(cs_pin, HIGH);
    SPI.endTransaction();
}

void spi_write_byte(uint8_t cs_pin, uint8_t reg, uint8_t value) {
    uint8_t tx[3] = { MCP23S17::OPCODE_W, reg, value };
    spi_transfer(cs_pin, tx, nullptr, 3);
}

uint8_t spi_read_byte(uint8_t cs_pin, uint8_t reg) {
    uint8_t tx[3] = { MCP23S17::OPCODE_R, reg, 0x00 };
    uint8_t rx[3] = {};
    spi_transfer(cs_pin, tx, rx, 3);
    return rx[2];
}

// --- Port Expander (MCP23S17) ---

void expander_init(uint8_t cs_pin) {
    gpio_mode_output(cs_pin);
    gpio_write(cs_pin, true);   // CS high = deselected
    // Default: all pins input (0xFF)
}

void expander_set_direction(uint8_t cs_pin, uint8_t port, uint8_t direction) {
    uint8_t reg = (port == 0) ? MCP23S17::IODIRA : MCP23S17::IODIRB;
    spi_write_byte(cs_pin, reg, direction);
}

void expander_set_pullups(uint8_t cs_pin, uint8_t port, uint8_t pullups) {
    uint8_t reg = (port == 0) ? MCP23S17::GPPUA : MCP23S17::GPPUB;
    spi_write_byte(cs_pin, reg, pullups);
}

void expander_write_port(uint8_t cs_pin, uint8_t port, uint8_t value) {
    uint8_t reg = (port == 0) ? MCP23S17::OLATA : MCP23S17::OLATB;
    spi_write_byte(cs_pin, reg, value);
}

uint8_t expander_read_port(uint8_t cs_pin, uint8_t port) {
    uint8_t reg = (port == 0) ? MCP23S17::GPIOA : MCP23S17::GPIOB;
    return spi_read_byte(cs_pin, reg);
}

// --- Analog MUX (CD74HC4067) ---

void mux_select(uint8_t channel) {
    // Write MUX select bits (S0-S3) to MCP23S17 #1 Port B bits 4-7
    // Preserve lower 4 bits (wiper switches)
    uint8_t current = expander_read_port(Pin::SPI_CS_EXPANDER1, 1);
    uint8_t select_bits = (channel & 0x0F) << 4;
    uint8_t new_val = (current & 0x0F) | select_bits;
    expander_write_port(Pin::SPI_CS_EXPANDER1, 1, new_val);
}

// --- Watchdog ---

void watchdog_init(uint32_t timeout_ms) {
    // Teensy 4.1 WDOG1 — use IMXRT watchdog
    // For prototype, use simple software watchdog
    // Production S32K358 uses SWT (Software Watchdog Timer)
    (void)timeout_ms;
    // TODO: Implement IMXRT1062 WDOG configuration
}

void watchdog_feed() {
    // TODO: Implement IMXRT1062 WDOG service sequence
    // WDOG->WSR = 0x5555; WDOG->WSR = 0xAAAA;
}

// --- Timing ---

uint32_t millis() {
    return ::millis();
}

uint32_t micros() {
    return ::micros();
}

void delay_ms(uint32_t ms) {
    delay(ms);
}

void delay_us(uint32_t us) {
    delayMicroseconds(us);
}

// --- Serial Debug ---

void serial_init(uint32_t baud) {
    Serial.begin(baud);
}

void serial_print(const char* str) {
    Serial.print(str);
}

void serial_println(const char* str) {
    Serial.println(str);
}

} // namespace HAL

#endif // PDCM_TARGET_TEENSY
