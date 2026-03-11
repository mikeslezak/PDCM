# PDCM — Claude Code Instructions

## What This Is
Power Distribution & Control Module for a 1998 Chevy Silverado custom electronics platform. Manages all vehicle power distribution, driver switch inputs, lighting, relay control, and 4WD. The PDCM is a passive executor — it receives commands and reports states but does not make engine control decisions.

Single Teensy 4.1 on the vehicle CAN FD bus. Defined by ECM ADR-016.

## Platform Submodule
This repo includes `silverado-platform` as a git submodule at `firmware/platform/`. That submodule is the single source of truth for:
- Vehicle bus CAN IDs (`can/VehicleCAN.h`)
- Shared message structs (`can/VehicleMessages.h`)
- Vehicle-wide types (`types/VehicleTypes.h`)
- Module IDs (`types/ModuleIDs.h`)

**Never duplicate CAN IDs or shared types** — always reference the platform headers.

To update the platform submodule:
```bash
cd firmware/platform
git pull origin main
cd ../..
git add firmware/platform
git commit -m "Update platform submodule"
```

## Module Identity
- **Module ID**: `0x10`
- **CAN Range**: `0x350–0x36F`
- **Heartbeat**: `0x36F` @ 500ms

## Systems Owned
1. **Switch Inputs** — Multifunction stalks (turn/cruise), hazard, horn, wiper/washer, HVAC controls, key position
2. **Lighting** — Headlights, turn signals, hazards, brake lights, reverse, interior, DRL
3. **Power Distribution** — Fuel pump relay (shared with ST), cooling fans, A/C clutch, horn, wipers, blower, accessory
4. **4WD Controls** — Transfer case encoder motor (NP246/NP261), front axle actuator
5. **Brake Override** — Dual brake switch inputs for BOP safety

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

### Consumes (5 messages)
| ID | Source | Message |
|----|--------|---------|
| 0x360 | ECM | Fan Command |
| 0x361 | HMI | Light Command |
| 0x362 | HMI | 4WD Command |
| 0x363 | ECM | Relay Command |
| 0x310 | ECM | Drive Mode |

## Engineering Rules
- No hacky shit. No workarounds. No "just for now" code.
- CAN FD everywhere — 1 Mbps arb / 8 Mbps data.
- All CAN structs packed, little-endian.
- PDCM is a passive executor — never make engine decisions.
- Brake state at 50ms rate — safety-critical path for BOP.
- Document decisions in DECISIONS.md.

## Environment
- Firmware: C++ (Arduino/Teensy framework), PlatformIO
- Target: Teensy 4.1 (IMXRT1062, 600 MHz Cortex-M7)
- CAN library: FlexCAN_T4
- Platform headers: `firmware/platform/` (git submodule)

## Session Protocol
**Start**: Read CLAUDE.md → DECISIONS.md → TODO.md → ask what we're working on.
**End**: Update TODO.md, DECISIONS.md, CLAUDE.md if needed. Commit with meaningful message.

## Key Files
- `DECISIONS.md` — Architecture decision records
- `TODO.md` — Current phase and work items
- `docs/can_protocol.md` — Full CAN protocol documentation
- `firmware/platform/` — Silverado platform submodule
- `firmware/shared/PDCMConfig.h` — Module configuration
- `firmware/src/main.cpp` — Main firmware entry point

## Dependencies
- **Required**: ECM (fan commands, relay commands)
- **Optional**: HMI (lighting/4WD commands from touchscreen), TCU (transfer case feedback)

## Current Phase
**Phase 1 — Hardware Design**
