#pragma once

#include "stdint.h"
#include "OcppTypes.h"
#include "OcppPlatform.h"

#include <memory>

bool deadline_elapsed(uint32_t deadline_ms);

bool lookup_key(size_t *result, const char *key, const char * const *array, size_t items_in_array);

Opt<int32_t> parse_int(const char *c);

template <typename T>
std::unique_ptr<T[]> heap_alloc_array(size_t n) {
    return std::unique_ptr<T[]>{new T[n]};
}
