# Proton-precession Python demo

Run with Python 3 after installing `numpy`, `scipy`, and `matplotlib`:

```bash
python proton_precession_demo.py
```

The script simulates a 50,000 nT field, adds deterministic noise and 60 Hz interference, applies a 1.7–2.5 kHz Butterworth band-pass filter, estimates frequency with an FFT and damped-sine fit, and converts frequency back to magnetic field.

It writes:

- `proton_precession_demo_results.png`
- `proton_precession_demo_results.txt`

The process exits with an error if the recovered field differs by more than 1.0 nT. The checked run recovered 50,000.223 nT, an error of +0.223 nT.
