#include "v2i_ximpedance22x_lut.h"
#include <stdlib.h>
#include "mathex.h"

static const int32_t v2i_ximpedance22x_lut_uv2ni[20][2] = {
    {13411, 4742},
    {123277, 2812},
    {156705, 1338},
    {194235, -620},
    {234222, -2589},
    {244235, -3073},
    {297777, -6012},
    {422222, -12877},
    {497058, -17280},
    {819444, -31496},
    {830411, -31981},
    {929647, -36394},
    {940944, -36878},
    {975000, -38352},
    {1052888, -41785},
    {1099388, -43744},
    {1303529, -53054},
    {1972944, -82960},
    {1994333, -83949},
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
