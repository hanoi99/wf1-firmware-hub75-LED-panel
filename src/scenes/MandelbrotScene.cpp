#include "MandelbrotScene.h"
#include <math.h>

namespace {
struct Target { double x, y; };
const Target TARGETS[] = {
    {-0.743643887037151, 0.131825904205330},    // seahorse valley
    {-0.7746806106269039, -0.1374168856037867}, // spiral
    {0.2549870375144766, 0.0005679790528465},   // elephant valley
};
constexpr int N_TARGETS = sizeof(TARGETS) / sizeof(TARGETS[0]);
constexpr double HOME_X = -0.5, HOME_Y = 0.0, HOME_WIDTH = 3.0;
constexpr double MIN_SCALE = 7e-6;               // ~ 2e-5 view width, limit of Q28 precision
constexpr int FRAC = 28;
inline int32_t toQ(double v) { return (int32_t)llround(v * (double)(1 << FRAC)); }
}

MandelbrotScene::MandelbrotScene(DisplayDriver& display)
    : BaseScene(display), palette_(scratch::block().palette) {
    addSelect("pal", "Farbpalette", "Feuer|Eis|Regenbogen|Psycho", 2);
    addRange("speed", "Zoom-Tempo", 1, 10, 5);
    addRange("cycle", "Farbwechsel", 0, 10, 4);
    buildPalette();
}

void MandelbrotScene::buildPalette() {
    static const uint8_t KIND[] = {1, 4, 3, 0};
    pal::smooth(palette_, KIND[constrain((int)val(O_PAL), 0, 3)]);
}

void MandelbrotScene::begin() {
    scratch::claim(this);
    scale_ = 1.0; zoomDir_ = -1; holdFrames_ = 0;
    buildPalette();
}

void MandelbrotScene::nextTarget() { target_ = (uint8_t)((target_ + 1) % N_TARGETS); }

void MandelbrotScene::render(uint32_t now) {
    // advance zoom: exponential in, exponential out, short hold at the extremes
    if (holdFrames_ > 0) --holdFrames_;
    else {
        const double f = 1.0 - (0.010 + 0.004 * val(O_SPEED));
        if (zoomDir_ < 0) { scale_ *= f; if (scale_ <= MIN_SCALE) { scale_ = MIN_SCALE; zoomDir_ = 1; holdFrames_ = 12; } }
        else { scale_ /= f * f; if (scale_ >= 1.0) { scale_ = 1.0; zoomDir_ = -1; holdFrames_ = 10; nextTarget(); } }
    }
    const Target& tg = TARGETS[target_];
    // centre moves from the home view onto the target as we zoom (quadratic keeps the target on screen)
    const double k = scale_ * scale_;
    const int32_t cx = toQ(tg.x + (HOME_X - tg.x) * k);
    const int32_t cy = toQ(tg.y + (HOME_Y - tg.y) * k);
    const int32_t step = (int32_t)llround((HOME_WIDTH / 64.0) * (double)(1 << FRAC) * scale_);
    const int maxIter = (int)min(170.0, 36.0 + 14.0 * log2(1.0 / scale_));
    const uint8_t shift = (uint8_t)(now * (uint32_t)val(O_CYCLE) / 40);
    const int64_t FOUR = (int64_t)4 << FRAC;

    for (int py = 0; py < 64; ++py) {
        const int32_t ci = cy + (py - 32) * step;
        const int64_t ci2 = ((int64_t)ci * ci) >> FRAC;
        for (int px = 0; px < 64; ++px) {
            const int32_t cr = cx + (px - 32) * step;
            // skip the main cardioid and the period-2 bulb (always inside)
            const int64_t xm = (int64_t)cr - (1 << (FRAC - 2));
            const int64_t q = ((xm * xm) >> FRAC) + ci2;
            bool inside = ((q * (q + xm)) >> FRAC) < (ci2 >> 2);
            if (!inside) {
                const int64_t xp = (int64_t)cr + (1 << FRAC);
                inside = (((xp * xp) >> FRAC) + ci2) < ((int64_t)1 << (FRAC - 4));
            }
            int it = 0;
            if (!inside) {
                int32_t zr = 0, zi = 0;
                for (; it < maxIter; ++it) {
                    const int64_t zr2 = ((int64_t)zr * zr) >> FRAC, zi2 = ((int64_t)zi * zi) >> FRAC;
                    if (zr2 + zi2 > FOUR) break;
                    zi = (int32_t)((((int64_t)zr * zi) >> (FRAC - 1)) + ci);
                    zr = (int32_t)(zr2 - zi2 + cr);
                }
            } else it = maxIter;
            if (it >= maxIter) d_.px(px, py, 0, 0, 0);
            else { const fm::RGB c = palette_[(uint8_t)(it * 7 + shift)]; d_.px(px, py, c.r, c.g, c.b); }
        }
    }
}
