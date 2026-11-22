#include "ManualTextScene.h"
#include "display/TextRenderer.h"

ManualTextScene::ManualTextScene(DisplayDriver& display) : BaseScene(display) {
    addSelect("font", "Schriftart", TextRenderer::FONT_CHOICES, 0);
    addRange("size", "Größe", 1, 3, 1);
    addSelect("dir", "Laufrichtung", "Links|Rechts|Hoch|Runter|Statisch|Pendeln", 0);
    addSelect("fx", "Effekt", "Normal|Blinken|Pulsieren|Regenbogen|Farbverlauf", 0);
    addRange("speed", "Geschwindigkeit", 1, 10, 4);
    addColor("color", "Textfarbe", 0x33CCFF);
    addColor("bg", "Hintergrund", 0x000000);
}

bool ManualTextScene::setText(const String& utf8) {
    if (utf8.length() > MAX_TEXT) return false;
    raw_ = utf8;
    dirty_ = true;
    return true;
}

void ManualTextScene::begin() { dirty_ = true; lastMs_ = 0; }

void ManualTextScene::rebuild() {
    dirty_ = false;
    const uint8_t font = (uint8_t)val(O_FONT), size = (uint8_t)val(O_SIZE);
    const int dir = val(O_DIR);
    const String prep = TextRenderer::prepare(raw_, font);
    lineH_ = TextRenderer::lineHeight(font, size);
    nLines_ = 0;
    if (dir == DIR_LEFT || dir == DIR_RIGHT || dir == DIR_PENDULUM) {
        line_ = prep;
        line_.replace('\n', ' ');
        lineW_ = TextRenderer::width(font, size, line_);
        totalH_ = lineH_;
    } else {
        nLines_ = TextRenderer::wrap(font, size, prep, DisplayDriver::W, lines_, MAX_LINES);
        totalH_ = nLines_ * lineH_;
    }
    switch (dir) {
        case DIR_LEFT: pos_ = DisplayDriver::W * 256; break;
        case DIR_RIGHT: pos_ = -lineW_ * 256; break;
        case DIR_UP: pos_ = DisplayDriver::H * 256; break;
        case DIR_DOWN: pos_ = -totalH_ * 256; break;
        case DIR_PENDULUM: pos_ = (lineW_ <= DisplayDriver::W ? 0 : (DisplayDriver::W - lineW_) * 256); pendDir_ = 1; break;
        default: break;
    }
}

uint16_t ManualTextScene::colorFor(int index, int x, uint32_t now) const {
    const fm::RGB base = fm::fromHex((uint32_t)val(O_COLOR));
    switch (val(O_FX)) {
        case 2: return fm::to565(fm::scale(base, (uint8_t)(70 + fm::sin8((uint8_t)(now / 5)) * 185 / 255)));
        case 3: return fm::to565(fm::hsv((uint8_t)(index * 22 + now / 12), 255, 255));
        case 4: return fm::to565(fm::hsv((uint8_t)(x * 4 + now / 25), 255, 255));
        default: return fm::to565(base);
    }
}

void ManualTextScene::drawLine(const String& s, int x, int yTop, uint32_t now, int firstIndex) {
    const uint8_t font = (uint8_t)val(O_FONT), size = (uint8_t)val(O_SIZE);
    for (size_t i = 0; i < s.length(); ++i) {
        const uint8_t c = (uint8_t)s[i];
        const int adv = TextRenderer::advance(font, size, c);
        if (x + adv > 0 && x < DisplayDriver::W)
            TextRenderer::drawChar(d_, font, size, c, x, yTop, colorFor(firstIndex + (int)i, x, now));
        x += adv;
        if (x >= DisplayDriver::W) break;
    }
}

void ManualTextScene::render(uint32_t now) {
    if (dirty_) rebuild();
    const uint32_t dt = lastMs_ == 0 ? 33 : min<uint32_t>(now - lastMs_, 100);
    lastMs_ = now;

    const uint32_t bg = (uint32_t)val(O_BG);
    if (bg == 0) d_.clear();
    else d_.matrix().fillScreenRGB888((uint8_t)(bg >> 16), (uint8_t)(bg >> 8), (uint8_t)bg);
    if (raw_.length() == 0) return;
    if (val(O_FX) == 1 && ((now / 500) & 1)) return;   // blink: off phase

    const int32_t step = (int32_t)((6 + val(O_SPEED) * 7) * dt * 256 / 1000);
    const int W = DisplayDriver::W, H = DisplayDriver::H;
    const int dir = val(O_DIR);

    if (dir == DIR_LEFT || dir == DIR_RIGHT || dir == DIR_PENDULUM) {
        const int yTop = (H - lineH_) / 2;
        if (dir == DIR_LEFT) {
            pos_ -= step;
            if ((pos_ >> 8) < -lineW_) pos_ = W * 256;
        } else if (dir == DIR_RIGHT) {
            pos_ += step;
            if ((pos_ >> 8) > W) pos_ = -lineW_ * 256;
        } else {
            const int32_t lo = (lineW_ <= W ? 0 : (W - lineW_) * 256);
            const int32_t hi = (lineW_ <= W ? (W - lineW_) * 256 : 0);
            pos_ += pendDir_ * step;
            if (pos_ >= hi) { pos_ = hi; pendDir_ = -1; }
            if (pos_ <= lo) { pos_ = lo; pendDir_ = 1; }
        }
        drawLine(line_, pos_ >> 8, yTop, now, 0);
        return;
    }

    int y0;
    if (dir == DIR_UP) { pos_ -= step; if ((pos_ >> 8) < -totalH_) pos_ = H * 256; y0 = pos_ >> 8; }
    else if (dir == DIR_DOWN) { pos_ += step; if ((pos_ >> 8) > H) pos_ = -totalH_ * 256; y0 = pos_ >> 8; }
    else y0 = totalH_ <= H ? (H - totalH_) / 2 : 0;

    const uint8_t font = (uint8_t)val(O_FONT), size = (uint8_t)val(O_SIZE);
    int index = 0;
    for (uint8_t l = 0; l < nLines_; ++l) {
        const int y = y0 + l * lineH_;
        if (y + lineH_ > 0 && y < H) {
            const int w = TextRenderer::width(font, size, lines_[l]);
            drawLine(lines_[l], (W - w) / 2, y, now, index);
        }
        index += lines_[l].length();
    }
}
