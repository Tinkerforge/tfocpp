#include "OcppTools.h"

#include "OcppPlatform.h"

#include "string.h"
#include <ArduinoJson.h>

bool deadline_elapsed(uint32_t deadline_ms)
{
    uint32_t now = platform_now_ms();

    return ((uint32_t)(now - deadline_ms)) < (UINT32_MAX / 2);
}

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

Opt<int32_t> parse_int(const char *c) {
    StaticJsonDocument<16> doc;
    if (deserializeJson(doc, c) != DeserializationError::Ok)
        return {false};

    if (!doc.is<int32_t>())
        return {false};

    int32_t parsed = doc.as<int32_t>();
    return {parsed};
}
