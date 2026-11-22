#pragma once
#include <Arduino.h>
class File { public: operator bool() const { return true; } void close() {} };
class LittleFSFS {
public:
    bool begin(bool formatOnFail = false, const char* base = "/littlefs", uint8_t max = 10, const char* label = "spiffs") { return true; }
    File open(const char* path, const char* mode = "r", bool create = false) { return File(); }
    File open(const String& path, const char* mode = "r", bool create = false) { return File(); }
    bool exists(const char* path) { return true; }
    size_t usedBytes() { return 0; }
    size_t totalBytes() { return 0; }
};
extern LittleFSFS LittleFS;
