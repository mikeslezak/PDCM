# PDCM Schematic Notes

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

### Analog MUX — CD74HC4067 (Teensy only)
- **Part**: TI CD74HC4067M96 (SOIC-24)
- **Qty**: 3 (16 channels each × 3 = 48 channels)
- **Circuit**: S0-S3 → MCP23S17 #1 Port B (shared select lines), COM → ADC pin, C0-C15 → INA180 outputs
- **Not needed on S32K358** — enough direct ADC channels

### Port Expander — MCP23S17 (Teensy only)
- **Part**: Microchip MCP23S17-E/SP (DIP-28) or MCP23S17-E/SS (SSOP-28)
- **Qty**: 2
- **MCP23S17 #1**: Switch inputs (Port A) + MUX select (Port B upper nibble)
- **MCP23S17 #2**: Tier 3 gate driver outputs (16 channels)
- **SPI**: 8 MHz clock, Mode 0
- **Not needed on S32K358** — enough direct GPIO

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
