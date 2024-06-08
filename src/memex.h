#ifndef MEMEX_H
#define MEMEX_H

#include <string.h>

#define CMP_ALL(xs, xs_len, x) \
    ((xs)[0] == (x) && \
        memcmp((xs), (xs) + 1, ((xs_len) - 1) * sizeof((xs)[0])) == 0)

#endif // MEMEX_H
