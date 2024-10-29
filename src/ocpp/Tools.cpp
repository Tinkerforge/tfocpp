#include "Tools.h"

#include "Platform.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifndef OCPP_PLATFORM_ESP32
bool deadline_elapsed(uint32_t deadline_ms)
{
    uint32_t now = platform_now_ms();

    return (now - deadline_ms) < (UINT32_MAX / 2);
}
#endif

bool lookup_key(size_t *result, const char *key, const char * const *array, size_t array_length, const char * const *aliases, const size_t * const alias_indices, size_t alias_length) {
    for(size_t i = 0; i < array_length; ++i) {
        if (strcmp(key, array[i]) != 0)
            continue;

        if (result != nullptr)
            *result = i;
        return true;
    }

    if (aliases == nullptr || alias_length == 0)
        return false;

    for(size_t i = 0; i < alias_length; ++i) {
        if (strcmp(key, aliases[i]) != 0)
            continue;

        if (result != nullptr)
            *result = alias_indices[i];
        log_warn("Alias %s used to look up key %s", aliases[i], key);
        return true;
    }

    return false;
}

Option<int32_t> parse_int(const char *c) {
    errno = 0;

    char *p;
    int64_t parsed = strtol(c, &p, 10);

    if (errno != 0 || c == p)
        return {false};

    p += strspn(p, " \r\n\t");

    if (*p != '\0')
        return {false};

    if (parsed > INT32_MAX || parsed < INT32_MIN)
        return {false};

    return {(int32_t)parsed};
}
