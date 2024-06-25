#!/usr/bin/env python

from __future__ import annotations

import sys
import numpy as np
import matplotlib.pyplot as plt
import pathlib

# Get this from process.py
KNOWN_CURRENT_TO_VOLTAGE_RATIOS: tuple[tuple[float, ...], tuple[float, ...]] = (
    (-80.0e-6, -70.0e-6, -60.0e-6, -50.0e-6, -40.0e-6, -30.0e-6, -20.0e-6, -10.0e-6),
    (1897.01e-3, 1674.18e-3, 1457.56e-3, 1236.95e-3, 1020.39e-3, 797.671e-3, 573.201e-3,
     368.117e-3),
)


def load_sweep(p: pathlib.Path) -> np.ndarray:
    data = []
    with p.open("r") as sweep_f:
        for i, line in enumerate(sweep_f.readlines()):
            spl = line.split(" ")
            if len(spl) != 4:
                raise ValueError("Failed to load the sweep because the file "
                                 f"is malformed. See {p}:{i + 1}")
            # File is in millivolts
            data.append(float(spl[2]) * 1e-3)
    return np.array(data)


def demarcate_sweep(sample_points: np.ndarray) -> list[list[tuple[float, float]]]:
    # This algorithm operates by traversing the input signal. It assumes the
    # signal has a demarcation point at the start (the start of the signal).
    # A greedy reader will traverse the signal computing the rolling average
    # x^[n] and the stdev from the last demarcation point v^2[n]. For each
    # new point x[n], the reader will compare the difference between x[n] -
    # x^[n - 1] against the rolling stdev. If a certain threshold is
    # reached a demarcation is made. Note that this algorithm will also delete
    # points near the demarcation point to help alleviate concerns due to
    # transients.
    alpha = 3.5  # The coefficient for determining demarcation windows.
    beta = int(18)  # The minimum number of points per demarcation. Must be >1
    gamma = int(4)  # The number of samples to slice away from the head and tail of each
                    # demarcation.
    delta = 0.001   # The minimum stddev for marking a demarcation

    out: list[list[tuple[float, float]]] = []

    demarcation_window: list[tuple[float, float]] = [(0.0, sample_points[0])]
    for i, point in enumerate(sample_points[1:]):
        if len(demarcation_window) < beta:
            demarcation_window.append((i + 1, point))
            continue

        _, demarcated_ys = zip(*demarcation_window)
        demarcation_window_mean = np.mean(demarcated_ys)
        demarcation_window_stddev = np.std(demarcated_ys)

        point_delta = abs(point - demarcation_window_mean)
        if point_delta > alpha * max(demarcation_window_stddev, delta):
            # This new point that we're observing belongs to another
            # demarcation window. Flush this demarcation window into the output
            # and start a new one.
            out.append(demarcation_window[gamma:-gamma])
            demarcation_window = [(i + 1, point)]
            continue

        # Otherwise, this point belongs to the current demarcation_window.
        demarcation_window.append((i + 1, point))

    # Add the last window
    out.append(demarcation_window[gamma:-gamma])

    return out


def compute_demarcation_stats(
    xs: list[list[tuple[float, float]]],
) -> list[tuple[float, float, float]]:
    """Compute the x midpoint, y mean and y stdev of each demarcation."""
    out: list[tuple[float, float, float]] = []
    for demarcation_window in xs:
        demarcation_xs, demarcation_ys = zip(*demarcation_window)
        out.append((
            (max(demarcation_xs) - min(demarcation_xs)) / 2 + min(demarcation_xs),
            np.mean(demarcation_ys),
            float(np.std(demarcation_ys)),
        ))
    return out


def main():
    sweep = load_sweep(pathlib.Path(sys.argv[1]))
    plt.subplot(221)
    plt.title("Raw Sweep Samples")
    plt.plot(sweep, '.')
    plt.ylabel("Voltage [V]")
    plt.grid()

    demarcations = compute_demarcation_stats(demarcate_sweep(sweep))
    # Attempt to compute the linfit of the demarcated values. Ignore the heads and tails
    # because they are most definitely distorted.
    demarcation_xs, demarcation_ys, _ = zip(*demarcations)
    coeffs_demarcation = np.polyfit(demarcation_xs[10:-10], demarcation_ys[10:-10], 1)
    k_demarcation = coeffs_demarcation[0]
    m_demarcation = coeffs_demarcation[1]

    plt.subplot(222)
    plt.title(f"Demarcated Steps\nk={k_demarcation:.4} m={m_demarcation:.4}")
    for x_midpoint, y_mean, y_stdev, in demarcations:
        plt.errorbar(x_midpoint, y_mean, fmt='.', yerr=y_stdev)
    plt.plot(demarcation_xs, k_demarcation * np.array(demarcation_xs) + m_demarcation,
             '--')
    plt.ylabel("Voltage [V]")
    plt.grid()


    # Attempt to compute the linfit of the known ratios
    coeffs = np.polyfit(*KNOWN_CURRENT_TO_VOLTAGE_RATIOS, 1)
    k = coeffs[0]
    m = coeffs[1]

    plt.subplot(223)
    plt.title(f"Known current-voltage ratios\nk={k:.4} m={m:.4}")
    plt.plot(np.linspace(-100e-6, 0), k * np.linspace(-100e-6, 0) + m, '--')
    plt.plot(*KNOWN_CURRENT_TO_VOLTAGE_RATIOS, "bx")
    plt.xlabel("Current [A]")
    plt.ylabel("Voltage [V]")
    plt.grid()

    # Now we have to correlate the demarcations with the known current-voltage ratios.
    # Let k2 and m2 be the terms in y=kx+m of the well-defined measured I-V ratio.
    # Let k1 and m1 be the terms in y=kx+m of the sweep linear portion.
    # Then alpha = k2/k1
    #      beta = (m1 - m2)/k2
    alpha = k / k_demarcation
    beta = (m_demarcation - m) / k
    iv_curve = [
        ((1/alpha) * x_midpoint + beta, y_mean, y_stdev)
        for x_midpoint, y_mean, y_stdev in demarcations
    ]

    plt.subplot(224)
    plt.title("I-V Ratio of the amplifier")
    i_known, v_known = KNOWN_CURRENT_TO_VOLTAGE_RATIOS
    i_swept, v_swept, _ = zip(*iv_curve)
    plt.plot(np.array(i_known) * 1e6, np.array(v_known) * 1000, "bx",
             label="Manually Measured")
    plt.plot(np.array(i_swept) * 1e6, np.array(v_swept) * 1000,
             label="Swept")
    plt.xlabel("Current [uA]")
    plt.ylabel("Voltage [mV]")
    plt.legend()
    plt.grid()

    plt.tight_layout()
    plt.show()



if __name__ == "__main__":
    main()
