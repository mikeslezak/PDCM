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
- [ ] S32K358 pin allocation (PDCMConfig_S32K358.h)
- [ ] Schematic — Power input (12V, TVS, reverse polarity, 5V + 3.3V regulators)
- [ ] Schematic — S32K358 MCU (HDQFP-172, decoupling, crystal, reset)
- [ ] Schematic — CAN FD transceiver (MCP2562FD)
- [ ] Schematic — TC4427A gate driver circuits (24 ICs × 2 channels)
- [ ] Schematic — IRFZ44N MOSFET output stages (46 channels)
- [ ] Schematic — Per-channel upstream fusing (PTC or blade)
- [ ] Schematic — Current sense amplifiers (INA180 × 48) → direct to S32K358 ADC
- [ ] Schematic — DRV8876 H-bridge (4WD encoder motor)
- [ ] Schematic — Switch input conditioning (stalks, buttons) → direct GPIO
- [ ] Schematic — Dual brake switch inputs (direct GPIO)
- [ ] Schematic — Key position resistor ladder → ADC
- [ ] Schematic — Battery voltage divider → ADC
- [ ] Schematic — 4WD position potentiometer → ADC
- [ ] Schematic — Seat heater redundant MOSFET (safety)
- [ ] Schematic — Input protection (TVS + series resistors)
- [ ] Schematic — JTAG/SWD debug header
- [ ] PCB layout
- [ ] Connector pinout documentation (Deutsch connectors)
- [ ] BOM

### Phase 4 — S32K358 Toolchain & HAL
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
