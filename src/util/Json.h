#pragma once
#include <Arduino.h>

// Escapes a string for use inside a JSON string literal.
inline String jsonEscape(const String& input) {
    String out;
    out.reserve(input.length() + 8);
    for (size_t i = 0; i < input.length(); ++i) {
        const uint8_t c = static_cast<uint8_t>(input[i]);
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char e[7];
                    snprintf(e, sizeof(e), "\\u%04X", c);
                    out += e;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}
