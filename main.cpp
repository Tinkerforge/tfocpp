#include "RaccoonWSClient/WsRaccoonClient.h"
#include <string>
#include <iostream>
#include <functional>
#include <unistd.h>

#include "stdint.h"

#include "ArduinoJson/ArduinoJson-v6.19.4.h"
//#include "OcppMessages.h"
#include "Ocpp.h"

uint32_t expectedMessageId;

// these callbacks will be set when sending a RPC call
std::function<void(JsonObject)> onCallResult;
std::function<void(const char*, const char*, JsonObject)> onCallError;

// will be called if an RPC call is received from the central system
std::function<void(const char *, JsonObject)> onCall;

void print_timestamp(time_t tt) {
    char buf[100] = {0};
    const tm *t = gmtime(&tt);

    strftime(buf, 100, "%FT%TZ", t);
    platform_printfln("%s", buf);
}

void start_charging(WsRaccoonClient *wsClient) {
    DynamicJsonDocument doc{0};
    time_t now = time(NULL);
    StartTransaction(&doc, 0, "00:11:22:33:44:55:66", 123456, now);

    std::string message;
    serializeJson(doc, message);

    onCallResult = [](JsonObject obj) {
        auto result = parseStartTransactionResponse(obj);
        if (result.result == CallErrorCode::OK) {
            platform_printfln("start transaction response okay!");
            StartTransactionResponseView resp{obj};

            platform_printfln("transaction id %u", resp.transactionId());
            platform_printfln("idTagInfo: status %d, parentIdTag %s expiryDate %s",
                    resp.idTagInfo().status(),
                    !resp.idTagInfo().parentIdTag().is_set() ? "not set" : resp.idTagInfo().parentIdTag().get(),
                    !resp.idTagInfo().expiryDate().is_set() ? "not set\n" : "");

            if (resp.idTagInfo().expiryDate().is_set())
                print_timestamp(resp.idTagInfo().expiryDate().get());
        }
        else
            platform_printfln("boot notification response error %d: %s", result.result, result.error_description);

    };

    wsClient->sendMessage(&message);
}

int main(int args, char **argv) {
    Ocpp ocpp;

    ocpp.start("ws://localhost:8180/steve/websocket/CentralSystemService", "CP_1");

    while(true) {
        ocpp.tick();
        for(int i = 0; i < 10; ++i)
            usleep(100);
    }

    return 0;
}
