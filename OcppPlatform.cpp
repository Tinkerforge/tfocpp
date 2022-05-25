#include "OcppPlatform.h"

#include "time.h"

#include "RaccoonWSClient/WsRaccoonClient.h"

#define URL_PARSER_IMPLEMENTATION
#include "url_parse/url.h"

class ClientCallback : public WsClientCallback {
public:
    WsRaccoonClient wsClient;
    bool connected = false;
    std::function<void(char *, size_t)> cb;

    ClientCallback(std::string *websocket_host, uint16_t websocket_port, std::string *websocket_path) : wsClient(websocket_host, websocket_port, websocket_path, this)  {
        wsClient.start();
    }


    void onConnSuccess(std::string *sessionId) override {
        platform_printfln("connection succeeded");
        connected = true;
/*
        DynamicJsonDocument doc{0};
        BootNotification(&doc, "Warp 2 Charger Pro", "Tinkerforge GmbH", "warp2-X8A");

        std::string message;
        serializeJson(doc, message);

        onCallResult = [this](JsonObject obj) {
            auto result = parseBootNotificationResponse(obj);
            if (result.result == CallErrorCode::OK) {
                platform_printfln("boot notfication response okay!");
                BootNotificationResponseView resp{obj};

                char buf[100] = {0};
                time_t tt = resp.currentTime();
                const tm *t = gmtime(&tt);

                strftime(buf, 100, "%FT%TZ", t),

                platform_printfln("    current time: %s (%u)", buf, resp.currentTime());

                platform_printfln("    heartbeat interval: %d", resp.interval());
                switch(resp.status()) {
                    case BootNotificationResponseStatus::ACCEPTED:
                        platform_printfln("    accepted!");
                        break;
                    case BootNotificationResponseStatus::PENDING:
                        platform_printfln("    pending!");
                        break;
                    case BootNotificationResponseStatus::REJECTED:
                        platform_printfln("    rejected!");
                        break;

                }
            }
            else
                platform_printfln("boot notification response error %d: %s", result.result, result.error_description);

            start_charging(wsClient);

        };

        onCallError = [this](const char *error_code, const char *error_description, JsonObject payload) {
            std::string lalala;
            serializeJson(payload, lalala);
            platform_printfln("Received call error: %s %s %s", error_code, error_description, lalala.c_str());
        };

        wsClient->sendMessage(&message);
*/
    }

    void onConnClosed() override {
        platform_printfln("Connection closed");
        connected = false;
    }

    void onConnError() override {
        platform_printfln("Connection error");
        connected = false;
    }

    void onReceiveMessage(std::string *message) override {
        if (!cb)
            return;

        char *useless_copy = (char *) malloc(message->length());
        memcpy(useless_copy, message->c_str(), message->length());

        cb(useless_copy, message->length());

        free(useless_copy);
    };
};

void* platform_init(const char *websocket_url)
{
    parsed_url *parsed = parse_url(websocket_url, nullptr, 0);
    if (parsed == nullptr)
        return nullptr;

    // Why tf does the WsRaccoonClient constructor take string* ?!?
    // It copies over the content anyway.
    std::string host_useless_copy{parsed->host};
    std::string path_useless_copy{parsed->path};

    ClientCallback *platform_ctx = new ClientCallback{&host_useless_copy, parsed->port, &path_useless_copy};

    free(parsed);
    return platform_ctx;
}

uint32_t platform_now_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts );
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000L);
}

void platform_set_system_time(void *ctx, time_t t)
{
    platform_printfln("(Fake) Setting system time to %u", t);
}

bool platform_ws_connected(void *ctx)
{
    ClientCallback *cc = (ClientCallback *)ctx;
    return cc->connected;
}

void platform_ws_send(void *ctx, const char *buf, size_t buf_len)
{
    ClientCallback *cc = (ClientCallback *)ctx;
    if (!cc->connected)
        return;

    std::string useless_copy{buf, buf_len};
    cc->wsClient.sendMessage(&useless_copy);
}

void platform_ws_register_receive_callback(void *ctx, std::function<void(char *, size_t)> cb)
{
    ClientCallback *cc = (ClientCallback *)ctx;
    cc->cb = cb;
}

void platform_printfln(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    puts("");
}
