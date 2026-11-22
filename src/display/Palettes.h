#pragma once
#include "FastMath.h"

// 256-entry palette builders (RAM, 768 bytes each).
namespace pal {

inline fm::RGB lerp3(fm::RGB a, fm::RGB b, int i, int span) {
    return fm::mix(a, b, (uint8_t)(i * 255 / span));
}

// black -> dark color -> bright color -> white. kind: 0 fire, 1 blue, 2 green, 3 violet
inline void heat(fm::RGB* p, int kind) {
    static const fm::RGB DARK[4] = {{200, 20, 0}, {0, 30, 200}, {0, 150, 10}, {140, 0, 200}};
    static const fm::RGB BRIGHT[4] = {{255, 170, 0}, {0, 180, 255}, {120, 255, 40}, {255, 60, 255}};
    const fm::RGB k{0, 0, 0}, w{255, 255, 255};
    const fm::RGB d = DARK[kind & 3], b = BRIGHT[kind & 3];
    for (int i = 0; i < 256; ++i) {
        if (i < 85) p[i] = lerp3(k, d, i, 84);
        else if (i < 170) p[i] = lerp3(d, b, i - 85, 84);
        else p[i] = lerp3(b, w, i - 170, 85);
    }
}

// cosine-style smooth palettes. kind: 0 nebula, 1 lava, 2 ocean, 3 rainbow, 4 ice
inline void smooth(fm::RGB* p, int kind) {
    for (int i = 0; i < 256; ++i) {
        const uint8_t a = (uint8_t)i;
        int r, g, b;
        switch (kind) {
            case 0:  // deep blue / purple / magenta / cyan sparks
                r = 70 + fm::isin((uint8_t)(a + 40)) * 70 / 127 + (a > 200 ? (a - 200) * 3 : 0);
                g = 10 + (fm::isin((uint8_t)(a * 2)) + 127) / 8 + (a > 220 ? (a - 220) * 5 : 0);
                b = 110 + fm::isin(a) * 100 / 127;
                break;
            case 1:  // lava
                r = 128 + fm::isin((uint8_t)(a - 64)) * 127 / 127;
                g = (fm::isin((uint8_t)(a * 2 - 90)) + 127) * 90 / 254;
                b = 10;
                break;
            case 2:  // ocean
                r = 10;
                g = 90 + fm::isin((uint8_t)(a + 30)) * 80 / 127;
                b = 140 + fm::isin(a) * 110 / 127;
                break;
            case 4:  // ice
                r = 100 + fm::isin((uint8_t)(a + 90)) * 100 / 127;
                g = 150 + fm::isin((uint8_t)(a + 40)) * 90 / 127;
                b = 230 + fm::isin(a) * 25 / 127;
                break;
            default: {
                const fm::RGB c = fm::hsv(a, 255, 255);
                r = c.r; g = c.g; b = c.b;
            }
        }
        p[i] = {(uint8_t)constrain(r, 0, 255), (uint8_t)constrain(g, 0, 255), (uint8_t)constrain(b, 0, 255)};
    }
}

}  // namespace pal
