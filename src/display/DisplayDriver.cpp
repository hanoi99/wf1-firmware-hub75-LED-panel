#include "DisplayDriver.h"
#include "board/BoardConfig.h"
#include <new>

bool DisplayDriver::begin() {
    HUB75_I2S_CFG::i2s_pins pins = {
        WF1_R1_PIN, WF1_G1_PIN, WF1_B1_PIN,
        WF1_R2_PIN, WF1_G2_PIN, WF1_B2_PIN,
        WF1_A_PIN, WF1_B_PIN, WF1_C_PIN, WF1_D_PIN, WF1_E_PIN,
        WF1_LAT_PIN, WF1_OE_PIN, WF1_CLK_PIN
    };

    // Double buffering removes the flicker caused by clear-then-redraw, but needs a
    // second DMA buffer. If that allocation fails, fall back to a single buffer.
    for (int attempt = 0; attempt < 2 && matrix_ == nullptr; ++attempt) {
        HUB75_I2S_CFG config(BoardConfig::PANEL_WIDTH, BoardConfig::PANEL_HEIGHT,
                             BoardConfig::PANEL_CHAIN, pins);
        config.i2sspeed = HUB75_I2S_CFG::HZ_20M;
        config.latch_blanking = 4;
        config.double_buff = (attempt == 0) && BoardConfig::USE_DOUBLE_BUFFER;
        config.driver = static_cast<HUB75_I2S_CFG::shift_driver>(BoardConfig::PANEL_DRIVER);

        MatrixPanel_I2S_DMA* candidate = new (std::nothrow) MatrixPanel_I2S_DMA(config);
        if (candidate != nullptr && candidate->begin()) {
            matrix_ = candidate;
            doubleBuf_ = config.double_buff;
        } else {
            delete candidate;
            Serial.printf("[display] init attempt %d failed (double_buff=%d)\n", attempt, config.double_buff);
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
    Serial.printf("[display] initialized: %dx%d, double buffer=%s\n", W, H, doubleBuf_ ? "yes" : "no");
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
