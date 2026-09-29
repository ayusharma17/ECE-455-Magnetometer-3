# Proton Magnetometer Polarization Circuit

## Purpose

Apply DC current to the sensor coil for a configurable duration, rapidly quench its magnetic field within $<400\ \mu\text{s}$, and disconnect the polarization supply before connecting the coil to the amplifier to measure proton precession.

---

## Circuit Structure and Component Grounding

| Component | Selected Part / Spec | Role in System | Grounding / Reference Source |
| --- | --- | --- | --- |
| **Polarization Supply** | 12 V Lead-Acid / LiFePO4 Battery | Supplies polarizing DC current (~2.2 A) | [PyPPM Sheet 12](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/12-pol.sch); [Sri Lanka 2008, p. 4](https://ipsl.lk/documents/TechSession/2008/ipsl0812.pdf) |
| **Reverse Protection** | S5GC (5 A / 400 V Rectifier) | Protects battery supply against reverse-connection | [PyPPM Sheet 12](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/12-pol.sch) |
| **Sensor Coil** | $L = 7.5\text{ mH}$, $R = 5.5\ \Omega$ | Creates polarizing B-field (~15–20 mT) and detects precession | [PyPPM Sheet 12/13](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/12-pol.sch); [JPM-4, p. 5](https://sensors.myu-group.co.jp/sm_pdf/SM2789.pdf) |
| **DPDT Relay** | OMRON G6A-2 (2 Form C, 2 A carry) | Galvanically breaks *both* coil leads between polarization and preamp | [PyPPM Sheet 13](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/13-relay.sch); [Sri Lanka 2008, p. 4](https://ipsl.lk/documents/TechSession/2008/ipsl0812.pdf) |
| **Relay Driver** | FDN327N / BSS138 ($V_{GS(th)} \le 1.5\text{ V}$) | Low-side N-channel MOSFET driving 5 V relay coil from 3.3 V/5 V MCU | [PyPPM Sheet 13](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/13-relay.sch) |
| **Relay Flyback** | SD101A Schottky / 1N4148 | Suppresses inductive turn-off spike across relay coil | [PyPPM Sheet 13](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/13-relay.sch) |
| **Power MOSFET** | 80 V–100 V N-ch (or IRLR024N 55 V) | Low-side power switch controlling polarization current | [PyPPM Sheet 12](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/12-pol.sch); [ECE 455 Final Slides, p. 13](file:///Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/PastProjects/ECE455_Final_Slides%20(1).pdf) |
| **Gate Driver** | FAN3111E (9 A peak, CMOS input) | Fast gate charging/discharging for clean sub-microsecond MOSFET turn-off | [PyPPM Sheet 12](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/12-pol.sch) |
| **TVS Clamp** | SMDJ45A (45 V standoff, 3000 W) | Clamps inductive back-EMF spike to ~51 V; rapidly depletes coil energy | [PyPPM Sheet 12](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/12-pol.sch); [JPM-4, p. 6](https://sensors.myu-group.co.jp/sm_pdf/SM2789.pdf) |
| **Bleed Resistor** | $R_{32} = 1\text{ k}\Omega$ across CCS+/CCS- | Parallel damping to suppress post-quench ringing across terminals | [PyPPM Sheet 12](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/12-pol.sch) |

---

## Connections and Current Path

During polarization, the main high-current path is:

$$\text{Supply (+)} \to \text{Relay NO 1} \to \text{Sensor Coil (+)} \to \text{Sensor Coil (-)} \to \text{Relay NO 2} \to \text{MOSFET Drain} \to \text{MOSFET Source} \to \text{Supply (-)}$$

* **TVS Diode**: Cathode connects to MOSFET Drain (`CCS-`), anode connects to GND (`Supply -`).
* **Relay Position**: De-energized state (NC) routes coil to preamplifier. Energized state (NO) routes coil to polarization supply.
* **Control Lines**:
  - `POLARIZE_EN`: Drives gate driver input (`CCS_EN`), with 47 kΩ pulldown.
  - `RELAY_EN`: Drives relay MOSFET gate, with 47 kΩ pulldown.
* **Grounding**: Control logic ground and 12 V battery negative share a single common return at the MOSFET source / battery negative terminal.

---

## Operating Timing and Break-Before-Make Sequence

To prevent contact arcing, the microcontroller must enforce strict break-before-make cold-switching:

| Step | Phase | Relay Position | MOSFET | Duration | Action |
| --- | --- | --- | --- | --- | --- |
| 1 | Relay Engage | Energized (NO) | OFF | 5 ms | Connects coil to polarization terminals; wait for contact bounce to settle |
| 2 | Polarize | Energized (NO) | ON | 0.5–2.0 s | Holds ~2.2 A DC current to align proton spins [Sri Lanka 2008, p. 4] |
| 3 | Quench | Energized (NO) | OFF | ~420 µs | Gate driver turns MOSFET OFF; TVS clamps back-EMF to 51.4 V, depleting current to zero |
| 4 | Settle Wait | Energized (NO) | OFF | 2.0 ms | Current and magnetic field fully collapse before opening contacts [Sri Lanka 2008, p. 4] |
| 5 | Relay Disengage| De-energized (NC)| OFF | 5–8 ms | De-energize relay; contacts transfer to preamp; wait for mechanical bounce to clear |
| 6 | Acquisition | De-energized (NC)| OFF | 1.0–1.5 s | Sample precession signal while relay coil remains unpowered (no EMI/thermal drift) |

> [!IMPORTANT]
> **Cold Switching Rule**: Never switch relay contacts while polarization current is flowing. Current must be fully quenched by the MOSFET and TVS ($t_q \approx 420\ \mu\text{s}$) during Step 3 before the relay is de-energized in Step 5.

---

## Design Calculations and Component Ratings

### 1. Polarization Current and Heating
For $V_{bat} = 12.0\text{ V}$ and $R_{coil} = 5.5\ \Omega$ (neglecting wiring and MOSFET $R_{DS(on)} \approx 65\text{ m}\Omega$):

$$I_0 \approx \frac{V_{bat}}{R_{coil}} \approx \frac{12.0\text{ V}}{5.5\ \Omega} \approx 2.18\text{ A} \approx 2.2\text{ A}$$

Peak power dissipation during polarization:

$$P_{coil} = I_0^2 R_{coil} \approx (2.18)^2 \times 5.5 \approx 26.2\text{ W}$$

For a 1.0 s pulse with a 5.0 s cycle time (20% duty cycle), average coil heating is $\bar{P} \approx 5.2\text{ W}$, which is readily dissipated by water cooling in the sample container.

### 2. Quench Dynamics and TVS Energy Absorption
When the MOSFET switches OFF, the inductor forces current to continue through the TVS diode. The drain voltage rises to the TVS clamping voltage $V_{clamp} \approx 51.4\text{ V}$.

The net voltage across the inductor opposing current is:

$$V_L = V_{clamp} - V_{bat} = 51.4\text{ V} - 12.0\text{ V} = 39.4\text{ V}$$

The quench duration (time for current to deplete from $I_0$ to zero) is:

$$t_q = \frac{L \cdot I_0}{V_{clamp} - V_{bat}} = \frac{7.5\times 10^{-3}\text{ H} \times 2.18\text{ A}}{39.4\text{ V}} \approx 415\ \mu\text{s}$$

This meets the design requirement that polarization current shut off within $<400\text{–}500\ \mu\text{s}$ to prevent premature transverse relaxation and loss of FID initial amplitude [JPM-4, p. 6], and collapses completely within the 2.0 ms quench wait period before relay switching [Sri Lanka 2008, p. 4].

The energy dissipated in the TVS clamp includes both stored magnetic energy and battery contribution during decay:

$$E_{mag} = \frac{1}{2} L I_0^2 = \frac{1}{2} (7.5\times 10^{-3}) (2.18)^2 \approx 17.8\text{ mJ}$$

$$E_{TVS} = \frac{1}{2} L I_0^2 \cdot \frac{V_{clamp}}{V_{clamp} - V_{bat}} = 17.8\text{ mJ} \times \frac{51.4\text{ V}}{39.4\text{ V}} \approx 23.2\text{ mJ}$$

The selected TVS (SMDJ45A) has a peak pulse power rating of $3000\text{ W}$ ($10/1000\ \mu\text{s}$ waveform, corresponding to $\approx 3\text{–}4\text{ J}$ pulse energy). A $23.2\text{ mJ}$ single-pulse dissipation utilizes $<1\%$ of its rated capacity. At a 5 s repetition rate, average TVS dissipation is $P_{avg} \approx 4.6\text{ mW}$, causing negligible thermal rise.

### 3. MOSFET and TVS Voltage Rating Margin
* **PyPPM Reference Choice**: PyPPM Sheet 12 pairs an `IRLR024N` ($V_{DSS} = 55\text{ V}$) with an `SMDJ45A` ($V_{BR} = 50.0\text{–}55.3\text{ V}$).
* **Margin Assessment**: At $2.2\text{ A}$, clamping voltage reaches $\approx 51.4\text{ V}$, leaving only $\approx 3.6\text{ V}$ (6.5%) margin below $V_{DSS} = 55\text{ V}$. Any inductive spike or temperature shift could push the MOSFET into uncontrolled avalanche breakdown.
* **Recommended Upgrade**: Use an 80 V to 100 V N-channel MOSFET (such as BSC0902NS, FDD8447L, or IRF540N) to provide $>50\%$ voltage safety headroom above the 51.4 V TVS clamp. Alternatively, if a 55 V MOSFET is used, select a 36 V TVS (SMBJ36A, $V_{BR} \approx 40\text{ V}$, $V_{clamp} \approx 43\text{ V}$ at 2.2 A, as in ECE 455 past projects) so clamping occurs safely below 55 V.

### 4. Relay Selection and Contact Ratings
* **OMRON G6A-2**: Rated for 2 A continuous carry current and 2 A switching at 30 VDC. At $I_0 \approx 2.2\text{ A}$, it is slightly above continuous DC rating, but because polarization is pulsed (20% duty cycle) and strictly *cold-switched* (contacts open/close with zero current), contacts will not arc or degrade.
* **OMRON G6K-2 Note**: G6K-2 is a subminiature surface-mount relay rated for only 1 A carry/switching. It is unsuitable for 2.2 A coils, but can be used for high-resistance / lower-current coils (such as the Sri Lanka 2008 coil at 0.71 A).

---

## References

* [PyPPM v1.3 Polarization Schematic (Sheet 12)](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/12-pol.sch)
* [PyPPM v1.3 Relay Schematic (Sheet 13)](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/13-relay.sch)
* [JPM-4 Polarization Quench Turn-Off, p. 6](https://sensors.myu-group.co.jp/sm_pdf/SM2789.pdf)
* [Sri Lanka PPM Polarization Timing and Relay Control, p. 4](https://ipsl.lk/documents/TechSession/2008/ipsl0812.pdf)
* [ECE 455 Past Projects: Switching and Input Protection, p. 13–14](file:///Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/PastProjects/ECE455_Final_Slides%20(1).pdf)

