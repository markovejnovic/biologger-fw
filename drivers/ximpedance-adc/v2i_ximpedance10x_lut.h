#ifndef V2I_XIMPEDANCE10X_LUT_H
#define V2I_XIMPEDANCE10X_LUT_H

#include <stdint.h>

/// @brief Return the nanoamps measured by the sensor from given microvolts.
int32_t v2i_ximpedance10x_lut_get_nanoamps_from_microvolts(int32_t microvolts);

#endif // V2I_XIMPEDANCE10X_LUT_H
