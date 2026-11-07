#pragma once
#include <Arduino.h>
class MDNSResponder {
public:
    bool begin(const char* host) { return true; }
    void end() {}
    bool addService(const char* s, const char* p, uint16_t port) { return true; }
};
extern MDNSResponder MDNS;
