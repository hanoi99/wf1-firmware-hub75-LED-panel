// Host-Simulator: kompiliert die echten Szenen/Manager/Renderer und schreibt Frames als .rgb (64x64x3).
// Aufruf: sim <outdir> <szene> <dauerMs> <alleMs> [utc=<epoch>] [text=...] [key=value ...]
//         sim selftest
#include <sys/stat.h>
#include "Arduino.h"
#include "app/SceneManager.h"
#include "services/TimeService.h"
#include "display/DisplayDriver.h"
#include "scenes/ClockScene.h"
#include "scenes/DigitalRainScene.h"
#include "scenes/FireScene.h"
#include "scenes/MandelbrotScene.h"
#include "scenes/AutumnLakeScene.h"
#include "scenes/DogFetchScene.h"
#include "scenes/DriveScene.h"
#include "scenes/MoonIslandScene.h"
#include "scenes/ManualTextScene.h"
#include "scenes/PlasmaScene.h"
#include "scenes/StarfieldScene.h"
#include "scenes/WarpScene.h"
#include "scenes/WorldClockScene.h"
#include "services/SolarCalc.h"
#include "storage/SettingsStore.h"

extern time_t sim_epoch0;

static DisplayDriver display;
static TimeService timeService;
static SceneManager scenes;
static ManualTextScene text(display);
static ClockScene clockS(display, timeService);
static WorldClockScene world(display, timeService);
static FireScene fire(display);
static DigitalRainScene rain(display);
static StarfieldScene stars(display);
static WarpScene warp(display);
static PlasmaScene plasma(display);
static MandelbrotScene mandel(display);
static AutumnLakeScene autumn(display);
static MoonIslandScene island(display);
static DogFetchScene dog(display);
static DriveScene drive(display);

static void setup() {
    fm::seed(12345);
    timeService.begin();
    display.begin(DisplayTuning());
    for (IScene* s : std::initializer_list<IScene*>{&text, &clockS, &world, &fire, &rain, &stars, &warp, &plasma, &mandel, &autumn, &island, &dog, &drive})
        scenes.add(*s);
}

static void dump(const char* dir, const char* name, int n) {
    char path[256];
    snprintf(path, sizeof path, "%s/%s_%03d.rgb", dir, name, n);
    FILE* f = fopen(path, "wb");
    fwrite(display.matrix().front(), 1, 64 * 64 * 3, f);
    fclose(f);
}

static int fails = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++fails; fprintf(stderr, "FAIL: " __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

static int selftest() {
    setup();
    sim_epoch0 = 1793000000;
    // 1) Optionen: Schlüssel-Längen (NVS <=15), Wertebereiche, Defaults
    for (uint8_t i = 0; i < scenes.count(); ++i) {
        IScene* s = scenes.at(i);
        CHECK(strlen(s->id()) <= 6, "id too long: %s", s->id());
        for (uint8_t k = 0; k < s->optionCount(); ++k) {
            const SceneOption* o = s->option(k);
            const String key = String(s->id()) + "." + o->key;
            CHECK(key.length() <= 15, "nvs key too long: %s", key.c_str());
            CHECK(o->value >= o->min && o->value <= o->max, "default out of range: %s", key.c_str());
            const int32_t def = o->value;
            CHECK(s->setOption(o->key, o->max), "setOption max failed %s", key.c_str());
            CHECK(!s->setOption(o->key, o->max + 1), "setOption accepted out-of-range %s", key.c_str());
            CHECK(s->setOption(o->key, def), "setOption def failed %s", key.c_str());
        }
    }
    // 2) jede Szene: Frames laufen lassen, mit allen Optionswerten (Select/Toggle), Front-Buffer nicht leer
    for (uint8_t i = 0; i < scenes.count(); ++i) {
        IScene* s = scenes.at(i);
        scenes.activate(s->id());
        auto runFrames = [&](int ms) { for (int t = 0; t < ms; t += 5) { sim_ms += 5; scenes.update(); } };
        runFrames(1500);
        const uint8_t* fb = display.matrix().front();
        long sum = 0; for (int p = 0; p < 4096 * 3; ++p) sum += fb[p];
        CHECK(sum > 0, "scene %s rendered nothing", s->id());
        for (uint8_t k = 0; k < s->optionCount(); ++k) {
            const SceneOption* o = s->option(k);
            if (o->type == OptType::Color) continue;
            const int32_t def = o->value;
            for (int32_t val = o->min; val <= o->max; ++val) { s->setOption(o->key, val); runFrames(300); }
            s->setOption(o->key, def);
        }
    }
    // 3) SceneManager Demo
    { DemoConfig dc; dc.enabled = true; dc.intervalSec = 5; dc.ids = "fire,rain,stars";
      scenes.activate("fire"); scenes.setDemo(dc); }
    std::string seen;
    for (int t = 0; t < 40000; t += 10) { sim_ms += 10; scenes.update(); if (seen.empty() || seen.back() != scenes.activeId()[0]) seen += scenes.activeId()[0]; }
    CHECK(seen.find('f') != std::string::npos && seen.find('r') != std::string::npos && seen.find('s') != std::string::npos, "demo did not cycle: %s", seen.c_str());
    CHECK(scenes.demo().ids == "fire,rain,stars", "demo ids roundtrip");
    // 4) Settings
    SettingsStore st; st.begin();
    st.putInt("x", 5); CHECK(st.getInt("x", 0) == 5, "settings int");
    CHECK(st.noteBoot() == 0, "boot0"); CHECK(st.noteBoot() == 1, "boot1"); st.markStable(); CHECK(st.noteBoot() == 0, "boot reset");
    // 6) Sonnenstand: 2026-06-21 12:00 UTC: Deklination ~23.4 Grad; Subsolarlänge ~0 (+/- 2 Grad Zeitgleichung)
    struct tm tm{}; tm.tm_year = 126; tm.tm_mon = 5; tm.tm_mday = 21; tm.tm_hour = 12;
    time_t t0 = timegm(&tm); auto sun = sunPosition(t0);
    CHECK(fabsf((float)sun.declRad * 57.2958f - 23.43f) < 0.3f, "decl %f", (float)sun.declRad * 57.2958f);
    CHECK(fabsf((float)sun.subLonDeg) < 3.0f, "sub lon %f", (float)sun.subLonDeg);
    fprintf(stderr, fails ? "SELFTEST FAILED (%d)\n" : "SELFTEST OK\n", fails);
    return fails ? 1 : 0;
}

int main(int argc, char** argv) {
    if (argc >= 2 && !strcmp(argv[1], "selftest")) return selftest();
    if (argc < 5) { fprintf(stderr, "usage: sim <outdir> <scene> <durMs> <everyMs> [k=v ...]\n"); return 2; }
    setup();
    const char* dir = argv[1]; const char* id = argv[2];
    const int dur = atoi(argv[3]), every = atoi(argv[4]);
    mkdir(dir, 0755);
    String textArg;
    for (int i = 5; i < argc; ++i) {
        std::string a = argv[i]; auto eq = a.find('=');
        if (eq == std::string::npos) continue;
        std::string k = a.substr(0, eq), v = a.substr(eq + 1);
        if (k == "utc") sim_epoch0 = atol(v.c_str());
        else if (k == "text") text.setText(v.c_str());
    }
    scenes.activate(id);
    IScene* s = scenes.find(id);
    for (int i = 5; i < argc; ++i) {
        std::string a = argv[i]; auto eq = a.find('=');
        if (eq == std::string::npos) continue;
        std::string k = a.substr(0, eq);
        if (k != "utc" && k != "text") {
            int val = (int)strtol(a.c_str() + eq + 1, nullptr, 0);
            if (!s->setOption(k.c_str(), val)) fprintf(stderr, "option rejected: %s\n", a.c_str());
        }
    }
    int n = 0, next = 0;
    for (int t = 0; t <= dur; t += 5) {
        sim_ms += 5; scenes.update();
        if (t >= next) { dump(dir, id, n++); next += every; }
    }
    return 0;
}
