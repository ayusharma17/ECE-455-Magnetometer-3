# ECE 455 - Proton Magnetometer Project

This repository contains the hardware designs, LTspice simulations, and microcontroller firmware for the ECE 455 Proton Magnetometer project, specifically focusing on the **Polarization Power Stage** and the **DPDT Signal Relay**.

---

## Repository Structure

| File / Folder | Purpose |
|---|---|
| **[POLARIZATION.md](POLARIZATION.md)** | Full specification, circuit design, component ratings, and calculations for the 12 V power switch, gate driver, TVS quench clamp, and sensor coil. |
| **[RELAY.md](RELAY.md)** | Circuit diagram, pinout table, microcontroller driver circuit, and cold-switching break-before-make sequence for the OMRON G6A-2 DPDT relay. |
| **[DESIGN.md](DESIGN.md)** | System overview, microcontroller timing budgets, measurement resolution targets, and reference literature citations. |
| **[TESTING.md](TESTING.md)** | Step-by-step bench testing instructions and safety verification procedures for the hardware. |
| **[LTSpice/](LTSpice/)** | LTspice simulations for both the **Polarization circuit** (`LTSpice/pyppm_12v_ltspice_starter (1)/`) and the **DPDT Relay cold-switching circuit** (`LTSpice/relay_circuit/`). |
| **[firmware/](firmware/)** | Arduino firmware (`ProtonMagnetometer.ino`) controlling the polarization pulse and relay switching sequence. |
| **[simulation/](simulation/)** | Python demo script (`proton_precession_demo.py`) demonstrating signal acquisition and frequency estimation. |

---

## Hardware Summary

| Subsystem | Components | Function |
|---|---|---|
| **Polarization Switch** | Power MOSFET (IRLR024N / IRF540N), $33.2\ \Omega$ Gate Resistor, Gate Driver (FAN3111E) | Switches 12 V, $\approx 2\text{ A}$ DC current through the sensor coil under microcontroller command (`POL_EN`). |
| **Quench Clamp** | TVS Diode (SMDJ45A / 50 V clamp), $1\text{ k}\Omega$ Damping Resistor | Clamps the inductive turn-off spike to $\approx 51\text{ V}$, depleting coil current in $<400\ \mu\text{s}$. |
| **DPDT Relay** | OMRON G6A-2 (2 Form C, 5 V coil) | Galvanically breaks both coil leads between the 12 V polarization supply and the signal readout lines. |
| **Relay Driver** | Logic-level N-FET (2N7002 / FDN327N), 1N4148 flyback diode, $47\text{ k}\Omega$ pull-down | Interfaces microcontroller GPIO (`RELAY_EN`) to safely switch the 5 V / 40 mA relay coil. |
| **Controller** | Arduino / Teensy / RP2040 | Executes the break-before-make timing sequence. |

---

## How to Run & Use

### 1. View or Run LTspice Simulations
- **Polarization Circuit**: Run batch simulation via Terminal (macOS):
  ```bash
  /Applications/LTspice.app/Contents/MacOS/LTspice -b "LTSpice/pyppm_12v_ltspice_starter (1)/pyppm_12v_polarizer.cir"
  ```
- **Relay & Cold-Switching Circuit**: Run batch simulation via Terminal (macOS):
  ```bash
  /Applications/LTspice.app/Contents/MacOS/LTspice -b "/Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/LTSpice/relay_circuit/relay_circuit.cir"
  ```

### 2. Upload Firmware
- Open `firmware/ProtonMagnetometer/ProtonMagnetometer.ino` in the Arduino IDE.
- Select your target board and upload.

### 3. Run Python Signal Demo
```bash
python3 simulation/proton_precession_demo.py
```
