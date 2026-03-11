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

---

## Published Messages (PDCM → Vehicle Bus)

### 0x350 — Switch States (8B, 100ms)

All driver switch inputs packed into a single message.

| Byte | Bits | Field | Description |
|------|------|-------|-------------|
| 0 | 7:6 | turnSignal | 0=off, 1=left, 2=right |
| 0 | 5 | highBeam | High beam active |
| 0 | 4 | flashToPass | Flash-to-pass active |
| 0 | 3 | hazard | Hazard switch on |
| 0 | 2 | horn | Horn button pressed |
| 0 | 1:0 | wiperMode | 0=off, 1=int, 2=low, 3=high |
| 1 | 7 | washer | Washer active |
| 1 | 6:4 | hvacFanSpeed | 0–7 fan speed |
| 1 | 3:2 | hvacMode | 0=off, 1=vent, 2=floor, 3=defrost |
| 1 | 1:0 | keyPosition | 0=off, 1=acc, 2=run, 3=start |
| 2 | 7 | acRequest | Driver A/C request |
| 2 | 6 | rearDefrost | Rear defrost switch |
| 2–7 | — | reserved | Future expansion |

### 0x351 — Light State (4B, 100ms)

Current lighting output state.

| Byte | Bits | Field | Description |
|------|------|-------|-------------|
| 0 | 7 | headlowL | Left low beam on |
| 0 | 6 | headlowR | Right low beam on |
| 0 | 5 | headhighL | Left high beam on |
| 0 | 4 | headhighR | Right high beam on |
| 0 | 3 | turnL | Left turn signal on |
| 0 | 2 | turnR | Right turn signal on |
| 0 | 1 | brakeL | Left brake light on |
| 0 | 0 | brakeR | Right brake light on |
| 1 | 7 | reverse | Reverse lights on |
| 1 | 6 | interior | Interior lights on |
| 1 | 5 | courtesy | Courtesy lights on |
| 1 | 4 | drl | DRL active |
| 1–3 | — | reserved | Future expansion |

### 0x352 — Power State (8B, 500ms)

Relay states and system current monitoring.

| Byte | Bits | Field | Description |
|------|------|-------|-------------|
| 0 | 7 | fuelPump | Fuel pump relay on |
| 0 | 6 | fan1 | Cooling fan 1 on |
| 0 | 5 | fan2 | Cooling fan 2 (if dual) |
| 0 | 4 | acClutch | A/C compressor clutch on |
| 0 | 3 | horn | Horn relay on |
| 0 | 2 | wiperMotor | Wiper motor on |
| 0 | 1 | blower | Blower motor on |
| 0 | 0 | accessory | Accessory power on |
| 1 | — | fanDuty | Cooling fan PWM duty (0–255 = 0–100%) |
| 2–3 | — | totalCurrent | Total system current draw (mA, uint16_t) |
| 4–5 | — | battVoltage | Battery voltage (mV, uint16_t) |
| 6–7 | — | reserved | Future expansion |

### 0x353 — Cruise Button (4B, on-event)

Cruise control stalk button events. Sent immediately on button press/release.

| Byte | Bits | Field | Description |
|------|------|-------|-------------|
| 0 | 7 | onOff | Cruise master on/off |
| 0 | 6 | set | Set button pressed |
| 0 | 5 | resume | Resume button pressed |
| 0 | 4 | accel | Accel (+) button pressed |
| 0 | 3 | decel | Decel (−) button pressed |
| 0 | 2 | cancel | Cancel button pressed |
| 0 | 1:0 | reserved | — |
| 1–3 | — | reserved | Future expansion |

### 0x354 — A/C State (4B, 500ms)

| Byte | Bits | Field | Description |
|------|------|-------|-------------|
| 0 | 7 | clutchOn | A/C compressor clutch engaged |
| 0 | 6 | driverRequest | Driver A/C request active |
| 0 | 5 | lowPressure | Low pressure cutout (if wired) |
| 0 | 4:0 | reserved | — |
| 1–3 | — | reserved | Future expansion |

### 0x355 — 4WD State (4B, 500ms)

| Byte | Bits | Field | Description |
|------|------|-------|-------------|
| 0 | 3:0 | mode | 0=2HI, 1=A4WD, 2=4HI, 3=4LO, 4=neutral |
| 0 | 7:4 | reserved | — |
| 1 | 7 | engaged | Transfer case fully engaged in target mode |
| 1 | 6 | shifting | Transfer case shift in progress |
| 1 | 5 | fault | Transfer case fault |
| 1 | 4 | frontAxle | Front axle actuator engaged |
| 1 | 3:0 | reserved | — |
| 2–3 | — | reserved | Future expansion |

### 0x356 — Brake State (4B, 50ms)

Dual brake switch inputs for brake override protection. 50ms rate for safety-critical BOP path.

| Byte | Bits | Field | Description |
|------|------|-------|-------------|
| 0 | 7 | switch1 | Brake switch 1 (factory tap) |
| 0 | 6 | switch2 | Brake switch 2 (dedicated) |
| 0 | 5 | bopActive | Brake Override Protection active (both switches agree) |
| 0 | 4 | disagree | Switches disagree (fault condition) |
| 0 | 3:0 | reserved | — |
| 1–3 | — | reserved | Future expansion |

### 0x357 — Faults (8B, 1000ms)

| Byte | Bits | Field | Description |
|------|------|-------|-------------|
| 0 | — | faultCount | Number of active faults |
| 1 | 7 | overCurrent | System overcurrent detected |
| 1 | 6 | canTimeout | CAN heartbeat timeout (ECM not responding) |
| 1 | 5 | brakeDisagree | Brake switch disagreement |
| 1 | 4 | tcFault | Transfer case fault |
| 1 | 3 | fanFault | Cooling fan circuit fault |
| 1 | 2 | fuelPumpFault | Fuel pump circuit fault |
| 1 | 1 | lightFault | Lighting circuit fault |
| 1 | 0 | lowVoltage | Battery voltage below threshold |
| 2–7 | — | reserved | Future fault expansion |

### 0x36F — Heartbeat (2B, 500ms)

| Byte | Field | Description |
|------|-------|-------------|
| 0 | moduleId | 0x10 (PDCM) |
| 1 | status | Status flags (TBD) |

---

## Consumed Messages (Vehicle Bus → PDCM)

### 0x360 — Fan Command (from ECM)

Cooling fan speed command. Thermal strategy owned by ECM.

| Byte | Field | Description |
|------|-------|-------------|
| 0 | fanDuty | Fan speed 0–255 (0–100%) |
| 1 | flags | Bit 7: force full speed |

### 0x361 — Light Command (from HMI)

Lighting mode overrides from HeadUnit touchscreen.

| Byte | Field | Description |
|------|-------|-------------|
| 0 | mode | Lighting mode (TBD) |
| 1 | flags | Override flags (TBD) |

### 0x362 — 4WD Command (from HMI)

4WD mode request from driver via HeadUnit touchscreen.

| Byte | Field | Description |
|------|-------|-------------|
| 0 | targetMode | 0=2HI, 1=A4WD, 2=4HI, 3=4LO, 4=neutral |

### 0x363 — Relay Command (from ECM)

Direct relay control commands.

| Byte | Bits | Field | Description |
|------|------|-------|-------------|
| 0 | 7 | fuelPump | Fuel pump relay command |
| 0 | 6 | acClutch | A/C compressor clutch command |
| 0 | 5:0 | reserved | — |

### 0x310 — Drive Mode (from ECM)

Active drive mode. Affects fan strategy and other power distribution behavior.

| Byte | Field | Description |
|------|-------|-------------|
| 0 | driveMode | Active drive mode enum (from VehicleTypes.h) |

---

## Safety Notes

- **Brake State (0x356)** runs at 50ms — fastest rate of any PDCM message. This is the safety-critical path for brake override protection (BOP). The ECM uses dual brake switch agreement from this message to force throttle closed.
- **Fuel pump relay** is shared with ST (Security Teensy in ECM). ST has an independent hardware cutoff path that cannot be overridden by PDCM firmware.
- **Heartbeat (0x36F)** must transmit every 500ms. Other modules monitor this to detect PDCM failure.
- **A/C compressor state** reported to ECM for idle-up compensation — ECM raises idle target when A/C is engaged.
