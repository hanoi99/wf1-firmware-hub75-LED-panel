#pragma once
#include <map>
#include <string>
#include "Arduino.h"
// In-memory stand-in for NVS (survives "reboots" inside the process).
class Preferences {
public:
    static std::map<std::string, std::string>& store() { static std::map<std::string, std::string> m; return m; }
    bool begin(const char*, bool) { return true; }
    int32_t getInt(const char* k, int32_t d) { auto it = store().find(k); return it == store().end() ? d : atoi(it->second.c_str()); }
    size_t putInt(const char* k, int32_t v) { store()[k] = std::to_string(v); return 4; }
    uint8_t getUChar(const char* k, uint8_t d) { return (uint8_t)getInt(k, d); }
    size_t putUChar(const char* k, uint8_t v) { return putInt(k, v); }
    uint16_t getUShort(const char* k, uint16_t d) { return (uint16_t)getInt(k, d); }
    size_t putUShort(const char* k, uint16_t v) { return putInt(k, v); }
    String getString(const char* k, const String& d = String()) { auto it = store().find(k); return it == store().end() ? d : String(it->second); }
    size_t putString(const char* k, const String& v) { store()[k] = v.c_str(); return v.length(); }
    bool remove(const char* k) { return store().erase(k) > 0; }
};
