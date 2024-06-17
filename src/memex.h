/**
 * @brief Utility module for memory-related operations.
 */
#ifndef MEMEX_H
#define MEMEX_H

#include <string.h>

/**
 * @brief Compare the elements in an array with a parameter, checking if
 *        they're the same.
 *
 * @param [in] xs A pointer to an array of elements.
 * @param [in] xs_len The length of the array of elements xs.
 * @param [in] x The element to compare each element of xs with.
 *
 * @return A boolean of whether every element of xs is equal to x.
 */
#define CMP_ALL(xs, xs_len, x) \
    ((xs)[0] == (x) && \
        memcmp((xs), (xs) + 1, ((xs_len) - 1) * sizeof((xs)[0])) == 0)

#endif /* MEMEX_H */
