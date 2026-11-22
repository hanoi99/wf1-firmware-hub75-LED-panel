#include "DriveScene.h"
#include "Poly.h"

namespace {
constexpr int VPX = 46, VPY = 22;      // vanishing point (road runs diagonally from top right to bottom left)
constexpr int K = 2400;                // perspective constant: screen row = VPY + K / depth
constexpr int LAMP_SPACING = 420, LAMP_COUNT = 6, LAMP_NEAR = 70;

inline int roadCenter(int y) { return VPX - ((y - VPY) * 11) / 16; }   // x of the road centre at row y
inline int roadHalf(int y) { return ((y - VPY) * 15) / 16; }

struct Lamp { int x, y; bool visible; };
}

DriveScene::DriveScene(DisplayDriver& display) : BaseScene(display) {
    addSelect("view", "Ansicht", "Seitenansicht|Frontansicht", 0);
    addRange("speed", "Tempo", 1, 5, 3);
    addToggle("lamps", "Straßenlaternen", true);
}

void DriveScene::begin() {
    scroll_ = 0;
    for (auto& s : stars_) { s.x = (uint8_t)fm::rnd(64); s.y = (uint8_t)fm::rnd(16); s.phase = (uint8_t)fm::rnd(256); }
}

void DriveScene::drawCar(int ox, int oy, int spin) {
    // Classic mid-size sedan seen from front-left: low wide hood, raked windshield, big
    // twin headlights with a wide grille, bumper with lower intake and plate (no brand emblem).
    static const fm::RGB BODY{30, 104, 64}, SIDE{22, 78, 50}, HOOD{44, 134, 84}, ROOF{50, 146, 92},
        GLASS{22, 32, 42}, GLINT{80, 108, 126}, BUMPER{24, 70, 46}, GRILLE{14, 16, 18}, CHROME{170, 180, 180}, TIRE{22, 22, 26};
    auto P = [&](const int (&pts)[4][2], fm::RGB c) { poly::fill(d_, pts, 4, c, ox, oy); };
    // side body + rear quarter
    { const int q[4][2] = {{7, 10}, {19, 10}, {16, 15}, {2, 15}}; P(q, SIDE); }
    { const int q[4][2] = {{2, 15}, {16, 15}, {16, 26}, {2, 25}}; P(q, SIDE); }
    // side window and roof
    { const int q[4][2] = {{19, 10}, {22, 3}, {10, 4}, {7, 10}}; P(q, GLASS); }
    { const int q[4][2] = {{19, 9}, {21, 4}, {16, 4}, {14, 9}}; P(q, GLINT); }
    { const int q[4][2] = {{22, 3}, {10, 4}, {13, 1}, {25, 0}}; P(q, ROOF); }
    { const int q[4][2] = {{25, 0}, {36, 0}, {38, 2}, {22, 3}}; P(q, ROOF); }
    // windshield (raked) and hood
    { const int q[4][2] = {{19, 10}, {41, 9}, {38, 2}, {22, 3}}; P(q, GLASS); }
    { const int q[4][2] = {{22, 9}, {30, 9}, {28, 4}, {24, 4}}; P(q, GLINT); }
    { const int q[4][2] = {{16, 15}, {44, 13}, {41, 9}, {19, 10}}; P(q, HOOD); }
    // front face
    { const int q[4][2] = {{16, 15}, {44, 13}, {44, 24}, {16, 26}}; P(q, BODY); }
    // grille with chrome frame, flanked by large headlights
    { const int q[4][2] = {{25, 16}, {35, 15}, {35, 19}, {25, 20}}; P(q, GRILLE); }
    for (int x = 25; x <= 35; ++x) d_.px(ox + x, oy + 15 + (35 - x) * 1 / 10 + (x < 30 ? 1 : 0), CHROME.r, CHROME.g, CHROME.b);
    { const int q[4][2] = {{17, 16}, {24, 16}, {24, 20}, {17, 21}}; P(q, (fm::RGB{235, 226, 170})); }
    { const int q[4][2] = {{36, 15}, {43, 14}, {43, 18}, {36, 19}}; P(q, (fm::RGB{235, 226, 170})); }
    d_.px(ox + 20, oy + 18, 255, 255, 235); d_.px(ox + 21, oy + 18, 255, 255, 235);
    d_.px(ox + 39, oy + 16, 255, 255, 235); d_.px(ox + 40, oy + 16, 255, 255, 235);
    // lower bumper, intake and plate
    { const int q[4][2] = {{16, 21}, {44, 19}, {44, 26}, {16, 27}}; P(q, BUMPER); }
    { const int q[4][2] = {{18, 24}, {42, 23}, {42, 25}, {18, 26}}; P(q, GRILLE); }
    { const int q[4][2] = {{25, 21}, {35, 21}, {35, 24}, {25, 25}}; P(q, (fm::RGB{196, 198, 190})); }
    // wheels: near-side front (oval), rear far away (small), hidden right wheel peeking under the bumper
    auto wheel = [&](int cx, int cy, int rx, int ry, bool hub) {
        for (int y = -ry; y <= ry; ++y) for (int x = -rx; x <= rx; ++x)
            if (x * x * ry * ry + y * y * rx * rx <= rx * rx * ry * ry) d_.px(ox + cx + x, oy + cy + y, TIRE.r, TIRE.g, TIRE.b);
        if (hub) {
            const int a = spin & 255;
            d_.px(ox + cx + (((int)fm::sin8((uint8_t)(a + 64)) - 128) * 1 / 128), oy + cy + (((int)fm::sin8((uint8_t)a) - 128) * 2 / 128), 150, 156, 160);
        }
    };
    wheel(13, 24, 3, 4, true);
    wheel(4, 22, 2, 3, false);
    for (int x = 41; x <= 45; ++x) d_.px(ox + x, oy + 26, TIRE.r, TIRE.g, TIRE.b);
}

void DriveScene::render(uint32_t now) {
    if (val(O_VIEW) == 0) { renderSide(now); return; }
    const int speed = val(O_SPEED);
    scroll_ += speed * 6;

    // lamps (depth based), computed first so the road can be lit by them
    Lamp lamps[LAMP_COUNT];
    const bool lampsOn = val(O_LAMPS);
    for (int i = 0; i < LAMP_COUNT; ++i) {
        int z = ((i * LAMP_SPACING) - (scroll_ % (LAMP_SPACING * LAMP_COUNT)) + LAMP_SPACING * LAMP_COUNT) % (LAMP_SPACING * LAMP_COUNT);
        z += LAMP_NEAR;
        const int y = VPY + K / z;
        const bool right = (i & 1) == 0;
        lamps[i].y = y;
        lamps[i].x = roadCenter(y) + (right ? 1 : -1) * (roadHalf(y) * 9 / 8);
        lamps[i].visible = lampsOn && y < 74 && y > VPY + 2;
    }

    // sky
    for (int y = 0; y < VPY; ++y) {
        const fm::RGB c = fm::mix(fm::RGB{4, 6, 22}, fm::RGB{30, 40, 78}, (uint8_t)(y * 255 / VPY));
        for (int x = 0; x < 64; ++x) d_.px(x, y, c.r, c.g, c.b);
    }
    for (const Star& s : stars_) {
        const int v = 70 + fm::sin8((uint8_t)(s.phase + now / 25)) / 3;
        d_.px(s.x, s.y, v, v, v + 25);
    }
    // dark tree line at the horizon
    for (int x = 0; x < 64; ++x) {
        const int h = 3 + (int)(poly::hash2(x / 2, 5) % 5);
        for (int y = VPY - h; y < VPY; ++y) d_.px(x, y, 6, 12, 14);
    }
    // ground beside the road + road
    for (int y = VPY; y < 64; ++y) {
        const int dy = y - VPY;
        const int xc = roadCenter(y), w = roadHalf(y);
        const int depth = dy > 0 ? K / dy : 9999;
        const int phase = (depth + scroll_) >> 4;
        for (int x = 0; x < 64; ++x) {
            const int off = x - xc;
            fm::RGB c;
            if (abs(off) > w + 1) {
                c = {10 + (int)(poly::hash2(x, y) & 3), 22 + (int)(poly::hash2(y, x) & 5), 20};         // verge
            } else if (abs(off) >= w) {
                c = {150, 150, 150};                                                                    // edge line
            } else {
                c = {38 + dy / 6, 40 + dy / 6, 50 + dy / 6};                                            // asphalt, brighter near
                const int cw = max(0, dy / 20);
                if (dy > 6 && abs(off) <= cw && (phase & 1)) c = {210, 205, 150};                       // centre dashes
            }
            // lamp light on road and verge
            int add = 0;
            for (int i = 0; i < LAMP_COUNT; ++i) {
                if (!lamps[i].visible) continue;
                const int gx = lamps[i].x - (lamps[i].x > xc ? w / 3 : -w / 3), gy = lamps[i].y;
                const int rr = 3 + (gy - VPY) / 3;
                const int dx = x - gx, dyy = (y - gy) * 2;
                const int d2 = dx * dx + dyy * dyy;
                if (d2 < rr * rr) add += (rr * rr - d2) * 90 / (rr * rr);
            }
            if (add) c = {(uint8_t)min(255, c.r + add), (uint8_t)min(255, c.g + add * 9 / 10), (uint8_t)min(255, c.b + add / 2)};
            d_.px(x, y, c.r, c.g, c.b);
        }
    }
    // lamp posts, far to near
    for (int i = 0; i < LAMP_COUNT; ++i) {
        // draw order does not matter much: posts do not overlap in depth because of the spacing
        if (!lamps[i].visible) continue;
        const int y = lamps[i].y, x = lamps[i].x;
        const int h = (y - VPY) * 17 / 20 + 3;
        const int dir = x > roadCenter(y) ? -1 : 1;              // arm reaches over the road
        const int arm = 2 + h / 3;
        const int thick = (y - VPY) > 26 ? 2 : 1;
        for (int j = 0; j < h; ++j) for (int t = 0; t < thick; ++t) d_.px(x + t, y - j, 52, 56, 66);
        for (int j = 0; j <= arm; ++j) d_.px(x + dir * j, y - h, 52, 56, 66);
        const int hx = x + dir * arm, hy = y - h;
        const int r = 1 + (y - VPY) / 22;
        poly::disc(d_, hx, hy + 1, r, {255, 236, 170});
        // soft halo
        for (int a = -r - 2; a <= r + 2; ++a) for (int b = -r - 2; b <= r + 2; ++b) {
            const int d2 = a * a + b * b;
            if (d2 > r * r && d2 <= (r + 2) * (r + 2)) d_.px(hx + a, hy + 1 + b, 120, 105, 60);
        }
    }
    // the car, slightly bobbing
    const int bob = ((int)fm::sin8((uint8_t)(now / 9)) > 215) ? 1 : 0;
    drawCar(6, 35 + bob, (int)(scroll_ * 3));
}

// ---------------------------------------------------------------------------------------------
// Side view: car in profile (front to the left), road and scenery streaming to the right.
// ---------------------------------------------------------------------------------------------
namespace {
constexpr int GROUND = 50;        // first road row; wheels touch it
inline int lerpi(int a, int b, int t, int n) { return a + (b - a) * t / n; }
// roof/hood profile of the 56 px long body: y of the upper edge for column x
int topAt(int x) {
    static const int px_[] = {0, 2, 8, 20, 26, 40, 48, 54, 55};
    static const int py_[] = {9, 7, 6, 4, 1, 1, 5, 5, 6};
    for (int i = 0; i < 8; ++i)
        if (x <= px_[i + 1]) return lerpi(py_[i], py_[i + 1], x - px_[i], px_[i + 1] - px_[i]);
    return 6;
}
}

void DriveScene::drawSideCar(int ox, int oy, int spin) {
    static const fm::RGB LIGHTG{58, 150, 94}, MIDG{30, 104, 64}, DARKG{18, 62, 40}, GLASS{16, 24, 34}, GLINT{70, 98, 118};
    auto px = [&](int x, int y, fm::RGB c) { d_.px(ox + x, oy + y, c.r, c.g, c.b); };
    // body: every column from its roof/hood line down to the sill (row 14)
    for (int x = 0; x < 56; ++x) {
        const int t = topAt(x);
        const int bot = (x < 3 || x > 52) ? 14 : 14;
        for (int y = t; y <= bot; ++y) {
            fm::RGB c = MIDG;
            if (y == t) c = LIGHTG;
            else if (y >= 12) c = DARKG;
            else if (y == 8 && x > 3 && x < 53) c = fm::RGB{44, 126, 80};     // shoulder crease
            px(x, y, c);
        }
    }
    // bumpers
    for (int y = 11; y <= 15; ++y) { px(0, y, fm::RGB{20, 24, 26}); px(1, y, fm::RGB{20, 24, 26}); if (y > 11) px(2, y, fm::RGB{20, 24, 26}); }
    for (int y = 10; y <= 15; ++y) { px(55, y, fm::RGB{20, 24, 26}); px(54, y, fm::RGB{20, 24, 26}); if (y > 10) px(53, y, fm::RGB{20, 24, 26}); }
    // side glass with raked pillars
    for (int y = 2; y <= 6; ++y) {
        const int x0 = 22 + (6 - y) * 5 / 5 - 0, x1 = 45 - (6 - y);
        for (int x = x0 - 1; x <= x1; ++x) {
            if (x == 33 || x == 34) { px(x, y, DARKG); continue; }   // B pillar
            fm::RGB c = y <= 3 ? fm::RGB{34, 48, 64} : GLASS;
            if (((x + y * 2) % 9) < 2) c = GLINT;                       // diagonal reflections
            px(x, y, c);
        }
    }
    // door cuts, handles, mirror
    for (int y = 7; y <= 12; ++y) { px(21, y, DARKG); px(34, y, DARKG); px(46, y, DARKG); }
    px(30, 8, fm::RGB{150, 170, 160}); px(31, 8, fm::RGB{150, 170, 160}); px(42, 8, fm::RGB{150, 170, 160});
    px(19, 6, fm::RGB{40, 120, 76}); px(20, 6, fm::RGB{40, 120, 76}); px(19, 7, DARKG);
    // lights
    px(1, 9, fm::RGB{255, 246, 190}); px(2, 9, fm::RGB{255, 246, 190}); px(1, 10, fm::RGB{200, 190, 130});
    px(55, 7, fm::RGB{220, 30, 30}); px(55, 8, fm::RGB{220, 30, 30}); px(54, 7, fm::RGB{140, 20, 20});
    // wheels: dark arch, tyre, silver rim with rotating spokes
    auto wheel = [&](int cx, int cy) {
        for (int y = -5; y <= 5; ++y) for (int x = -5; x <= 5; ++x)
            if (x * x + y * y <= 26 && cy + y <= 14) px(cx + x, cy + y, fm::RGB{8, 8, 10});             // arch cut-out
        for (int y = -4; y <= 4; ++y) for (int x = -4; x <= 4; ++x) {
            const int d2 = x * x + y * y;
            if (d2 <= 18) px(cx + x, cy + y, fm::RGB{20, 20, 24});                                  // tyre
            if (d2 <= 9) px(cx + x, cy + y, fm::RGB{168, 174, 180});                                // rim
        }
        for (int k = 0; k < 5; ++k) {                                                              // rotating spokes
            const uint8_t a = (uint8_t)(-spin + k * 51);
            const int cxs = (int)fm::sin8((uint8_t)(a + 64)) - 128, cys = (int)fm::sin8(a) - 128;
            px(cx + cxs * 1 / 128, cy + cys * 1 / 128, fm::RGB{70, 74, 80});
            px(cx + cxs * 2 / 128, cy + cys * 2 / 128, fm::RGB{70, 74, 80});
        }
        px(cx, cy, fm::RGB{40, 42, 46});
    };
    wheel(11, 13); wheel(44, 13);
}

void DriveScene::renderSide(uint32_t now) {
    const int speed = val(O_SPEED);
    scroll_ += speed * 2;                       // quarter pixels per frame
    const int off = scroll_ >> 2;               // pixels the world has moved to the right
    // night sky
    for (int y = 0; y < GROUND; ++y) {
        const fm::RGB c = fm::mix(fm::RGB{4, 6, 22}, fm::RGB{34, 44, 84}, (uint8_t)(min(255, y * 255 / 46)));
        for (int x = 0; x < 64; ++x) d_.px(x, y, c.r, c.g, c.b);
    }
    for (const Star& s : stars_) {
        const int v = 70 + fm::sin8((uint8_t)(s.phase + now / 25)) / 3;
        d_.px(s.x, s.y % 18, v, v, v + 25);
    }
    poly::disc(d_, 52, 9, 3, fm::RGB{226, 226, 200});                    // moon
    d_.px(51, 8, 200, 200, 176); d_.px(53, 10, 200, 200, 176);
    // distant skyline (slow parallax) with a few lit windows
    const int far = off / 4;
    for (int x = 0; x < 64; ++x) {
        const int wx = (((x - far) % 1024) + 1024) % 1024;
        const int b = wx / 9;
        const int h = 8 + (int)(poly::hash2(b, 3) % 14);
        for (int y = 44 - h; y < 44; ++y) {
            fm::RGB c{12, 16, 32};
            if ((wx % 3) == 1 && (y % 3) == 1 && (poly::hash2(wx, y) % 6) == 0) c = {190, 160, 80};
            d_.px(x, y, c.r, c.g, c.b);
        }
    }
    // hedge / trees along the road at half speed
    const int mid = off / 2;
    for (int x = 0; x < 64; ++x) {
        const int wx = (((x - mid) % 1024) + 1024) % 1024;
        const int h = 3 + (int)(poly::hash2(wx / 3, 9) % 4);
        for (int y = 46 - h; y < 46; ++y) d_.px(x, y, 6, 20 + (int)(poly::hash2(wx, y) & 7), 16);
    }
    // pavement strip behind the car
    for (int y = 46; y < GROUND; ++y) for (int x = 0; x < 64; ++x) d_.px(x, y, y == 46 ? 70 : 44, y == 46 ? 72 : 46, y == 46 ? 80 : 56);
    // street lamps (full speed), drawn before the car
    constexpr int SPACING = 48;
    int lampX[3]; int nl = 0;
    for (int i = 0; i < 3; ++i) {
        if (!val(O_LAMPS)) break;
        const int x = ((i * SPACING + off) % (SPACING * 3)) - 20;
        lampX[nl++] = x;
        for (int y = 20; y < 47; ++y) { d_.px(x, y, 58, 62, 72); d_.px(x + 1, y, 40, 44, 52); }
        for (int j = 1; j <= 6; ++j) d_.px(x - j, 20 - (j > 4 ? 1 : 0), 58, 62, 72);
        for (int a = -2; a <= 2; ++a) for (int b = -2; b <= 1; ++b) {
            const int d2 = a * a + b * b;
            if (d2 > 2 && d2 <= 6) d_.px(x - 7 + a, 21 + b, 110, 96, 54);
        }
        d_.px(x - 8, 21, 255, 240, 180); d_.px(x - 7, 21, 255, 244, 196); d_.px(x - 6, 21, 255, 240, 180); d_.px(x - 7, 22, 255, 232, 160);
    }
    // road
    for (int y = GROUND; y < 64; ++y) for (int x = 0; x < 64; ++x) {
        int add = 0;
        for (int i = 0; i < nl; ++i) { const int dx = x - (lampX[i] - 7); if (abs(dx) < 16) add += (16 - abs(dx)) * (64 - y) / 14; }
        const int base = 36 + (y - GROUND) / 2;
        d_.px(x, y, min(255, base + add), min(255, base + 2 + add * 9 / 10), min(255, base + 12 + add / 2));
    }
    for (int x = 0; x < 64; ++x) d_.px(x, GROUND, 120, 122, 130);                      // kerb edge
    // dashed centre line
    for (int x = 0; x < 64; ++x) { const int wx = (((x - off) % 24) + 24) % 24; if (wx < 12) { d_.px(x, 58, 210, 205, 150); d_.px(x, 59, 190, 186, 135); } }
    // car shadow
    for (int x = 3; x < 58; ++x) d_.px(x, GROUND + 1, 14, 16, 22);
    const int bob = ((int)fm::sin8((uint8_t)(now / 11)) > 225) ? 1 : 0;
    drawSideCar(4, 33 + bob, off * 10);
}
