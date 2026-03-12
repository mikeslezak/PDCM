# PDCM Pin Allocation

## NXP S32K358 — HDQFP-172

Building directly on S32K358 (ADR-009). No Teensy prototype phase.

S32K358 has 172 pins total. Key I/O resources:
- **GPIO**: 100+ available (more than enough for all 47 outputs + all switch inputs)
- **ADC**: 2× SAR ADC instances, 40+ external channels (direct per-shunt, no MUX needed)
- **CAN FD**: 6× native controllers (using 1 for vehicle bus)
- **PWM (eMIOS/eTIMER)**: 24+ channels for PWM-capable outputs
- **SWT**: Hardware watchdog timer
- **JTAG/SWD**: Debug interface

### Pin Allocation Strategy

**Detailed pin-to-pad mapping will be completed during schematic capture** using the S32K358 reference manual (RM) and package ball map. The allocation below defines functional groups.

### Gate Driver Outputs (47 GPIO)

All 47 channels on direct S32K358 GPIO. No port expanders.

| Group | Channels | Count | Notes |
|-------|----------|-------|-------|
| Tier 1 power | Fuel pump, fans, blower, A/C | 5 | Some PWM-capable |
| Tier 1 lights | Low/high beam, turns, brake, reverse, DRL, interior, courtesy | 13 | All PWM-capable |
| Tier 1 misc | Horn, wiper, accessory, front axle, seat heaters, light bar | 7 | Seat heaters PWM |
| Tier 2 enables | Amp remotes ×4, HeadUnit | 5 | Low current |
| Tier 3 sub-loads | Cameras, sensors, modules, aux lighting, expansion | 17 | On/off only |
| **Total** | | **47** | |

### H-Bridge (DRV8876) — 4 GPIO + 1 ADC

| Function | Type | Notes |
|----------|------|-------|
| IN1 | GPIO (PWM) | Motor direction/speed |
| IN2 | GPIO (PWM) | Motor direction/speed |
| nSLEEP | GPIO output | Low = sleep mode |
| nFAULT | GPIO input (pullup) | Low = fault |
| IPROPI | ADC input | Current sense output |

### Switch Inputs (direct GPIO) — ~15 GPIO

All switch inputs on direct S32K358 GPIO. No port expanders.

| Input | Type | Notes |
|-------|------|-------|
| Turn left | GPIO input, pullup | Active low |
| Turn right | GPIO input, pullup | Active low |
| High beam | GPIO input, pullup | Active low |
| Flash-to-pass | GPIO input, pullup | Active low |
| Hazard | GPIO input, pullup | Active low |
| Horn | GPIO input, pullup | Active low |
| Reverse | GPIO input, pullup | Active low |
| A/C request | GPIO input, pullup | Active low |
| Wiper intermittent | GPIO input, pullup | Active low |
| Wiper low | GPIO input, pullup | Active low |
| Wiper high | GPIO input, pullup | Active low |
| Washer | GPIO input, pullup | Active low |
| Brake switch 1 | **DIRECT GPIO** | Safety-critical |
| Brake switch 2 | **DIRECT GPIO** | Safety-critical |

### ADC Inputs — ~50 channels

All ADC inputs direct to S32K358 SAR ADC. No analog MUX.

| Function | Count | Notes |
|----------|-------|-------|
| Current sense shunts (via INA180) | 47 | One per output channel |
| Battery voltage divider | 1 | 10k/3.3k divider |
| Key position resistor ladder | 1 | 4-position decode |
| 4WD position potentiometer | 1 | Transfer case position |
| DRV8876 current sense | 1 | H-bridge motor current |
| **Total ADC** | **51** | Fits in 2× SAR ADC instances |

### Communication

| Function | Pins | Notes |
|----------|------|-------|
| CAN FD (vehicle bus) | 2 | CAN0 or CAN1, native S32K358 |
| JTAG/SWD debug | 4-5 | TCK, TMS, TDI, TDO, nRESET |

### Power

| Function | Notes |
|----------|-------|
| VDD_HV (3.3V–5V) | S32K358 power supply |
| VDD_LV (1.2V core) | Internal regulator or external |
| VDDA (analog supply) | Clean supply for ADC accuracy |
| Multiple VSS | Ground pins |

---

## Pin Count Summary

| Category | Count |
|----------|-------|
| Gate driver outputs | 47 |
| H-bridge control | 4 |
| Switch inputs | 14 |
| ADC (shunts + sensors) | 51 |
| CAN FD | 2 |
| JTAG/SWD | 5 |
| Power/ground | ~30 |
| Crystal | 2 |
| Reset | 1 |
| **Total used** | **~156** |
| **Available (172-pin)** | **~16 spare** |

---

## Teensy 4.1 Reference

The original Teensy 4.1 pin allocation is preserved in `PDCMConfig.h` under the `PDCM_TARGET_TEENSY` guard for reference. It required 2× MCP23S17 port expanders and 3× CD74HC4067 analog MUX to fit within 42 GPIO + 18 ADC pins.
