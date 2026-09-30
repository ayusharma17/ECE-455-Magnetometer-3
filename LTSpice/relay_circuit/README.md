# Proton Magnetometer Relay & Cold-Switching Simulation

This directory contains the standalone LTspice simulation of the DPDT signal relay and its microcontroller driver for the proton precession magnetometer.

## Files

- `relay_circuit.cir`: Standalone SPICE netlist modeling the microcontroller logic (`RELAY_EN`, `POL_EN`), low-side MOSFET relay driver with flyback protection, OMRON G6A-2 DPDT contact switching (break-before-make), sensor coil polarization, and fast TVS inductive quench.
- `relay_circuit_waveforms.png`: Verified waveform plot illustrating the complete cold-switching timeline across all four operating phases.
- `plot_relay_waveforms.py`: Python script parsing the LTspice simulation data and rendering the waveform plot.

## Key Simulation Results

1. **Break-Before-Make Cold Switching**:
   - `RELAY_EN` energizes the relay at $1.0\text{ ms}$, connecting the coil to the polarization supply before current starts flowing.
   - At $12.0\text{ ms}$, `POL_EN` turns off the main power MOSFET.
   - The TVS diode (`TVS45A`) clamps the inductive flyback to $51.26\text{ V}$, depleting the $1.98\text{ A}$ coil current to zero in $181.6\ \mu\text{s}$.
   - The relay remains energized through a $2.0\text{ ms}$ post-quench safety window ($12.0\text{ to }14.0\text{ ms}$), guaranteeing zero current before contacts open.
2. **Readout Isolation & Safety**:
   - When the relay de-energizes at $14.0\text{ ms}$, contacts cleanly transfer the coil to the readout port with zero hot-switching voltage spikes.
   - The induced $2214\text{ Hz}$ Larmor precession signal appears cleanly at the readout terminals.

## How to Run

Execute batch simulation via Terminal (macOS):
```bash
/Applications/LTspice.app/Contents/MacOS/LTspice -b "/Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/LTSpice/relay_circuit/relay_circuit.cir"
```
Re-plot waveforms:
```bash
python3 LTSpice/relay_circuit/plot_relay_waveforms.py
```
