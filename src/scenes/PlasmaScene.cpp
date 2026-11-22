#include "PlasmaScene.h"

PlasmaScene::PlasmaScene(DisplayDriver& display)
    : BaseScene(display), palette_(scratch::block().palette), dist_(scratch::block().bytes) {
    addSelect("pal", "Farbpalette", "Nebula|Lava|Ozean|Regenbogen|Eis", 0);
    addRange("speed", "Geschwindigkeit", 1, 10, 4);
    addRange("scale", "Muster-Größe", 1, 10, 5);
}

void PlasmaScene::begin() {
    scratch::claim(this);
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x)
            dist_[y * 64 + x] = (uint8_t)min<uint32_t>(255, fm::isqrt((uint32_t)((x - 32) * (x - 32) + (y - 32) * (y - 32))) * 6);
    pal::smooth(palette_, (int)val(O_PAL));
}

void PlasmaScene::render(uint32_t now) {
    const uint32_t t = now * (uint32_t)val(O_SPEED) / 14;
    const int sc = 12 - val(O_SCALE);        // smaller factor = bigger pattern
    uint8_t col[64], row[64];
    for (int i = 0; i < 64; ++i) {
        col[i] = fm::sin8((uint8_t)(i * 24 / sc + t));
        row[i] = fm::sin8((uint8_t)(i * 20 / sc + t * 3 / 4));
    }
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) {
            const int diag = fm::sin8((uint8_t)((x + y) * 14 / sc + t / 2));
            const int rad = fm::sin8((uint8_t)(dist_[y * 64 + x] * 2 / sc + t * 5 / 4));
            int v = (col[x] + row[y] + diag + rad) / 4;          // ~mid-range
            v = constrain((v - 64) * 2, 0, 255);                    // stretch contrast
            const fm::RGB c = palette_[v];
            d_.px(x, y, c.r, c.g, c.b);
        }
}
