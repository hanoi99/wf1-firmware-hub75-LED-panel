#pragma once

#include <Arduino.h>

// Description of one user-adjustable scene setting. The web UI builds its
// controls from these descriptors, so new scenes never need UI or API changes.
enum class OptType : uint8_t { Select, Range, Toggle, Color };

struct SceneOption {
    const char* key;      // short, <= 8 chars (NVS key is "<sceneId>.<key>", max 15)
    const char* label;
    OptType type;
    int32_t min;
    int32_t max;
    int32_t value;
    const char* choices;  // Select only: "A|B|C" (values 0..n-1)
};

class IScene {
public:
    virtual ~IScene() = default;
    virtual const char* id() const = 0;
    virtual const char* title() const = 0;
    virtual const char* category() const { return "effect"; }  // text | info | effect
    virtual bool available() const { return true; }            // false = skipped by demo mode
    virtual uint16_t frameIntervalMs() const { return 33; }
    virtual void begin() = 0;
    virtual void update() = 0;   // render exactly one frame (called at frameIntervalMs)
    virtual void end() = 0;
    virtual uint8_t optionCount() const { return 0; }
    virtual const SceneOption* option(uint8_t) const { return nullptr; }
    virtual bool setOption(const char*, int32_t) { return false; }
    virtual String info() const { return String(); }           // short live status for the UI
};
