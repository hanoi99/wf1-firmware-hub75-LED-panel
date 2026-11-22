#include "IssTrackerScene.h"
#include "display/TextRenderer.h"

IssTrackerScene::IssTrackerScene(DisplayDriver& display, IssService& iss, TimeService& time)
    : BaseScene(display), iss_(iss), time_(time) {
    addToggle("track", "Bahn anzeigen", true);
    addToggle("night", "Tag/Nacht-Schattierung", true);
    addToggle("info", "Koordinaten anzeigen", true);
}

void IssTrackerScene::begin() {
    iss_.setWanted(true);
    lastShadeMs_ = trackBuiltMs_ = 0;
    trackValid_ = false;
}

void IssTrackerScene::end() { iss_.setWanted(false); }

String IssTrackerScene::info() const {
    if (!haveFix_) return "Warte auf Daten";
    char buf[48];
    snprintf(buf, sizeof(buf), "%.1f / %.1f", lat_, lon_);
    return String(buf);
}

void IssTrackerScene::rebuildTrack(const IssOrbit& orbit, int32_t ageSec) {
    int n = 0;
    for (int i = -PAST_PTS; i < FUTURE_PTS; ++i, ++n) {
        float la, lo;
        orbit.positionAt(ageSec + i * TRACK_STEP_S, la, lo);
        int x, y;
        if (MapView::project(la, lo, x, y)) { tx_[n] = (uint8_t)x; ty_[n] = (uint8_t)y; }
        else { tx_[n] = 255; ty_[n] = 255; }
    }
    trackValid_ = true;
}

void IssTrackerScene::render(uint32_t now) {
    if (lastShadeMs_ == 0 || now - lastShadeMs_ >= 30000UL) {
        map_.setTime(time_.utcNow());
        lastShadeMs_ = now ? now : 1;
    }
    d_.clear();
    map_.draw(d_, val(O_NIGHT), true, false);

    IssOrbit orbit;
    haveFix_ = iss_.snapshot(orbit);
    const char* status = "WARTE";
    if (haveFix_) {
        const int32_t age = (int32_t)(now - orbit.fixMs()) / 1000;
        orbit.positionAt(age, lat_, lon_);
        status = age < 60 ? "LIVE" : (age < 900 ? "SCHAETZ" : "ALT");
        if (orbit.ready() && val(O_TRACK) &&
            (!trackValid_ || trackFixMs_ != orbit.fixMs() || now - trackBuiltMs_ > 20000UL)) {
            rebuildTrack(orbit, age);   // centred on "now", i.e. the fix age
            trackFixMs_ = orbit.fixMs();
            trackBuiltMs_ = now;
        }
        if (orbit.ready() && val(O_TRACK) && trackValid_) {
            for (int i = 0; i < PAST_PTS + FUTURE_PTS; ++i) {
                if (tx_[i] == 255) continue;
                const bool future = i >= PAST_PTS;
                if ((i & 1) && !future) continue;
                if (future) d_.px(tx_[i], ty_[i], 0, 210, 230);
                else d_.px(tx_[i], ty_[i], 0, 90, 110);
            }
        }
        int x, y;
        if (MapView::project(lat_, lon_, x, y)) {
            const bool on = (now / 400) & 1;
            for (int dx = -1; dx <= 1; ++dx)
                for (int dy = -1; dy <= 1; ++dy) d_.px(x + dx, y + dy, on ? 255 : 160, on ? 40 : 20, on ? 40 : 20);
            d_.px(x, y, 255, 255, 255);
        }
    }

    // text block below the map
    const int y0 = MapView::H + 2;
    TextRenderer::drawString(d_, 0, 1, "ISS", 2, y0, fm::to565({255, 90, 90}));
    const uint16_t statusColor = fm::to565(haveFix_ && status[0] == 'L' ? fm::RGB{80, 255, 120} : fm::RGB{255, 200, 80});
    TextRenderer::drawString(d_, 0, 1, status, 62 - 6 * (int)strlen(status), y0, statusColor);
    if (!haveFix_) {
        TextRenderer::drawString(d_, 0, 1, "Hole Position", 4, y0 + 14, fm::to565({170, 180, 190}));
        TextRenderer::drawString(d_, 0, 1, "WLAN noetig", 6, y0 + 24, fm::to565({120, 130, 140}));
        return;
    }
    if (!val(O_INFO)) return;
    char buf[24];
    snprintf(buf, sizeof(buf), "LAT %5.1f%c", fabsf(lat_), lat_ >= 0 ? 'N' : 'S');
    TextRenderer::drawString(d_, 0, 1, buf, 2, y0 + 9, fm::to565({200, 215, 230}));
    snprintf(buf, sizeof(buf), "LON %5.1f%c", fabsf(lon_), lon_ >= 0 ? 'E' : 'W');
    TextRenderer::drawString(d_, 0, 1, buf, 2, y0 + 18, fm::to565({200, 215, 230}));
    const String over = TextRenderer::prepare(MapView::landAt(lat_, lon_) ? "Über Land" : "Über Meer", 0);
    TextRenderer::drawString(d_, 0, 1, over, 2, y0 + 27, fm::to565({120, 135, 150}));
}
