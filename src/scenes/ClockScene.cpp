#include "ClockScene.h"
#include "display/TextRenderer.h"

namespace {
const char* const WEEKDAY[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char* const MONTH[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void text(DisplayDriver& d, uint8_t size, const char* s, int x, int y, fm::RGB c) {
    TextRenderer::drawString(d, TextRenderer::FONT_STD, size, String(s), x, y, fm::to565(c));
}
}

ClockScene::ClockScene(DisplayDriver& display, TimeService& time) : BaseScene(display), time_(time) {
    addSelect("style", "Stil", "Digital|Analog", 0);
    addToggle("h24", "24-Stunden-Format", true);
    addToggle("secs", "Sekunden anzeigen", true);
    addColor("color", "Farbe", 0x50DCFF);
}

void ClockScene::render(uint32_t now) {
    d_.clear();
    struct tm t{};
    if (!time_.getLocalTime(t)) {
        text(d_, 1, "Warte auf", 7, 24, {255, 120, 80});
        text(d_, 1, "Zeit (NTP)", 4, 34, {255, 120, 80});
        return;
    }
    if (val(O_STYLE) == 1) analog(t); else digital(t, now);
}

void ClockScene::digital(const struct tm& t, uint32_t now) {
    const fm::RGB c = fm::fromHex((uint32_t)val(O_COLOR));
    int hour = t.tm_hour;
    if (!val(O_H24)) { hour %= 12; if (hour == 0) hour = 12; }
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d", hour);
    text(d_, 2, buf, 3, 10, c);
    snprintf(buf, sizeof(buf), "%02d", t.tm_min);
    text(d_, 2, buf, 37, 10, c);
    const bool colon = !val(O_SECS) || (t.tm_sec & 1) == 0 || (now % 1000) < 500;
    if (colon) { d_.px(31, 15, c.r, c.g, c.b); d_.px(32, 15, c.r, c.g, c.b);
                 d_.px(31, 21, c.r, c.g, c.b); d_.px(32, 21, c.r, c.g, c.b); }
    if (!val(O_H24)) text(d_, 1, t.tm_hour >= 12 ? "PM" : "AM", 26, 26, fm::scale(c, 140));

    if (val(O_SECS)) {   // seconds bar
        const int len = (t.tm_sec * 60) / 59;
        for (int x = 0; x < 60; ++x) {
            const fm::RGB b = x <= len ? fm::scale(c, 200) : fm::scale(c, 28);
            d_.px(2 + x, 37, b.r, b.g, b.b);
            d_.px(2 + x, 38, b.r, b.g, b.b);
        }
    }
    char date[32];
    snprintf(date, sizeof(date), "%s %d %s", WEEKDAY[t.tm_wday], t.tm_mday, MONTH[t.tm_mon]);
    text(d_, 1, date, (64 - (6 * (int)strlen(date) - 1)) / 2, 45, {190, 205, 220});  // zentriert
    snprintf(date, sizeof(date), "%04d", t.tm_year + 1900);
    text(d_, 1, date, 20, 55, {120, 135, 150});
}

void ClockScene::analog(const struct tm& t) {
    auto& m = d_.matrix();
    const fm::RGB c = fm::fromHex((uint32_t)val(O_COLOR));
    const int cx = 32, cy = 32;
    for (int i = 0; i < 60; ++i) {               // ticks
        const uint8_t a = (uint8_t)(i * 256 / 60);
        const bool hour = i % 5 == 0;
        if (!hour && i % 15 != 0 && false) continue;
        const int r0 = hour ? 25 : 29, r1 = 30;
        for (int r = r0; r <= r1; ++r) {
            const int x = cx + (r * fm::isin(a) + 64) / 127, y = cy - (r * fm::icos(a) + 64) / 127;
            const fm::RGB k = hour ? fm::RGB{200, 200, 210} : fm::RGB{50, 55, 70};
            d_.px(x, y, k.r, k.g, k.b);
        }
    }
    auto hand = [&](uint8_t a, int len, fm::RGB col) {
        const int ex = cx + (len * fm::isin(a)) / 127, ey = cy - (len * fm::icos(a)) / 127;
        m.drawLine(cx, cy, ex, ey, fm::to565(col));
    };
    const int minutes = t.tm_min * 60 + t.tm_sec;
    hand((uint8_t)((((t.tm_hour % 12) * 3600 + minutes) * 256L) / 43200), 15, c);
    hand((uint8_t)((minutes * 256L) / 3600), 23, c);
    if (val(O_SECS)) hand((uint8_t)((t.tm_sec * 256) / 60), 27, {255, 60, 60});
    d_.px(cx, cy, 255, 255, 255);
}
