# PDCM — Claude Code Instructions

## What This Is
Full solid-state Power Distribution & Control Module for a 1998 Chevy Silverado custom electronics platform. 47 TC4427A-driven MOSFET channels + 1 DRV8876 H-bridge = 48 switched outputs. Per-channel current sensing, software overcurrent protection, 4-tier load shedding.

Passive executor — receives commands and reports states but does not make engine control decisions.

## Architecture
- **Output topology**: GPIO → TC4427A dual gate driver → 100Ω → IRFZ44N gate, 10kΩ pulldown
- **Current sensing**: Low-side shunt resistors on every channel, read via 3× CD74HC4067 analog MUX (Teensy) or direct ADC (S32K358)
- **Safety**: 3-layer protection (upstream fuse + firmware stuck-on detection + redundant switching on fire-risk channels)
- **47 channels**: 25 switched + 5 enable + 17 sub-loads + H-bridge
- **Zero mechanical relays**

## MCU Targets
- **Prototype**: Teensy 4.1 (IMXRT1062, 600 MHz Cortex-M7)
- **Production**: NXP S32K358 (dual CM7 @ 240MHz, AEC-Q100, lockstep, 6× CAN FD)
- **HAL**: `firmware/hal/` abstracts all MCU-specific code. Firmware modules are platform-independent.
- Teensy build: `pio run -e PDCM`
- S32K358 build: CMake + NXP S32 SDK (future)

## Platform Submodule
This repo includes `silverado-platform` as a git submodule at `firmware/platform/`. Single source of truth for:
- Vehicle bus CAN IDs (`can/VehicleCAN.h`)
- Shared message structs (`can/VehicleMessages.h`)
- Vehicle-wide types (`types/VehicleTypes.h`)
- Module IDs (`types/ModuleIDs.h`)

**Never duplicate CAN IDs or shared types** — always reference the platform headers.

## Module Identity
- **Module ID**: `0x10`
- **CAN Range**: `0x350–0x36F`
- **Heartbeat**: `0x36F` @ 500ms

## CAN Contract
### Publishes (9 messages)
| ID | Message | Rate |
|----|---------|------|
| 0x350 | Switch States | 100ms |
| 0x351 | Light State | 100ms |
| 0x352 | Power State | 500ms |
| 0x353 | Cruise Button | on-event |
| 0x354 | A/C State | 500ms |
| 0x355 | 4WD State | 500ms |
| 0x356 | Brake State | 50ms |
| 0x357 | Faults | 1000ms |
| 0x36F | Heartbeat | 500ms |

### Consumes (6 messages)
| ID | Source | Message |
|----|--------|---------|
| 0x310 | ECM | Drive Mode |
| 0x31F | ECM | Heartbeat |
| 0x360 | ECM | Fan Command |
| 0x361 | HMI | Light Command |
| 0x362 | HMI | 4WD Command |
| 0x363 | ECM | Relay Command |

## Firmware Module Layout
```
firmware/
├── hal/
│   ├── HAL.h                  (platform abstraction interface)
│   ├── teensy/TeensyHAL.cpp   (Arduino + FlexCAN_T4)
│   └── s32k358/S32KHAL.cpp    (NXP RTD SDK — placeholder)
├── shared/
│   ├── PDCMConfig.h           (pin maps, ADC config, constants)
│   └── PDCMTypes.h            (enums, structs, timing)
├── src/
│   ├── main.cpp               (init + scheduler)
│   ├── GateDriver.h/.cpp      (TC4427A output control)
│   ├── CurrentSense.h/.cpp    (shunt current measurement)
│   ├── HBridge.h/.cpp         (DRV8876 4WD motor)
│   ├── SwitchInput.h/.cpp     (switch reading + debounce)
│   ├── BrakeMonitor.h/.cpp    (dual brake + BOP)
│   ├── BatteryMonitor.h/.cpp  (voltage monitoring)
│   ├── LightController.h/.cpp (headlights, turns, fade, welcome/goodbye)
│   ├── PowerManager.h/.cpp    (fuel pump, A/C, horn, wiper, blower, heaters)
│   ├── FanController.h/.cpp   (PWM fans + CAN timeout failsafe)
│   ├── FourWDController.h/.cpp(NP246 state machine)
│   ├── FaultManager.h/.cpp    (per-channel + system faults)
│   ├── LoadShedder.h/.cpp     (4-tier voltage shedding)
│   └── CANManager.h/.cpp      (9 published + 6 consumed messages)
└── platform/                  (silverado-platform submodule)
```

## Engineering Rules
- No hacky shit. No workarounds. No "just for now" code.
- CAN FD everywhere — 1 Mbps arb / 8 Mbps data.
- All CAN structs packed, little-endian.
- PDCM is a passive executor — never make engine decisions.
- Brake switches on DIRECT GPIO — never behind SPI/I2C.
- Brake state at 50ms rate — safety-critical path for BOP.
- Document decisions in DECISIONS.md (ADR-001 through ADR-008).

## Environment
- Firmware: C++ (Arduino/Teensy framework for prototype), PlatformIO
- CAN library: FlexCAN_T4
- SPI devices: MCP23S17 (port expander), CD74HC4067 (analog MUX)
- H-bridge: TI DRV8876 (4WD encoder motor)
- Gate driver: TC4427A (×24 ICs, dual channel)
- MOSFET: IRFZ44N (×~46)

## Dependencies
- **Required**: ECM (fan commands, relay commands, heartbeat)
- **Optional**: HMI (lighting/4WD commands), TCU (transfer case feedback)

## Session Protocol
**Start**: Read CLAUDE.md → DECISIONS.md → TODO.md → ask what we're working on.
**End**: Update TODO.md, DECISIONS.md, CLAUDE.md if needed. Commit with meaningful message.

## Current Phase
**Phase 2 — Firmware Architecture** (complete)
**Phase 3 — Hardware Design** (next)
