/**
 * S32KHAL.cpp — NXP S32K358 HAL Implementation (Placeholder)
 *
 * Production target using NXP RTD SDK.
 * This file will be implemented when the S32K358 eval board arrives.
 *
 * Key differences from Teensy HAL:
 *   - Direct GPIO for all 47 output channels (no port expanders needed)
 *   - Direct ADC for all current sense channels (no analog MUX needed)
 *   - 6× CAN FD controllers (FlexCAN hardware, not FlexCAN_T4 library)
 *   - SWT (Software Watchdog Timer) for hardware watchdog
 *   - Lockstep mode for safety-critical brake monitoring
 *   - HDQFP-172 package — 172 pins available
 *
 * NXP S32K358GHT1MPCST specs:
 *   - Dual Cortex-M7 @ 240MHz
 *   - 8MB flash, 1MB SRAM
 *   - AEC-Q100 Grade 1
 *   - -40°C to +150°C junction temp
 *
 * See ADR-002 and ADR-007 for rationale.
 */

#ifdef PDCM_TARGET_S32K358

#include "../HAL.h"

// TODO: Include NXP RTD SDK headers
// #include "S32K358.h"
// #include "Siul2_Dio_Ip.h"
// #include "Adc_Sar_Ip.h"
// #include "FlexCAN_Ip.h"
// #include "Swt_Ip.h"

namespace HAL {

void init() {
    // TODO: S32K358 system clock init (PLL to 240MHz)
    // TODO: Port mux configuration (SIUL2)
    // TODO: Peripheral clock gating
}

void gpio_mode_output(uint8_t pin) { (void)pin; }
void gpio_mode_input(uint8_t pin, bool pullup) { (void)pin; (void)pullup; }
void gpio_write(uint8_t pin, bool state) { (void)pin; (void)state; }
bool gpio_read(uint8_t pin) { (void)pin; return false; }

void pwm_write(uint8_t pin, uint16_t duty_permille) { (void)pin; (void)duty_permille; }
void pwm_set_frequency(uint8_t pin, uint32_t freq_hz) { (void)pin; (void)freq_hz; }

uint16_t adc_read(uint8_t pin) { (void)pin; return 0; }
uint16_t adc_read_mv(uint8_t pin) { (void)pin; return 0; }

void can_init() {}
bool can_send(uint32_t id, const uint8_t* data, uint8_t len) {
    (void)id; (void)data; (void)len; return false;
}
void can_set_callback(uint32_t id, HAL_CANCallback callback) {
    (void)id; (void)callback;
}
void can_poll() {}

void spi_init() {}
void spi_transfer(uint8_t cs_pin, const uint8_t* tx, uint8_t* rx, size_t len) {
    (void)cs_pin; (void)tx; (void)rx; (void)len;
}
void spi_write_byte(uint8_t cs_pin, uint8_t reg, uint8_t value) {
    (void)cs_pin; (void)reg; (void)value;
}
uint8_t spi_read_byte(uint8_t cs_pin, uint8_t reg) {
    (void)cs_pin; (void)reg; return 0;
}

// S32K358 has enough GPIO — port expanders not needed
void expander_init(uint8_t cs_pin) { (void)cs_pin; }
void expander_set_direction(uint8_t cs_pin, uint8_t port, uint8_t direction) {
    (void)cs_pin; (void)port; (void)direction;
}
void expander_set_pullups(uint8_t cs_pin, uint8_t port, uint8_t pullups) {
    (void)cs_pin; (void)port; (void)pullups;
}
void expander_write_port(uint8_t cs_pin, uint8_t port, uint8_t value) {
    (void)cs_pin; (void)port; (void)value;
}
uint8_t expander_read_port(uint8_t cs_pin, uint8_t port) {
    (void)cs_pin; (void)port; return 0;
}

// S32K358 has enough ADC — analog MUX not needed
void mux_select(uint8_t channel) { (void)channel; }

void watchdog_init(uint32_t timeout_ms) {
    (void)timeout_ms;
    // TODO: Configure S32K358 SWT (Software Watchdog Timer)
}
void watchdog_feed() {
    // TODO: Service S32K358 SWT
}

uint32_t millis() { return 0; }   // TODO: Systick-based
uint32_t micros() { return 0; }   // TODO: DWT cycle counter
void delay_ms(uint32_t ms) { (void)ms; }
void delay_us(uint32_t us) { (void)us; }

void serial_init(uint32_t baud) { (void)baud; }
void serial_print(const char* str) { (void)str; }
void serial_println(const char* str) { (void)str; }

} // namespace HAL

#endif // PDCM_TARGET_S32K358
