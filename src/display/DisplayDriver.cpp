#include "DisplayDriver.h"
#include "board/BoardConfig.h"
#include <new>

namespace { constexpr uint32_t MIN_FREE_HEAP = 80000; }   // heap that must stay free after the display driver started

bool DisplayDriver::begin(const DisplayTuning& tuning) {
    HUB75_I2S_CFG::i2s_pins pins = {
        WF1_R1_PIN, WF1_G1_PIN, WF1_B1_PIN,
        WF1_R2_PIN, WF1_G2_PIN, WF1_B2_PIN,
        WF1_A_PIN, WF1_B_PIN, WF1_C_PIN, WF1_D_PIN, WF1_E_PIN,
        WF1_LAT_PIN, WF1_OE_PIN, WF1_CLK_PIN
    };

    tuning_ = tuning;
    // The DMA buffers (frame data + descriptors) take most of the RAM. WiFi, the web server and
    // everything else need roughly MIN_FREE_HEAP afterwards, otherwise clients cannot join the
    // setup AP. So try, in this order: requested depth with double buffer, requested depth with
    // single buffer, then 5 and 4 bit single buffer. An attempt is rejected when too little heap is left.
    struct Attempt { uint8_t bits; bool dbl; };
    const Attempt attempts[] = {
        {tuning.bits, tuning.doubleBuf}, {tuning.bits, false}, {5, false}, {4, false},
    };
    constexpr size_t N = sizeof(attempts) / sizeof(attempts[0]);
    for (size_t i = 0; i < N && matrix_ == nullptr; ++i) {
        const Attempt& at = attempts[i];
        if (i > 0 && at.bits == attempts[i - 1].bits && at.dbl == attempts[i - 1].dbl) continue;
        if (i > 0 && at.bits > tuning.bits) continue;
        const DisplayTuning& t = tuning;
        HUB75_I2S_CFG config(BoardConfig::PANEL_WIDTH, BoardConfig::PANEL_HEIGHT,
                             BoardConfig::PANEL_CHAIN, pins);
        config.i2sspeed = (t.speedMHz >= 20) ? HUB75_I2S_CFG::HZ_20M
                        : (t.speedMHz >= 16) ? HUB75_I2S_CFG::HZ_16M : HUB75_I2S_CFG::HZ_8M;
        config.latch_blanking = t.latch;
        config.clkphase = t.clkphase;
        config.setPixelColorDepthBits(at.bits);
        config.min_refresh_rate = t.minHz;
        config.double_buff = at.dbl;
        config.driver = static_cast<HUB75_I2S_CFG::shift_driver>(t.driver);

        MatrixPanel_I2S_DMA* candidate = new (std::nothrow) MatrixPanel_I2S_DMA(config);
        const bool ok = candidate != nullptr && candidate->begin();
        const bool last = (i + 1 == N);
        const uint32_t freeHeap = ESP.getFreeHeap();
        if (ok && (freeHeap >= MIN_FREE_HEAP || last)) {
            matrix_ = candidate;
            doubleBuf_ = config.double_buff;
            bitsUsed_ = at.bits;
            refreshHz_ = candidate->calculated_refresh_rate;
        } else {
            delete candidate;
            Serial.printf("[display] attempt %u rejected (bits=%u double_buff=%d ok=%d free heap=%u)\n",
                          (unsigned)i, at.bits, at.dbl, ok, (unsigned)freeHeap);
        }
    }
    if (matrix_ == nullptr) {
        Serial.println("[display] HUB75 initialization failed");
        return false;
    }

    matrix_->setTextWrap(false);
    setBrightness(BoardConfig::DEFAULT_BRIGHTNESS);
    clear();
    present();
    Serial.printf("[display] initialized: %dx%d, double buffer=%s (requested %s), %u bit colour, %u MHz, latch=%u, clkphase=%d, minHz=%u, refresh~%d Hz, free heap=%u\n",
                  W, H, doubleBuf_ ? "yes" : "no", tuning_.doubleBuf ? "yes" : "no",
                  bitsUsed_, tuning_.speedMHz, tuning_.latch, tuning_.clkphase ? 1 : 0, tuning_.minHz, refreshHz_,
                  (unsigned)ESP.getFreeHeap());
    return true;
}

MatrixPanel_I2S_DMA& DisplayDriver::matrix() { return *matrix_; }

void DisplayDriver::clear() {
    if (matrix_ != nullptr) matrix_->clearScreen();
}

void DisplayDriver::present() {
    if (matrix_ != nullptr && doubleBuf_) matrix_->flipDMABuffer();
}

void DisplayDriver::setBrightness(uint8_t brightness) {
    brightness_ = min(brightness, BoardConfig::MAX_BRIGHTNESS);
    if (matrix_ != nullptr) matrix_->setBrightness8(brightness_);
}
