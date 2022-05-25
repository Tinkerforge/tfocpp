#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <time.h>

#include <functional>

void *platform_init(const char *websocket_url);

uint32_t platform_now_ms();
void platform_set_system_time(void *ctx, time_t t);

bool platform_ws_connected(void *ctx);
void platform_ws_send(void *ctx, const char *buf, size_t buf_len);

void platform_ws_register_receive_callback(void *ctx, std::function<void(char *, size_t)> cb);

void platform_printfln(const char *fmt, ...) __attribute__((__format__(__printf__, 1, 2)));
