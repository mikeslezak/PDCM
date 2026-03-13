# PDCM Connector Pinout — Deutsch Automotive Connectors

All physical connectors for the PDCM use Deutsch automotive-grade connectors, grouped by **truck routing zone** rather than channel number. This keeps harness branches clean — one connector per physical area of the truck.

**Signal topology**: Low-side switching. Each load connector pin carries the MOSFET drain (ground-return path). The load's +12V comes from the upstream fuse panel, not from PDCM connectors. One wire per channel, no GND returns needed on load connectors.

---

## Connector Summary

| Ref | Series | Pins | Purpose | Wire Gauge | Location |
|-----|--------|------|---------|------------|----------|
| J1 | HD 2-pin | 2 | Main battery power | 8 AWG | Battery/firewall |
| J2 | DTM 4-pin | 4 | CAN FD bus | 22 AWG | Internal bus |
| J3 | DT 8-pin | 8 | Engine bay loads | 14-16 AWG | Engine bay |
| J4 | DT 12-pin | 12 | Front lighting | 14-16 AWG | Front of truck |
| J5 | DT 6-pin | 6 | Rear lighting | 16 AWG | Rear of truck |
| J6 | DT 8-pin | 8 | Cabin / firewall | 14-18 AWG | Firewall pass-through |
| J7 | DT 6-pin | 6 | 4WD motor + position | 16-22 AWG | Under truck / t-case |
| J8 | DTM 8-pin | 8 | Steering column stalks | 22 AWG | Steering column |
| J9 | DTM 8-pin | 8 | Dash switches | 22 AWG | Dashboard |
| J10 | DTM 4-pin | 4 | Brake switches (isolated) | 20 AWG | Brake pedal |
| J11 | DTM 4-pin | 4 | Reverse + transmission | 22 AWG | Transmission |
| J12 | DT 12-pin | 12 | Electronics / modules | 18-22 AWG | Cabin / under dash |
| J13 | DT 12-pin | 12 | Exterior aux + expansion | 16-18 AWG | Various exterior |

**Totals**: 94 pins (83 signals + 5 GND returns + 2 power + 4 sensor supply), 16 spare

---

## Pin Assignments

### J1 — Main Battery Power (HD 2-pin)
Heavy-duty connector for main battery feed. 8 AWG, 100A/pin rating.

| Pin | Signal | Ch # | Wire | Color | Fuse |
|-----|--------|------|------|-------|------|
| 1 | +12V_BAT | — | 8 AWG | Red | 150A ANL |
| 2 | GND | — | 8 AWG | Black | — |

**Header**: Deutsch HD10-2-16P (panel mount)
**Receptacle**: Deutsch HD30-2-16S (harness side)
**Contacts**: 16 AWG (size 16) solid, 100A

---

### J2 — CAN FD Bus (DTM 4-pin)
Shielded twisted pair for CAN FD. 1 Mbps arb / 8 Mbps data.

| Pin | Signal | Ch # | Wire | Color | Notes |
|-----|--------|------|------|-------|-------|
| 1 | CAN_H | — | 22 AWG | Yellow | CAN+ (twisted pair) |
| 2 | CAN_L | — | 22 AWG | Green | CAN− (twisted pair) |
| 3 | GND | — | 22 AWG | Black | Shield drain |
| 4 | +12V | — | 22 AWG | Red | Transceiver supply |

**Header**: Deutsch DTM04-4P
**Receptacle**: Deutsch DTM06-4S
**Contacts**: Size 20, 7.5A

---

### J3 — Engine Bay Loads (DT 8-pin)
High-current switched outputs routed to engine compartment.

| Pin | Signal | Ch # | Wire | Color | Fuse |
|-----|--------|------|------|-------|------|
| 1 | LOAD_0 | 0 | 14 AWG | Orange | 20A (Fuel pump) |
| 2 | LOAD_1 | 1 | 14 AWG | Blue | 25A (Fan 1) |
| 3 | LOAD_2 | 2 | 14 AWG | Blue/White | 25A (Fan 2) |
| 4 | LOAD_4 | 4 | 16 AWG | Purple | 15A (A/C clutch) |
| 5 | LOAD_17 | 17 | 16 AWG | Green/White | 15A (Horn) |
| 6 | LOAD_20 | 20 | 16 AWG | Brown | 20A (Front axle actuator) |
| 7 | — | — | — | — | Spare |
| 8 | — | — | — | — | Spare |

**Header**: Deutsch DT04-8P
**Receptacle**: Deutsch DT06-8S
**Contacts**: Size 16, 13A (16 AWG) / 25A (14 AWG)

---

### J4 — Front Lighting (DT 12-pin)
All front-facing lighting loads.

| Pin | Signal | Ch # | Wire | Color | Fuse |
|-----|--------|------|------|-------|------|
| 1 | LOAD_5 | 5 | 14 AWG | White | 15A (Low beam L) |
| 2 | LOAD_6 | 6 | 14 AWG | White/Black | 15A (Low beam R) |
| 3 | LOAD_7 | 7 | 14 AWG | Yellow | 15A (High beam L) |
| 4 | LOAD_8 | 8 | 14 AWG | Yellow/Black | 15A (High beam R) |
| 5 | LOAD_9 | 9 | 16 AWG | Green | 10A (Turn signal L) |
| 6 | LOAD_10 | 10 | 16 AWG | Green/Black | 10A (Turn signal R) |
| 7 | LOAD_14 | 14 | 16 AWG | Pink | 10A (DRL) |
| 8 | LOAD_23 | 23 | 14 AWG | Orange/Black | 30A (Light bar) |
| 9 | — | — | — | — | Spare |
| 10 | — | — | — | — | Spare |
| 11 | — | — | — | — | Spare |
| 12 | — | — | — | — | Spare |

**Header**: Deutsch DT04-12P
**Receptacle**: Deutsch DT06-12S
**Contacts**: Size 16, 13-25A

---

### J5 — Rear Lighting (DT 6-pin)
Tail/brake/reverse/bed lights.

| Pin | Signal | Ch # | Wire | Color | Fuse |
|-----|--------|------|------|-------|------|
| 1 | LOAD_11 | 11 | 16 AWG | Red | 10A (Brake light L) |
| 2 | LOAD_12 | 12 | 16 AWG | Red/Black | 10A (Brake light R) |
| 3 | LOAD_13 | 13 | 16 AWG | White/Green | 10A (Reverse lights) |
| 4 | LOAD_38 | 38 | 16 AWG | Blue/Red | 10A (Bed lights) |
| 5 | — | — | — | — | Spare |
| 6 | — | — | — | — | Spare |

**Header**: Deutsch DT04-6P
**Receptacle**: Deutsch DT06-6S
**Contacts**: Size 16, 13A

---

### J6 — Cabin / Firewall (DT 8-pin)
Interior loads routed through the firewall.

| Pin | Signal | Ch # | Wire | Color | Fuse |
|-----|--------|------|------|-------|------|
| 1 | LOAD_3 | 3 | 14 AWG | Blue/Yellow | 25A (Blower motor) |
| 2 | LOAD_18 | 18 | 16 AWG | Tan | 15A (Wiper motor) |
| 3 | LOAD_15 | 15 | 16 AWG | Gray | 10A (Interior light) |
| 4 | LOAD_16 | 16 | 16 AWG | Gray/Black | 10A (Courtesy light) |
| 5 | LOAD_19 | 19 | 16 AWG | Pink/Black | 15A (Accessory power) |
| 6 | LOAD_21 | 21 | 16 AWG | Brown/White | 15A (Seat heater L) |
| 7 | LOAD_22 | 22 | 16 AWG | Brown/Black | 15A (Seat heater R) |
| 8 | — | — | — | — | Spare |

**Header**: Deutsch DT04-8P
**Receptacle**: Deutsch DT06-8S
**Contacts**: Size 16, 13-25A

---

### J7 — 4WD Motor + Position (DT 6-pin)
Transfer case encoder motor (H-bridge) and position sensor.

| Pin | Signal | Ch # | Wire | Color | Fuse |
|-----|--------|------|------|-------|------|
| 1 | MOTOR_OUT | 24 (H) | 16 AWG | Violet | — (DRV8876 out A) |
| 2 | MOTOR_GND | 24 (H) | 16 AWG | Violet/Black | — (DRV8876 out B) |
| 3 | 4WD_POS_RAW | — | 22 AWG | Orange/White | — (Pot wiper) |
| 4 | +5V | — | 22 AWG | Red/White | — (Pot supply) |
| 5 | GND | — | 22 AWG | Black | — (Pot ground) |
| 6 | — | — | — | — | Spare |

**Header**: Deutsch DT04-6P
**Receptacle**: Deutsch DT06-6S
**Contacts**: Size 16/20 mixed

---

### J8 — Steering Column Stalks (DTM 8-pin)
Turn signal, high beam, and cruise switch inputs from steering column.

| Pin | Signal | Ch # | Wire | Color | Notes |
|-----|--------|------|------|-------|-------|
| 1 | SW_TURN_L_IN | — | 22 AWG | Green | Turn stalk left |
| 2 | SW_TURN_R_IN | — | 22 AWG | Green/Black | Turn stalk right |
| 3 | SW_HIGH_BEAM_IN | — | 22 AWG | Yellow/White | High beam switch |
| 4 | SW_FLASH_PASS_IN | — | 22 AWG | Yellow/Green | Flash-to-pass |
| 5 | SW_HORN_IN | — | 22 AWG | Green/White | Horn button |
| 6 | GND | — | 22 AWG | Black | Switch common |
| 7 | — | — | — | — | Spare |
| 8 | — | — | — | — | Spare |

**Header**: Deutsch DTM04-8P
**Receptacle**: Deutsch DTM06-8S
**Contacts**: Size 20, 7.5A

---

### J9 — Dash Switches (DTM 8-pin)
Dashboard-mounted switch inputs.

| Pin | Signal | Ch # | Wire | Color | Notes |
|-----|--------|------|------|-------|-------|
| 1 | SW_HAZARD_IN | — | 22 AWG | Red/Yellow | Hazard button |
| 2 | SW_AC_REQ_IN | — | 22 AWG | Purple/White | A/C request |
| 3 | SW_WIPER_INT_IN | — | 22 AWG | Tan/White | Wiper intermittent |
| 4 | SW_WIPER_LO_IN | — | 22 AWG | Tan/Black | Wiper low |
| 5 | SW_WIPER_HI_IN | — | 22 AWG | Tan/Red | Wiper high |
| 6 | SW_WASHER_IN | — | 22 AWG | Tan/Green | Washer button |
| 7 | SW_START_BTN_IN | — | 22 AWG | White/Red | Push-button start |
| 8 | GND | — | 22 AWG | Black | Switch common |

**Header**: Deutsch DTM04-8P
**Receptacle**: Deutsch DTM06-8S
**Contacts**: Size 20, 7.5A

---

### J10 — Brake Switches (DTM 4-pin) — SAFETY ISOLATED
Dedicated connector for dual brake switches. Safety-critical 50ms BOP path gets its own connector with individual GND returns per switch to eliminate ground-loop noise.

| Pin | Signal | Ch # | Wire | Color | Notes |
|-----|--------|------|------|-------|-------|
| 1 | SW_BRAKE_SW1_IN | — | 20 AWG | Red/Green | Brake switch 1 |
| 2 | GND | — | 20 AWG | Black | SW1 dedicated return |
| 3 | SW_BRAKE_SW2_IN | — | 20 AWG | Red/Blue | Brake switch 2 |
| 4 | GND | — | 20 AWG | Black/White | SW2 dedicated return |

**Header**: Deutsch DTM04-4P
**Receptacle**: Deutsch DTM06-4S
**Contacts**: Size 20, 7.5A

---

### J11 — Reverse + Transmission (DTM 4-pin)
Reverse lamp switch and misc transmission signals.

| Pin | Signal | Ch # | Wire | Color | Notes |
|-----|--------|------|------|-------|-------|
| 1 | SW_REVERSE_IN | — | 22 AWG | White/Green | Reverse switch |
| 2 | GND | — | 22 AWG | Black | Switch return |
| 3 | — | — | — | — | Spare |
| 4 | — | — | — | — | Spare |

**Header**: Deutsch DTM04-4P
**Receptacle**: Deutsch DTM06-4S
**Contacts**: Size 20, 7.5A

---

### J12 — Electronics / Modules (DT 12-pin)
Enable signals and power for electronic modules (cameras, ADAS, amps, head unit).

| Pin | Signal | Ch # | Wire | Color | Fuse |
|-----|--------|------|------|-------|------|
| 1 | LOAD_25 | 25 | 18 AWG | Yellow/Blue | 5A (Amp remote 1) |
| 2 | LOAD_26 | 26 | 18 AWG | Yellow/Red | 5A (Amp remote 2) |
| 3 | LOAD_27 | 27 | 18 AWG | White/Blue | 5A (HeadUnit enable) |
| 4 | LOAD_28 | 28 | 18 AWG | Orange/Green | 5A (Front camera) |
| 5 | LOAD_29 | 29 | 18 AWG | Orange/Blue | 5A (Rear camera) |
| 6 | LOAD_30 | 30 | 18 AWG | Orange/Red | 5A (Side cameras) |
| 7 | LOAD_31 | 31 | 18 AWG | White/Orange | 5A (Parking sensors) |
| 8 | LOAD_32 | 32 | 18 AWG | White/Yellow | 5A (Radar / BSM) |
| 9 | LOAD_33 | 33 | 18 AWG | White/Brown | 5A (GCM power) |
| 10 | LOAD_34 | 34 | 18 AWG | White/Purple | 5A (GPS / cellular) |
| 11 | LOAD_35 | 35 | 18 AWG | White/Gray | 5A (Dash cam) |
| 12 | LOAD_36 | 36 | 18 AWG | Gray/Red | 5A (Future module) |

**Header**: Deutsch DT04-12P
**Receptacle**: Deutsch DT06-12S
**Contacts**: Size 16, 13A

---

### J13 — Exterior Aux + Expansion (DT 12-pin)
External aux lighting and spare expansion channels.

| Pin | Signal | Ch # | Wire | Color | Fuse |
|-----|--------|------|------|-------|------|
| 1 | LOAD_37 | 37 | 16 AWG | Blue/Green | 15A (Rock lights) |
| 2 | LOAD_39 | 39 | 16 AWG | Blue/Orange | 10A (Puddle / underbody) |
| 3 | LOAD_40 | 40 | 18 AWG | Gray/Green | 5A (Future exterior) |
| 4 | LOAD_41 | 41 | 18 AWG | Violet/White | 5A (Expansion 1) |
| 5 | LOAD_42 | 42 | 18 AWG | Violet/Red | 5A (Expansion 2) |
| 6 | LOAD_43 | 43 | 18 AWG | Violet/Blue | 5A (Expansion 3) |
| 7 | LOAD_44 | 44 | 18 AWG | Violet/Green | 5A (Expansion 4) |
| 8 | LOAD_45 | 45 | 18 AWG | Violet/Yellow | 5A (Expansion 5) |
| 9 | LOAD_46 | 46 | 18 AWG | Violet/Black | 5A (Expansion 6) |
| 10 | — | — | — | — | Spare |
| 11 | — | — | — | — | Spare |
| 12 | — | — | — | — | Spare |

**Header**: Deutsch DT04-12P
**Receptacle**: Deutsch DT06-12S
**Contacts**: Size 16, 13A

---

## Wire Color Scheme

| Color | Usage |
|-------|-------|
| Red | +12V battery power |
| Black | Ground / returns |
| White, White/* | Low beam, headunit, general signal |
| Yellow, Yellow/* | High beam, CAN_H, amp enables |
| Green, Green/* | CAN_L, turn signals, horn |
| Blue, Blue/* | Fans, exterior lighting |
| Orange, Orange/* | Fuel pump, cameras, light bar |
| Brown, Brown/* | Axle actuator, GCM |
| Purple (Violet), Violet/* | A/C, 4WD motor, expansion |
| Gray, Gray/* | Interior lights, module enables |
| Tan, Tan/* | Wiper switches |
| Pink, Pink/* | DRL, accessory |
| Red/* (compound) | Brake / safety signals |

Stripe convention: `Base/Stripe` (e.g., White/Black = white wire with black stripe).

---

## Deutsch Part Number Reference

### HD Series (J1 — Battery Power)
| Part | Description |
|------|-------------|
| HD10-2-16P | 2-pin header (panel mount, size 16 contacts) |
| HD30-2-16S | 2-pin receptacle (harness side) |
| 0462-209-16141 | HD size 16 pin contact (stamped, 8-10 AWG) |
| 0460-215-16141 | HD size 16 socket contact (stamped, 8-10 AWG) |

### DT Series (J3, J4, J5, J6, J7, J12, J13 — Loads)
| Part | Description |
|------|-------------|
| DT04-2P through DT04-12P | 2-12 pin header (panel mount) |
| DT06-2S through DT06-12S | 2-12 pin receptacle (harness side) |
| 0462-201-16141 | DT size 16 pin contact (stamped, 14-16 AWG) |
| 0460-202-16141 | DT size 16 socket contact (stamped, 14-16 AWG) |
| 0462-209-16141 | DT size 16 pin contact (stamped, 16-18 AWG) |
| 0460-215-16141 | DT size 16 socket contact (stamped, 16-18 AWG) |
| W2P, W2S, etc. | Wedgelocks (required, one per connector) |

### DTM Series (J2, J8, J9, J10, J11 — Signals/Switches)
| Part | Description |
|------|-------------|
| DTM04-2P through DTM04-8P | 2-8 pin header (panel mount) |
| DTM06-2S through DTM06-8S | 2-8 pin receptacle (harness side) |
| 0460-202-20141 | DTM size 20 pin contact (stamped, 20-24 AWG) |
| 0462-201-20141 | DTM size 20 socket contact (stamped, 20-24 AWG) |
| DTM04-xP-Lxxx | Wedgelocks (included or separate per config) |

---

## Schematic Label → Connector Pin Mapping

Quick reference for tracing signals from schematic global labels to physical pins.

### Load Outputs (LOAD_N → Connector)
| Label | Ch | Connector | Pin | Load |
|-------|-----|-----------|-----|------|
| LOAD_0 | 0 | J3 | 1 | Fuel pump |
| LOAD_1 | 1 | J3 | 2 | Fan 1 |
| LOAD_2 | 2 | J3 | 3 | Fan 2 |
| LOAD_3 | 3 | J6 | 1 | Blower motor |
| LOAD_4 | 4 | J3 | 4 | A/C clutch |
| LOAD_5 | 5 | J4 | 1 | Low beam L |
| LOAD_6 | 6 | J4 | 2 | Low beam R |
| LOAD_7 | 7 | J4 | 3 | High beam L |
| LOAD_8 | 8 | J4 | 4 | High beam R |
| LOAD_9 | 9 | J4 | 5 | Turn signal L |
| LOAD_10 | 10 | J4 | 6 | Turn signal R |
| LOAD_11 | 11 | J5 | 1 | Brake light L |
| LOAD_12 | 12 | J5 | 2 | Brake light R |
| LOAD_13 | 13 | J5 | 3 | Reverse lights |
| LOAD_14 | 14 | J4 | 7 | DRL |
| LOAD_15 | 15 | J6 | 3 | Interior light |
| LOAD_16 | 16 | J6 | 4 | Courtesy light |
| LOAD_17 | 17 | J3 | 5 | Horn |
| LOAD_18 | 18 | J6 | 2 | Wiper motor |
| LOAD_19 | 19 | J6 | 5 | Accessory power |
| LOAD_20 | 20 | J3 | 6 | Front axle actuator |
| LOAD_21 | 21 | J6 | 6 | Seat heater L |
| LOAD_22 | 22 | J6 | 7 | Seat heater R |
| LOAD_23 | 23 | J4 | 8 | Light bar |
| MOTOR_OUT | 24 | J7 | 1 | 4WD encoder motor |
| LOAD_25 | 25 | J12 | 1 | Amp remote 1 |
| LOAD_26 | 26 | J12 | 2 | Amp remote 2 |
| LOAD_27 | 27 | J12 | 3 | HeadUnit enable |
| LOAD_28 | 28 | J12 | 4 | Front camera |
| LOAD_29 | 29 | J12 | 5 | Rear camera |
| LOAD_30 | 30 | J12 | 6 | Side cameras |
| LOAD_31 | 31 | J12 | 7 | Parking sensors |
| LOAD_32 | 32 | J12 | 8 | Radar / BSM |
| LOAD_33 | 33 | J12 | 9 | GCM power |
| LOAD_34 | 34 | J12 | 10 | GPS / cellular |
| LOAD_35 | 35 | J12 | 11 | Dash cam |
| LOAD_36 | 36 | J12 | 12 | Future module |
| LOAD_37 | 37 | J13 | 1 | Rock lights |
| LOAD_38 | 38 | J5 | 4 | Bed lights |
| LOAD_39 | 39 | J13 | 2 | Puddle / underbody |
| LOAD_40 | 40 | J13 | 3 | Future exterior |
| LOAD_41 | 41 | J13 | 4 | Expansion 1 |
| LOAD_42 | 42 | J13 | 5 | Expansion 2 |
| LOAD_43 | 43 | J13 | 6 | Expansion 3 |
| LOAD_44 | 44 | J13 | 7 | Expansion 4 |
| LOAD_45 | 45 | J13 | 8 | Expansion 5 |
| LOAD_46 | 46 | J13 | 9 | Expansion 6 |

### Switch Inputs (SW_*_IN → Connector)
| Label | Connector | Pin |
|-------|-----------|-----|
| SW_TURN_L_IN | J8 | 1 |
| SW_TURN_R_IN | J8 | 2 |
| SW_HIGH_BEAM_IN | J8 | 3 |
| SW_FLASH_PASS_IN | J8 | 4 |
| SW_HORN_IN | J8 | 5 |
| SW_HAZARD_IN | J9 | 1 |
| SW_AC_REQ_IN | J9 | 2 |
| SW_WIPER_INT_IN | J9 | 3 |
| SW_WIPER_LO_IN | J9 | 4 |
| SW_WIPER_HI_IN | J9 | 5 |
| SW_WASHER_IN | J9 | 6 |
| SW_START_BTN_IN | J9 | 7 |
| SW_BRAKE_SW1_IN | J10 | 1 |
| SW_BRAKE_SW2_IN | J10 | 3 |
| SW_REVERSE_IN | J11 | 1 |

### Power / Bus / Sensors
| Label | Connector | Pin |
|-------|-----------|-----|
| +12V_BAT | J1 | 1 |
| GND (battery) | J1 | 2 |
| CAN_H | J2 | 1 |
| CAN_L | J2 | 2 |
| 4WD_POS_RAW | J7 | 3 |
| +5V (pot supply) | J7 | 4 |
