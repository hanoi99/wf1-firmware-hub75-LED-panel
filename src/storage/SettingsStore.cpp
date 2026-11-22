#include "SettingsStore.h"
#include "board/BoardConfig.h"

bool SettingsStore::begin() {
    ready_ = preferences_.begin("matrix-app", false);
    return ready_;
}

AppSettings SettingsStore::load() {
    AppSettings s;
    if (!ready_) return s;

    s.activeScene = preferences_.getString("scene", s.activeScene);
    s.brightness = preferences_.getUChar("brightness", BoardConfig::DEFAULT_BRIGHTNESS);
    s.text = preferences_.getString("text", s.text);
    s.demoEnabled = preferences_.getUChar("demoOn", 0) != 0;
    s.demoIntervalSec = preferences_.getUShort("demoSec", 20);
    s.demoIds = preferences_.getString("demoIds", "");

    // NVS values can outlive firmware versions: validate before use.
    if (s.brightness < 1 || s.brightness > BoardConfig::MAX_BRIGHTNESS)
        s.brightness = BoardConfig::DEFAULT_BRIGHTNESS;
    if (s.text.length() > MAX_TEXT_BYTES) s.text = s.text.substring(0, MAX_TEXT_BYTES);
    if (s.demoIntervalSec < 3 || s.demoIntervalSec > 3600) s.demoIntervalSec = 20;
    if (s.demoIds.length() > 200) s.demoIds = "";
    return s;
}

void SettingsStore::saveActiveScene(const String& sceneId) {
    if (ready_) preferences_.putString("scene", sceneId);
}

void SettingsStore::saveBrightness(uint8_t brightness) {
    if (ready_ && brightness >= 1 && brightness <= BoardConfig::MAX_BRIGHTNESS)
        preferences_.putUChar("brightness", brightness);
}

void SettingsStore::saveText(const String& text) {
    if (ready_ && text.length() <= MAX_TEXT_BYTES) preferences_.putString("text", text);
}

void SettingsStore::saveDemo(bool enabled, uint16_t intervalSec, const String& ids) {
    if (!ready_) return;
    preferences_.putUChar("demoOn", enabled ? 1 : 0);
    preferences_.putUShort("demoSec", intervalSec);
    preferences_.putString("demoIds", ids);
}

int32_t SettingsStore::getInt(const char* key, int32_t def) {
    return ready_ ? preferences_.getInt(key, def) : def;
}

void SettingsStore::putInt(const char* key, int32_t value) {
    if (ready_ && preferences_.getInt(key, value + 1) != value) preferences_.putInt(key, value);
}

String SettingsStore::getString(const char* key, const String& def) {
    return ready_ ? preferences_.getString(key, def) : def;
}

void SettingsStore::putString(const char* key, const String& value) {
    if (ready_) preferences_.putString(key, value);
}

uint8_t SettingsStore::noteBoot() {
    if (!ready_) return 0;
    const uint8_t n = preferences_.getUChar("bootN", 0);
    preferences_.putUChar("bootN", n < 250 ? n + 1 : n);
    return n;  // unstable boots *before* this one
}

void SettingsStore::markStable() {
    if (ready_ && preferences_.getUChar("bootN", 0) != 0) preferences_.putUChar("bootN", 0);
}
