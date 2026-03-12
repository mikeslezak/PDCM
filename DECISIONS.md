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

## ADR-002: NXP S32K358 Production MCU (Platform-Wide)

**Date**: 2026-03-11
**Status**: Accepted

**MCU**: NXP S32K358GHT1MPCST — dual Cortex-M7 @ 240MHz, AEC-Q100 Grade 1, HDQFP-172.
- 8MB flash, 1MB SRAM
- 6× CAN FD controllers
- Hardware lockstep safety core
- -40°C to +150°C junction

**Decision:**
- S32K358 replaces Teensy 4.1 across ALL vehicle modules (ECM, PDCM, GCM, future)
- Teensy 4.1 remains the prototype/dev platform — firmware written with HAL for both targets
- S32K358 dev hardware (eval board + JTAG probe) required before production port
- 50× S32K358 already on hand

**Rationale:**
- AEC-Q100 Grade 1 automotive qualification
- Lockstep cores provide hardware fault detection for safety-critical functions (brakes)
- 6× CAN FD — enough for any module, no external CAN controllers needed
- 172 pins — enough GPIO for 47 outputs without port expanders in production
- Dual CM7 @ 240MHz — vastly more capable than needed, room for future features
- Platform-wide standardization reduces BOM complexity and tooling overhead

---

## ADR-003: 47-Channel Output Allocation (Three Tiers)

**Date**: 2026-03-11
**Status**: Accepted

### Tier 0 — Always Hot (own battery feed, PDCM cannot control)
- ECM (safety-critical, independent power)
- PDCM itself (own fused battery feed)
- Starter motor (direct from ignition switch)

### Tier 1 — PDCM Switched (25 channels, current through MOSFETs)

| Ch | Load | MOSFET | Shunt | Notes |
|----|------|--------|-------|-------|
| 1 | Fuel pump | IRFZ44N (49A) | 10mΩ | Soft-start, prime-and-timeout |
| 2 | Cooling fan 1 | IRFZ44N | 10mΩ | PWM variable speed |
| 3 | Cooling fan 2 | IRFZ44N | 10mΩ | PWM variable speed |
| 4 | Blower motor | IRFZ44N | 10mΩ | PWM variable speed |
| 5 | A/C compressor clutch | IRFZ44N | 10mΩ | Reports to ECM for idle-up |
| 6 | Low beam left | IRFZ44N | 50mΩ | PWM soft-start |
| 7 | Low beam right | IRFZ44N | 50mΩ | PWM soft-start |
| 8 | High beam left | IRFZ44N | 50mΩ | PWM soft-start |
| 9 | High beam right | IRFZ44N | 50mΩ | PWM soft-start |
| 10 | Turn signal left | IRFZ44N | 100mΩ | Flash timer |
| 11 | Turn signal right | IRFZ44N | 100mΩ | Flash timer |
| 12 | Brake light left | IRFZ44N | 100mΩ | Direct from brake GPIO |
| 13 | Brake light right | IRFZ44N | 100mΩ | Direct from brake GPIO |
| 14 | Reverse lights | IRFZ44N | 100mΩ | |
| 15 | DRL | IRFZ44N | 100mΩ | PWM dimming |
| 16 | Interior light | IRFZ44N | 100mΩ | PWM fade |
| 17 | Courtesy light | IRFZ44N | 100mΩ | PWM fade |
| 18 | Horn | IRFZ44N | 50mΩ | Direct from switch |
| 19 | Wiper motor | IRFZ44N | 50mΩ | Intermittent timing |
| 20 | Accessory power | IRFZ44N | 50mΩ | Key-switched |
| 21 | Front axle actuator | IRFZ44N | 100mΩ | 4WD engage/disengage |
| 22 | Seat heater left | IRFZ44N | 10mΩ | PWM temperature control |
| 23 | Seat heater right | IRFZ44N | 10mΩ | PWM temperature control |
| 24 | Light bar | IRFZ44N | 10mΩ | High current, switched |
| 25 | 4WD encoder motor | H-bridge (DRV8876) | built-in | Bidirectional, see ADR-008 |

### Tier 2 — PDCM Enable (low-current turn-on signal, load has own battery feed)

| Ch | Load | MOSFET | Notes |
|----|------|--------|-------|
| 26 | Amp remote #1 | IRFZ44N | SQ system amp 1 |
| 27 | Amp remote #2 | IRFZ44N | SQ system amp 2 |
| 28 | Amp remote #3 | IRFZ44N | SQ system amp 3 |
| 29 | Amp remote #4 | IRFZ44N | SQ system amp 4 |
| 30 | HeadUnit enable | IRFZ44N | Jetson Orin Nano |

### Tier 3 — Individually Switched Sub-Loads

| Ch | Load | MOSFET | Shunt | Group |
|----|------|--------|-------|-------|
| 31 | Front camera | IRFZ44N | 100mΩ | ADAS |
| 32 | Rear camera | IRFZ44N | 100mΩ | ADAS |
| 33 | Side cameras (pair) | IRFZ44N | 100mΩ | ADAS |
| 34 | Parking sensor array | IRFZ44N | 100mΩ | ADAS |
| 35 | Radar / blind spot | IRFZ44N | 100mΩ | ADAS |
| 36 | Gauge cluster (GCM) | IRFZ44N | 100mΩ | Modules |
| 37 | GPS / cellular | IRFZ44N | 100mΩ | Modules |
| 38 | Dash cam | IRFZ44N | 100mΩ | Modules |
| 39 | Future module | IRFZ44N | 100mΩ | Modules |
| 40 | Rock lights | IRFZ44N | 50mΩ | Exterior |
| 41 | Bed lights | IRFZ44N | 100mΩ | Exterior |
| 42 | Puddle / underbody | IRFZ44N | 100mΩ | Exterior |
| 43 | Future exterior | IRFZ44N | 100mΩ | Exterior |
| 44 | Expansion 1 | IRFZ44N | 100mΩ | Spare |
| 45 | Expansion 2 | IRFZ44N | 100mΩ | Spare |
| 46 | Expansion 3 | IRFZ44N | 100mΩ | Spare |
| 47 | Expansion 4 | IRFZ44N | 100mΩ | Spare |

**Total: 47 TC4427A-driven channels + 1 H-bridge IC (DRV8876) = 48 switched outputs**
**TC4427A count: 24 ICs** (47 channels ÷ 2 outputs per IC, rounded up) — ~$30
**Upstream fuse count: 47** (PTC resettable or blade, per channel)

---

## ADR-004: Per-Channel Current Sensing

**Date**: 2026-03-11
**Status**: Accepted

Low-side shunt resistor on every output channel. Shunt values sized per channel:
- **10mΩ** — heavy loads (fuel pump, fans, blower, A/C, seat heaters, light bar)
- **50mΩ** — medium loads (headlights, horn, wiper, accessory, rock lights)
- **100mΩ** — light loads (turn signals, brake lights, reverse, DRL, interior, cameras, modules)

**Teensy prototype**: 3× CD74HC4067 (16:1 analog MUX) → 3 ADC pins, MUX select lines driven by MCP23S17 SPI port expander (saves GPIO). 48 channels → 3 MUX ICs × 16 channels each.

**S32K358 production**: Direct ADC — S32K358 has enough ADC channels for every shunt (no MUX needed, lower latency, simpler BOM).

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
- Hardware watchdog enabled (IMXRT1062 WDOG on Teensy, S32K358 SWT on production)
- S32K358 lockstep mode for brake monitoring (production)
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

## ADR-007: Dual-Target Firmware (Teensy + S32K358)

**Date**: 2026-03-11
**Status**: Accepted

Hardware Abstraction Layer (HAL) isolates all MCU-specific code. Firmware logic (light controller, fan controller, fault manager, etc.) is platform-independent.

**HAL implementations:**
- `firmware/hal/teensy/TeensyHAL.cpp` — Arduino framework + FlexCAN_T4
- `firmware/hal/s32k358/S32KHAL.cpp` — NXP RTD SDK (placeholder until dev board arrives)

**Build targets:**
- Teensy: PlatformIO (`pio run -e PDCM`)
- S32K358: CMake + NXP S32 Design Studio (future)

**HAL interface covers:**
- GPIO read/write
- PWM output (0–1000 = 0.0–100.0%)
- ADC read (12-bit)
- CAN FD send/receive with callbacks
- SPI transfer (for MCP23S17 + CD74HC4067 on Teensy)
- Hardware watchdog feed
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
