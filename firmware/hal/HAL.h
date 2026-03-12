#ifndef PDCM_HAL_H
#define PDCM_HAL_H

/**
 * HAL.h — Hardware Abstraction Layer Interface
 *
 * Platform-independent interface for all MCU peripherals.
 * All firmware modules call these functions instead of direct hardware access.
 *
 * Implementations:
 *   - teensy/TeensyHAL.cpp  (Arduino + FlexCAN_T4, prototype)
 *   - s32k358/S32KHAL.cpp   (NXP RTD SDK, production)
 *
 * See ADR-007 for design rationale.
 */

#include <stdint.h>
#include <stddef.h>

// ============================================================================
// CAN FD Types
// ============================================================================

struct HAL_CANMessage {
    uint32_t id;
    uint8_t  data[64];
    uint8_t  len;
};

// CAN receive callback type
typedef void (*HAL_CANCallback)(const HAL_CANMessage& msg);

// ============================================================================
// HAL Interface — Implemented per platform
// ============================================================================

namespace HAL {

    // --- Initialization ---
    void init();                    // Initialize all peripherals

    // --- GPIO ---
    void gpio_mode_output(uint8_t pin);
    void gpio_mode_input(uint8_t pin, bool pullup = false);
    void gpio_write(uint8_t pin, bool state);
    bool gpio_read(uint8_t pin);

    // --- PWM ---
    // duty: 0–1000 (0.0–100.0%)
    void pwm_write(uint8_t pin, uint16_t duty_permille);
    void pwm_set_frequency(uint8_t pin, uint32_t freq_hz);

    // --- ADC ---
    // Returns raw 12-bit value (0–4095)
    uint16_t adc_read(uint8_t pin);
    // Returns voltage in mV (0–3300 for 3.3V ref)
    uint16_t adc_read_mv(uint8_t pin);

    // --- CAN FD ---
    void can_init();
    bool can_send(uint32_t id, const uint8_t* data, uint8_t len);
    void can_set_callback(uint32_t id, HAL_CANCallback callback);
    void can_poll();                // Process incoming CAN messages

    // --- SPI ---
    void spi_init();
    void spi_transfer(uint8_t cs_pin, const uint8_t* tx, uint8_t* rx, size_t len);
    void spi_write_byte(uint8_t cs_pin, uint8_t reg, uint8_t value);
    uint8_t spi_read_byte(uint8_t cs_pin, uint8_t reg);

    // --- Port Expander (MCP23S17) ---
    // Higher-level functions for SPI port expander (Teensy only — S32K358 uses direct GPIO)
    void expander_init(uint8_t cs_pin);
    void expander_set_direction(uint8_t cs_pin, uint8_t port, uint8_t direction);  // 0=output, 1=input
    void expander_set_pullups(uint8_t cs_pin, uint8_t port, uint8_t pullups);
    void expander_write_port(uint8_t cs_pin, uint8_t port, uint8_t value);
    uint8_t expander_read_port(uint8_t cs_pin, uint8_t port);

    // --- Analog MUX (CD74HC4067) ---
    // Select MUX channel (0-15) via MCP23S17 port B bits 4-7
    void mux_select(uint8_t channel);

    // --- Watchdog ---
    void watchdog_init(uint32_t timeout_ms);
    void watchdog_feed();

    // --- Timing ---
    uint32_t millis();
    uint32_t micros();
    void delay_ms(uint32_t ms);
    void delay_us(uint32_t us);

    // --- Serial Debug ---
    void serial_init(uint32_t baud);
    void serial_print(const char* str);
    void serial_println(const char* str);
}

#endif // PDCM_HAL_H
