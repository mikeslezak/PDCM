# PDCM CAN FD Protocol

## Bus Configuration

| Parameter | Value |
|-----------|-------|
| Bus | Vehicle CAN FD |
| Arbitration Rate | 1 Mbps |
| Data Rate | 8 Mbps |
| Transceiver | MCP2562FD |
| CAN ID Format | Standard 11-bit |
| Module ID | 0x10 |
| CAN Range | 0x350–0x36F |

**Source of truth**: CAN IDs and struct definitions are in the `silverado-platform` submodule (`can/VehicleCAN.h` and `can/VehicleMessages.h`). This document provides field-level detail.

---

## Published Messages (PDCM → Vehicle Bus)

### 0x350 — Switch States (8B, 100ms) — `VehMsgSwitchState`

All driver switch inputs packed into a single message.

| Byte | Field | Description |
|------|-------|-------------|
| 0 | stalk_left | b0=turn_left, b1=turn_right, b2=high_beam, b3=flash_to_pass |
| 1 | stalk_right | Cruise stalk (raw state, events sent via 0x353) |
| 2 | hazards | b0=hazard_on, b1=horn |
| 3 | wipers | WiperMode enum (0=off, 1=int, 2=low, 3=high, 4=wash) |
| 4 | hvac_fan | Blower speed 0–7 |
| 5 | hvac_mode | Mode selector (0=off, 1=vent, 2=floor, 3=defrost) |
| 6 | key_position | KeyPosition enum (0=off, 1=acc, 2=run, 3=start) |
| 7 | reserved | — |

### 0x351 — Light State (4B, 100ms) — `VehMsgLightState`

Current lighting output state (actual MOSFET state, not switch position).

| Byte | Field | Description |
|------|-------|-------------|
| 0 | headlights | b7=lowL, b6=lowR, b5=hiL, b4=hiR, b3=turnL, b2=turnR, b1=brakeL, b0=brakeR |
| 1 | aux_lights | b7=reverse, b6=interior, b5=courtesy, b4=drl |
| 2–3 | reserved | — |

### 0x352 — Power State (8B, 500ms) — `VehMsgPowerState`

Output states and system monitoring.

| Byte | Field | Description |
|------|-------|-------------|
| 0 | relay_states | b7=fuelPump, b6=fan1, b5=fan2, b4=acClutch, b3=horn, b2=wiper, b1=blower, b0=accessory |
| 1 | fan_duty | Cooling fan PWM duty (0–255 = 0–100%) |
| 2–3 | total_current_mA | Total system current draw (mA, uint16_t LE) |
| 4–5 | battery_mv | Battery voltage (mV, uint16_t LE) |
| 6–7 | reserved | — |

### 0x353 — Cruise Button (4B, on-event) — `VehMsgCruiseBtn`

Cruise control stalk button events. Sent immediately on button press.

| Byte | Field | Description |
|------|-------|-------------|
| 0 | event | 0=set, 1=resume, 2=accel, 3=decel, 4=cancel, 5=on/off |
| 1 | hold_ms_hi | Hold duration ms (high byte) |
| 2 | hold_ms_lo | Hold duration ms (low byte) |
| 3 | reserved | — |

### 0x354 — A/C State (4B, 500ms) — `VehMsgACState`

| Byte | Field | Description |
|------|-------|-------------|
| 0 | ac_flags | b7=clutchOn, b6=driverRequest, b5=lowPressure |
| 1–3 | reserved | — |

### 0x355 — 4WD State (4B, 500ms) — `VehMsgFourWDState`

| Byte | Field | Description |
|------|-------|-------------|
| 0 | mode | Lower nibble: TransferCaseMode enum (0=2HI, 1=A4WD, 2=4HI, 3=Neutral, 4=4LO) |
| 1 | status | b7=engaged, b6=shifting, b5=fault, b4=frontAxle |
| 2–3 | reserved | — |

### 0x356 — Brake State (4B, 50ms) — `VehMsgBrakeState`

Dual brake switch inputs for brake override protection. **50ms rate — fastest of any PDCM message.** Safety-critical BOP path.

| Byte | Field | Description |
|------|-------|-------------|
| 0 | brake_flags | b7=switch1, b6=switch2, b5=bopActive, b4=disagree |
| 1–3 | reserved | — |

### 0x357 — Faults (8B, 1000ms) — `VehMsgPDCMFaults`

| Byte | Field | Description |
|------|-------|-------------|
| 0 | fault_count | Number of active faults |
| 1 | fault_flags | b7=overcurrent, b6=canTimeout, b5=brakeDisagree, b4=tcFault, b3=fanFault, b2=fuelPumpFault, b1=lightFault, b0=lowVoltage |
| 2–7 | reserved | Future fault expansion |

### 0x36F — Heartbeat (2B, 500ms) — `VehMsgHeartbeat`

| Byte | Field | Description |
|------|-------|-------------|
| 0 | module_id | 0x10 (PDCM) |
| 1 | status | HeartbeatStatus enum (0=OK, 1=warning, 2=fault, 0xFF=shutting_down) |

---

## Consumed Messages (Vehicle Bus → PDCM)

### 0x360 — Fan Command (4B, on-demand, ECM → PDCM) — `VehMsgFanCmd`

| Byte | Field | Description |
|------|-------|-------------|
| 0 | fan_duty | Fan speed 0–255 (0–100%) |
| 1 | flags | b7=forceFullSpeed |
| 2–3 | reserved | — |

### 0x361 — Light Command (4B, on-demand, HMI → PDCM) — `VehMsgLightCmd`

| Byte | Field | Description |
|------|-------|-------------|
| 0 | mode | LightMode enum |
| 1 | flags | Override flags (TBD) |
| 2–3 | reserved | — |

### 0x362 — 4WD Command (4B, on-demand, HMI → PDCM) — `VehMsgFourWDCmd`

| Byte | Field | Description |
|------|-------|-------------|
| 0 | target_mode | TransferCaseMode enum (0=2HI, 1=A4WD, 2=4HI, 3=Neutral, 4=4LO) |
| 1–3 | reserved | — |

### 0x363 — Relay Command (4B, on-demand, ECM → PDCM) — `VehMsgRelayCmd`

| Byte | Field | Description |
|------|-------|-------------|
| 0 | relay_flags | b7=fuelPump, b6=acClutch |
| 1–3 | reserved | — |

### 0x310 — Drive Mode (8B, on-change, ECM → all) — `VehMsgDriveMode`

Active drive mode. PDCM uses this for fan strategy adjustments.

### 0x31F — ECM Heartbeat (2B, 500ms) — `VehMsgHeartbeat`

ECM alive indicator. If no heartbeat received within 3000ms, PDCM sets CAN_TIMEOUT fault and forces fans to 100%.

---

## CAN Timeout Failsafe States

If ECM heartbeat is lost for >3 seconds:

| Load | Failsafe Action | Rationale |
|------|-----------------|-----------|
| Cooling fans | 100% | Prevent engine overheat |
| Fuel pump | OFF | Prevent fuel flood |
| Headlights | Maintain current | Don't lose visibility |
| A/C compressor | OFF | Reduce alternator load |
| All other outputs | Maintain current | Conservative approach |

---

## Safety Notes

- **Brake State (0x356)** runs at 50ms — fastest rate of any PDCM message. Safety-critical path for brake override protection (BOP). Brake switches are on DIRECT MCU GPIO — never behind SPI/I2C.
- **Fuel pump** has ECM/ST independent hardware cutoff (cannot be overridden by PDCM firmware).
- **Seat heaters** have dual MOSFETs in series for fire prevention if primary MOSFET fails short.
- **Heartbeat (0x36F)** must transmit every 500ms. Other modules monitor for PDCM failure.
- **A/C compressor state** reported to ECM for idle-up compensation.
