# PDCM Schematic Notes

## MCU — NXP S32K358

- **Part**: NXP S32K358GHT1MPCST
- **Package**: HDQFP-172 (0.5mm pitch)
- **On hand**: 50 units
- **Key specs**: Dual Cortex-M7 @ 240MHz, 8MB flash, 1MB SRAM, 6× CAN FD, AEC-Q100 Grade 1
- **Power**: 3.3V I/O, 1.2V core (internal regulator or external LDO)
- **ADC**: 2× SAR ADC, 40+ external channels, 12-bit
- **PWM**: eMIOS + eTIMER modules, 24+ PWM channels
- **Watchdog**: SWT (Software Watchdog Timer)
- **Safety**: Lockstep mode (dual-core comparison) for brake monitoring
- **Crystal**: 8 MHz or 16 MHz external (check RM for PLL config)
- **Debug**: JTAG/SWD via 10-pin Cortex Debug header
- **Decoupling**: Multiple VDD/VSS pairs — each needs 100nF ceramic + shared bulk

### Why S32K358 eliminates SPI peripherals
With 172 pins, all 47 gate driver outputs, all switch inputs, and all ADC channels connect directly to S32K358 GPIO/ADC. No MCP23S17 port expanders. No CD74HC4067 analog MUX. No SPI bus. This removes 7 ICs and simplifies the PCB significantly.

## Component Selection

### Gate Driver — TC4427A
- **Part**: Microchip TC4427AEPA (DIP-8) or TC4427ACOA (SOIC-8)
- **Qty**: 24 (47 channels ÷ 2 channels per IC, rounded up)
- **On hand**: 50 units
- **Key specs**: Dual non-inverting, 1.5A peak output, 4.5–18V supply, AEC-Q100
- **Circuit**: VDD → 12V, GND → GND, IN_A/IN_B → MCU GPIO (3.3V), OUT_A/OUT_B → 100Ω → MOSFET gate
- **Why 100Ω gate resistor**: Limits dV/dt on MOSFET gate to reduce EMI. Without it, the TC4427A's 1.5A peak drives the gate so fast it rings and radiates.
- **Why 10kΩ pulldown**: Ensures MOSFET gate is pulled to GND during MCU boot/reset when GPIO pins are tri-state. Prevents uncontrolled turn-on.

### MOSFET — IRFZ44N
- **Part**: Infineon IRFZ44NPBF (TO-220)
- **Qty**: ~46 (47 channels minus H-bridge)
- **Key specs**: 55V, 49A, 17.5mΩ Rds(on), TO-220
- **Low-side switching**: Drain → load, Source → shunt → GND
- **Heat**: At 10A continuous, P = I²R = 1.75W. TO-220 can handle this without heatsink at typical ambient. At 20A, 7W — needs heatsink or airflow.

### H-Bridge — DRV8876
- **Part**: TI DRV8876RGTR (WSON-8)
- **Qty**: 1
- **Key specs**: 37V, 3.5A, AEC-Q100, built-in current sense, nFAULT output
- **Circuit**: VM → 12V, IN1/IN2 → MCU GPIO (PWM), nSLEEP → MCU GPIO, nFAULT → MCU GPIO (input, pullup), IPROPI → 1kΩ to GND + ADC
- **Used for**: NP246 transfer case encoder motor (bidirectional, ~2A)

### Current Sense Amplifier — INA180
- **Part**: TI INA180A1IDBVR (SOT-23-5), gain = 20
- or INA180A3IDBVR, gain = 100 (for high-resistance shunts)
- **Qty**: 48 (one per channel)
- **Circuit**: IN+ → MOSFET source (high side of shunt), IN- → GND side of shunt, OUT → MUX input
- **Gain selection per shunt value**:
  - 10mΩ shunt, 10A max: V_shunt = 100mV, gain 20 → V_out = 2.0V ✓
  - 50mΩ shunt, 8A max: V_shunt = 400mV, gain 20 → V_out = 8.0V (clipped to VCC). Use gain 5 or 10.
  - 100mΩ shunt, 3A max: V_shunt = 300mV, gain 10 → V_out = 3.0V ✓
- **Note**: Gain selection needs per-group tuning. May use INA180A1 (×20) for 10mΩ shunts, INA180A2 (×50) for 50mΩ, INA180A1 for 100mΩ.

### ~~Analog MUX — CD74HC4067~~ (REMOVED — ADR-009)
Not needed. S32K358 has 2× SAR ADC with 40+ external channels — direct ADC per shunt.

### ~~Port Expander — MCP23S17~~ (REMOVED — ADR-009)
Not needed. S32K358 HDQFP-172 has enough GPIO for all 47 outputs + all switch inputs directly.

### Shunt Resistors
- **10mΩ**: Fuel pump, fans, blower, A/C, seat heaters, light bar (6 channels)
- **50mΩ**: Headlights, horn, wiper, accessory, rock lights (9 channels)
- **100mΩ**: All remaining channels (32 channels)
- **Package**: 2512 or 2010 (need adequate power rating)
- **Tolerance**: 1% for accurate current measurement

### Upstream Fuses
- PTC resettable fuse per channel (or blade fuse holder)
- Sized per channel: heavy loads get higher trip current
- Must trip BEFORE MOSFET thermal limit

## Circuit Topology

```
12V Battery
    │
    ├── [Fuse] ── Load ── MOSFET Drain
    │                         │
    │                    MOSFET Source
    │                         │
    │                    [Shunt Resistor]
    │                         │
    │                        GND
    │
    ├── TC4427A VDD
    │
    └── 5V Regulator → 3.3V Regulator → MCU

MCU GPIO ──→ TC4427A IN ──→ TC4427A OUT ──→ [100Ω] ──→ MOSFET Gate
                                                           │
                                                        [10kΩ]
                                                           │
                                                          GND
```

## Power Budget (Estimated)

| Category | Max Current | Notes |
|----------|-------------|-------|
| Headlights (4×) | 32A | 8A per halogen bulb |
| Cooling fans (2×) | 30A | 15A each |
| Fuel pump | 10A | |
| Blower motor | 10A | |
| Seat heaters (2×) | 14A | 7A each |
| Light bar | 15A | |
| All other outputs | ~20A | Combined estimate |
| **Total theoretical max** | **~131A** | Not all loads run simultaneously |
| **Typical running** | **~40-60A** | Normal driving |

## Input Protection
- TVS diode (P6KE18A or equivalent) on all GPIO and ADC inputs
- 1kΩ series resistor between input connector and TVS/MCU
- Schottky clamp diodes to VCC rail on sensitive inputs

## BOM Summary (S32K358 build)

| Component | Qty | Notes |
|-----------|-----|-------|
| NXP S32K358 | 1 | HDQFP-172, on hand |
| TC4427A gate driver | 24 | Dual channel, on hand |
| IRFZ44N MOSFET | ~46 | TO-220 |
| DRV8876 H-bridge | 1 | WSON-8, 4WD motor |
| INA180 current sense amp | 48 | SOT-23-5 |
| MCP2562FD CAN transceiver | 1 | CAN FD, 8-pin |
| Shunt resistors (2512) | 47 | 10/50/100mΩ per channel |
| Voltage regulator (5V) | 1 | 12V → 5V |
| Voltage regulator (3.3V) | 1 | 5V → 3.3V |
| Crystal (8/16 MHz) | 1 | For S32K358 PLL |
| Upstream fuses | 47 | PTC or blade, per channel |
| ~~CD74HC4067 MUX~~ | ~~0~~ | Removed — direct ADC |
| ~~MCP23S17 expander~~ | ~~0~~ | Removed — direct GPIO |

**Total active ICs: ~78** (1 MCU + 24 gate drivers + 48 current sense amps + 1 H-bridge + 1 CAN xcvr + 2 regulators + 1 crystal)
