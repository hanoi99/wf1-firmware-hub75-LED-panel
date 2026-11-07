// Compile-check stub with the Arduino-ESP32 WebServer signatures used by the firmware.
#pragma once
#include <Arduino.h>
#include <functional>
#include "LittleFS.h"
enum HTTPMethod { HTTP_GET = 1, HTTP_POST = 3 };
class WebServer {
public:
    typedef std::function<void(void)> THandlerFunction;
    WebServer(int port = 80) {}
    void begin() {}
    void handleClient() {}
    void on(const String& uri, HTTPMethod m, THandlerFunction f) {}
    void onNotFound(THandlerFunction f) {}
    String arg(const String& name) const { return String(); }
    bool hasArg(const String& name) const { return false; }
    String uri() const { return String(); }
    void sendHeader(const String& n, const String& v, bool first = false) {}
    void send(int code, const char* type, const String& content) {}
    void send(int code, const char* type, const char* content) {}
    void send(int code, const String& type, const String& content) {}
    size_t streamFile(File& f, const String& contentType) { return 0; }
    bool authenticate(const char* user, const char* pass) { return true; }
    void requestAuthentication() {}
};
