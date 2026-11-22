#pragma once
#include <Arduino.h>
#include "WiFi.h"
#define HTTP_CODE_OK 200
class HTTPClient {
public:
    void setConnectTimeout(int32_t ms) {}
    void setTimeout(uint16_t ms) {}
    bool begin(WiFiClient& c, const String& url) { return true; }
    int GET() { return 200; }
    String getString() { return String(); }
    void end() {}
};
