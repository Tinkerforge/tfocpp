#pragma once

#include <algorithm>
#include <common/Platform.h>

#ifndef OCPP_LOG_FULL_PAYLOADS
#define OCPP_LOG_FULL_PAYLOADS 0
#endif

inline void ocpp_log_payload(const char *prefix, const char *buf, size_t buf_len)
{
#if OCPP_LOG_LEVEL >= OCPP_LOG_LEVEL_DEBUG
#if OCPP_LOG_FULL_PAYLOADS
    constexpr size_t chunk_size = 200;
    if (buf_len <= chunk_size) {
        log_debug("%s (len %zu) %.*s", prefix, buf_len, static_cast<int>(buf_len), buf);
        return;
    }
    for (size_t offset = 0; offset < buf_len; offset += chunk_size) {
        log_debug("%s (len %zu, offset %zu) %.*s", prefix, buf_len, offset,
                  static_cast<int>(std::min(buf_len - offset, chunk_size)), buf + offset);
    }
#else
    constexpr size_t preview_size = 100;
    log_debug("%s (len %zu) %.*s%s", prefix, buf_len,
              static_cast<int>(std::min(buf_len, preview_size)), buf,
              buf_len > preview_size ? " ..." : "");
#endif
#else
    (void)prefix;
    (void)buf;
    (void)buf_len;
#endif
}
