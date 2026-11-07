#include "WebController.h"

#include <LittleFS.h>
#include <cctype>
#include <cstdlib>

#include "app/OptionKeys.h"
#include "board/BoardConfig.h"

#if __has_include("../secrets.h")
  #include "../secrets.h"
#endif
#ifndef WEB_PASS
  #define WEB_PASS ""
#endif

#ifndef APP_VERSION
#define APP_VERSION "0.2.0-dev"
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
    route("/api/scene", HTTP_POST, &WebController::setScene);
    route("/api/option", HTTP_POST, &WebController::setOption);
    route("/api/text", HTTP_POST, &WebController::setText);
    route("/api/brightness", HTTP_POST, &WebController::setBrightness);
    route("/api/demo", HTTP_POST, &WebController::setDemo);
    route("/api/wifi", HTTP_GET, &WebController::getWifi);
    route("/api/wifi", HTTP_POST, &WebController::postWifi);
    route("/api/wifi/scan", HTTP_POST, &WebController::postWifiScan);
    route("/api/wifi/scan", HTTP_GET, &WebController::getWifiScan);
    server_.onNotFound([this]() { if (authorized()) serveFile(); });
    server_.begin();
    Serial.println("[web] HTTP server started on port 80");
}

void WebController::update() { server_.handleClient(); }

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
    if (path != "/index.html" && path != "/style.css" && path != "/app.js") {
        server_.send(404, "text/plain; charset=utf-8", "Not found");
        return;
    }
    File file = LittleFS.open(path, "r");
    if (!file) {
        server_.send(404, "text/plain; charset=utf-8",
                     "Web UI files missing. Run: pio run -t uploadfs");
        return;
    }
    server_.sendHeader("Cache-Control", "no-cache");
    server_.sendHeader("X-Content-Type-Options", "nosniff");
    server_.streamFile(file, mimeType(path));
    file.close();
}

void WebController::appendScene(String& json, IScene& scene) {
    json += "{\"id\":\"" + String(scene.id()) + "\",\"title\":\"" + jsonEscape(scene.title()) +
            "\",\"cat\":\"" + scene.category() + "\",\"avail\":" + (scene.available() ? "true" : "false") +
            ",\"options\":[";
    for (uint8_t i = 0; i < scene.optionCount(); ++i) {
        const SceneOption* o = scene.option(i);
        if (i) json += ",";
        const char* type = o->type == OptType::Select ? "select" : o->type == OptType::Range ? "range"
                         : o->type == OptType::Toggle ? "toggle" : "color";
        json += "{\"key\":\"" + String(o->key) + "\",\"label\":\"" + jsonEscape(o->label) +
                "\",\"type\":\"" + type + "\",\"min\":" + String(o->min) + ",\"max\":" + String(o->max) +
                ",\"value\":" + String(o->value);
        if (o->choices) json += ",\"choices\":\"" + jsonEscape(o->choices) + "\"";
        json += "}";
    }
    json += "]}";
}

void WebController::sendStatus() {
    String json;
    json.reserve(5200);
    json += "{\"version\":\"" APP_VERSION "\"";
    json += ",\"uptime\":" + String(millis() / 1000);
    json += ",\"heap\":" + String(ESP.getFreeHeap());
    json += ",\"minHeap\":" + String(ESP.getMinFreeHeap());
    json += ",\"ip\":\"" + network_.ipAddress() + "\"";
    json += ",\"wifi\":" + String(network_.connected() ? "true" : "false");
    json += ",\"ap\":" + String(network_.apActive() ? "true" : "false");
    json += ",\"rssi\":" + String(network_.rssi());
    json += ",\"timeValid\":" + String(time_.isValid() ? "true" : "false");
    json += ",\"doubleBuffer\":" + String(display_.doubleBuffered() ? "true" : "false");
    json += ",\"activeScene\":\"" + String(scenes_.activeId()) + "\"";
    json += ",\"activeTitle\":\"" + jsonEscape(scenes_.activeTitle()) + "\"";
    json += ",\"brightness\":" + String(display_.brightness());
    json += ",\"maxBrightness\":" + String(BoardConfig::MAX_BRIGHTNESS);
    const DemoConfig& demo = scenes_.demo();
    json += ",\"demo\":{\"enabled\":" + String(demo.enabled ? "true" : "false") +
            ",\"interval\":" + String(demo.intervalSec) + ",\"ids\":\"" + jsonEscape(demo.ids) + "\"}";
    json += ",\"text\":\"" + jsonEscape(textScene_.text()) + "\"";
    json += ",\"scenes\":[";
    for (uint8_t i = 0; i < scenes_.count(); ++i) {
        if (i) json += ",";
        appendScene(json, *scenes_.at(i));
    }
    json += "]}";
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

void WebController::setDemo() {
    DemoConfig cfg = scenes_.demo();
    if (server_.hasArg("interval")) {
        long sec = 0;
        if (!parseInt(server_.arg("interval"), 3, 3600, sec)) { sendError(400, "interval must be 3-3600 s"); return; }
        cfg.intervalSec = (uint16_t)sec;
    }
    if (server_.hasArg("ids")) {
        const String ids = server_.arg("ids");
        if (!validIdList(ids)) { sendError(400, "invalid scene list"); return; }
        cfg.ids = ids;
    }
    if (server_.hasArg("enabled")) cfg.enabled = server_.arg("enabled") == "1";
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

void WebController::getWifi() {
    sendJson(200, String("{\"connected\":") + (network_.connected() ? "true" : "false") +
                  ",\"ap\":" + (network_.apActive() ? "true" : "false") +
                  ",\"ssid\":\"" + jsonEscape(network_.ssid()) + "\",\"ip\":\"" + network_.ipAddress() +
                  "\",\"rssi\":" + String(network_.rssi()) + "}");
}

void WebController::postWifi() {
    if (!network_.saveCredentials(server_.arg("ssid"), server_.arg("pass"))) {
        sendError(400, "SSID 1-32 chars, password empty or 8-63 chars");
        return;
    }
    sendJson(200, "{\"ok\":true}");
}

void WebController::postWifiScan() {
    network_.startScan();
    sendJson(200, "{\"ok\":true}");
}

void WebController::getWifiScan() {
    sendJson(200, String("{\"networks\":") + network_.scanJson() + "}");
}
