#include "StarfieldScene.h"

StarfieldScene::StarfieldScene(DisplayDriver& display) : BaseScene(display) {
    addRange("dens", "Sternanzahl", 1, 10, 6);
    addRange("twinkle", "Funkeln", 1, 10, 5);
    addToggle("mw", "Milchstraße", true);
    addToggle("shoot", "Sternschnuppen", true);
    milky_ = scratch::block().bytes;
}

void StarfieldScene::generate() {
    for (auto& s : stars_) {
        s.x = (uint8_t)fm::rnd(64); s.y = (uint8_t)fm::rnd(64);
        s.base = (uint8_t)(90 + fm::rnd(166));
        s.phase = (uint8_t)fm::rnd(256); s.rate = (uint8_t)(2 + fm::rnd(8));
        const uint32_t r = fm::rnd(10);
        s.tint = r < 6 ? 0 : (r < 8 ? 1 : (r < 9 ? 2 : 3));   // white, blue, yellow, red
    }
    // diagonal band with soft noise
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) {
            const int dist = abs(x + y - 70);
            const int band = max(0, 26 - dist);
            const int noise = (fm::sin8((uint8_t)(x * 9 + y * 5)) + fm::sin8((uint8_t)(x * 3 - y * 11 + 40)) + 255) / 3;
            milky_[y * 64 + x] = (uint8_t)(band * noise / 255 * 2);
        }
}

void StarfieldScene::begin() { scratch::claim(this); generate(); shooter_.life = 0; }

void StarfieldScene::render(uint32_t now) {
    d_.clear();
    if (val(O_MW)) {
        for (int i = 0; i < 64 * 64; ++i) {
            const int v = milky_[i];
            if (v < 3) continue;
            d_.px(i & 63, i >> 6, (uint8_t)(v * 6 / 10), (uint8_t)(v * 5 / 10), (uint8_t)min(255, v * 11 / 10));
        }
    }
    const int count = 20 + val(O_DENS) * 11;
    const int depth = val(O_TWINKLE) * 22;           // twinkle amplitude 22..220
    for (int i = 0; i < count && i < MAX_STARS; ++i) {
        const Star& s = stars_[i];
        const int wave = fm::sin8((uint8_t)(now * s.rate / 40 + s.phase));   // 0..255
        const int b = s.base * (255 - depth + depth * wave / 255) / 255;
        uint8_t r = (uint8_t)b, g = (uint8_t)b, bl = (uint8_t)b;
        switch (s.tint) {
            case 1: r = (uint8_t)(b * 6 / 10); g = (uint8_t)(b * 8 / 10); break;
            case 2: bl = (uint8_t)(b * 5 / 10); break;
            case 3: g = (uint8_t)(b * 5 / 10); bl = (uint8_t)(b * 4 / 10); break;
        }
        d_.px(s.x, s.y, r, g, bl);
    }
    if (val(O_SHOOT)) {
        if (shooter_.life == 0 && fm::rnd(1000) < 8) {
            shooter_.x = (int16_t)(fm::rnd(40) * 256); shooter_.y = (int16_t)(fm::rnd(24) * 256);
            shooter_.vx = (int16_t)(500 + fm::rnd(300)); shooter_.vy = (int16_t)(220 + fm::rnd(200));
            shooter_.life = (uint8_t)(18 + fm::rnd(10));
        }
        if (shooter_.life) {
            --shooter_.life;
            shooter_.x += shooter_.vx; shooter_.y += shooter_.vy;
            for (int k = 0; k < 7; ++k) {
                const int px = (shooter_.x - shooter_.vx * k / 2) >> 8, py = (shooter_.y - shooter_.vy * k / 2) >> 8;
                const int v = 255 - k * 36;
                d_.px(px, py, (uint8_t)v, (uint8_t)v, (uint8_t)min(255, v + 20));
            }
        }
    }
}
