#!/usr/bin/env python3
"""Simulate and recover a noisy proton-precession magnetometer signal."""

from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
from scipy.optimize import curve_fit
from scipy.signal import butter, sosfiltfilt


GAMMA_HZ_PER_UT = 42.57747892
SAMPLE_RATE_HZ = 10_000.0
DURATION_S = 1.5
TRUE_FIELD_NT = 50_000.0
DECAY_TIME_S = 0.55
RNG_SEED = 20260916


def damped_sine(t, amplitude, decay_s, frequency_hz, phase_rad, offset):
    return amplitude * np.exp(-t / decay_s) * np.sin(
        2.0 * np.pi * frequency_hz * t + phase_rad
    ) + offset


def parabolic_peak_frequency(freqs, spectrum, index):
    """Refine an FFT-bin peak with quadratic interpolation in log magnitude."""
    if index <= 0 or index >= len(spectrum) - 1:
        return freqs[index]
    y = np.log(np.maximum(spectrum[index - 1 : index + 2], 1e-30))
    denominator = y[0] - 2.0 * y[1] + y[2]
    delta = 0.0 if denominator == 0 else 0.5 * (y[0] - y[2]) / denominator
    return freqs[index] + delta * (freqs[1] - freqs[0])


def run_demo(output_dir: Path):
    rng = np.random.default_rng(RNG_SEED)
    t = np.arange(0.0, DURATION_S, 1.0 / SAMPLE_RATE_HZ)
    true_frequency_hz = GAMMA_HZ_PER_UT * (TRUE_FIELD_NT / 1000.0)

    clean = damped_sine(t, 1.0, DECAY_TIME_S, true_frequency_hz, 0.35, 0.0)
    interference = 0.22 * np.sin(2.0 * np.pi * 60.0 * t)
    noise = rng.normal(0.0, 0.32, t.size)
    raw = clean + interference + noise

    sos = butter(
        4,
        [1700.0, 2500.0],
        btype="bandpass",
        fs=SAMPLE_RATE_HZ,
        output="sos",
    )
    filtered = sosfiltfilt(sos, raw)

    window = np.hanning(t.size)
    spectrum = np.abs(np.fft.rfft(filtered * window))
    freqs = np.fft.rfftfreq(t.size, 1.0 / SAMPLE_RATE_HZ)
    search = (freqs >= 1700.0) & (freqs <= 2500.0)
    peak_index = np.flatnonzero(search)[np.argmax(spectrum[search])]
    coarse_frequency_hz = parabolic_peak_frequency(freqs, spectrum, peak_index)

    fit_mask = t <= 1.15
    initial = [
        np.std(filtered[fit_mask]) * np.sqrt(2.0),
        0.5,
        coarse_frequency_hz,
        0.0,
        0.0,
    ]
    bounds = ([0.0, 0.1, 1700.0, -2 * np.pi, -0.2], [2.0, 2.0, 2500.0, 2 * np.pi, 0.2])
    params, _ = curve_fit(
        damped_sine,
        t[fit_mask],
        filtered[fit_mask],
        p0=initial,
        bounds=bounds,
        maxfev=30_000,
    )
    estimated_frequency_hz = params[2]
    estimated_field_nt = estimated_frequency_hz / GAMMA_HZ_PER_UT * 1000.0
    field_error_nt = estimated_field_nt - TRUE_FIELD_NT

    fit = damped_sine(t, *params)
    plt.style.use("default")
    fig = plt.figure(figsize=(14.22, 8.0), facecolor="#f7f5f0")
    grid = fig.add_gridspec(2, 2, height_ratios=[1.0, 1.05], hspace=0.34, wspace=0.24)
    ax1 = fig.add_subplot(grid[0, :])
    ax2 = fig.add_subplot(grid[1, 0])
    ax3 = fig.add_subplot(grid[1, 1])

    navy = "#102d46"
    blue = "#2563eb"
    teal = "#0f766e"
    orange = "#d97706"
    for ax in (ax1, ax2, ax3):
        ax.set_facecolor("#f7f5f0")
        ax.spines[["top", "right"]].set_visible(False)
        ax.tick_params(colors=navy, labelsize=10)
        ax.xaxis.label.set_color(navy)
        ax.yaxis.label.set_color(navy)
        ax.title.set_color(navy)
        ax.grid(alpha=0.18)

    time_ms = t * 1000.0
    view = t <= 0.080
    ax1.plot(time_ms[view], raw[view], color="#94a3b8", lw=1.0, label="Noisy ADC input")
    ax1.plot(time_ms[view], filtered[view], color=blue, lw=1.5, label="Band-pass filtered")
    ax1.set_title("Time-domain signal", loc="left", fontsize=15, fontweight="bold")
    ax1.set_xlabel("Time (ms)")
    ax1.set_ylabel("Normalized voltage")
    ax1.legend(frameon=False, ncol=2, loc="upper right")

    zoom = (freqs >= 1950.0) & (freqs <= 2300.0)
    normalized_spectrum = spectrum / spectrum[zoom].max()
    ax2.plot(freqs[zoom], normalized_spectrum[zoom], color=teal, lw=1.8)
    ax2.axvline(estimated_frequency_hz, color=orange, lw=1.5, ls="--")
    ax2.set_title("Frequency estimate", loc="left", fontsize=15, fontweight="bold")
    ax2.set_xlabel("Frequency (Hz)")
    ax2.set_ylabel("Normalized magnitude")
    ax2.text(
        0.04,
        0.90,
        f"fit = {estimated_frequency_hz:.4f} Hz",
        transform=ax2.transAxes,
        color=navy,
        fontsize=12,
        fontweight="bold",
    )

    fit_view = t <= 0.025
    ax3.plot(time_ms[fit_view], filtered[fit_view], color=blue, lw=1.4, label="Filtered")
    ax3.plot(time_ms[fit_view], fit[fit_view], color=orange, lw=1.2, ls="--", label="Damped-sine fit")
    ax3.set_title("Model fit", loc="left", fontsize=15, fontweight="bold")
    ax3.set_xlabel("Time (ms)")
    ax3.set_ylabel("Normalized voltage")
    ax3.legend(frameon=False, loc="upper right")

    fig.suptitle(
        "Python subsystem demo: noisy proton signal to magnetic field",
        x=0.055,
        y=0.98,
        ha="left",
        color=navy,
        fontsize=22,
        fontweight="bold",
    )
    fig.text(
        0.055,
        0.015,
        f"True field {TRUE_FIELD_NT:,.1f} nT   Estimated {estimated_field_nt:,.2f} nT   Error {field_error_nt:+.2f} nT   Seed {RNG_SEED}",
        color=navy,
        fontsize=12,
        fontweight="bold",
    )
    fig.savefig(output_dir / "proton_precession_demo_results.png", dpi=150, bbox_inches="tight")
    plt.close(fig)

    summary = (
        f"true_frequency_hz={true_frequency_hz:.6f}\n"
        f"coarse_fft_frequency_hz={coarse_frequency_hz:.6f}\n"
        f"estimated_frequency_hz={estimated_frequency_hz:.6f}\n"
        f"true_field_nt={TRUE_FIELD_NT:.3f}\n"
        f"estimated_field_nt={estimated_field_nt:.3f}\n"
        f"field_error_nt={field_error_nt:+.3f}\n"
    )
    (output_dir / "proton_precession_demo_results.txt").write_text(summary)
    print(summary, end="")
    return abs(field_error_nt)


if __name__ == "__main__":
    output = Path(__file__).resolve().parent
    error = run_demo(output)
    if error > 1.0:
        raise SystemExit(f"Demo failed accuracy check: {error:.3f} nT")
    print("PASS: absolute field error is below 1.0 nT")
