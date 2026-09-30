# Complete Proton Precession Magnetometer Circuit Simulation

This directory contains the unified, end-to-end LTspice simulation connecting all stages of the proton precession magnetometer hardware.

## Files

- `complete_system.cir`: Unified SPICE netlist integrating the 12 V polarization supply, power MOSFET switch, TVS quench clamp, DPDT signal relay, sensor coil, resonant tuning tank, Schottky protection diodes, instrumentation amplifier, active MFB bandpass filter, and fast zero-crossing comparator.
- `complete_system_schematic.png`: High-level architecture and signal flow diagram connecting all functional blocks.
- `complete_system_waveforms.png`: Complete transient simulation waveform plot showing the full operational cycle from polarization, inductive quench, relay transition, common-mode rejection, resonant amplification, and logic digitization.

## Simulation Highlights

1. **Polarization Phase (1.2 ms to 12 ms)**:
   - `RELAY_EN` ramps high from 1.0 to 1.2 ms and connects the sensor coil to the polarization supply.
   - `POL_EN` drives the MOSFET from 2 ms to 12 ms, building a $1.98\text{ A}$ current in the $7.5\text{ mH}, 5.5\ \Omega$ sensor coil.
2. **Fast Quench Phase (12 ms)**:
   - MOSFET turns off; the inductive flyback voltage is clamped cleanly to $51.3\text{ V}$ by the TVS diode (`TVS45A`), dissipating coil current to zero in $\approx 185\ \mu\text{s}$.
3. **Break-Before-Make Relay Settling (12 ms onward)**:
   - A $2.0\text{ ms}$ quench delay ensures zero hot-switching of the relay contacts.
   - `RELAY_EN` ramps low from 14.0 to 14.2 ms; the modeled contacts transfer during this ramp, returning the coil to the measuring preamplifier.
4. **Resonant Precession Acquisition (>14 ms)**:
   - Input Schottky diodes clamp residual switching noise to safe sub-volt levels.
   - The parallel LC tank ($C_{tune} = 690\text{ nF}$) resonates at the $2.214\text{ kHz}$ Larmor frequency ($Q \approx 15.5$).
   - The simulation applies $100\text{ mV}$ of $60\text{ Hz}$ interference and $10\text{ mV}$ of $10\text{ kHz}$ ripple at the coil input.
   - The instrumentation amplifier rejects the modeled common-mode interference (>100 dB CMRR).
   - The active MFB bandpass filter and gain stage deliver a clean $\sim 1.0\text{ V}$ peak sinusoid.
   - The comparator digitizes zero-crossings into a $0\text{ to }3.3\text{ V}$ logic square wave for microcontroller timer capture.
