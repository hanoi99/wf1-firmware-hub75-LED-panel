#include "WorldClockScene.h"
#include "board/BoardConfig.h"
#include "display/TextRenderer.h"

WorldClockScene::WorldClockScene(DisplayDriver& display, TimeService& time) : BaseScene(display), time_(time) {
    addSelect("zone", "Hauptzeit", "Lokalzeit|UTC", 0);
    addToggle("secs", "Sekundenbalken", true);
    addToggle("night", "Tag/Nacht-Schattierung", true);
    addToggle("home", "Heimat-Marker", true);
}

void WorldClockScene::render(uint32_t now) {
    const time_t utc = time_.utcNow();
    if (lastShadeMs_ == 0 || now - lastShadeMs_ >= 30000UL) {   // sun moves ~0.25 deg/min
        map_.setTime(utc);
        lastShadeMs_ = now ? now : 1;
    }
    d_.clear();
    map_.draw(d_, val(O_NIGHT), true, true);
    if (val(O_HOME) && ((now / 500) & 1)) {
        int x, y;
        if (MapView::project(BoardConfig::HOME_LAT, BoardConfig::HOME_LON, x, y)) d_.px(x, y, 255, 40, 40);
    }

    struct tm local{}, gm{};
    if (utc == 0 || !time_.getLocalTime(local)) {
        TextRenderer::drawString(d_, 0, 1, "Warte auf Zeit", 5, 40, fm::to565({255, 120, 80}));
        return;
    }
    gmtime_r(&utc, &gm);
    const bool showUtc = val(O_ZONE) == 1;
    const struct tm& main = showUtc ? gm : local;
    const struct tm& sub = showUtc ? local : gm;

    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d", main.tm_hour, main.tm_min);
    const fm::RGB c = showUtc ? fm::RGB{255, 210, 120} : fm::RGB{90, 220, 255};
    TextRenderer::drawString(d_, 0, 2, buf, 2, 30, fm::to565(c));
    if (val(O_SECS)) {
        const int len = (main.tm_sec * 60) / 59;
        for (int x = 0; x < 60; ++x) {
            const fm::RGB b = x <= len ? fm::scale(c, 180) : fm::scale(c, 24);
            d_.px(2 + x, 46, b.r, b.g, b.b);
        }
    }
    snprintf(buf, sizeof(buf), "%02d.%02d.%04d", main.tm_mday, main.tm_mon + 1, main.tm_year + 1900);
    TextRenderer::drawString(d_, 0, 1, buf, 2, 49, fm::to565({190, 205, 220}));
    snprintf(buf, sizeof(buf), "%s %02d:%02d", showUtc ? "LOK" : "UTC", sub.tm_hour, sub.tm_min);
    TextRenderer::drawString(d_, 0, 1, buf, 2, 57, fm::to565({110, 125, 145}));
}
