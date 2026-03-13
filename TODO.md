# TODO — PDCM Project

## Current Phase: Phase 3 — Hardware Design (S32K358)

---

### Phase 1 — Project Setup ✅
- [x] Repository setup with platform submodule
- [x] CAN protocol documentation
- [x] Initial firmware skeleton

### Phase 2 — Firmware Architecture ✅
- [x] Architecture decisions (ADR-001 through ADR-009)
- [x] PDCMTypes.h — Enums, structs, timing constants
- [x] HAL interface (HAL.h) — Platform abstraction
- [x] GateDriver — TC4427A output control (47 channels)
- [x] CurrentSense — Per-channel shunt measurement
- [x] HBridge — DRV8876 4WD motor control
- [x] SwitchInput — Switch reading + debounce
- [x] BrakeMonitor — Dual brake switch + BOP logic
- [x] BatteryMonitor — Voltage monitoring for load shedding
- [x] LightController — Headlights, turns, fade, welcome/goodbye
- [x] PowerManager — Fuel pump, A/C, horn, wiper, blower, heaters
- [x] FanController — PWM fans + CAN timeout failsafe
- [x] FourWDController — NP246 state machine
- [x] FaultManager — Per-channel + system fault tracking
- [x] LoadShedder — 4-tier voltage priority shedding
- [x] CANManager — 9 published + 6 consumed messages
- [x] main.cpp — Init sequence + scheduler
- [x] Platform structs — VehicleMessages.h updated with all PDCM message structs

### Phase 3 — Hardware Design (CURRENT)
- [x] Channel allocation update — 2 amp remotes (was 4), 6 expansion (was 4)
- [x] Universal output circuit — two shunt sizes (ADR-010: 10mΩ ×36 + 50mΩ ×10, 100mΩ eliminated)
- [x] KiCad schematic generator (`hardware/schematic/generate_schematic.py`)
- [x] Schematic — Power input (12V, TVS, reverse polarity, LM2596S-5 → AMS1117-3.3)
- [x] Schematic — S32K358 MCU (HDQFP-172, 15× decoupling, crystal, reset, JTAG)
- [x] Schematic — CAN FD transceiver (MCP2562FD)
- [x] Schematic — TC4427A gate driver circuits (23 ICs × 2 channels)
- [x] Schematic — IRFZ44N MOSFET output stages (46 channels + shunts + INA180)
- [x] Schematic — DRV8876 H-bridge (4WD encoder motor)
- [x] Schematic — Switch input conditioning (15 switches incl. START_BTN, direct GPIO)
- [x] Schematic — Connectors (power, CAN, loads, switches, sensors)
- [x] Schematic — Root sheet with 10 hierarchical sub-sheets
- [x] S32K358 pin allocation — 120 pins assigned from IOMUX spreadsheet
  - [x] PDCMConfig_S32K358.h — complete with real HDQFP-172 pad numbers
  - [x] eMIOS1 PWM: 14 channels (CH2-CH18) on verified bonded pins
  - [x] CAN FD: PTC2/PTC3 (pads 49/50), JTAG: PTA4/PTA10/PTC4/PTC5
  - [x] 34 gate driver GPIO + 2 H-bridge ctrl + 15 switch inputs
  - [x] 49 ADC channels: ADC0 (20) + ADC1 (16) + ADC2 (13) — all unique
  - [x] Physical pad numbers from NXP S32K358_IOMUX.xlsx (extracted from Reference Manual)
  - [x] Allocation script: `hardware/schematic/s32k358_pin_allocator.py`
  - [x] Report: `hardware/schematic/pin_allocation_report.txt`
- [x] Schematic review — all 232 inter-sheet labels verified, 0 mismatches
- [x] ERC pass in KiCad 9 (0 errors, 2 warnings — custom symbols only)
- [ ] PCB layout
- [x] Connector pinout documentation (Deutsch connectors, ADR-012, 13 connectors by routing zone)
- [ ] BOM finalization

### Phase 4 — S32K358 Toolchain & HAL
- [ ] Push-button start firmware module (state machine: OFF→ACC→RUN→CRANK)
- [ ] Authentication CAN message definition (HeadUnit → PDCM auth grant)
- [ ] Starter solenoid output channel assignment + crank timeout logic
- [ ] Set up NXP S32 Design Studio + RTD SDK
- [ ] Acquire PEmicro Multilink or Lauterbach JTAG debugger
- [ ] Implement S32KHAL.cpp (GPIO, ADC, PWM, CAN FD, SWT watchdog)
- [ ] CMake build system
- [ ] Simplify CurrentSense — direct ADC reads (remove MUX stepping)
- [ ] Simplify GateDriver — all direct GPIO (remove expander path)
- [ ] Simplify SwitchInput — all direct GPIO (remove expander reads)
- [ ] Clean HAL interface — remove SPI/expander/MUX functions
- [ ] Build verification (all modules compile for S32K358)
- [ ] Lockstep mode validation for brake monitoring

### Phase 5 — Prototype Build
- [ ] Order remaining components (INA180, DRV8876, fuses, connectors)
- [ ] PCB fabrication
- [ ] Component assembly
- [ ] Power-on smoke test
- [ ] Individual channel verification (each MOSFET + gate driver)
- [ ] Current sense calibration (per-channel shunt + amplifier)
- [ ] CAN bus integration test with ECM

### Phase 6 — Firmware Integration
- [ ] Switch input bring-up (stalk reading, debounce tuning)
- [ ] Lighting bring-up (headlights, turns, brakes, interior)
- [ ] Power output bring-up (fuel pump, fans, A/C, horn, wiper)
- [ ] 4WD bring-up (H-bridge, position sensor calibration)
- [ ] Current sense calibration and overcurrent threshold tuning
- [ ] Brake state validation (50ms BOP path latency)
- [ ] CAN message validation (all 9 published messages)
- [ ] CAN callback validation (all 6 consumed messages)
- [ ] Load shedding testing
- [ ] Welcome/goodbye lighting sequence tuning
- [ ] Fuel pump prime-and-timeout validation

### Phase 7 — Production
- [ ] Environmental testing (-40°C to +85°C)
- [ ] Production BOM and cost analysis
- [ ] Vehicle installation
