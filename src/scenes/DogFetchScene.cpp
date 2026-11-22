#include "DogFetchScene.h"
#include "Poly.h"

namespace {
constexpr int HORIZON = 34;
constexpr int FEET = 56;                 // y of the dog's paws
constexpr int START_X = 6;
constexpr fm::RGB FUR{232, 190, 108}, FUR_DARK{196, 144, 72}, FUR_LIGHT{248, 222, 160}, NOSE{30, 24, 22}, BALL{214, 236, 52};
}

DogFetchScene::DogFetchScene(DisplayDriver& display) : BaseScene(display) {
    addToggle("clouds", "Wolken", true);
    addRange("speed", "Tempo", 1, 3, 2);
}

void DogFetchScene::begin() {
    dog_ = SIT; ball_ = NONE; t_ = 0; bt_ = 0; dogX_ = START_X * 16; tick_ = 0;
}

void DogFetchScene::step() {
    ++t_; ++tick_;
    const int step = 18 + val(O_SPEED) * 6;           // x16 per frame
    switch (dog_) {
        case SIT:
            if (t_ == 30) { ball_ = FLY; bt_ = 0; targetX_ = 38 + (int)fm::rnd(18); }
            if (ball_ == FLY && t_ > 40) { dog_ = RUN_OUT; t_ = 0; }
            break;
        case RUN_OUT:
            if (ball_ == REST && (dogX_ >> 4) + 14 >= ballX_ - 1) { dog_ = PICK; t_ = 0; break; }
            if ((dogX_ >> 4) + 14 < targetX_ + 3) dogX_ += step;
            break;
        case PICK:
            if (t_ == 8) ball_ = CARRIED;
            if (t_ > 14) { dog_ = RUN_BACK; t_ = 0; }
            break;
        case RUN_BACK:
            dogX_ -= step + 4;
            if (dogX_ <= START_X * 16) { dogX_ = START_X * 16; dog_ = DROP; t_ = 0; }
            break;
        case DROP:
            if (t_ == 6) { ball_ = GROUND_AT_DOG; }
            if (t_ > 26) { ball_ = NONE; dog_ = SIT; t_ = 0; }
            break;
    }
    // ball
    if (ball_ == FLY) {
        ++bt_;
        constexpr int FLIGHT = 34;
        const int p = min(bt_, FLIGHT);
        ballX_ = 1 + (targetX_ - 1) * p / FLIGHT;
        const int base = 36 + (FEET - 2 - 36) * p / FLIGHT;
        ballY_ = base - (p * (FLIGHT - p) * 4 * 20) / (FLIGHT * FLIGHT);   // parabola, apex ~20 px above the line
        if (bt_ >= FLIGHT) { ball_ = REST; bounce_ = 6; }
    } else if (ball_ == REST && bounce_ > 0) {
        --bounce_;
        ballY_ = FEET - 2 - ((bounce_ & 1) ? 0 : bounce_ / 2);
    } else if (ball_ == CARRIED) {
        ballX_ = (dogX_ >> 4) + (dog_ == RUN_BACK ? -2 : 14);
    }
}

void DogFetchScene::drawBall(int x, int y) {
    poly::rect(d_, x - 1, y - 1, 3, 3, BALL);
    d_.px(x, y - 1, 250, 255, 150);
}

void DogFetchScene::drawDog(int x, int y, int dir, DogPhase pose, int frame) {
    // sprite coordinates are for a dog facing right; (dx) is mirrored when dir < 0 (sprite is 15 px wide)
    auto P = [&](int dx, int dy, fm::RGB c) { d_.px(dir > 0 ? x + dx : x + 14 - dx, dy, c.r, c.g, c.b); };   // dy is an absolute row
    auto R = [&](int dx, int dy, int w, int h, fm::RGB c) { for (int j = 0; j < h; ++j) for (int i = 0; i < w; ++i) P(dx + i, dy + j, c); };
    const bool sit = pose == SIT || pose == DROP;
    const bool run = pose == RUN_OUT || pose == RUN_BACK;
    const bool low = pose == PICK;                              // head down at the ball
    const int bob = run ? ((frame & 1) ? 1 : 0) : 0;
    const int by = y - 7 + (sit ? 2 : 0) + bob;                // body top

    // shadow
    for (int i = -1; i < 16; ++i) d_.px(dir > 0 ? x + i : x + 14 - i, FEET + 1, 40, 90, 32);
    // tail (wags when waiting)
    const int wag = ((tick_ / 3) & 1);
    if (sit) { R(-1 + wag, by + 4, 2, 1, FUR); R(-2 + wag, by + 3, 1, 2, FUR); }
    else { P(0, by + 1, FUR); P(-1, by, FUR); P(-1 - (run ? 0 : wag), by - 1, FUR); P(-1, by - 2, FUR_LIGHT); }
    // body
    if (sit) {
        R(1, by + 1, 6, 5, FUR);            // haunches
        R(5, by - 2, 4, 8, FUR);            // chest, upright
        R(6, by + 4, 3, 2, FUR_LIGHT);
        R(5, by + 6, 2, 2, FUR);            // front legs straight
        R(8, by + 6, 2, 2, FUR_DARK);
        R(1, by + 5, 4, 2, FUR_DARK);       // folded hind leg
    } else {
        R(1, by, 9, 4, FUR);
        R(2, by + 3, 7, 1, FUR_LIGHT);      // lighter belly
        R(7, by - 1, 3, 3, FUR);            // shoulders
        // legs: gallop alternates between stretched and tucked
        const int ph = run ? (frame & 3) : 0;
        const int fd = run ? (ph < 2 ? 2 : -1) : 0, bd = run ? (ph < 2 ? -2 : 1) : 0;
        auto leg = [&](int lx, int d, fm::RGB c) { P(lx, by + 4, c); P(lx + d / 2, by + 5, c); P(lx + d, by + 6, c); P(lx + d, by + 7, c); };
        leg(8, fd, FUR); leg(9, fd + (run ? -1 : 0), FUR_DARK);
        leg(1, bd, FUR); leg(2, bd + (run ? 1 : 0), FUR_DARK);
        if (!run) { P(8, by + 6, FUR); P(9, by + 7, FUR_DARK); P(1, by + 6, FUR); P(2, by + 7, FUR_DARK); }
    }
    // head
    const int hy = low ? by + 2 : (sit ? by - 5 : by - 4);
    const int hx = low ? 10 : (sit ? 8 : 9);
    R(hx, hy, 4, 4, FUR);
    R(hx + 3, hy + 2, 3, 2, FUR_LIGHT);           // snout
    P(hx + 5, hy + 2, NOSE);
    P(hx + 2, hy + 1, NOSE);                       // eye
    // pointed ears
    P(hx, hy - 1, FUR_DARK); P(hx + 1, hy - 1, FUR_DARK); P(hx + 1, hy - 2, FUR_DARK); P(hx + 1, hy - 3, FUR_DARK);
    P(hx + 3, hy - 1, FUR_DARK); P(hx + 2, hy - 1, FUR_DARK); P(hx + 2, hy - 2, FUR_DARK);
}

void DogFetchScene::render(uint32_t now) {
    step();
    // sky
    for (int y = 0; y < HORIZON; ++y) {
        const fm::RGB c = fm::mix(fm::RGB{96, 170, 240}, fm::RGB{196, 228, 250}, (uint8_t)(y * 255 / HORIZON));
        for (int x = 0; x < 64; ++x) d_.px(x, y, c.r, c.g, c.b);
    }
    poly::disc(d_, 54, 8, 4, {255, 244, 170});
    if (val(O_CLOUDS)) {
        for (int c = 0; c < 3; ++c) {
            const int cx = ((c * 37 + (int)(now / (700 + c * 250))) % 100) - 18, cy = 7 + c * 7;
            for (int dy = -2; dy <= 2; ++dy)
                for (int dx = -7; dx <= 7; ++dx)
                    if (dx * dx * 4 + dy * dy * 24 <= 4 * 49 && cx + dx >= 0 && cx + dx < 64) d_.px(cx + dx, cy + dy, 248, 250, 255);
        }
    }
    // far hills and tree line
    for (int x = 0; x < 64; ++x) {
        const int h = 27 + ((int)fm::sin8((uint8_t)(x * 4 + 30)) - 128) / 32;
        for (int y = h; y < HORIZON; ++y) d_.px(x, y, 112, 160, 130);
        const int tr = (int)(poly::hash2(x / 3, 7) & 3);
        for (int y = HORIZON - 3 - tr; y < HORIZON; ++y) d_.px(x, y, 56, 118, 68);
    }
    // meadow with texture and flowers
    for (int y = HORIZON; y < 64; ++y) {
        const fm::RGB base = fm::mix(fm::RGB{98, 176, 62}, fm::RGB{44, 118, 38}, (uint8_t)((y - HORIZON) * 255 / (64 - HORIZON)));
        for (int x = 0; x < 64; ++x) {
            const int n = (int)(poly::hash2(x, y) & 15) - 7;
            d_.px(x, y, constrain(base.r + n, 0, 255), constrain(base.g + n * 2, 0, 255), constrain(base.b + n, 0, 255));
        }
    }
    for (int i = 0; i < 14; ++i) {
        const int fx = (int)(poly::hash2(i, 3) % 64), fy = HORIZON + 4 + (int)(poly::hash2(i, 9) % 22);
        d_.px(fx, fy, (i & 1) ? 250 : 255, (i & 1) ? 240 : 200, (i & 1) ? 250 : 80);
    }
    // ball (behind dog when carried)
    if (ball_ == FLY || ball_ == REST) {
        if (ball_ == FLY || bounce_ > 0) d_.px(ballX_, FEET + 1, 40, 90, 32);
        drawBall(ballX_, ballY_);
    }
    // dog
    const int dx = dogX_ >> 4;
    const int dir = (dog_ == RUN_BACK) ? -1 : 1;
    const int frame = tick_ / 2;
    drawDog(dx, FEET - 1, dir, dog_, frame);
    if (ball_ == CARRIED) drawBall(dir > 0 ? dx + 15 : dx - 1, FEET - 7 + ((dog_ == PICK) ? 5 : 3));
    if (ball_ == GROUND_AT_DOG) drawBall(dx + 17, FEET - 1);
    // foreground grass tufts swaying
    for (int x = 1; x < 64; x += 4) {
        const int sway = ((int)fm::sin8((uint8_t)(now / 12 + x * 9)) - 128) / 96;
        const int y = 62 - (int)(poly::hash2(x, 1) & 1);
        d_.px(x + sway, y - 2, 52, 140, 44); d_.px(x, y - 1, 46, 128, 40); d_.px(x, y, 38, 110, 34);
        d_.px(x + 1 - sway, y - 1, 60, 150, 50);
    }
}
