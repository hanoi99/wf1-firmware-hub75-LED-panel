#include <Arduino.h>
#include <LittleFS.h>
#include <esp_partition.h>
#include <esp_random.h>
#include <esp_system.h>

#include "app/OptionKeys.h"
#include "app/SceneManager.h"
#include "board/BoardConfig.h"
#include "display/DisplayDriver.h"
#include "network/NetworkService.h"
#include "network/WebController.h"
#include "scenes/ClockScene.h"
#include "scenes/DigitalRainScene.h"
#include "scenes/FireScene.h"
#include "scenes/ManualTextScene.h"
#include "scenes/AutumnLakeScene.h"
#include "scenes/MandelbrotScene.h"
#include "scenes/DogFetchScene.h"
#include "scenes/DriveScene.h"
#include "scenes/MoonIslandScene.h"
#include "scenes/SysInfoScene.h"
#include "scenes/PlasmaScene.h"
#include "scenes/StarfieldScene.h"
#include "scenes/WarpScene.h"
#include "scenes/WorldClockScene.h"
#include "services/TimeService.h"
#include "storage/SettingsStore.h"

#ifndef APP_VERSION
#define APP_VERSION "0.2.28-dev"
#endif

namespace {
constexpr uint32_t BOOT_INFO_MAX_MS = 45000;   // diagnostics screen after boot (ends earlier once WiFi is connected)
constexpr uint32_t BOOT_INFO_CONNECTED_MS = 8000;
String restoreSceneId;                         // scene to switch to when the boot info ends
constexpr uint32_t STABLE_AFTER_MS = 30000;   // clean run needed before boots count as stable
constexpr uint8_t SAFE_MODE_AFTER = 3;        // consecutive unstable boots -> safe mode

DisplayDriver display;
TimeService timeService;
SettingsStore settings;
NetworkService network(settings);
SceneManager scenes;

ManualTextScene textScene(display);
ClockScene clockScene(display, timeService);
WorldClockScene worldScene(display, timeService);
FireScene fireScene(display);
DigitalRainScene rainScene(display);
StarfieldScene starScene(display);
WarpScene warpScene(display);
PlasmaScene plasmaScene(display);
MandelbrotScene mandelScene(display);
AutumnLakeScene autumnScene(display);
MoonIslandScene islandScene(display);
DogFetchScene dogScene(display);
DriveScene driveScene(display);
SysInfoScene sysInfoScene(display, network, APP_VERSION, "?", false, 0);

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

    if (!settings.begin()) Serial.println("[settings] NVS unavailable; defaults will be used");
    if (!display.begin(DisplayTuning::load(settings))) haltWithError("[boot] display initialization failed");

    // Die Weboberflaeche ist in die Firmware eingebettet; LittleFS ist nur optional (WEB_UI_FROM_FS).
    const esp_partition_t* fsPart =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, nullptr);
    if (fsPart)
        Serial.printf("[fs] data partition '%s' at 0x%06X, size 0x%06X\n", fsPart->label,
                      (unsigned)fsPart->address, (unsigned)fsPart->size);
    else
        Serial.println("[fs] NO spiffs/littlefs partition in the flashed partition table");
#ifdef WEB_UI_FROM_FS   // not mounted otherwise: saves RAM, the web UI is embedded in the firmware
    if (LittleFS.begin(false))
        Serial.printf("[fs] LittleFS mounted (%u of %u bytes used)\n", (unsigned)LittleFS.usedBytes(),
                      (unsigned)LittleFS.totalBytes());
    else
        Serial.println("[fs] LittleFS mount failed (optional - embedded web UI is used)");
#endif

    const uint8_t unstableBoots = settings.noteBoot();
    const bool safeMode = unstableBoots >= SAFE_MODE_AFTER;
    if (safeMode) Serial.printf("[boot] SAFE MODE: %u unstable boots in a row\n", unstableBoots);

    AppSettings saved = settings.load();
    display.setBrightness(saved.brightness);

    scenes.add(textScene);
    scenes.add(clockScene);
    scenes.add(worldScene);
    scenes.add(fireScene);
    scenes.add(rainScene);
    scenes.add(starScene);
    scenes.add(warpScene);
    scenes.add(plasmaScene);
    scenes.add(mandelScene);
    scenes.add(autumnScene);
    scenes.add(islandScene);
    scenes.add(dogScene);
    scenes.add(driveScene);
    scenes.add(sysInfoScene);

    restoreOptions();
    textScene.setText(saved.text);

    network.begin();
    timeService.begin();

    DemoConfig demo;
    demo.enabled = saved.demoEnabled && !safeMode;
    demo.intervalSec = saved.demoIntervalSec;
    demo.ids = saved.demoIds;
    scenes.setDemo(demo);

    const String start = safeMode ? String("fire") : saved.activeScene;
    if (!scenes.activate(start)) scenes.activate("fire");  // settings are left untouched in safe mode
    {
        const esp_reset_reason_t rr = esp_reset_reason();
        const char* txt = "?";
        bool bad = false;
        switch (rr) {
            case ESP_RST_POWERON: txt = "POWER-ON"; break;
            case ESP_RST_SW: txt = "SOFTWARE"; break;
            case ESP_RST_EXT: txt = "EXTERNAL"; break;
            case ESP_RST_PANIC: txt = "CRASH"; bad = true; break;
            case ESP_RST_INT_WDT: case ESP_RST_TASK_WDT: case ESP_RST_WDT: txt = "WATCHDOG"; bad = true; break;
            case ESP_RST_BROWNOUT: txt = "BROWNOUT"; bad = true; break;
            case ESP_RST_DEEPSLEEP: txt = "SLEEP"; break;
            default: break;
        }
        sysInfoScene.setBoot(txt, bad, unstableBoots);
        restoreSceneId = scenes.activeId();
        scenes.activate("sysinfo");   // does not touch the saved scene setting
    }

    web.begin();
    Serial.println("[boot] initialization complete");
}

void loop() {
    network.update();
    timeService.update();
    web.update();
    scenes.update();
    if (restoreSceneId.length() > 0) {
        const uint32_t ms = millis();
        if (strcmp(scenes.activeId(), "sysinfo") != 0) {
            restoreSceneId = "";   // the user picked something else; leave it alone
        } else if (ms > BOOT_INFO_MAX_MS || (network.connected() && ms > BOOT_INFO_CONNECTED_MS)) {
            scenes.activate(restoreSceneId.c_str());
            restoreSceneId = "";
        }
    }
    if (!markedStable && millis() > STABLE_AFTER_MS) {
        settings.markStable();
        display.tuning().confirm(settings);
        markedStable = true;
    }
    delay(1);
}
