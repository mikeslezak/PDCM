# Architecture Decision Records — PDCM

All significant technical decisions are logged here in ADR format.

---

## ADR-001: Full Discrete Solid-State Power Distribution

**Date**: 2026-03-11
**Status**: Accepted

All 47 output channels use TC4427A dual gate driver + N-channel MOSFET. Same proven circuit topology as ECM (ADR-025): GPIO → TC4427A → 100Ω gate resistor → MOSFET gate, 10kΩ pulldown to GND.

**Decision:**
- Zero mechanical relays anywhere in the PDCM
- Low-side switching with upstream fusing for wire protection
- TC4427A provides 12V gate drive from 3.3V MCU logic, 1.5A peak sink/source
- 100Ω gate resistor controls slew rate for EMI reduction
- 10kΩ pulldown ensures MOSFET OFF during MCU boot/reset
- All channels individually fused upstream of the MOSFET

**Rationale:**
- Mechanical relays have limited cycle life, arc, corrode contacts, and fail mechanically
- Solid-state is silent, instant, PWM-capable, and infinitely cycleable
- TC4427A is AEC-Q100, automotive-grade, proven in ECM gate driver circuits
- 50× TC4427A already on hand

---

## ADR-002: NXP S32K358 as Primary MCU (No Teensy Prototype)

**Date**: 2026-03-11
**Updated**: 2026-03-12
**Status**: Accepted

**MCU**: NXP S32K358GHT1MPCST — dual Cortex-M7 @ 240MHz, AEC-Q100 Grade 1, HDQFP-172.
- 8MB flash, 1MB SRAM
- 6× CAN FD controllers
- Hardware lockstep safety core
- -40°C to +150°C junction

**Decision:**
- Build directly on S32K358 from day one — no Teensy 4.1 prototype phase
- S32K358 is the only target MCU for PDCM
- 50× S32K358 already on hand
- Teensy HAL retained in repo for reference/testing but is not the build target

**Rationale:**
- 50 chips on hand — no reason to prototype on a different MCU and port later
- Skipping Teensy eliminates an entire prototype→port cycle
- S32K358 has enough GPIO and ADC channels to remove all SPI peripherals (MCP23S17, CD74HC4067) — drastically simpler hardware
- AEC-Q100 Grade 1 automotive qualification
- Lockstep cores provide hardware fault detection for safety-critical functions (brakes)
- 6× CAN FD — enough for any module, no external CAN controllers needed
- 172 pins — enough GPIO for all 47 outputs + all switch inputs + all ADC, no expanders
- Platform-wide standardization (same MCU across ECM, PDCM, GCM, future modules)

---

## ADR-003: 47-Channel Output Allocation (Two Shunt Sizes)

**Date**: 2026-03-11
**Updated**: 2026-03-12
**Status**: Accepted

Channels are **generic pools** — any channel of the right shunt size can drive any matching load. Load-to-channel assignment happens at install time (firmware config + fuse selection). See ADR-010.

### Tier 0 — Always Hot (own battery feed, PDCM cannot control)
- ECM (safety-critical, independent power)
- PDCM itself (own fused battery feed)
- Starter solenoid — controlled by PDCM via any available 10mΩ output channel, triggered by push-button start state machine (see ADR-011)

### Tier 1 — PDCM Switched (25 channels, current through MOSFETs)

| Ch | Load | MOSFET | Shunt | Notes |
|----|------|--------|-------|-------|
| 1 | Fuel pump | IRFZ44N (49A) | 10mΩ | Soft-start, prime-and-timeout |
| 2 | Cooling fan 1 | IRFZ44N | 10mΩ | PWM variable speed |
| 3 | Cooling fan 2 | IRFZ44N | 10mΩ | PWM variable speed |
| 4 | Blower motor | IRFZ44N | 10mΩ | PWM variable speed |
| 5 | A/C compressor clutch | IRFZ44N | 10mΩ | Reports to ECM for idle-up |
| 6 | Low beam left | IRFZ44N | 10mΩ | PWM soft-start |
| 7 | Low beam right | IRFZ44N | 10mΩ | PWM soft-start |
| 8 | High beam left | IRFZ44N | 10mΩ | PWM soft-start |
| 9 | High beam right | IRFZ44N | 10mΩ | PWM soft-start |
| 10 | Turn signal left | IRFZ44N | 10mΩ | Flash timer |
| 11 | Turn signal right | IRFZ44N | 10mΩ | Flash timer |
| 12 | Brake light left | IRFZ44N | 10mΩ | Direct from brake GPIO |
| 13 | Brake light right | IRFZ44N | 10mΩ | Direct from brake GPIO |
| 14 | Reverse lights | IRFZ44N | 10mΩ | |
| 15 | DRL | IRFZ44N | 10mΩ | PWM dimming |
| 16 | Interior light | IRFZ44N | 10mΩ | PWM fade |
| 17 | Courtesy light | IRFZ44N | 10mΩ | PWM fade |
| 18 | Horn | IRFZ44N | 10mΩ | Direct from switch |
| 19 | Wiper motor | IRFZ44N | 10mΩ | Intermittent timing |
| 20 | Accessory power | IRFZ44N | 10mΩ | Key-switched |
| 21 | Front axle actuator | IRFZ44N | 10mΩ | 4WD engage/disengage |
| 22 | Seat heater left | IRFZ44N | 10mΩ | PWM temperature control |
| 23 | Seat heater right | IRFZ44N | 10mΩ | PWM temperature control |
| 24 | Light bar | IRFZ44N | 10mΩ | High current, switched |
| 25 | 4WD encoder motor | H-bridge (DRV8876) | built-in | Bidirectional, see ADR-008 |

### Tier 2 — PDCM Enable (low-current turn-on signal, load has own battery feed)

| Ch | Load | MOSFET | Shunt | Notes |
|----|------|--------|-------|-------|
| 26 | Amp remote #1 | IRFZ44N | 50mΩ | SQ system amp 1 |
| 27 | Amp remote #2 | IRFZ44N | 50mΩ | SQ system amp 2 |
| 28 | HeadUnit enable | IRFZ44N | 50mΩ | Jetson Orin Nano |

### Tier 3 — Individually Switched Sub-Loads + Expansion

| Ch | Load | MOSFET | Shunt | Group |
|----|------|--------|-------|-------|
| 29 | Front camera | IRFZ44N | 10mΩ | ADAS |
| 30 | Rear camera | IRFZ44N | 10mΩ | ADAS |
| 31 | Side cameras (pair) | IRFZ44N | 10mΩ | ADAS |
| 32 | Parking sensor array | IRFZ44N | 10mΩ | ADAS |
| 33 | Radar / blind spot | IRFZ44N | 10mΩ | ADAS |
| 34 | Gauge cluster (GCM) | IRFZ44N | 10mΩ | Modules |
| 35 | GPS / cellular | IRFZ44N | 10mΩ | Modules |
| 36 | Dash cam | IRFZ44N | 10mΩ | Modules |
| 37 | Future module | IRFZ44N | 10mΩ | Modules |
| 38 | Rock lights | IRFZ44N | 10mΩ | Exterior |
| 39 | Bed lights | IRFZ44N | 10mΩ | Exterior |
| 40 | Puddle / underbody | IRFZ44N | 10mΩ | Exterior |
| 41 | Future exterior | IRFZ44N | 50mΩ | Exterior |
| 42 | Expansion 1 | IRFZ44N | 50mΩ | Spare |
| 43 | Expansion 2 | IRFZ44N | 50mΩ | Spare |
| 44 | Expansion 3 | IRFZ44N | 50mΩ | Spare |
| 45 | Expansion 4 | IRFZ44N | 50mΩ | Spare |
| 46 | Expansion 5 | IRFZ44N | 50mΩ | Spare |
| 47 | Expansion 6 | IRFZ44N | 50mΩ | Spare |

**Shunt summary: 10mΩ × 36 channels + 50mΩ × 10 channels + 1 H-bridge (built-in sense) = 47**
**TC4427A count: 24 ICs** (47 channels ÷ 2 outputs per IC, rounded up) — ~$30
**Upstream fuse count: 47** (PTC resettable or blade, per channel)

---

## ADR-004: Per-Channel Current Sensing

**Date**: 2026-03-11
**Updated**: 2026-03-12
**Status**: Accepted

Low-side shunt resistor on every output channel. Two shunt sizes (see ADR-010):
- **10mΩ × 36 channels** — heavy loads (1–16A measurement range)
- **50mΩ × 10 channels** — light loads / enable signals (0.3–3.3A range)

**Current sense amplifier:** INA180A1 (gain = 20) universal across all channels. Single gain variant simplifies BOM and eliminates per-tier part number tracking.

**Implementation**: Direct ADC — S32K358 has 2× SAR ADC instances with 40+ external channels. Every shunt gets its own dedicated ADC channel. No analog MUX needed.

**Decision:**
- Software overcurrent thresholds configurable per channel
- Scan all channels every ~2ms for fault detection
- Open-load detection: if commanded ON but current reads zero, flag open-load fault
- Stuck-on detection: if commanded OFF but current reads non-zero, flag stuck-on fault

---

## ADR-005: Safety Architecture — Three Layers of Protection

**Date**: 2026-03-11
**Status**: Accepted

### Layer 1 — Per-channel upstream fuse
Every output channel has a fuse UPSTREAM of the MOSFET (PTC resettable or blade fuse). Protects against MOSFET fails-short: if MOSFET dies shorted, fuse limits current and prevents fire.

Circuit: `12V → [fuse] → load → MOSFET drain → source → [shunt] → GND`

### Layer 2 — Firmware stuck-on detection
If current sense reads >0 when output is commanded OFF → "stuck-on" fault. Log fault, report on CAN (0x357), alert via HeadUnit.

### Layer 3 — Redundant switching on fire-risk channels
- **Fuel pump**: ECM's ST (Security Teensy) has independent cutoff relay — already in architecture
- **Seat heaters (×2)**: Second MOSFET in series per heater, controlled by separate TC4427A channel. If primary MOSFET shorts, secondary can still open the circuit. Also wired through key-on power — key off = heaters off regardless

These are the only loads where MOSFET fails-short = fire risk.

### Additional safety measures
- Brake switches on DIRECT MCU GPIO (never behind SPI/I2C — no bus in safety path)
- Hardware watchdog: S32K358 SWT (Software Watchdog Timer)
- S32K358 lockstep mode for brake monitoring
- CAN timeout failsafe states per output:
  - Fans → 100% (prevent overheat)
  - Fuel pump → off (prevent flood)
  - Lights → maintain current state
  - A/C → off (reduce load)
- Fuel pump prime-and-timeout: 2s prime on key-to-RUN, off if engine doesn't start within 5s
- Input protection: TVS + series resistors on all GPIO and ADC inputs

---

## ADR-006: Load Shedding Priority

**Date**: 2026-03-11
**Status**: Accepted

Four-tier priority system for battery voltage protection.

| Tier | Trigger | Loads | Action |
|------|---------|-------|--------|
| CRITICAL | Never shed | Brake lights, fuel pump, CAN, horn | Always powered |
| SAFETY | Never shed | Headlights, turn signals | Always powered |
| DRIVING | Batt < 10.5V | Fans, DRL, reverse, wiper, light bar, rock lights | Shed if battery critical |
| COMFORT | Batt < 11.5V | Blower, interior, courtesy, accessory, A/C, amps, seat heaters, ADAS bus, module bus, bed/puddle lights, expansion | Shed first |

Recovery with 0.5V hysteresis (e.g., COMFORT loads restore when battery recovers to 12.0V).

---

## ADR-007: HAL-Abstracted Firmware (S32K358 Primary)

**Date**: 2026-03-11
**Updated**: 2026-03-12
**Status**: Accepted

Hardware Abstraction Layer (HAL) isolates all MCU-specific code. Firmware logic (light controller, fan controller, fault manager, etc.) is platform-independent.

**Primary HAL implementation:**
- `firmware/hal/s32k358/S32KHAL.cpp` — NXP RTD SDK (real implementation)

**Secondary (reference/test):**
- `firmware/hal/teensy/TeensyHAL.cpp` — Arduino framework + FlexCAN_T4 (retained for reference)

**Build system:**
- CMake + NXP S32 Design Studio + RTD package
- PlatformIO config retained for Teensy reference builds

**HAL interface covers:**
- GPIO read/write (all 47 outputs + switch inputs direct, no expanders)
- PWM output (0–1000 = 0.0–100.0%)
- ADC read (direct per-channel, no MUX)
- CAN FD send/receive with callbacks (native S32K358 FlexCAN)
- Hardware watchdog (SWT)
- Millisecond/microsecond timers

---

## ADR-008: NP246 4WD — H-Bridge IC

**Date**: 2026-03-11
**Status**: Accepted

**IC**: TI DRV8876 (~$1, 3.5A continuous, AEC-Q100)

Replaces 4 discrete MOSFETs + 2 TC4427A channels for the transfer case encoder motor — simpler, cheaper, and includes built-in current sense and fault reporting.

**Features used:**
- Bidirectional DC motor control (forward/reverse for mode changes)
- Built-in current limiting for stall detection
- Built-in thermal shutdown
- nFAULT output for hardware fault detection

**Position feedback:**
- Transfer case position potentiometer via ADC
- Position values mapped to each mode (2HI, A4WD, 4HI, Neutral, 4LO)

**Safety:**
- Motor timeout: 5s max per shift attempt
- Stall current detection via DRV8876 built-in current sense
- 4LO inhibited above 3 mph (uses wheel speed from ECM via CAN)

**Future use:**
Same approach applies to any bidirectional motor loads added later (power windows, mirrors, seats).

Controlled by FourWDController state machine in firmware.

---

## ADR-009: Skip Teensy Prototype — Build S32K358 from Day One

**Date**: 2026-03-12
**Status**: Accepted

**Decision:**
Skip the Teensy 4.1 prototype phase entirely. Build PDCM hardware directly around the NXP S32K358.

**Hardware simplification (vs Teensy prototype):**
- **Remove 3× CD74HC4067 analog MUX** — S32K358 has 2× SAR ADC with 40+ external channels, direct ADC per shunt
- **Remove 2× MCP23S17 port expanders** — S32K358 has 172 pins, enough GPIO for all outputs and inputs directly
- **Remove SPI bus** — no SPI peripherals needed (CAN FD is native to S32K358)
- **Net: 7 fewer ICs**, simpler PCB, fewer failure modes, lower latency on current sense and switch reads

**Firmware impact:**
- All 13 firmware modules unchanged (they only call HAL functions)
- `S32KHAL.cpp` becomes the real implementation (was placeholder)
- `TeensyHAL.cpp` retained in repo for reference
- `CurrentSense` simplifies: direct ADC reads instead of MUX stepping
- `GateDriver` simplifies: all direct GPIO, no expander path
- `SwitchInput` simplifies: all direct GPIO reads, no expander
- HAL interface removes SPI/expander/MUX functions (not needed)
- Build system: CMake + NXP RTD SDK replaces PlatformIO

**Rationale:**
- 50× S32K358 already on hand — no cost barrier
- Prototyping on Teensy then porting wastes a full development cycle
- The hardware is dramatically simpler on S32K358 (fewer ICs, no SPI bus)
- Firmware is HAL-abstracted — modules don't care which MCU runs them

---

## ADR-010: Universal Output Circuit — Two Shunt Sizes

**Date**: 2026-03-12
**Status**: Accepted

All 46 TC4427A output circuits are identical: GPIO → TC4427A → 100Ω → IRFZ44N gate (10kΩ pulldown), low-side shunt → INA180A1 (gain 20) → ADC. The **only** variable is the shunt resistor value.

**Two shunt tiers:**
- **10mΩ × 36 channels** — Heavy loads, 1–16A measurement range (Ch 0–23, 28–39)
- **50mΩ × 10 channels** — Light loads / enable signals, 0.3–3.3A range (Ch 25–27, 40–46)

**Eliminated:**
- **100mΩ tier removed entirely** — No sub-1A loads that need precision current sensing on PDCM. Sensors and low-power modules get power from their own dedicated circuits, not PDCM switched channels.

**Current sense amplifier:**
- INA180A1 (gain = 20) universal across all channels. No per-tier gain variants.
- 10mΩ × 16A × 20 = 3.2V output → within 3.3V ADC reference ✓
- 50mΩ × 3.3A × 20 = 3.3V output → at ADC ceiling ✓

**Channel philosophy:**
- Channels are **generic pools** — any channel of the right shunt size can drive any matching load
- Load-to-channel assignment happens at install time (firmware config + fuse selection)
- Fuse in holder provides hard current limit, firmware provides soft protection
- If more channels of a size are needed → build an expansion board (same circuit)

**Door motors (windows, locks, mirrors):**
Handled by future satellite PDCMs, not the main board. Main PDCM keeps just 1 H-bridge (4WD transfer case).

**Rationale:**
- Universal circuit = one layout block replicated 46×, fewer BOM line items, simpler assembly
- Two shunt sizes cover the full load spectrum without wasted ADC resolution
- 100mΩ was only needed for sub-1A precision — those loads don't exist on PDCM
- Generic pools decouple PCB design from specific vehicle wiring — adapt at install time

---

## ADR-011: Push-Button Start (Keyless Ignition)

**Date**: 2026-03-12
**Status**: Accepted

**Decision:**
Replace key ignition switch with push-button start. PDCM reads a simple momentary button via GPIO (conditioned like all other switch inputs). Authentication is handled entirely upstream.

**Physical interface:**
- Push button → PDCM switch input (SW_START_BTN, digital GPIO, debounced)
- Button LED ring driven by MCU PWM output (direct, not through TC4427A channel)

**State machine (firmware):**
- OFF → ACC → RUN → CRANK
- Transitions based on button press + brake state
- Short press without brake: OFF ↔ ACC
- Short press with brake in ACC/RUN: → CRANK (momentary)
- Long press in any state: → OFF (emergency shutdown)

**Authentication hierarchy:**
1. Phone (BLE) — primary, automatic proximity unlock
2. Key fob (RF) — secondary, traditional remote
3. HeadUnit PIN entry — tertiary, emergency backup

**Architecture:**
- Auth handled by HeadUnit/security module, NOT by PDCM
- HeadUnit sends "authorized" CAN message to PDCM
- PDCM will not transition out of OFF without valid auth
- PDCM controls starter solenoid relay via one 10mΩ output channel

**Removed:**
- IGN_SW connector pin (was on J1 power connector)
- KEY_RAW sensor input + KEY_ADC voltage divider
- Key position resistor ladder circuit

**Rationale:**
- Modern keyless experience — phone-as-key is the primary interface
- Simpler wiring — no ignition switch harness, no key position sensing
- More flexible — authentication can be updated via firmware/software
- Starter control through PDCM enables software-controlled crank limits and anti-flood logic

---

## ADR-012: Deutsch Connector Selection — Physical Routing Groups

**Date**: 2026-03-13
**Status**: Accepted

**Decision:**
13 Deutsch automotive connectors grouped by truck routing zone, not by channel number. Three connector series matched to wire gauge requirements.

**Connector series:**
- **HD (1 connector)**: J1 battery power — 8 AWG, 100A/pin rating. Handles 60-80A peak aggregate with margin.
- **DT (8 connectors)**: J3-J7, J12-J13 load outputs — 14-18 AWG, 13-25A/pin. Matched to MOSFET output currents.
- **DTM (4 connectors)**: J2, J8-J11 signals/switches — 20-24 AWG, 7.5A max. Appropriate for switch inputs and CAN bus.

**Physical routing zones:**
| Zone | Connectors | Rationale |
|------|-----------|-----------|
| Battery/firewall | J1 | Short heavy-gauge run from battery |
| Internal bus | J2 | CAN FD shielded pair, stays inside PDCM enclosure area |
| Engine bay | J3 | Fuel pump, fans, A/C, horn, axle — all under hood |
| Front of truck | J4 | All forward-facing lights in one harness branch |
| Rear of truck | J5 | Tail/brake/reverse/bed — single rear harness run |
| Firewall pass-through | J6 | Cabin loads (blower, wiper, interior, seats) |
| Under truck / t-case | J7 | 4WD motor + position sensor — routed to transfer case |
| Steering column | J8 | Stalk switches — short run from column to PDCM |
| Dashboard | J9 | Dash-mounted switches |
| Brake pedal | J10 | **Isolated** — safety-critical BOP path, dedicated GND returns |
| Transmission | J11 | Reverse switch + future trans signals |
| Cabin electronics | J12 | Cameras, ADAS, amps, HU — all near dash/cabin |
| Exterior aux | J13 | Rock/puddle lights + 6 expansion channels |

**Key design decisions:**
- **J10 brake switches isolated**: 50ms BOP path gets its own connector with per-switch dedicated GND returns. Eliminates ground-loop noise on safety-critical brake sensing.
- **One wire per load**: Low-side switching means each connector pin carries the MOSFET drain only. Load +12V comes from upstream fuse panel, not from PDCM connectors.
- **17 spare pins**: Distributed across connectors for future expansion without new harness runs.
- **94 total pins**: 66 signals + 9 GND + 2 power + 17 spare.

**Rationale:**
- Grouping by zone means each connector = one clean harness branch to that area of the truck. No criss-crossing.
- Deutsch DT/DTM/HD are the automotive industry standard — IP67 sealed, vibration-rated, field-serviceable with crimped contacts.
- Matching connector series to wire gauge eliminates reducers and oversized pins.
- Spare pins on each connector allow adding loads to a zone without running new harness branches.
