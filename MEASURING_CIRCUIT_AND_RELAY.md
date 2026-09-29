# Proton Magnetometer Measuring Circuit and Relay Design

## Purpose

Connect the sensor coil to the measuring preamplifier after polarization, resonate the coil at the proton Larmor frequency, reject common-mode and environmental interference, amplify the microvolt-level free-induction-decay (FID) signal, and condition it for frequency measurement.

---

## Circuit Architecture

```
                  +-------------------------------------------------------+
                  |                     DPDT RELAY                        |
                  |                                                       |
Polarization  ----+ NO 1                                             NC 1 +---- INA_IN+  (To Measuring Preamp)
Supply (12V)      |        COM 1 ------------------- COM 2                |
                  |          |                         |                  |
                  |     Sensor Coil+              Sensor Coil-            |
                  |                                                       |
Polarization  ----+ NO 2                                             NC 2 +---- INA_IN-  (To Measuring Preamp)
MOSFET Drain      +-------------------------------------------------------+
```

```
                 MEASURING SIGNAL CONDITIONING CHAIN

 [Sensor Coil]
       |
       v
 [DPDT Relay (NC)]
       |
       v
 [LC Tuning Tank & Diode Protection]
   * Parallel Ctune (690 nF) resonates coil at ~2.2 kHz (Q ~ 10-18)
   * Damping Rd (2.2 kΩ) prevents uncontrolled oscillation
   * Antiparallel Schottky diodes (SD101A) clamp transients to ±0.3 V
   * 49.9 Ω series resistors + 10 nF common-mode RF capacitors
       |
       v
 [Stage 1: Low-Noise Instrumentation Amplifier]
   * High CMRR (>100 dB) rejects boat/motor common-mode noise
   * Gain G1 = 50 V/V (INA828) or G1 = 2000 V/V (AD8428)
   * 1 MΩ bias return resistors to analog ground
       |
       v
 [Stage 2: Mid-Stage Gain]
   * Low-noise op-amp (AD8597 / OPA1611) with Gain = 10 V/V to 20 V/V
       |
       v
 [Stage 3: Active Bandpass Filter]
   * Multiple Feedback (MFB) active bandpass filter (OP462 / OPA2211)
   * Center frequency f0 ≈ 2.2 kHz, bandwidth B ≈ 350-450 Hz
   * Rejects 60 Hz line hum, low-frequency DC drift, and motor EMI
       |
       v
 [Stage 4: Digitization / Detection Interface]
   * Option A (Frequency Counter): Fast comparator with hysteresis (LM311 / MCP6561) -> 3.3 V logic square wave for timer capture
   * Option B (ADC Sampling): Anti-aliasing RC filter -> 16-bit ADC (AD7680 / MCU internal ADC) for software FFT / zero-crossing
```

---

## Component Roles and Reference Grounding

| Subsystem | Component | Selected Part / Value | Role in System | Grounding / Reference Source |
| --- | --- | --- | --- | --- |
| **Relay** | DPDT Signal Relay | OMRON G6A-2 (2 Form C, 2 A carry) | Galvanically breaks *both* coil terminals between polarization and preamplifier; cold-switched | [PyPPM Sheet 13](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/13-relay.sch); [Sri Lanka 2008, p. 4](https://ipsl.lk/documents/TechSession/2008/ipsl0812.pdf); [Hook Line & Towfish, p. 14](file:///Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/PastProjects/ECE%20455%20Final%20Documentation%20Slides%20Hook%20Line%20and%20Towfish.pptx.pdf) |
| **Relay Driver** | Low-side MOSFET | FDN327N / BSS138 ($V_{GS(th)} \le 1.5\text{ V}$) | Drives relay coil from microcontroller 3.3 V/5 V GPIO (logic-level threshold) | [PyPPM Sheet 13](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/13-relay.sch); [POLARIZATION.md](file:///Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/POLARIZATION.md) |
| **Relay Flyback** | Fast Diode | SD101A Schottky / 1N4148 | Suppresses relay coil inductive turn-off spike | [PyPPM Sheet 13](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/13-relay.sch) |
| **Tuning Tank** | Resonant Capacitor | $C_{tune} = 690\text{ nF}$ (for $7.5\text{ mH}$) | Forms parallel LC tank with coil inductance to passively amplify Larmor frequency | [JPM-4, Eq. 4-5](https://sensors.myu-group.co.jp/sm_pdf/SM2789.pdf); [Sri Lanka 2008, Eq. 5](https://ipsl.lk/documents/TechSession/2008/ipsl0812.pdf); [ECE 455 Final Slides, p. 31](file:///Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/PastProjects/ECE455_Final_Slides%20(1).pdf) |
| **Tank Damping** | Damping Resistor | $R_d = 2.2\text{ k}\Omega$ ($Q \approx 10$) to $7.5\text{ k}\Omega$ ($Q \approx 15$) | Limits loaded tank Q to prevent oscillatory ringing while maintaining passive boost | [JPM-4, p. 6, Eq. 5](https://sensors.myu-group.co.jp/sm_pdf/SM2789.pdf) |
| **Input Protection** | Antiparallel Diodes | SD101A Schottky pair | Clamps residual switching transients to $\pm 0.35\text{ V}$; high dynamic resistance ($>10\text{ M}\Omega$) at microvolt levels | [PyPPM Sheet 7](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/07-asc-ina.sch); [ECE 455 Final Slides, p. 13 & 31](file:///Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/PastProjects/ECE455_Final_Slides%20(1).pdf) |
| **RFI / Bias** | Input Filter & Bias | $49.9\ \Omega$ series, $10\text{ nF}$ common-mode, $1\text{ M}\Omega$ to GND | Suppresses RF pickup with negligible thermal noise ($1.28\text{ nV}/\sqrt{\text{Hz}}$); provides in-amp bias return | [PyPPM Sheet 7](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/07-asc-ina.sch) |
| **Stage 1 Preamp** | Instrumentation Amp | INA828 ($G=50.4$, $R_G = 1\text{ k}\Omega$) or AD8428 ($G=2000$) | Rejects boat/motor common-mode noise (>110–130 dB CMRR); ultra-low noise ($1.3\text{–}7\text{ nV}/\sqrt{\text{Hz}}$) | [PyPPM Sheet 7](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/07-asc-ina.sch); [455 Presentation, p. 21–22](file:///Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/PastProjects/455%20Final%20Magnetometer%20Presentation.pptx.pdf); [ECE 455 Final Slides, p. 32](file:///Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/PastProjects/ECE455_Final_Slides%20(1).pdf) |
| **Stage 2 Gain** | Mid-Stage Op-Amp | AD8597 / ADA4898-2 ($G \approx 10\text{–}100\text{ V/V}$) | Amplifies intermediate signal before filtering (gain depends on Stage 1 choice) | [PyPPM Sheet 8](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/08-asc-gain.sch); [ECE 455 Final Slides, p. 16](file:///Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/PastProjects/ECE455_Final_Slides%20(1).pdf) |
| **Stage 3 Filter** | Active Bandpass | OP462 / ADA4898-2 (MFB topology) | Attenuates 60 Hz hum by $>40\text{ dB}$; band-limits signal ($f_0 \approx 2.2\text{ kHz}$) | [PyPPM Sheets 9-10](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/09-asc-fil-1of2.sch); [Sri Lanka 2008, p. 5](https://ipsl.lk/documents/TechSession/2008/ipsl0812.pdf); [Hook Line & Towfish, p. 20](file:///Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/PastProjects/ECE%20455%20Final%20Documentation%20Slides%20Hook%20Line%20and%20Towfish.pptx.pdf) |
| **Stage 4 Output** | Fast Comparator / ADC | LM311 / MCP6561 (Option A) or AD7680 (Option B) | Converts signal to 3.3 V square wave for timer capture, or digitizes for software FFT / zero-crossing | [Hook Line & Towfish, p. 27](file:///Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/PastProjects/ECE%20455%20Final%20Documentation%20Slides%20Hook%20Line%20and%20Towfish.pptx.pdf); [JPM-4, p. 5](https://sensors.myu-group.co.jp/sm_pdf/SM2789.pdf); [PyPPM Sheet 11](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/11-adc.sch); [Sri Lanka 2008, p. 4](https://ipsl.lk/documents/TechSession/2008/ipsl0812.pdf) |

---

## Operating Timing and Relay Sequence

To protect the sensitive front-end and avoid contact arcing, the microcontroller must strictly enforce break-before-make timing:

```
Timing Sequence:
Time:      t = 0         t = 5 ms        t = 1.0 s       t = 1.002 s     t = 1.010 s           t = 2.5 s
Signal:    |-------------|---------------|---------------|---------------|---------------------|
RELAY_EN:  HIGH (NO)     HIGH (NO)       HIGH (NO)       HIGH (NO)       LOW (NC -> Preamp)    LOW (Preamp)
POL_EN:    LOW           HIGH (Polarize) LOW (Quench)    LOW (Decayed)   LOW                   LOW
Phase:     [Relay Settling] [  Polarization  ] [ Quench Wait ] [Transit/Bounce] [ Signal Acquisition Window ]
```

1. **Relay Engage (NO Position)**: Microcontroller asserts `RELAY_EN`. Relay connects sensor coil to the polarization terminals (`CCS+`, `CCS-`). Wait $5\text{ ms}$ for contact bounce to settle.
2. **Polarization**: Microcontroller asserts `POLARIZE_EN` to the MOSFET gate driver. $2.2\text{ A}$ DC current aligns proton spins for $0.5\text{–}2.0\text{ s}$ [Sri Lanka 2008, p. 4].
3. **Quench**: Microcontroller de-asserts `POLARIZE_EN`. The TVS diode clamps the inductive back-EMF spike to $51.4\text{ V}$, depleting coil current from $2.2\text{ A}$ to zero in $t_q \approx 415\ \mu\text{s}$ [JPM-4, p. 6].
4. **Quench Settling Wait**: Microcontroller holds `RELAY_EN` HIGH for an additional $2.0\text{ ms}$ while current and magnetic field completely collapse. *Never switch relay contacts while coil current is flowing.* [Sri Lanka 2008, p. 4; POLARIZATION.md].
5. **Relay Disengage (NC Position)**: Microcontroller de-asserts `RELAY_EN`. Relay de-energizes, connecting coil to `INA_IN+` and `INA_IN-`. Wait $5\text{–}8\text{ ms}$ for mechanical transit and contact settling.
6. **Precession Acquisition**: Microcontroller captures the digitized zero-crossing edges (or samples ADC) over a $1.0\text{–}1.5\text{ s}$ window while the relay coil is unpowered (avoiding relay coil magnetic interference and heating).

---

## Design Calculations and Component Analysis

### 1. Expected Larmor Frequency and Geographic Bandwidth
For Earth's magnetic field in Madison, WI ($B_{earth} \approx 52.0\ \mu\text{T}$) and the NIST proton gyromagnetic ratio ($\gamma_p / 2\pi \approx 42.57747892\text{ Hz}/\mu\text{T}$):

$$f_0 = \frac{\gamma_p}{2\pi} \cdot B_{earth} \approx 42.5775 \times 52.0 \approx 2214.0\text{ Hz}$$

Across North America ($45\text{ to }60\ \mu\text{T}$), the precession frequency spans:

$$f_{min} = 42.5775 \times 45.0 \approx 1916\text{ Hz} \quad \text{to} \quad f_{max} = 42.5775 \times 60.0 \approx 2555\text{ Hz} \quad (\Delta f \approx 640\text{ Hz})$$

* **Local Madison Testing**: A narrow filter ($f_0 \approx 2.2\text{ kHz}$, $BW \approx 200\text{–}450\text{ Hz}$) maximizes SNR by rejecting 60 Hz harmonics and ambient motor noise [Sri Lanka 2008, p. 5; ECE 455 Final Slides, p. 15].
* **Regional Portable Operation**: Requires either widening the analog bandpass filter to $BW \approx 800\text{–}1000\text{ Hz}$ ($1.8\text{–}2.7\text{ kHz}$ as in Hook Line & Towfish p. 20 and JPM-4 p. 7) or providing switchable tuning capacitors / software DSP filtering.

### 2. Parallel Resonant Tuning Capacitor
To resonate with the sensor coil inductance ($L = 7.5\text{ mH}$) at $f_0 = 2214\text{ Hz}$:

$$C_{tune} = \frac{1}{(2\pi f_0)^2 L} = \frac{1}{(2\pi \times 2214)^2 \times 7.5 \times 10^{-3}} \approx 6.91 \times 10^{-7}\text{ F} \approx 690\text{ nF}$$

*(Note: For the past ECE 455 student coil where $L = 60\text{ mH}$, $C_{tune} \approx 86\text{ nF}$; for JPM-4 where $L = 34\text{ mH}$, $C_{tune} \approx 152\text{ nF}$; for Sri Lanka where $L = 31.6\text{ mH}$, $C_{tune} = 272\text{ nF}$).*

### 3. Tank Quality Factor and Damping Resistance
The unloaded coil quality factor is:

$$Q_0 = \frac{\omega_0 L}{R_{coil}} = \frac{2\pi \times 2214 \times 7.5\times 10^{-3}}{5.5} \approx 19.0$$

The equivalent parallel resistance of the resonant coil is:

$$R_p \approx Q_0^2 R_{coil} = \frac{\omega_0^2 L^2}{R_{coil}} \approx (18.97)^2 \times 5.5 \approx 1980\ \Omega$$

Per JPM-4 Eq. 5, a parallel damping resistor $R_d$ sets the loaded quality factor $Q$:

$$Q = \frac{R_d \parallel R_p}{\omega_0 L} = Q_0 \frac{R_d}{R_p + R_d}$$

* For $R_d = 2.2\text{ k}\Omega$: $Q = 18.97 \times \frac{2200}{1980 + 2200} \approx 10.0$ ($BW_{tank} = f_0/Q \approx 221\text{ Hz}$).
* For $R_d = 7.5\text{ k}\Omega$: $Q = 18.97 \times \frac{7500}{1980 + 7500} \approx 15.0$ ($BW_{tank} = f_0/Q \approx 148\text{ Hz}$).

Both provide a substantial passive voltage boost ($10\times$ to $15\times$) while preventing sustained oscillatory ringing upon relay closure [JPM-4, p. 6].

### 4. Front-End Protection, Noise, and Bias Return
* **SD101A Schottky Clamping**: At microvolt signal levels ($V_{sig} < 1\text{ mV}$), the diode dynamic resistance is $r_d = n V_T / I_s > 10\text{ M}\Omega$, presenting an open circuit with negligible parasitic capacitance ($C_j \approx 2\text{ pF}$) and zero distortion. Under residual switching spikes, it clamps differential voltage to $\pm 0.35\text{–}0.40\text{ V}$, protecting the in-amp inputs [PyPPM Sheet 7].
* **Input Series Resistors**: PyPPM specifies $R_{in} = 49.9\ \Omega$ per leg ($99.8\ \Omega$ total). The combined differential thermal noise is:

$$e_{n,Rin} = \sqrt{4 k_B T (2 \times 49.9\ \Omega)} \approx \sqrt{1.656\times 10^{-20} \times 99.8} \approx 1.28\text{ nV}/\sqrt{\text{Hz}}$$

This is well below the INA828 noise floor ($7\text{ nV}/\sqrt{\text{Hz}}$) and matches the AD8428 noise floor ($1.3\text{ nV}/\sqrt{\text{Hz}}$). Higher values (such as $470\ \Omega$ in ECE 455 slide 13, $e_n \approx 3.9\text{ nV}/\sqrt{\text{Hz}}$) would unnecessarily degrade the system noise figure.
* **Input Bias Return Paths**: Two $1\text{ M}\Omega$ resistors ($R_{11}, R_{12}$) connect the inverting and non-inverting inputs to analog ground. For in-amp bias currents ($I_B < 50\text{ nA}$), the resulting DC offset is $<50\text{ mV}$, preventing amplifier rail saturation. The differential input resistance of the pair ($2\text{ M}\Omega$) is three orders of magnitude above the loaded tank impedance ($R_{total} \approx 1.0\text{–}1.6\text{ k}\Omega$), causing zero tank loading.

### 5. Instrumentation Amplifier and Gain Budget
Two front-end in-amp topologies are established in the reference literature:

| Parameter | Option 1: INA828 (ECE 455 2024) | Option 2: AD8428 (PyPPM Sheet 7) |
| --- | --- | --- |
| **Gain Setting** | $G = 1 + \frac{49.4\text{ k}\Omega}{R_G} = 50.4\text{ V/V}$ ($R_G = 1\text{ k}\Omega$) | Fixed $G = 2000\text{ V/V}$ (internally trimmed) |
| **Input Voltage Noise** | $7.0\text{ nV}/\sqrt{\text{Hz}}$ at 1 kHz | $1.3\text{ nV}/\sqrt{\text{Hz}}$ at 1 kHz |
| **CMRR** | $\ge 110\text{ dB}$ (at $G = 50$) | $\ge 130\text{ dB}$ (at $G = 2000$) |
| **Mid-Stage Gain Needed** | High ($\approx 100\text{–}120\text{ V/V}$) | Low ($\approx 3\text{–}5\text{ V/V}$) |

#### Gain Budget Comparison
* **Sensor Coil FID**: $V_{in} \approx 10\ \mu\text{V peak}$
* **Tuned LC Tank ($Q \approx 12$)**: $V_{tank} \approx 120\ \mu\text{V peak}$
* **INA828 Path**:
  - Stage 1 Out: $V_{ina} = 50.4 \times 120\ \mu\text{V} \approx 6.05\text{ mV peak}$
  - Stages 2 & 3 (Gain + MFB Filter, $G \approx 150$): $V_{out} \approx 150 \times 6.05\text{ mV} \approx 0.91\text{ V peak}$ ($1.82\text{ V}_{pp}$)
* **AD8428 Path (PyPPM)**:
  - Stage 1 Out: $V_{ina} = 2000 \times 120\ \mu\text{V} \approx 240\text{ mV peak}$
  - Stages 2 & 3 (Buffer + MFB Filter, $G \approx 4\text{–}5$): $V_{out} \approx 4.5 \times 240\text{ mV} \approx 1.08\text{ V peak}$ ($2.16\text{ V}_{pp}$)
* **Stage 4 Output**:
  - *Option A (Comparator)*: $\pm 10\text{ mV}$ hysteresis slices the $\sim 2\text{ V}_{pp}$ sine wave into a clean 0–3.3 V square wave without noise chatter [Hook Line & Towfish, p. 27; JPM-4, p. 5].
  - *Option B (ADC)*: Buffered by ADA4841-1 into 16-bit ADC (AD7680) for software FFT / zero-crossing [PyPPM Sheet 11; Sri Lanka 2008, p. 4].

---

## SPICE Simulation Verification

The complete relay switching transient, resonant tuning, common-mode rejection, active filtering, and comparator digitization were verified in LTspice:
* Netlist file: [measuring_circuit.cir](file:///Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/LTSpice/measuring_circuit/measuring_circuit.cir)
* Simulation waveform plot: [measuring_circuit_waveforms.png](file:///Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/LTSpice/measuring_circuit/measuring_circuit_waveforms.png)
  - Successfully rejected $100\text{ mV}$ 60 Hz powerline hum and $10\text{ mV}$ 10 kHz motor ripple (>100 dB attenuation).
  - Delivered a clean $\approx 1.88\text{ V}$ peak-to-peak $2.2\text{ kHz}$ sinusoidal signal and digitized $3.3\text{ V}$ logic square wave within $3\text{ ms}$ after relay switching.

