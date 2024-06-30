#ifndef MATHEX_H
#define MATHEX_H

// @brief Perform a linear interpolation
#define LERP(x, x0, x1, y0, y1) \
    ((y0) * ((x1) - (x0))) + ((y1) * ((x) - (x0))) \
    / ((x1) - (x0))

#endif // MATHEX_H
