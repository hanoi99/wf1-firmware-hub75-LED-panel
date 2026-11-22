#include "WebController.h"

#include <LittleFS.h>
#include <cctype>
#include <cstdlib>

#include "app/OptionKeys.h"
#include "board/BoardConfig.h"
#include "data/WebUiData.h"

#if __has_include("../secrets.h")
  #include "../secrets.h"
#endif
#ifndef WEB_PASS
  #define WEB_PASS ""
#endif

#ifndef APP_VERSION
#define APP_VERSION "0.2.28-dev"
#endif

namespace {
String jsonEscape(const String& input) {
    String out;
    out.reserve(input.length() + 8);
    for (size_t i = 0; i < input.length(); ++i) {
        const uint8_t c = (uint8_t)input[i];
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) { char e[7]; snprintf(e, sizeof(e), "\\u%04X", c); out += e; }
                else out += (char)c;
        }
    }
    return out;
}

String mimeType(const String& path) {
    if (path.endsWith(".html")) return "text/html; charset=utf-8";
    if (path.endsWith(".css")) return "text/css; charset=utf-8";
    if (path.endsWith(".js")) return "application/javascript; charset=utf-8";
    if (path.endsWith(".svg")) return "image/svg+xml";
    return "text/plain; charset=utf-8";
}

bool parseInt(const String& value, long minimum, long maximum, long& result) {
    if (value.length() == 0 || value.length() > 10) return false;
    for (size_t i = 0; i < value.length(); ++i)
        if (!isDigit(value[i])) return false;
    const long parsed = value.toInt();
    if (parsed < minimum || parsed > maximum) return false;
    result = parsed;
    return true;
}

// Scene id lists: only lowercase letters, digits and commas.
bool validIdList(const String& ids) {
    if (ids.length() > 160) return false;
    for (size_t i = 0; i < ids.length(); ++i) {
        const char c = ids[i];
        if (!(islower((unsigned char)c) || isDigit(c) || c == ',')) return false;
    }
    return true;
}
}  // namespace

void WebController::route(const char* uri, HTTPMethod method, void (WebController::*handler)()) {
    server_.on(uri, method, [this, handler]() {
        if (!authorized()) return;
        (this->*handler)();
    });
}

bool WebController::authorized() {
    if (strlen(WEB_PASS) == 0) return true;
    if (server_.authenticate("admin", WEB_PASS)) return true;
    server_.requestAuthentication();
    return false;
}

void WebController::begin() {
    route("/", HTTP_GET, &WebController::serveFile);
    route("/style.css", HTTP_GET, &WebController::serveFile);
    route("/app.js", HTTP_GET, &WebController::serveFile);
    route("/api/status", HTTP_GET, &WebController::sendStatus);
    route("/api/state", HTTP_GET, &WebController::sendState);
    route("/api/scene", HTTP_POST, &WebController::setScene);
    route("/api/option", HTTP_POST, &WebController::setOption);
    route("/api/text", HTTP_POST, &WebController::setText);
    route("/api/brightness", HTTP_POST, &WebController::setBrightness);
    route("/api/display", HTTP_POST, &WebController::postDisplay);
    route("/api/demo", HTTP_POST, &WebController::setDemo);
    route("/api/wifi", HTTP_GET, &WebController::getWifi);
    route("/api/wifi", HTTP_POST, &WebController::postWifi);
    route("/api/wifi/scan", HTTP_POST, &WebController::postWifiScan);
    route("/api/wifi/forget", HTTP_POST, &WebController::postWifiForget);
    route("/api/wifi/scan", HTTP_GET, &WebController::getWifiScan);
    server_.onNotFound([this]() { if (authorized()) serveFile(); });
    server_.begin();
    Serial.println("[web] HTTP server started on port 80");
}

void WebController::update() {
    server_.handleClient();
    if (restartAt_ != 0 && (int32_t)(millis() - restartAt_) >= 0) ESP.restart();
}

void WebController::sendJson(int statusCode, const String& body) {
    server_.sendHeader("Cache-Control", "no-store");
    server_.sendHeader("X-Content-Type-Options", "nosniff");
    server_.send(statusCode, "application/json; charset=utf-8", body);
}

void WebController::sendError(int statusCode, const char* message) {
    sendJson(statusCode, String("{\"ok\":false,\"error\":\"") + message + "\"}");
}

void WebController::serveFile() {
    String path = server_.uri();
    if (path == "/") path = "/index.html";
    const WebAsset* asset = nullptr;
    for (size_t i = 0; i < WEBUI_ASSET_COUNT; ++i)
        if (path == WEBUI_ASSETS[i].path) asset = &WEBUI_ASSETS[i];
    if (!asset) {
        server_.send(404, "text/plain; charset=utf-8", "Not found");
        return;
    }
#ifdef WEB_UI_FROM_FS
    // Entwicklungsmodus: Dateien aus LittleFS (pio run -t uploadfs) statt der eingebetteten Kopie
    File file = LittleFS.open(path, "r");
    if (file) {
        server_.sendHeader("Cache-Control", "no-cache");
        server_.streamFile(file, asset->mime);
        file.close();
        return;
    }
#endif
    server_.sendHeader("Cache-Control", "no-cache");
    server_.sendHeader("X-Content-Type-Options", "nosniff");
    server_.sendHeader("Content-Encoding", "gzip");
    server_.send_P(200, asset->mime, reinterpret_cast<const char*>(asset->data), asset->len);
}

void WebController::appendScene(String& json, IScene& scene) {
    json += "{\"id\":\"" + String(scene.id()) + "\",\"title\":\"" + jsonEscape(scene.title()) +
            "\",\"cat\":\"" + scene.category() + "\",\"avail\":" + (scene.available() ? "true" : "false") +
            ",\"opts\":[";
    for (uint8_t i = 0; i < scene.optionCount(); ++i) {
        const SceneOption* o = scene.option(i);
        if (i) json += ",";
        const char* type = o->type == OptType::Select ? "select" : o->type == OptType::Range ? "range"
                         : o->type == OptType::Toggle ? "toggle" : "color";
        json += "{\"k\":\"" + String(o->key) + "\",\"l\":\"" + jsonEscape(o->label) +
                "\",\"t\":\"" + type + "\",\"min\":" + String(o->min) + ",\"max\":" + String(o->max) +
                ",\"v\":" + String(o->value);
        if (o->choices) json += ",\"c\":\"" + jsonEscape(o->choices) + "\"";
        json += "}";
    }
    json += "]}";
}

void WebController::sendStatus() {
    String json;
    json.reserve(4600);
    const DemoConfig& demo = scenes_.demo();
    json += "{\"version\":\"" APP_VERSION "\"";
    json += ",\"scene\":\"" + String(scenes_.activeId()) + "\"";
    json += ",\"brightness\":" + String(display_.brightness());
    json += ",\"maxBrightness\":" + String(BoardConfig::MAX_BRIGHTNESS);
    json += ",\"maxText\":" + String(SettingsStore::MAX_TEXT_BYTES);
    json += ",\"text\":\"" + jsonEscape(textScene_.text()) + "\"";
    json += ",\"demo\":{\"on\":" + String(demo.enabled ? "true" : "false") +
            ",\"sec\":" + String(demo.intervalSec) + ",\"ids\":\"" + jsonEscape(demo.ids) + "\"}";
    json += ",\"sys\":{\"ip\":\"" + network_.ipAddress() + "\"";
    json += ",\"heap\":" + String(ESP.getFreeHeap());
    json += ",\"minHeap\":" + String(ESP.getMinFreeHeap());
    json += ",\"up\":" + String(millis() / 1000);
    json += ",\"rssi\":" + String(network_.rssi());
    json += ",\"wifi\":" + String(network_.connected() ? "true" : "false");
    json += ",\"ap\":" + String(network_.apActive() ? "true" : "false");
    json += ",\"time\":" + String(time_.isValid() ? "true" : "false");
    json += ",\"fs\":" + String(LittleFS.totalBytes() > 0 ? "true" : "false");
    json += ",\"db\":" + String(display_.doubleBuffered() ? "true" : "false") + "}";
    const DisplayTuning& dt = display_.tuning();
    json += ",\"disp\":{\"db\":" + String(display_.doubleBuffered() ? "true" : "false");
    json += ",\"want\":" + String(dt.doubleBuf ? "true" : "false");
    json += ",\"spd\":" + String(dt.speedMHz) + ",\"lat\":" + String(dt.latch) + ",\"hzmin\":" + String(dt.minHz) + ",\"bits\":" + String(display_.colorBits()) + ",\"wbits\":" + String(dt.bits) + ",\"heap\":" + String(ESP.getFreeHeap());
    json += ",\"clk\":" + String(dt.clkphase ? "true" : "false");
    json += ",\"hz\":" + String(display_.refreshHz()) + "}";
    json += ",\"scenes\":[";
    for (uint8_t i = 0; i < scenes_.count(); ++i) {
        if (i) json += ",";
        appendScene(json, *scenes_.at(i));
    }
    json += "]}";
    sendJson(200, json);
}

// Leichter Abruf fuer das regelmaessige Polling der Weboberflaeche (alle 4 s).
void WebController::sendState() {
    IScene* cur = scenes_.active();
    String json = "{\"scene\":\"" + String(scenes_.activeId()) + "\"";
    json += ",\"demo\":" + String(scenes_.demo().enabled ? "true" : "false");
    json += ",\"note\":\"" + jsonEscape(cur ? cur->info() : String()) + "\"";
    json += ",\"heap\":" + String(ESP.getFreeHeap());
    json += ",\"rssi\":" + String(network_.rssi());
    json += ",\"up\":" + String(millis() / 1000) + "}";
    sendJson(200, json);
}

void WebController::setScene() {
    const String id = server_.arg("id");
    if (id.length() == 0) { sendError(400, "missing scene id"); return; }
    if (!scenes_.activate(id)) { sendError(400, "unknown scene"); return; }
    // A manual choice always ends the demo playlist.
    DemoConfig demo = scenes_.demo();
    if (demo.enabled) {
        demo.enabled = false;
        scenes_.setDemo(demo);
        settings_.saveDemo(false, demo.intervalSec, demo.ids);
    }
    settings_.saveActiveScene(id);
    sendJson(200, "{\"ok\":true}");
}

void WebController::setOption() {
    IScene* scene = scenes_.find(server_.arg("scene"));
    if (scene == nullptr) { sendError(400, "unknown scene"); return; }
    const String key = server_.arg("key");
    long value = 0;
    if (!parseInt(server_.arg("value"), 0, 0xFFFFFF, value)) { sendError(400, "invalid value"); return; }
    if (!scene->setOption(key.c_str(), (int32_t)value)) { sendError(400, "option rejected"); return; }
    char nvsKey[16];
    if (makeOptionKey(scene->id(), key.c_str(), nvsKey, sizeof(nvsKey)))
        settings_.putInt(nvsKey, (int32_t)value);
    sendJson(200, "{\"ok\":true}");
}

void WebController::setText() {
    if (!server_.hasArg("text")) { sendError(400, "missing text"); return; }
    const String value = server_.arg("text");
    if (value.length() > SettingsStore::MAX_TEXT_BYTES) { sendError(400, "text too long (max 240 bytes)"); return; }
    for (size_t i = 0; i < value.length(); ++i) {
        const uint8_t c = (uint8_t)value[i];
        if (c < 0x20 && c != '\n' && c != '\t' && c != '\r') { sendError(400, "unsupported control character"); return; }
    }
    textScene_.setText(value);
    settings_.saveText(value);
    sendJson(200, "{\"ok\":true}");
}

void WebController::setBrightness() {
    long value = 0;
    if (!parseInt(server_.arg("value"), 1, BoardConfig::MAX_BRIGHTNESS, value)) {
        sendError(400, "brightness out of range");
        return;
    }
    display_.setBrightness((uint8_t)value);
    settings_.saveBrightness((uint8_t)value);
    sendJson(200, "{\"ok\":true}");
}

// Panel timing (flicker tuning). The I2S/LCD DMA engine can only be configured at boot,
// so the values are stored and the board restarts shortly after the reply.
void WebController::postDisplay() {
    DisplayTuning t = server_.arg("reset") == "1" ? DisplayTuning() : display_.tuning();
    long v = 0;
    if (server_.hasArg("spd")) {
        if (!parseInt(server_.arg("spd"), 8, 20, v) || !DisplayTuning::validSpeed(v)) { sendError(400, "speed must be 8, 16 or 20 MHz"); return; }
        t.speedMHz = (uint8_t)v;
    }
    if (server_.hasArg("lat")) {
        if (!parseInt(server_.arg("lat"), 0, 32, v)) { sendError(400, "latch must be 0-32"); return; }
        t.latch = (uint8_t)v;
    }
    if (server_.hasArg("clk")) {
        if (!parseInt(server_.arg("clk"), 0, 1, v)) { sendError(400, "clk must be 0 or 1"); return; }
        t.clkphase = v != 0;
    }
    if (server_.hasArg("bits")) {
        if (!parseInt(server_.arg("bits"), 4, 8, v)) { sendError(400, "bits must be 4-8"); return; }
        t.bits = (uint8_t)v;
    }
    if (server_.hasArg("hz")) {
        if (!parseInt(server_.arg("hz"), 60, 200, v)) { sendError(400, "hz must be 60-200"); return; }
        t.minHz = (uint8_t)v;
    }
    if (server_.hasArg("db")) {
        if (!parseInt(server_.arg("db"), 0, 1, v)) { sendError(400, "db must be 0 or 1"); return; }
        t.doubleBuf = v != 0;
    }
    if (!DisplayTuning::combinationOk(t.speedMHz, t.latch)) { sendError(400, "Latch 0 ist mit 16/20 MHz nicht erlaubt (Panel bleibt sonst dunkel)"); return; }
    t.save(settings_);
    restartAt_ = millis() + 800;
    if (restartAt_ == 0) restartAt_ = 1;
    sendJson(200, "{\"ok\":true,\"restart\":true}");
}

void WebController::setDemo() {
    DemoConfig cfg = scenes_.demo();
    const char* secArg = server_.hasArg("sec") ? "sec" : "interval";
    if (server_.hasArg(secArg)) {
        long sec = 0;
        if (!parseInt(server_.arg(secArg), 3, 3600, sec)) { sendError(400, "interval must be 3-3600 s"); return; }
        cfg.intervalSec = (uint16_t)sec;
    }
    if (server_.hasArg("ids")) {
        const String ids = server_.arg("ids");
        if (!validIdList(ids)) { sendError(400, "invalid scene list"); return; }
        cfg.ids = ids;
    }
    const char* onArg = server_.hasArg("on") ? "on" : "enabled";
    if (server_.hasArg(onArg)) cfg.enabled = server_.arg(onArg) == "1";
    scenes_.setDemo(cfg);
    settings_.saveDemo(cfg.enabled, cfg.intervalSec, cfg.ids);
    if (cfg.enabled) {
        IScene* cur = scenes_.active();
        if (cur == nullptr || !scenes_.demoIncludes(cur->id())) {
            for (uint8_t i = 0; i < scenes_.count(); ++i) {
                IScene* s = scenes_.at(i);
                if (scenes_.demoIncludes(s->id()) && s->available()) { scenes_.activate(s->id()); break; }
            }
        }
    }
    sendJson(200, "{\"ok\":true}");
}

void WebController::wifiJson(String& json) {
    if (scanning_) {
        const String res = network_.scanJson();   // "null" solange der Scan laeuft
        if (res != "null") { networks_ = res; scanning_ = false; }
    }
    json = String("{\"connected\":") + (network_.connected() ? "true" : "false") +
           ",\"ap\":" + (network_.apActive() ? "true" : "false") +
           ",\"ssid\":\"" + jsonEscape(network_.ssid()) + "\",\"ip\":\"" + network_.ipAddress() +
           "\",\"rssi\":" + String(network_.rssi()) +
           ",\"scanning\":" + (scanning_ ? "true" : "false") + ",\"networks\":" + networks_ + "}";
}

void WebController::getWifi() {
    String json;
    wifiJson(json);
    sendJson(200, json);
}

void WebController::postWifi() {
    if (!network_.saveCredentials(server_.arg("ssid"), server_.arg("pass"))) {
        sendError(400, "SSID 1-32 chars, password empty or 8-63 chars");
        return;
    }
    sendJson(200, "{\"ok\":true}");
}

// Deletes the stored WiFi credentials and restarts: the board then comes up with the setup AP only.
void WebController::postWifiForget() {
    network_.forgetCredentials();
    restartAt_ = millis() + 800;
    if (restartAt_ == 0) restartAt_ = 1;
    sendJson(200, "{\"ok\":true,\"restart\":true}");
}

void WebController::postWifiScan() {
    network_.startScan();
    scanning_ = true;
    sendJson(200, "{\"ok\":true}");
}

void WebController::getWifiScan() { getWifi(); }
