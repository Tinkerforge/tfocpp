#include "OcppTools.h"

#include "OcppPlatform.h"

#include "string.h"
#include "lib/ArduinoJson/ArduinoJson-v6.19.4.h"

bool deadline_elapsed(uint32_t deadline_ms)
{
    uint32_t now = platform_now_ms();

    return ((uint32_t)(now - deadline_ms)) < (UINT32_MAX / 2);
}

bool lookup_key(size_t *result, const char *key, const char **array, size_t items_in_array) {
    for(size_t i = 0; i < items_in_array; ++i) {
        if (strcmp(key, array[i]) != 0)
            continue;

        if (result != nullptr)
            *result = i;
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
