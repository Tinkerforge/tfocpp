#pragma once

#include "stdint.h"
#include "Types.h"

#include <memory>

#define OCPP_NODISCARD __attribute__ ((warn_unused_result))

bool deadline_elapsed(uint32_t deadline_ms);

bool lookup_key(size_t *result, // Result pointer. May be null if you only need to know if the key was found
                const char *key, // Key to look for
                const char * const *array, // Array of keys to search in
                size_t array_length,
                const char * const *aliases = nullptr, // Array of aliases for non-standard lookups. Only searched through if no key in array matches.
                const size_t * const alias_indices = nullptr, // If the n-th alias matches, the n-th entry in this array is returned
                size_t alias_length = 0) // Length of alias and alias_indices arrays.
                OCPP_NODISCARD;

Opt<int32_t> parse_int(const char *c);

#ifndef HEAP_ALLOC_ARRAY_DEFINED
#define HEAP_ALLOC_ARRAY_DEFINED
template <typename T>
std::unique_ptr<T[]> heap_alloc_array(size_t n) {
    return std::unique_ptr<T[]>{new T[n]()};
}
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define SILENCE_GCC_UNREACHABLE() assert(false)
#else
#define SILENCE_GCC_UNREACHABLE()
#endif
