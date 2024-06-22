/**
 * @brief Module for string utilities.
 */
#ifndef STR_H
#define STR_H

#include <stdlib.h>

/**
 * @brief Helper function equivalent to C++'s std::string_view.
 * @detail This structure contains a pointer into a C string and a length.
 * @warning This structure's length field is never to include a trailing zero
 *          if present.
 */
struct strv {
    /*!< A pointer to the underlying string */
    char* str;
    /*!< The length of the string, excluding a \0 if present. */
    size_t len;
};

#endif // STR_H
