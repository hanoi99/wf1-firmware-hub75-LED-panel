#pragma once
// Integer-only helpers. The ESP32-S2 has no FPU, so per-pixel code uses
// lookup tables and fixed-point math instead of float.
#include <Arduino.h>
#include <math.h>

namespace fm {

struct RGB { uint8_t r, g, b; };

inline uint32_t& rngState() { static uint32_t s = 2463534242u; return s; }
inline void seed(uint32_t v) { rngState() = v ? v : 2463534242u; }
inline uint32_t rnd() {  // xorshift32
    uint32_t x = rngState();
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return rngState() = x;
}
inline uint32_t rnd(uint32_t n) { return n ? rnd() % n : 0; }
inline int32_t rndRange(int32_t lo, int32_t hi) { return lo + (int32_t)rnd((uint32_t)(hi - lo)); }

struct SinTab {
    int8_t v[256];
    SinTab() { for (int i = 0; i < 256; ++i) v[i] = (int8_t)lroundf(127.0f * sinf(i * 6.2831853f / 256.0f)); }
};
inline const SinTab& sinTab() { static SinTab t; return t; }

// angle 0..255 = full circle; result -127..127
inline int isin(uint8_t a) { return sinTab().v[a]; }
inline int icos(uint8_t a) { return sinTab().v[(uint8_t)(a + 64)]; }
// 0..255
inline uint8_t sin8(uint8_t a) { return (uint8_t)(128 + sinTab().v[a]); }

inline uint8_t scale8(uint8_t v, uint8_t s) { return (uint8_t)(((uint16_t)v * (uint16_t)(s + 1)) >> 8); }

inline RGB hsv(uint8_t h, uint8_t s, uint8_t v) {
    const uint8_t region = h / 43;
    const uint8_t rem = (uint8_t)((h - region * 43) * 6);
    const uint8_t p = (uint8_t)((v * (255 - s)) >> 8);
    const uint8_t q = (uint8_t)((v * (255 - ((s * rem) >> 8))) >> 8);
    const uint8_t t = (uint8_t)((v * (255 - ((s * (255 - rem)) >> 8))) >> 8);
    switch (region) {
        case 0: return {v, t, p};
        case 1: return {q, v, p};
        case 2: return {p, v, t};
        case 3: return {p, q, v};
        case 4: return {t, p, v};
        default: return {v, p, q};
    }
}

inline RGB mix(RGB a, RGB b, uint8_t t) {
    return {(uint8_t)(a.r + (((int)b.r - a.r) * t >> 8)),
            (uint8_t)(a.g + (((int)b.g - a.g) * t >> 8)),
            (uint8_t)(a.b + (((int)b.b - a.b) * t >> 8))};
}
inline RGB scale(RGB c, uint8_t s) { return {scale8(c.r, s), scale8(c.g, s), scale8(c.b, s)}; }
inline RGB fromHex(uint32_t c) { return {(uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c}; }
inline uint16_t to565(RGB c) { return (uint16_t)(((c.r & 0xF8) << 8) | ((c.g & 0xFC) << 3) | (c.b >> 3)); }

inline uint32_t isqrt(uint32_t n) {
    uint32_t r = 0, bit = 1u << 30;
    while (bit > n) bit >>= 2;
    while (bit) {
        if (n >= r + bit) { n -= r + bit; r = (r >> 1) + bit; } else r >>= 1;
        bit >>= 2;
    }
    return r;
}

}  // namespace fm
