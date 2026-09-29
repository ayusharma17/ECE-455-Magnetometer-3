# Measuring Circuit & Relay LTspice Starter

This directory contains the LTspice simulation and schematic for the proton precession magnetometer measuring circuit and relay switching front-end.

## Files

- `measuring_circuit.cir`: SPICE netlist modeling relay switching, resonant coil tuning, input protection diodes, high-CMRR instrumentation amplifier, active bandpass filter, and zero-crossing comparator.
- `measuring_circuit_graphical.asc`: LTspice graphical schematic of the analog front-end and input protection stage.
- `measuring_circuit_schematic.png`: Rendered image of the front-end schematic.
- `measuring_circuit_waveforms.png`: Simulation waveforms demonstrating common-mode noise rejection, resonant signal amplification, active bandpass filtering, and comparator digitization.

## References

- [PyPPM v1.3 Schematics (Sheets 7, 8, 9, 10, 13)](https://github.com/geekysuavo/pyppm/tree/main/designs/ppm-1.3)
- [JPM-4 Design Paper (Sensors and Materials, 2021)](https://sensors.myu-group.co.jp/sm_pdf/SM2789.pdf)
- [Construction of a Proton Magnetometer (Institute of Physics – Sri Lanka, 2008)](https://ipsl.lk/documents/TechSession/2008/ipsl0812.pdf)
- [Past UW-Madison ECE 455 Final Projects (Hook Line & Towfish, 455 Final Magnetometer Presentation)](file:///Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/PastProjects/)
