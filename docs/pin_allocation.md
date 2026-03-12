# PDCM Pin Allocation

## Teensy 4.1 — Prototype Target

### Direct GPIO — Gate Driver Outputs (Tier 1-2)

| Teensy Pin | Channel | Load | Notes |
|------------|---------|------|-------|
| 2 | CH 1 | Fuel pump | Soft-start |
| 3 | CH 2 | Cooling fan 1 | PWM |
| 4 | CH 3 | Cooling fan 2 | PWM |
| 5 | CH 4 | Blower motor | PWM |
| 6 | CH 5 | A/C compressor clutch | |
| 7 | CH 6 | Low beam left | PWM soft-start |
| 8 | CH 7 | Low beam right | PWM soft-start |
| 9 | CH 8 | High beam left | PWM soft-start |
| 10 | CH 9 | High beam right | PWM soft-start |
| 11 | — | SPI MOSI | Shared SPI bus |
| 12 | — | SPI MISO | Shared SPI bus |
| 13 | — | SPI SCK | Shared SPI bus |
| 14 | — | H-bridge IN1 | DRV8876, PWM |
| 15 | — | H-bridge IN2 | DRV8876, PWM |
| 16 | — | H-bridge nSLEEP | DRV8876 |
| 17 | — | H-bridge nFAULT | DRV8876, input |
| 20 | — | Brake switch 1 | **DIRECT GPIO** |
| 21 | — | Brake switch 2 | **DIRECT GPIO** |
| 22 | — | CAN1 TX | CAN FD |
| 23 | — | CAN1 RX | CAN FD |
| 24 | CH 12 | Brake light left | |
| 25 | CH 13 | Brake light right | |
| 26 | CH 14 | Reverse lights | |
| 27 | CH 15 | DRL | PWM |
| 28 | CH 16 | Interior light | PWM fade |
| 29 | CH 17 | Courtesy light | PWM fade |
| 30 | CH 18 | Horn | |
| 31 | CH 19 | Wiper motor | |
| 32 | CH 20 | Accessory power | |
| 33 | CH 21 | Front axle actuator | |
| 34 | CH 22 | Seat heater left | PWM |
| 35 | CH 23 | Seat heater right | PWM |
| 36 | CH 24 | Light bar | |
| 37 | CH 26 | Amp remote #1 | |
| 38 | CH 27 | Amp remote #2 | |
| 39 | CH 28 | Amp remote #3 | |
| 40 | CH 29 | Amp remote #4 | |
| 41 | CH 30 | HeadUnit enable | |

**Direct GPIO total: 34 output + 2 brake input + 4 H-bridge + 3 SPI + 2 CAN = 45 pins**

### SPI Bus — Port Expanders + MUX Select

| Pin | Function |
|-----|----------|
| 0 | SPI CS — MCP23S17 #1 (switch inputs + MUX select) |
| 1 | SPI CS — MCP23S17 #2 (Tier 3 outputs) |

### ADC Inputs

| Teensy Pin | Function |
|------------|----------|
| A0 (14) | Battery voltage divider |
| A1 (15) | Key position resistor ladder |
| A2 (16) | 4WD position potentiometer |
| A3 (17) | CD74HC4067 MUX #1 output (channels 0-15) |
| A6 (20) | CD74HC4067 MUX #2 output (channels 16-31) |
| A7 (21) | CD74HC4067 MUX #3 output (channels 32-47) |
| A8 (22) | DRV8876 current sense output |

**Note**: A0-A2 share physical pins with GPIO 14-16, but these are used as H-bridge control in the direct GPIO map. ADC pins A0/A1/A2 on Teensy 4.1 map to different physical pins. See Teensy 4.1 pinout card for exact mapping.

### MCP23S17 #1 — Switch Inputs + MUX Select

| Port | Bit | Function |
|------|-----|----------|
| A0 | 0 | Left stalk: turn left |
| A1 | 1 | Left stalk: turn right |
| A2 | 2 | Left stalk: high beam |
| A3 | 3 | Left stalk: flash-to-pass |
| A4 | 4 | Hazard switch |
| A5 | 5 | Horn button |
| A6 | 6 | Reverse switch |
| A7 | 7 | A/C request switch |
| B0 | 0 | Wiper: intermittent (input) |
| B1 | 1 | Wiper: low (input) |
| B2 | 2 | Wiper: high (input) |
| B3 | 3 | Washer pump (input) |
| B4 | 4 | MUX select S0 (output) |
| B5 | 5 | MUX select S1 (output) |
| B6 | 6 | MUX select S2 (output) |
| B7 | 7 | MUX select S3 (output) |

### MCP23S17 #2 — Tier 3 Outputs

| Port | Bit | Channel | Load |
|------|-----|---------|------|
| A0 | 0 | CH 31 | Front camera |
| A1 | 1 | CH 32 | Rear camera |
| A2 | 2 | CH 33 | Side cameras |
| A3 | 3 | CH 34 | Parking sensors |
| A4 | 4 | CH 35 | Radar/BSM |
| A5 | 5 | CH 36 | GCM power |
| A6 | 6 | CH 37 | GPS/cellular |
| A7 | 7 | CH 38 | Dash cam |
| B0 | 0 | CH 39 | Future module |
| B1 | 1 | CH 40 | Rock lights |
| B2 | 2 | CH 41 | Bed lights |
| B3 | 3 | CH 42 | Puddle lights |
| B4 | 4 | CH 43 | Future exterior |
| B5 | 5 | CH 44 | Expansion 1 |
| B6 | 6 | CH 45 | Expansion 2 |
| B7 | 7 | CH 46 | Expansion 3 |

---

## NXP S32K358 — Production Target (Placeholder)

HDQFP-172 package — 172 pins available. No port expanders or analog MUX needed.

Pin allocation to be completed when eval board arrives. Key differences:
- All 47 output channels on direct GPIO (no MCP23S17 needed)
- All 48 ADC channels direct (no CD74HC4067 MUX needed)
- 6× CAN FD controllers — dedicated controller for each bus if needed
- SWT (Software Watchdog Timer) for hardware safety
- Lockstep mode for brake monitoring core
