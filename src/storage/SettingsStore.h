#pragma once

#include <Arduino.h>
#include <Preferences.h>

struct AppSettings {
    String activeScene = "fire";
    uint8_t brightness = 96;
    String text = "Hallo Welt";
    bool demoEnabled = false;
    uint16_t demoIntervalSec = 20;
    String demoIds = "";  // comma separated scene ids; empty = all
};

// Persistent settings in NVS (Preferences). Scene options are stored by the
// generic getInt/putInt with keys of the form "<sceneId>.<option>".
class SettingsStore {
public:
    static constexpr size_t MAX_TEXT_BYTES = 240;

    bool begin();
    AppSettings load();

    void saveActiveScene(const String& sceneId);
    void saveBrightness(uint8_t brightness);
    void saveText(const String& text);
    void saveDemo(bool enabled, uint16_t intervalSec, const String& ids);

    int32_t getInt(const char* key, int32_t def);
    void putInt(const char* key, int32_t value);
    String getString(const char* key, const String& def);
    void putString(const char* key, const String& value);

    // Boot-loop guard: counts boots that did not reach a stable state.
    uint8_t noteBoot();   // call at boot, returns number of consecutive unstable boots
    void markStable();    // call after the firmware has run fine for a while

private:
    Preferences preferences_;
    bool ready_ = false;
};
