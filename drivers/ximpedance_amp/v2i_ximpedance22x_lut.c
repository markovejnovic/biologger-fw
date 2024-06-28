#include "v2i_ximpedance22x_lut.h"
#include <stdlib.h>
#include "mathex.h"

static const int32_t v2i_ximpedance22x_lut_uv2ni[20][2] = {
    {4806, 20356},
    {4806, 20356},
    {4806, 20356},
    {4806, 20356},
    {4806, 20356},
    {4806, 20356},
    {4806, 20356},
    {13411, 4742},
    {105941, 3297},
    {123277, 2812},
    {214722, -1600},
    {297777, -6012},
    {486722, -16795},
    {698588, -26105},
    {830411, -31981},
    {874058, -33940},
    {1109055, -44239},
    {1685333, -70218},
    {1764294, -73650},
    {2047000, -92773}
};

int32_t v2i_ximpedance22x_lut_get_nanoamps_from_microvolts(int32_t microvolts) {
    // This function is monotonically decreasing.
    if (microvolts <= v2i_ximpedance22x_lut_uv2ni[0][0]) {
        return v2i_ximpedance22x_lut_uv2ni[0][1];
    }

    if (microvolts >= v2i_ximpedance22x_lut_uv2ni[20 - 1][0]) {
        return v2i_ximpedance22x_lut_uv2ni[20 - 1][1];
    }

    for (size_t i = 0; i < 20 - 1; i++) {
        const int32_t candidate_voltage_higher = v2i_ximpedance22x_lut_uv2ni[i][0];
        const int32_t candidate_voltage_lower = v2i_ximpedance22x_lut_uv2ni[i + 1][0];
        if (candidate_voltage_lower <= microvolts && microvolts >= candidate_voltage_higher) {
            // Perform a lerp between the two points.
            return LERP(microvolts, candidate_voltage_lower, candidate_voltage_higher,
                        v2i_ximpedance22x_lut_uv2ni[i][1], v2i_ximpedance22x_lut_uv2ni[i + 1][1]);
        }
    }

    __builtin_unreachable();
}
