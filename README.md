# PDCM — Power Distribution & Control Module

A fully custom power distribution and control module for a 1998 Chevy Silverado running custom vehicle electronics. Manages all driver switch inputs, lighting, relay control, power distribution, and 4WD.

## Systems

| System | Description |
|--------|-------------|
| Switch Inputs | Multifunction stalks (turn/cruise), hazard, horn, wiper/washer, HVAC controls, key position |
| Lighting | Headlights (low/high), turn signals, hazards, brake lights, reverse, interior, DRL |
| Power Distribution | Fuel pump relay, cooling fans, A/C compressor clutch, horn, wipers, blower, accessory |
| 4WD Controls | Transfer case encoder motor (NP246/NP261), front axle actuator, indicator |
| Brake Override | Dual brake switch inputs for brake override protection (BOP) |

## CAN Bus

The PDCM communicates over a vehicle-wide CAN FD bus (1 Mbps arbitration, 8 Mbps data). The 1998 GMT400 has no factory CAN bus — the entire vehicle network is custom-built.

| Direction | CAN IDs | Messages |
|-----------|---------|----------|
| Publishes | 0x350–0x357, 0x36F | 9 messages — switch states, lighting, power, cruise buttons, A/C, 4WD, brake, faults, heartbeat |
| Consumes | 0x360–0x363, 0x310 | 5 messages — fan command, light command, 4WD command, relay command, drive mode |

**Module ID**: `0x10` — see `docs/can_protocol.md` for full message definitions.

## Vehicle Platform

Part of the [silverado-platform](https://github.com/mikeslezak/silverado-platform) vehicle electronics network. The PDCM is a passive executor — it receives commands and reports states but does not make engine control decisions. Boundary defined by ECM ADR-016.

## Hardware

- Single Teensy 4.1 (IMXRT1062, 600 MHz Cortex-M7)
- MCP2562FD CAN FD transceiver
- High-side switches and relay drivers for power distribution
- Deutsch DT/DTM automotive connectors
- Custom PCB (design in progress)

## Firmware

- C++ (Arduino/Teensy framework)
- Built with PlatformIO (`pio run -e PDCM`)
- Vehicle CAN contract from [silverado-platform](https://github.com/mikeslezak/silverado-platform) (git submodule at `firmware/platform/`)

## Current Phase

**Phase 1 — Hardware Design**

## Author

Mike Slezak — TruckLabs
