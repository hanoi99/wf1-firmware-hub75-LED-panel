#include "DigitalRainScene.h"
#include "display/TextRenderer.h"

namespace {
const char GLYPHS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ=+*#<>/?%";
char randomGlyph() { return GLYPHS[fm::rnd(sizeof(GLYPHS) - 1)]; }
}

DigitalRainScene::DigitalRainScene(DisplayDriver& display) : BaseScene(display) {
    addSelect("color", "Farbe", "Grün|Blau|Rot|Gold|Weiß|Regenbogen", 0);
    addRange("speed", "Geschwindigkeit", 1, 10, 5);
    addRange("dens", "Dichte", 1, 10, 7);
}

void DigitalRainScene::respawn(Column& c, bool initial) {
    c.head = (int8_t)(initial ? -(int)fm::rnd(ROWS + 6) : -(int)fm::rnd(6));
    c.len = (uint8_t)(4 + fm::rnd(6));
    c.acc = (uint16_t)fm::rnd(100);
    c.active = (int)fm::rnd(10) < val(O_DENS);
    c.wait = (uint8_t)fm::rnd(20);
}

void DigitalRainScene::begin() {
    for (auto& c : col_) respawn(c, true);
    for (auto& row : glyph_) for (auto& g : row) g = randomGlyph();
}

void DigitalRainScene::render(uint32_t now) {
    d_.clear();
    const int speed = 20 + val(O_SPEED) * 16;   // cell steps per 100 frames: 36 .. 180
    for (auto& c : col_) {
        if (c.wait) { --c.wait; continue; }
        if (!c.active) { respawn(c, false); continue; }
        c.acc = (uint16_t)(c.acc + speed);
        while (c.acc >= 100) {
            c.acc -= 100;
            c.head++;
            if (c.head - c.len >= ROWS) { respawn(c, false); break; }
        }
    }
    for (int i = 0; i < 6; ++i) glyph_[fm::rnd(ROWS)][fm::rnd(COLS)] = randomGlyph();

    fm::RGB base;
    for (int cx = 0; cx < COLS; ++cx) {
        const Column& c = col_[cx];
        if (!c.active || c.wait) continue;
        switch (val(O_COLOR)) {
            case 1: base = {40, 140, 255}; break;
            case 2: base = {255, 50, 50}; break;
            case 3: base = {255, 190, 40}; break;
            case 4: base = {200, 200, 210}; break;
            case 5: base = fm::hsv((uint8_t)(cx * 16 + now / 20), 255, 255); break;
            default: base = {0, 255, 70}; break;
        }
        for (int t = 0; t < c.len; ++t) {
            const int row = c.head - t;
            if (row < 0 || row >= ROWS) continue;
            fm::RGB color;
            if (t == 0) color = fm::mix(base, {255, 255, 255}, 170);
            else color = fm::scale(base, (uint8_t)(255 - t * 200 / c.len));
            TextRenderer::drawChar(d_, TextRenderer::FONT_MINI, 1, (uint8_t)glyph_[row][cx],
                                   cx * 4, row * 6 + 2, fm::to565(color));
        }
    }
}
