# PDCM — Power Distribution & Control Module

Full solid-state power distribution module for a 1998 Chevy Silverado custom electronics platform. Part of a vehicle-wide CAN FD network alongside the ECM, HeadUnit (HMI), TCM, GCM, and other modules.

## Architecture

**Zero mechanical relays.** Every output channel uses a TC4427A dual gate driver + IRFZ44N N-channel MOSFET with per-channel current sensing and software overcurrent protection.

| | Count |
|--|-------|
| **Output channels** | 47 (+ 1 H-bridge) |
| **TC4427A gate drivers** | 24 ICs |
| **IRFZ44N MOSFETs** | ~46 |
| **DRV8876 H-bridge** | 1 (4WD encoder motor) |
| **Current sense channels** | 48 |
| **CAN FD messages** | 9 published, 6 consumed |

### Output Tiers

- **Tier 1 (25 ch)**: PDCM-switched loads — fuel pump, cooling fans, blower, A/C, headlights, turn signals, brake lights, reverse, DRL, interior, horn, wiper, accessory, front axle, seat heaters, light bar, 4WD motor
- **Tier 2 (5 ch)**: Enable signals — 4× amp remotes, HeadUnit power
- **Tier 3 (18 ch)**: Sub-loads — ADAS cameras, GCM, GPS, dash cam, auxiliary lighting, expansion

### Safety (3 Layers)

1. **Upstream fuse** per channel (PTC or blade) — protects against MOSFET fails-short
2. **Firmware fault detection** — overcurrent, open-load, stuck-on, CAN timeout
3. **Redundant switching** — fuel pump (ECM/ST independent cutoff), seat heaters (dual MOSFETs in series)

### Smart Features

- Headlight soft-start (PWM ramp for halogen inrush)
- Turn signal flash timer with hazard override
- Interior/courtesy light PWM fade
- Welcome/goodbye lighting sequences
- Fuel pump prime-and-timeout
- Wiper intermittent timing
- 4-tier load shedding (voltage-based priority)
- CAN timeout failsafe (fans → 100%, fuel pump → off)
- NP246 transfer case state machine with stall detection

## MCU — NXP S32K358

| | Spec |
|--|------|
| **Part** | NXP S32K358GHT1MPCST |
| **Package** | HDQFP-172 |
| **Core** | Dual Cortex-M7 @ 240MHz |
| **Flash / RAM** | 8MB / 1MB |
| **CAN** | 6× CAN FD (native) |
| **Safety** | Hardware lockstep + SWT watchdog |
| **Grade** | AEC-Q100 Grade 1 |

Building directly on S32K358 — no Teensy prototype phase. 172 pins eliminates the need for port expanders (MCP23S17) and analog MUX (CD74HC4067). All outputs, inputs, and ADC channels are direct. Firmware uses a HAL — all modules are platform-independent.

## CAN Bus

Vehicle CAN FD bus: 1 Mbps arbitration, 8 Mbps data, MCP2562FD transceivers.

**Module ID**: `0x10` | **CAN Range**: `0x350–0x36F` | **Heartbeat**: `0x36F @ 500ms`

## Build

```bash
# S32K358 (CMake + NXP S32 Design Studio)
cmake -B build -G "Unix Makefiles"
cmake --build build

# Teensy reference build (retained, not primary)
pio run -e PDCM
```

## Project Structure

```
PDCM/
├── firmware/
│   ├── hal/           HAL interface + platform implementations
│   ├── shared/        PDCMConfig.h, PDCMTypes.h
│   ├── src/           All firmware modules + main.cpp
│   └── platform/      silverado-platform submodule
├── hardware/
│   ├── schematic/     Schematic files
│   ├── pcb/           PCB layout
│   ├── datasheets/    Component datasheets
│   └── bom/           Bill of materials
├── docs/              CAN protocol, pin allocation, schematic notes
├── DECISIONS.md       Architecture decision records (ADR-001–008)
├── TODO.md            Development phases and work items
└── platformio.ini     Build configuration
```

## Current Phase

**Phase 2 — Firmware Architecture**: Complete. All 13 firmware modules, HAL, and 9 ADRs written.
**Phase 3 — Hardware Design (S32K358)**: Current. Schematic design around S32K358.

## Author

Mike Slezak — TruckLabs
