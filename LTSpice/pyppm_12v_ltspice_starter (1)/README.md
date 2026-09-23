# PyPPM 12 V polarizer — LTspice starter simulation

This package models the 12 V polarization path from the PyPPM v1.3 design. It is meant to teach the switching sequence and provide a starting point for your own coil measurements. It now includes both a graphical LTspice schematic and the original text netlist.

## Run it

1. Open `pyppm_12v_polarizer_graphical.asc` in LTspice to see the actual circuit and its wiring. The `.cir` file is also included as a text-only version.
2. Select **Simulate → Run**.
3. In the waveform window, use **Plot Settings → Add Trace** and add:
   - `I(LCOIL)` — polarizing current
   - `V(CCS_MINUS)` — MOSFET drain voltage / turn-off spike
   - `V(GATE)` — gate-driver output after R31
   - `I(DTVS)` — current absorbed by the TVS path
4. Open **View → SPICE Error Log** to see the measured maximum coil current, maximum drain voltage, and approximate quench time.

## What should happen

| Time | Command | Circuit behavior |
|---|---:|---|
| 0–1 ms | LOW | MOSFET open; coil current is approximately zero |
| 1–11 ms | HIGH | MOSFET closed; coil current rises toward about 2.16 A |
| After 11 ms | LOW | MOSFET opens; the drain rises to the TVS clamp region and coil current rapidly falls |

With a 12 V source, 5.5 ohm coil resistance, and 0.065 ohm switch resistance, the approximate steady current is:

`I = 12 V / (5.5 ohm + 0.065 ohm) = 2.16 A`

The turn-on time constant is approximately:

`tau = 7.5 mH / 5.565 ohm = 1.35 ms`

The coil stores about 17.5 mJ at 2.16 A. On turn-off, the simulated TVS limits the drain to roughly the 50–55 V region, allowing the current to disappear much faster than it would with an ordinary flyback diode.

## Change it for your coil

Edit these lines in the netlist:

```spice
RCOIL CCS_PLUS COIL_INTERNAL 5.5
LCOIL COIL_INTERNAL CCS_MINUS 7.5m
VBAT CCS_PLUS 0 12
```

Use your measured coil resistance and inductance. Do not raise `VBAT` in hardware until the MOSFET, TVS, relay contacts, capacitors, wiring, connector, coil heating, and battery/fuse ratings have all been checked.

## Model limitations

- In the graphical schematic, Q01 is a simplified generic NMOS adjusted to behave roughly like the power switch in this operating region. In the text netlist, it is a voltage-controlled-switch approximation. Neither is the complete IRLR024N manufacturer model.
- The TVS model captures basic avalanche clamping but not every transient or thermal behavior of a real SMDJ45A.
- Wiring inductance, relay contact behavior, battery internal resistance, and PCB parasitics are omitted.
- Therefore, use this to understand behavior and estimate values—not to certify that real components are safe.

The next refinement should replace Q01 and DTVS with vendor SPICE models and add measured battery, wiring, and coil parasitics.
