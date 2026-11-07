#include "IssService.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <stdlib.h>

namespace {
constexpr const char* ISS_URL = "http://api.open-notify.org/iss-now.json";
constexpr uint32_t POLL_MS = 15000;
constexpr uint32_t MAX_BACKOFF_MS = 120000;

// Reads a number following `"key"` in a small JSON body (value may be quoted).
bool readNumber(const String& body, const char* key, float& out) {
    const String needle = String("\"") + key + "\"";
    int i = body.indexOf(needle);
    if (i < 0) return false;
    i = body.indexOf(':', i);
    if (i < 0) return false;
    ++i;
    while (i < (int)body.length() && (body[i] == ' ' || body[i] == '"')) ++i;
    const char* start = body.c_str() + i;
    char* end = nullptr;
    const float v = strtof(start, &end);
    if (end == start) return false;
    out = v;
    return true;
}
}  // namespace

void IssService::begin() {
    xTaskCreate(taskEntry, "iss", 6144, this, 1, nullptr);
}

void IssService::taskEntry(void* self) {
    static_cast<IssService*>(self)->run();
}

bool IssService::snapshot(IssOrbit& out) const {
    portENTER_CRITICAL(&mux_);
    out = orbit_;
    portEXIT_CRITICAL(&mux_);
    return out.hasFix();
}

bool IssService::fetchOnce() {
    WiFiClient client;
    HTTPClient http;
    http.setTimeout(4000);
    http.setConnectTimeout(4000);
    if (!http.begin(client, ISS_URL)) return false;
    const int code = http.GET();
    bool ok = false;
    if (code == 200) {
        const String body = http.getString();
        float lat = 0, lon = 0;
        if (readNumber(body, "latitude", lat) && readNumber(body, "longitude", lon) &&
            lat >= -90 && lat <= 90 && lon >= -180 && lon <= 180) {
            portENTER_CRITICAL(&mux_);
            orbit_.addFix(lat, lon, millis());
            portEXIT_CRITICAL(&mux_);
            lastOkMs_ = millis();
            ok = true;
        }
    }
    http.end();
    return ok;
}

void IssService::run() {
    uint32_t waitMs = 2000;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(waitMs));
        if (!wanted_ || WiFi.status() != WL_CONNECTED) { waitMs = 1000; continue; }
        if (fetchOnce()) {
            failures_ = 0;
            waitMs = POLL_MS;
        } else {
            if (failures_ < 250) failures_ = failures_ + 1;
            const uint8_t f = failures_;  // volatile -> lokale Kopie
            const uint32_t backoff = POLL_MS * (1u << (f < 3 ? f : 3));
            waitMs = backoff < MAX_BACKOFF_MS ? backoff : MAX_BACKOFF_MS;
            Serial.printf("[iss] fetch failed (%u), retry in %lu ms\n", failures_, (unsigned long)waitMs);
        }
    }
}
