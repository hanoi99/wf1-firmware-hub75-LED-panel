#pragma once

#include "BaseScene.h"

// User text with font, direction (left/right/up/down/static/pendulum) and colour effects.
class ManualTextScene : public BaseScene {
public:
    static constexpr size_t MAX_TEXT = 240;
    explicit ManualTextScene(DisplayDriver& display);
    const char* id() const override { return "text"; }
    const char* title() const override { return "Text"; }
    const char* category() const override { return "text"; }
    void begin() override;
    bool setText(const String& utf8);   // false (unchanged) if longer than MAX_TEXT bytes
    String text() const { return raw_; }

protected:
    void render(uint32_t now) override;
    void onOption(uint8_t) override { dirty_ = true; }

private:
    enum { O_FONT, O_SIZE, O_DIR, O_FX, O_SPEED, O_COLOR, O_BG };
    enum { DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN, DIR_STATIC, DIR_PENDULUM };
    static constexpr uint8_t MAX_LINES = 16;

    void rebuild();
    uint16_t colorFor(int index, int x, uint32_t now) const;
    void drawLine(const String& s, int x, int yTop, uint32_t now, int firstIndex);

    String raw_ = "Hallo Welt";
    String line_;                 // single line (horizontal modes)
    String lines_[MAX_LINES];     // wrapped lines (vertical / static modes)
    uint8_t nLines_ = 0;
    int lineW_ = 0, lineH_ = 8, totalH_ = 8;
    int32_t pos_ = 0;             // 1/256 px
    int8_t pendDir_ = 1;
    uint32_t lastMs_ = 0;
    bool dirty_ = true;
};
