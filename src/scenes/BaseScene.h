#pragma once

#include <cstring>
#include "IScene.h"
#include "display/DisplayDriver.h"
#include "display/FastMath.h"
#include "display/Palettes.h"

// Common base: owns the option table and presents the frame after render().
class BaseScene : public IScene {
public:
    explicit BaseScene(DisplayDriver& display) : d_(display) {}
    void update() override { render(millis()); d_.present(); }
    void end() override {}

    uint8_t optionCount() const override { return n_; }
    const SceneOption* option(uint8_t i) const override { return i < n_ ? &opts_[i] : nullptr; }
    bool setOption(const char* key, int32_t value) override {
        for (uint8_t i = 0; i < n_; ++i) {
            if (strcmp(opts_[i].key, key) != 0) continue;
            SceneOption& o = opts_[i];
            if (value < o.min || value > o.max) return false;
            o.value = value;
            onOption(i);
            return true;
        }
        return false;
    }

protected:
    virtual void render(uint32_t now) = 0;
    virtual void onOption(uint8_t) {}
    uint8_t add(const char* key, const char* label, OptType type, int32_t min, int32_t max,
                int32_t def, const char* choices = nullptr) {
        if (n_ >= MAX_OPTS) return 0;
        opts_[n_] = {key, label, type, min, max, def, choices};
        return n_++;
    }
    uint8_t addSelect(const char* key, const char* label, const char* choices, int32_t def) {
        int count = 1;
        for (const char* p = choices; *p; ++p) if (*p == '|') ++count;
        return add(key, label, OptType::Select, 0, count - 1, def, choices);
    }
    uint8_t addRange(const char* key, const char* label, int32_t min, int32_t max, int32_t def) {
        return add(key, label, OptType::Range, min, max, def);
    }
    uint8_t addToggle(const char* key, const char* label, bool def) {
        return add(key, label, OptType::Toggle, 0, 1, def ? 1 : 0);
    }
    uint8_t addColor(const char* key, const char* label, uint32_t def) {
        return add(key, label, OptType::Color, 0, 0xFFFFFF, static_cast<int32_t>(def));
    }
    int32_t val(uint8_t i) const { return opts_[i].value; }

    static constexpr uint8_t MAX_OPTS = 8;
    DisplayDriver& d_;
    SceneOption opts_[MAX_OPTS]{};
    uint8_t n_ = 0;
};
