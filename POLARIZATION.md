# Proton Magnetometer Polarization Circuit

## Purpose

Apply DC current to the sensor coil for a configurable duration, rapidly remove its magnetic field, then reconnect the coil to the amplifier to measure proton precession.

## Circuit structure

| Component | Role |
| --- | --- |
| External DC supply/battery | Supplies polarization current |
| Sensor coil around water container | Creates the polarizing field and later detects the proton signal |
| DPDT relay | Switches both sensor-coil wires between polarization circuit and amplifier |
| Power N-channel MOSFET | Switches polarization current on/off |
| MOSFET gate driver | Quickly charges/discharges the MOSFET gate |
| TVS diode across MOSFET | Limits switch-off voltage and dissipates stored coil energy |
| Relay-driver transistor + flyback diode | Allows Arduino control of the relay’s electromagnet |
| Arduino | Coordinates switching and measurement timing |

## Connections

During polarization, the main current path is:

$$\text{Supply (+)} \to \text{relay contact} \to \text{sensor coil} \to \text{relay contact} \to \text{MOSFET drain} \to \text{MOSFET source} \to \text{supply (-)}$$

The TVS connects across the MOSFET’s drain and source.

The Arduino uses two outputs:
* `POLARIZE_EN`: controls the MOSFET through its gate driver.
* `RELAY_EN`: controls the DPDT relay through its transistor driver.

The Arduino, gate driver, and polarization supply need a shared control-ground reference for this non-isolated arrangement. The relay contacts disconnect both sensor-coil wires from the polarization circuit during measurement.

## Operating sequence

| Step | Relay position | MOSFET | Action |
| --- | --- | --- | --- |
| 1 | Polarization | OFF | Wait for relay contacts to settle |
| 2 | Polarization | ON | Hold DC current for the configured polarization time |
| 3 | Polarization | OFF | TVS quenches coil current; wait for it to decay |
| 4 | Amplifier | OFF | Wait for relay contacts and amplifier to settle |
| 5 | Amplifier | OFF | Acquire the proton signal |

Never move the relay while significant polarization current is flowing.

## Initial design calculations

For a $12\text{ V}$ supply and $5.5\ \Omega$ coil, the approximate settled current is:

$$I \approx \frac{V}{R} \approx 2.2\text{ A}$$

Coil heating during polarization is approximately:

$$P = I^2 R \approx 26\text{ W}$$

The energy that the quench circuit must handle is:

$$E = \frac{1}{2} L I^2$$

These are example values, not finalized specifications. This switched circuit does not actively regulate current; coil resistance and supply voltage largely determine it.

## Checks before building

* Measure the sensor coil’s resistance and inductance.
* Select MOSFET voltage rating above the TVS’s actual clamping voltage, with margin.
* Size the TVS for pulse energy and repeated measurement cycles.
* Verify relay DC contact-current rating and switching times.
* Include a supply fuse and suitable wiring.
* Make both control outputs default to OFF during startup/reset.
* Test current and switch-off voltage before connecting the amplifier.

## References

* [PyPPM v1.3 polarization schematic](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/12-pol.sch)
* [PyPPM v1.3 relay schematic](https://github.com/geekysuavo/pyppm/blob/main/designs/ppm-1.3/13-relay.sch)
