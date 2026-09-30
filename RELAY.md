# Proton Magnetometer Relay Circuit

## Purpose

Electromechanically isolate the sensitive measurement circuit from the high-voltage (12 V) and high-current (~2 A) polarization circuit. The relay switches both sensor coil leads between polarization and signal readout.

---

## Circuit Architecture

```
                  +-------------------------------------------------------+
                  |                     DPDT RELAY                        |
                  |                                                       |
Polarization  ----+ NO 1                                             NC 1 +---- Preamp (+)
Supply (12V)      |        COM 1 ------------------- COM 2                |
                  |          |                         |                  |
                  |     Sensor Coil+              Sensor Coil-            |
                  |                                                       |
Polarization  ----+ NO 2                                             NC 2 +---- Preamp (-)
MOSFET Drain      +-------------------------------------------------------+
```

---

## Component Selection and Grounding

| Subsystem | Component | Selected Part / Spec | Role in System | Grounding / Reference Source |
| --- | --- | --- | --- | --- |
| **Relay** | DPDT Signal Relay | OMRON G6A-2 (2 Form C, 2 A carry) | Galvanically breaks *both* coil terminals between polarization and preamplifier; cold-switched | [PyPPM Sheet 13](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/13-relay.sch); [Sri Lanka 2008, p. 4](https://ipsl.lk/documents/TechSession/2008/ipsl0812.pdf); [Hook Line & Towfish, p. 14](file:///Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/PastProjects/ECE%20455%20Final%20Documentation%20Slides%20Hook%20Line%20and%20Towfish.pptx.pdf) |
| **Relay Driver** | Low-side MOSFET | FDN327N / 2N7002 ($V_{GS(th)} \le 1.5\text{ V}$) | Drives 5 V / 40 mA relay coil from microcontroller 3.3 V/5 V GPIO | [PyPPM Sheet 13](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/13-relay.sch); [POLARIZATION.md](file:///Users/ayushsharma/Documents/ChatGPT/Proton_Magnetometer_ECE_455/POLARIZATION.md) |
| **Relay Flyback** | Fast Diode | SD101A Schottky / 1N4148 | Suppresses relay coil inductive turn-off spike | [PyPPM Sheet 13](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/13-relay.sch) |
| **Gate Resistors**| Series / Pull-down | $100\ \Omega$ series, $47\text{ k}\Omega$ pulldown to GND | Prevents gate ringing; guarantees relay defaults OFF during MCU reset | [PyPPM Sheet 13](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/13-relay.sch) |

---

## Relay Pinout Mapping (OMRON G6A-2)

| Pin Number | Function | Connected To |
|:---:|:---|:---|
| **1** | Coil (+) | $+5\text{ V}$ Rail |
| **10** | Coil (-) | Drain of Low-side MOSFET Driver |
| **4** | Pole 1 Common (`COM1`) | **Sensor Coil (+)** lead |
| **3** | Pole 1 Normally Open (`NO1`) | $+12\text{ V}$ Polarization Bus (`CCS_PLUS`) |
| **5** | Pole 1 Normally Closed (`NC1`) | Preamp Input (+) |
| **7** | Pole 2 Common (`COM2`) | **Sensor Coil (-)** lead |
| **8** | Pole 2 Normally Open (`NO2`) | Polarization MOSFET Drain (`CCS_MINUS`) |
| **6** | Pole 2 Normally Closed (`NC2`) | Preamp Input (-) |

---

## Operating Timing and Break-Before-Make Sequence

To prevent contact arcing and protect measuring circuitry, the microcontroller strictly enforces cold-switching:

```
Timing Sequence:
Time:      t = 0         t = 5 ms        t = 1.0 s       t = 1.002 s     t = 1.010 s           t = 2.5 s
Signal:    |-------------|---------------|---------------|---------------|---------------------|
RELAY_EN:  HIGH (NO)     HIGH (NO)       HIGH (NO)       HIGH (NO)       LOW (NC -> Preamp)    LOW (Preamp)
POL_EN:    LOW           HIGH (Polarize) LOW (Quench)    LOW (Decayed)   LOW                   LOW
Phase:     [Relay Settling] [  Polarization  ] [ Quench Wait ] [Transit/Bounce] [ Signal Acquisition Window ]
```

1. **Relay Engage (NO Position)**: Microcontroller asserts `RELAY_EN`. Relay connects sensor coil to the polarization terminals. Wait $5\text{ ms}$ for contact bounce to settle.
2. **Polarization**: Microcontroller asserts `POLARIZE_EN` to the MOSFET gate driver. $2.2\text{ A}$ DC current aligns proton spins for $0.5\text{–}2.0\text{ s}$ [Sri Lanka 2008, p. 4].
3. **Quench**: Microcontroller de-asserts `POLARIZE_EN`. The TVS diode clamps back-EMF, depleting coil current from $2.2\text{ A}$ to zero in $<400\ \mu\text{s}$ [JPM-4, p. 6].
4. **Quench Settling Wait**: Microcontroller holds `RELAY_EN` HIGH for an additional $2.0\text{ ms}$ while magnetic field completely collapses. *Never switch relay contacts while coil current is flowing.* [Sri Lanka 2008, p. 4; POLARIZATION.md].
5. **Relay Disengage (NC Position)**: Microcontroller de-asserts `RELAY_EN`. Relay de-energizes, connecting coil to preamp. Wait $5\text{–}8\text{ ms}$ for mechanical transit and contact settling.
6. **Precession Acquisition**: Microcontroller measures precession over a $1.0\text{–}1.5\text{ s}$ window while the relay coil is unpowered (avoiding relay coil magnetic interference and heating).
