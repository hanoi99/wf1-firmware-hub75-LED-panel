#include <Arduino.h>
#include <LittleFS.h>
#include <esp_random.h>
#include <esp_system.h>

#include "app/OptionKeys.h"
#include "app/SceneManager.h"
#include "board/BoardConfig.h"
#include "display/DisplayDriver.h"
#include "network/NetworkService.h"
#include "network/WebController.h"
#include "scenes/AsciiScene.h"
#include "scenes/ClockScene.h"
#include "scenes/DigitalRainScene.h"
#include "scenes/FireScene.h"
#include "scenes/IssTrackerScene.h"
#include "scenes/ManualTextScene.h"
#include "scenes/MandelbrotScene.h"
#include "scenes/PlasmaScene.h"
#include "scenes/StarfieldScene.h"
#include "scenes/WarpScene.h"
#include "scenes/WorldClockScene.h"
#include "services/IssService.h"
#include "services/TimeService.h"
#include "storage/SettingsStore.h"

#ifndef APP_VERSION
#define APP_VERSION "0.2.0-dev"
#endif

namespace {
constexpr uint32_t STABLE_AFTER_MS = 30000;   // clean run needed before boots count as stable
constexpr uint8_t SAFE_MODE_AFTER = 3;        // consecutive unstable boots -> safe mode

DisplayDriver display;
TimeService timeService;
SettingsStore settings;
NetworkService network(settings);
IssService iss;
SceneManager scenes;

ManualTextScene textScene(display);
ClockScene clockScene(display, timeService);
WorldClockScene worldScene(display, timeService);
IssTrackerScene issScene(display, iss, timeService);
FireScene fireScene(display);
DigitalRainScene rainScene(display);
StarfieldScene starScene(display);
WarpScene warpScene(display);
PlasmaScene plasmaScene(display);
AsciiScene asciiScene(display);
MandelbrotScene mandelScene(display);

WebController web(scenes, textScene, display, settings, network, timeService);
bool markedStable = false;

void haltWithError(const char* message) {
    Serial.println(message);
    while (true) delay(1000);
}

void restoreOptions() {
    for (uint8_t i = 0; i < scenes.count(); ++i) {
        IScene* scene = scenes.at(i);
        for (uint8_t k = 0; k < scene->optionCount(); ++k) {
            const SceneOption* o = scene->option(k);
            char key[16];
            if (!makeOptionKey(scene->id(), o->key, key, sizeof(key))) {
                Serial.printf("[settings] option key too long: %s.%s\n", scene->id(), o->key);
                continue;
            }
            const int32_t saved = settings.getInt(key, o->value);
            if (saved != o->value) scene->setOption(o->key, saved);  // validated by the scene
        }
    }
}
}  // namespace

void setup() {
    Serial.begin(115200);
    delay(150);
    Serial.println("\n[boot] LED Matrix Firmware " APP_VERSION);
    Serial.printf("[boot] reset reason: %d\n", (int)esp_reset_reason());
    fm::seed(esp_random());

    if (!display.begin()) haltWithError("[boot] display initialization failed");

    if (!LittleFS.begin(false))
        Serial.println("[fs] LittleFS mount failed; web UI files unavailable (run uploadfs)");

    if (!settings.begin()) Serial.println("[settings] NVS unavailable; defaults will be used");
    const uint8_t unstableBoots = settings.noteBoot();
    const bool safeMode = unstableBoots >= SAFE_MODE_AFTER;
    if (safeMode) Serial.printf("[boot] SAFE MODE: %u unstable boots in a row\n", unstableBoots);

    AppSettings saved = settings.load();
    display.setBrightness(saved.brightness);

    scenes.add(textScene);
    scenes.add(clockScene);
    scenes.add(worldScene);
    scenes.add(issScene);
    scenes.add(fireScene);
    scenes.add(rainScene);
    scenes.add(starScene);
    scenes.add(warpScene);
    scenes.add(plasmaScene);
    scenes.add(asciiScene);
    scenes.add(mandelScene);

    restoreOptions();
    textScene.setText(saved.text);

    network.begin();
    timeService.begin();
    iss.begin();

    DemoConfig demo;
    demo.enabled = saved.demoEnabled && !safeMode;
    demo.intervalSec = saved.demoIntervalSec;
    demo.ids = saved.demoIds;
    scenes.setDemo(demo);

    const String start = safeMode ? String("fire") : saved.activeScene;
    if (!scenes.activate(start)) scenes.activate("fire");  // settings are left untouched in safe mode

    web.begin();
    Serial.println("[boot] initialization complete");
}

void loop() {
    network.update();
    timeService.update();
    web.update();
    scenes.update();
    if (!markedStable && millis() > STABLE_AFTER_MS) {
        settings.markStable();
        markedStable = true;
    }
    delay(1);
}
