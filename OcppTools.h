#pragma once

#include "stdint.h"
#include "OcppTypes.h"

bool deadline_elapsed(uint32_t deadline_ms);

bool lookup_key(size_t *result, const char *key, const char **array, size_t items_in_array);

Opt<int32_t> parse_int(const char *c);
