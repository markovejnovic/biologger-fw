#!/usr/bin/env python3

import matplotlib.pyplot as plt
import numpy as np
import pathlib
import sys
import subprocess


def main():
    data_dir = pathlib.Path(sys.argv[1])
    xs = []
    ys = []
    for data in data_dir.iterdir():
        if data.name == "sweep.txt":
            continue

        current = float(data.name[1:-1] if data.name.startswith("n")
                        else data.name[0:-1]) \
                * (-1 if data.name.startswith("n") else 1)
        average_voltage = float(subprocess.check_output(
            f"cat {data} "
            "| grep 'ADC = ' "
            "| awk '{ sum += $3; count += 1 } END { print sum / count }'",
            shell=True,
        ).strip())
        xs.append(current)
        ys.append(average_voltage)

    xs, ys = zip(*sorted(zip(xs, ys)))

    print(xs, ys)
    plt.plot(xs, ys, '.')
    plt.title(sys.argv[1])
    plt.xlabel("Input current [uA]")
    plt.ylabel("Output Voltage [mV]")
    plt.grid()
    plt.show()


if __name__ == "__main__":
    main()
