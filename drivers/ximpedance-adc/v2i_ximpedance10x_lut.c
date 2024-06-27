#include "v2i_ximpedance10x_lut.h"
#include <stdlib.h>
#include "mathex.h"

static const int32_t v2i_ximpedance10x_lut_uv2ni[20][2] = {
    {5714, 30247},
    {5714, 30247},
    {4044, 19183},
    {3072, 15401},
    {10437, 13429},
    {25823, 11943},
    {129937, 1395},
    {135066, 920},
    {204923, -6097},
    {204923, -6097},
    {589000, -44698},
    {593260, -45213},
    {1241812, -110370},
    {1271388, -113383},
    {1301000, -116407},
    {1306117, -116913},
    {1605000, -146968},
    {1605000, -146968},
    {1904411, -177104},
    {2047000, -193557}
};

int32_t v2i_ximpedance10x_lut_get_nanoamps_from_microvolts(int32_t microvolts) {
    // This function is monotonically decreasing.
    if (microvolts <= v2i_ximpedance10x_lut_uv2ni[0][0]) {
        return v2i_ximpedance10x_lut_uv2ni[0][1];
    }

    if (microvolts >= v2i_ximpedance10x_lut_uv2ni[20 - 1][0]) {
        return v2i_ximpedance10x_lut_uv2ni[20 - 1][1];
    }

    for (size_t i = 0; i < 20 - 1; i++) {
        const int32_t candidate_voltage_higher = v2i_ximpedance10x_lut_uv2ni[i][0];
        const int32_t candidate_voltage_lower = v2i_ximpedance10x_lut_uv2ni[i + 1][0];
        if (candidate_voltage_lower <= microvolts && microvolts >= candidate_voltage_higher) {
            // Perform a lerp between the two points.
            return LERP(microvolts, candidate_voltage_lower, candidate_voltage_higher,
                        v2i_ximpedance10x_lut_uv2ni[i][1], v2i_ximpedance10x_lut_uv2ni[i + 1][1]);
        }
    }

    __builtin_unreachable();
}
