# TODO — PDCM Project

## Current Phase: Phase 3 — Hardware Design

---

### Phase 1 — Project Setup ✅
- [x] Repository setup with platform submodule
- [x] CAN protocol documentation
- [x] Initial firmware skeleton

### Phase 2 — Firmware Architecture ✅
- [x] Architecture decisions (ADR-001 through ADR-008)
- [x] PDCMConfig.h — Complete Teensy 4.1 pin allocation
- [x] PDCMTypes.h — Enums, structs, timing constants
- [x] HAL interface (HAL.h) — Platform abstraction
- [x] TeensyHAL.cpp — Arduino/FlexCAN_T4 implementation
- [x] S32KHAL.cpp — Placeholder for NXP RTD SDK
- [x] GateDriver — TC4427A output control (47 channels)
- [x] CurrentSense — Per-channel shunt measurement via analog MUX
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
- [ ] Schematic — Power input (12V, TVS, reverse polarity, 5V + 3.3V regulators)
- [ ] Schematic — CAN FD transceiver (MCP2562FD)
- [ ] Schematic — Teensy 4.1 connections and pin allocation
- [ ] Schematic — TC4427A gate driver circuits (24 ICs × 2 channels)
- [ ] Schematic — IRFZ44N MOSFET output stages (46 channels)
- [ ] Schematic — Per-channel upstream fusing (PTC or blade)
- [ ] Schematic — Current sense amplifiers (INA180 × 48)
- [ ] Schematic — CD74HC4067 analog MUX (3 ICs)
- [ ] Schematic — MCP23S17 SPI port expanders (2 ICs)
- [ ] Schematic — DRV8876 H-bridge (4WD encoder motor)
- [ ] Schematic — Switch input conditioning (stalks, buttons)
- [ ] Schematic — Dual brake switch inputs (direct GPIO)
- [ ] Schematic — Key position resistor ladder
- [ ] Schematic — Battery voltage divider
- [ ] Schematic — 4WD position potentiometer input
- [ ] Schematic — Seat heater redundant MOSFET (safety)
- [ ] Schematic — Input protection (TVS + series resistors)
- [ ] PCB layout
- [ ] Connector pinout documentation (Deutsch connectors)
- [ ] BOM

### Phase 4 — Teensy Prototype Build
- [ ] Order remaining components (INA180, CD74HC4067, MCP23S17, DRV8876, fuses, connectors)
- [ ] Prototype PCB fabrication
- [ ] Component assembly
- [ ] Power-on smoke test
- [ ] Individual channel verification (each MOSFET + gate driver)
- [ ] Current sense calibration (per-channel shunt + amplifier)
- [ ] CAN bus integration test with ECM

### Phase 5 — Firmware Integration
- [ ] PlatformIO build verification (resolve any header/include issues)
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

### Phase 6 — S32K358 Port
- [ ] Acquire S32K358 eval board + JTAG probe
- [ ] Set up NXP S32 Design Studio + RTD SDK
- [ ] Implement S32KHAL.cpp (GPIO, ADC, CAN, SWT)
- [ ] CMake build system for S32K358 target
- [ ] Port verification (all modules compile + function)
- [ ] Lockstep mode validation for brake monitoring

### Phase 7 — Production
- [ ] Production PCB design (S32K358 native, no Teensy)
- [ ] Direct ADC for current sense (no analog MUX needed)
- [ ] Direct GPIO for all 47 outputs (no port expanders needed)
- [ ] Production BOM and cost analysis
- [ ] Environmental testing (-40°C to +85°C)
- [ ] Vehicle installation
