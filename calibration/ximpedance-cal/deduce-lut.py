#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt
import textwrap
import collections

def strictly_decreasing(L):
    return all(x>y for x, y in zip(L, L[1:]))


def codegen(name: str, arr: np.ndarray):
    data = np.array((arr[1], arr[0])).T

    plt.subplot(223)
    plt.plot(arr[1], arr[0], 'x--')
    plt.grid()

    with open(f"{name}.h", "w+") as header:
        header.write(textwrap.dedent(f"""\
        #ifndef {name.upper()}_H
        #define {name.upper()}_H

        #include <stdint.h>

        /// @brief Return the nanoamps measured by the sensor from given microvolts.
        int32_t {name}_get_nanoamps_from_microvolts(int32_t microvolts);

        #endif // {name.upper()}_H
        """))

    with open(f"{name}.c", "w+") as source_f:
        source_f.write(textwrap.dedent(f"""\
        #include "{name}.h"
        #include <stdlib.h>
        #include "mathex.h"

        static const int32_t {name}_uv2ni[{data.shape[0]}][{data.shape[1]}] = {{
            {',\n            '.join('{' + str(int(dp[0] * 1e6)) + ', ' + str(int(dp[1] * 1e9)) + '}' for dp in data)}
        }};

        int32_t {name}_get_nanoamps_from_microvolts(int32_t microvolts) {{
            // This function is monotonically decreasing.
            if (microvolts <= {name}_uv2ni[0][0]) {{
                return {name}_uv2ni[0][1];
            }}

            if (microvolts >= {name}_uv2ni[{data.shape[0]} - 1][0]) {{
                return {name}_uv2ni[{data.shape[0]} - 1][1];
            }}

            for (size_t i = 0; i < {data.shape[0]} - 1; i++) {{
                const int32_t candidate_voltage_higher = {name}_uv2ni[i][0];
                const int32_t candidate_voltage_lower = {name}_uv2ni[i + 1][0];
                if (candidate_voltage_lower <= microvolts && microvolts >= candidate_voltage_higher) {{
                    // Perform a lerp between the two points.
                    return LERP(microvolts, candidate_voltage_lower, candidate_voltage_higher,
                                {name}_uv2ni[i][1], {name}_uv2ni[i + 1][1]);
                }}
            }}

            __builtin_unreachable();
        }}
        """))


def optimally_decimate_array(arr: np.ndarray) -> np.ndarray:
    interesting_values = np.array([[p[0], p[1]] for p in arr.T if p[0] <
                                   1e-5]).T
    diff_ord2_magn = np.abs(np.diff(np.diff(interesting_values[1])))
    normalized_interest_dist = diff_ord2_magn / np.sum(diff_ord2_magn)
    N_lut = 20
    while True:
        samples = sorted(np.random.choice(len(normalized_interest_dist), size=N_lut - 2, p=normalized_interest_dist))
        xs = np.array([interesting_values[0][0]] + list(interesting_values[0][samples]) +
                      [interesting_values[0][-1]])
        ys = np.array([interesting_values[1][0]] + list(interesting_values[1][samples]) +
                      [interesting_values[1][-1]])
        if all(c == 1 for _, c in collections.Counter(xs).items()) and \
            strictly_decreasing(xs):
            return np.array((xs, ys))


def main():
    iv_10x = np.load("data10.npy")
    iv_22x = np.load("data22.npy")

    iv_10x_cont = np.ascontiguousarray(optimally_decimate_array(iv_10x))
    iv_22x_cont = np.ascontiguousarray(optimally_decimate_array(iv_22x))

    codegen("v2i_ximpedance10x_lut", iv_10x_cont)
    codegen("v2i_ximpedance22x_lut", iv_22x_cont)

    plt.subplot(221)
    plt.title("G=10K")
    plt.plot(iv_10x[0], iv_10x[1])
    plt.xlabel("I [A]")
    plt.ylabel("V [V]")
    plt.grid()

    plt.subplot(222)
    plt.title("G=22K")
    plt.plot(iv_22x[0], iv_22x[1])
    plt.xlabel("I [A]")
    plt.ylabel("V [V]")
    plt.grid()

    plt.show()


if __name__ == "__main__":
    main()
