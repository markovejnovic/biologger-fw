#include "v2i_ximpedance10x_lut.h"
#include <stdlib.h>
#include "mathex.h"

static const int32_t v2i_ximpedance10x_lut_uv2ni[20][2] = {
    {45470, 9940},
    {70411, 7432},
    {139210, 414},
    {149846, -556},
    {154863, -1071},
    {194600, -5076},
    {199260, -5572},
    {578999, -43707},
    {598937, -45770},
    {623470, -48247},
    {628588, -48753},
    {937611, -79809},
    {1256687, -111887},
    {1271388, -113383},
    {1276470, -113899},
    {1555647, -142033},
    {1605000, -146968},
    {1929222, -179602},
    {1934058, -180117},
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

