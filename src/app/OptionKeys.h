#pragma once
#include <Arduino.h>

// NVS keys are limited to 15 characters: "<sceneId>.<option>".
inline bool makeOptionKey(const char* sceneId, const char* optKey, char* out, size_t outSize) {
    const int n = snprintf(out, outSize, "%s.%s", sceneId, optKey);
    return n > 0 && (size_t)n < outSize && n <= 15;
}
