#include "AsciiScene.h"
#include <math.h>
#include "display/TextRenderer.h"

namespace { const char RAMP[] = " .:-=+*#%@"; }   // 10 levels

AsciiScene::AsciiScene(DisplayDriver& display) : BaseScene(display) {
    addSelect("mode", "Effekt", "Plasma|Feuer|Tunnel", 0);
    addSelect("color", "Farbe", "Terminal|Bernstein|Cyan|Regenbogen", 0);
    addRange("speed", "Geschwindigkeit", 1, 10, 5);
    for (int y = 0; y < ROWS; ++y)
        for (int x = 0; x < COLS; ++x) {
            const float dx = (x - (COLS - 1) / 2.0f) * 1.0f, dy = (y - (ROWS - 1) / 2.0f) * 1.5f;  // cells are 4x6 px
            dist_[y][x] = (uint8_t)min(255.0f, sqrtf(dx * dx + dy * dy) * 22.0f);
            ang_[y][x] = (uint8_t)((atan2f(dy, dx) + 3.14159f) * 255.0f / 6.28318f);
        }
}

void AsciiScene::begin() { memset(heat_, 0, sizeof(heat_)); }

void AsciiScene::fieldPlasma(uint32_t t) {
    for (int y = 0; y < ROWS; ++y)
        for (int x = 0; x < COLS; ++x) {
            const int v = (fm::sin8((uint8_t)(x * 22 + t)) + fm::sin8((uint8_t)(y * 30 + t * 4 / 3)) +
                           fm::sin8((uint8_t)((x + y) * 16 + t / 2)) + fm::sin8((uint8_t)(dist_[y][x] + t))) / 4;
            value_[y][x] = (uint8_t)constrain((v - 70) * 3 / 2, 0, 255);
        }
}

void AsciiScene::fieldTunnel(uint32_t t) {
    for (int y = 0; y < ROWS; ++y)
        for (int x = 0; x < COLS; ++x) {
            const int a = fm::sin8((uint8_t)(dist_[y][x] * 2 - t * 2));
            const int b = fm::sin8((uint8_t)(ang_[y][x] * 4 + t));
            value_[y][x] = (uint8_t)constrain(((a + b) / 2 - 40) * 2, 0, 255);
        }
}

void AsciiScene::fieldFire() {
    for (int x = 0; x < COLS; ++x) heat_[FIRE_ROWS - 1][x] = (uint8_t)(170 + fm::rnd(86));
    for (int y = 0; y < FIRE_ROWS - 1; ++y)
        for (int x = 0; x < COLS; ++x) {
            const int sum = heat_[y + 1][max(0, x - 1)] + heat_[y + 1][x] + heat_[y + 1][min(COLS - 1, x + 1)] +
                            heat_[min(FIRE_ROWS - 1, y + 2)][x];
            const int v = sum / 4 - (int)fm::rnd(46);
            heat_[y][x] = (uint8_t)(v > 0 ? v : 0);
        }
    for (int y = 0; y < ROWS; ++y)
        for (int x = 0; x < COLS; ++x) value_[y][x] = heat_[y + FIRE_ROWS - ROWS][x];
}

void AsciiScene::render(uint32_t now) {
    d_.clear();
    const uint32_t t = now * (uint32_t)val(O_SPEED) / 30;
    switch (val(O_MODE)) {
        case 1: fieldFire(); break;
        case 2: fieldTunnel(t); break;
        default: fieldPlasma(t); break;
    }
    for (int y = 0; y < ROWS; ++y)
        for (int x = 0; x < COLS; ++x) {
            const int v = value_[y][x];
            const int level = v * 9 / 255;
            if (level == 0) continue;
            fm::RGB c;
            switch (val(O_COLOR)) {
                case 1: c = {(uint8_t)(120 + v / 2), (uint8_t)(50 + v / 3), 0}; break;
                case 2: c = {(uint8_t)(v / 5), (uint8_t)(90 + v / 2), (uint8_t)(150 + v / 2)}; break;
                case 3: c = fm::hsv((uint8_t)(x * 10 + y * 8 + now / 25), 255, (uint8_t)(110 + v / 2)); break;
                default: c = {(uint8_t)(level > 7 ? 150 : 0), (uint8_t)(70 + v * 185 / 255), (uint8_t)(level > 7 ? 120 : 30)}; break;
            }
            TextRenderer::drawChar(d_, TextRenderer::FONT_MINI, 1, (uint8_t)RAMP[level], x * 4, y * 6 + 2, fm::to565(c));
        }
}
