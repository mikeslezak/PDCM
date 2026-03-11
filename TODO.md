# TODO — PDCM Project

## Current Phase: Phase 1 — Hardware Design

---

### Phase 1 — Hardware Design
- [ ] Select MCU mounting approach (Teensy 4.1 daughtercard vs bare IMXRT1062)
- [ ] Schematic — Power input (12V automotive, TVS, reverse polarity, 5V regulator)
- [ ] Schematic — CAN FD transceiver (MCP2562FD)
- [ ] Schematic — Teensy 4.1 connections and pin allocation
- [ ] Schematic — High-side switch circuits for lighting outputs
- [ ] Schematic — Relay driver circuits (fuel pump, fans, A/C clutch, horn, wipers, blower)
- [ ] Schematic — Switch input conditioning (stalks, hazard, horn, wiper, key position)
- [ ] Schematic — Cruise stalk input conditioning
- [ ] Schematic — Dual brake switch inputs (BOP)
- [ ] Schematic — 4WD outputs (transfer case encoder motor driver, front axle actuator)
- [ ] Schematic — Current sensing for power monitoring
- [ ] PCB layout
- [ ] Connector pinout documentation
- [ ] BOM

### Phase 2 — Firmware Skeleton
- [ ] CAN bus init and heartbeat (0x36F)
- [ ] CAN receive callbacks for consumed messages
- [ ] Pin assignments in PDCMConfig.h
- [ ] Basic main loop timing structure

### Phase 3 — Switch Inputs
- [ ] Multifunction stalk left (turn signals, high beams, flash-to-pass)
- [ ] Multifunction stalk right (cruise buttons) — on-event CAN (0x353)
- [ ] Hazard switch, horn button
- [ ] Wiper/washer switch
- [ ] HVAC controls (fan speed, mode)
- [ ] Key position detection (off/acc/run/start)
- [ ] Pack and publish 0x350 Switch States @ 100ms

### Phase 4 — Lighting
- [ ] Headlight control (low/high beam switching)
- [ ] Turn signal logic (with hazard override)
- [ ] Brake light output
- [ ] Reverse light output
- [ ] Interior/courtesy lights
- [ ] DRL logic
- [ ] HMI light command processing (0x361)
- [ ] Pack and publish 0x351 Light State @ 100ms

### Phase 5 — Power Distribution
- [ ] Fuel pump relay control (coordinate with ST safety cutoff)
- [ ] Cooling fan PWM output (from ECM fan command 0x360)
- [ ] A/C compressor clutch relay
- [ ] Horn relay
- [ ] Wiper motor relay
- [ ] Blower motor (HVAC)
- [ ] Accessory power
- [ ] Current sensing and monitoring
- [ ] ECM relay command processing (0x363)
- [ ] Pack and publish 0x352 Power State @ 500ms
- [ ] Pack and publish 0x354 A/C State @ 500ms

### Phase 6 — 4WD Controls
- [ ] Transfer case encoder motor driver (NP246/NP261)
- [ ] Front axle actuator control
- [ ] 4WD position feedback
- [ ] HMI 4WD command processing (0x362)
- [ ] Pack and publish 0x355 4WD State @ 500ms

### Phase 7 — Integration Testing
- [ ] Brake state publication (0x356 @ 50ms) — validate BOP path latency
- [ ] Fault detection and reporting (0x357 @ 1000ms)
- [ ] Drive mode reception (0x310) — validate fan strategy changes
- [ ] End-to-end CAN message validation with ECM
- [ ] End-to-end CAN message validation with HMI
- [ ] Heartbeat monitoring — verify 500ms cadence
- [ ] Power-on self-test sequence
