#include "NetworkService.h"

#include <WiFi.h>
#include <ESPmDNS.h>

#if __has_include("../secrets.h")
  #include "../secrets.h"
#endif
#ifndef WIFI_SSID
  #define WIFI_SSID ""
#endif
#ifndef WIFI_PASS
  #define WIFI_PASS ""
#endif
#ifndef AP_PASS
  #define AP_PASS "ledmatrix"   // min. 8 characters for WPA2
#endif

#include "board/BoardConfig.h"

namespace {
constexpr unsigned long AP_AFTER_MS = 25000UL;       // no connection -> open setup AP
constexpr unsigned long AP_LINGER_MS = 30000UL;      // keep AP after a successful connect
}

void NetworkService::loadCredentials() {
    ssid_ = settings_.getString("wifi.ssid", "");
    pass_ = settings_.getString("wifi.pass", "");
    if (ssid_.length() == 0) { ssid_ = WIFI_SSID; pass_ = WIFI_PASS; }
}

void NetworkService::connectStation() {
    connectStartMs_ = millis();
    lastReconnectAttemptMs_ = connectStartMs_;
    if (ssid_.length() == 0) return;
    WiFi.begin(ssid_.c_str(), pass_.c_str());
    Serial.printf("[wifi] connecting to %s\n", ssid_.c_str());
}

void NetworkService::begin() {
    loadCredentials();
    WiFi.persistent(false);   // credentials live in our NVS namespace, not the WiFi driver's
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    connectStartMs_ = millis();
    if (ssid_.length() == 0) {
        Serial.println("[wifi] no credentials configured; starting setup access point");
        startAp();
        return;
    }
    connectStation();
}

void NetworkService::startAp() {
    if (apRunning_) return;
    WiFi.mode(WIFI_AP_STA);
    apRunning_ = WiFi.softAP(AP_SSID, AP_PASS);
    Serial.printf("[wifi] setup AP \"%s\" %s, http://%s/\n", AP_SSID,
                  apRunning_ ? "started" : "FAILED", WiFi.softAPIP().toString().c_str());
}

void NetworkService::stopAp() {
    if (!apRunning_) return;
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    apRunning_ = false;
    Serial.println("[wifi] setup AP stopped");
}

void NetworkService::update() {
    const unsigned long now = millis();
    if (WiFi.status() == WL_CONNECTED) {
        if (connectedSinceMs_ == 0) connectedSinceMs_ = now ? now : 1;
        if (apRunning_ && now - connectedSinceMs_ >= AP_LINGER_MS) stopAp();
        // MDNS.begin() can fail temporarily; retry slowly, never every loop pass.
        if (!mdnsStarted_ && (lastMdnsAttemptMs_ == 0 || now - lastMdnsAttemptMs_ >= 5000UL)) {
            lastMdnsAttemptMs_ = now;
            mdnsStarted_ = MDNS.begin(BoardConfig::MDNS_HOSTNAME);
            if (mdnsStarted_) {
                MDNS.addService("http", "tcp", 80);
                Serial.printf("[wifi] web UI: http://%s.local/ (%s)\n",
                              BoardConfig::MDNS_HOSTNAME, WiFi.localIP().toString().c_str());
            }
        }
        return;
    }

    if (mdnsStarted_) MDNS.end();
    mdnsStarted_ = false;
    lastMdnsAttemptMs_ = 0;
    connectedSinceMs_ = 0;

    if (!apRunning_ && now - connectStartMs_ >= AP_AFTER_MS) startAp();
    if (ssid_.length() > 0 && now - lastReconnectAttemptMs_ >= 15000UL) {
        lastReconnectAttemptMs_ = now;
        Serial.println("[wifi] reconnecting...");
        WiFi.disconnect();
        WiFi.begin(ssid_.c_str(), pass_.c_str());
    }
}

bool NetworkService::saveCredentials(const String& ssid, const String& pass) {
    if (ssid.length() == 0 || ssid.length() > 32 || pass.length() > 63) return false;
    if (pass.length() > 0 && pass.length() < 8) return false;
    settings_.putString("wifi.ssid", ssid);
    settings_.putString("wifi.pass", pass);
    ssid_ = ssid;
    pass_ = pass;
    connectedSinceMs_ = 0;
    WiFi.disconnect();
    connectStation();
    return true;
}

void NetworkService::startScan() {
    if (WiFi.scanComplete() != WIFI_SCAN_RUNNING) WiFi.scanNetworks(true);
}

String NetworkService::scanJson() {
    const int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING) return "null";
    String out = "[";
    for (int i = 0; i < n && i < 20; ++i) {
        if (i) out += ",";
        String name = WiFi.SSID(i);
        name.replace("\\", "\\\\");
        name.replace("\"", "\\\"");
        out += "{\"ssid\":\"" + name + "\",\"rssi\":" + String(WiFi.RSSI(i)) +
               ",\"open\":" + (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "true" : "false") + "}";
    }
    out += "]";
    WiFi.scanDelete();
    return out;
}

bool NetworkService::connected() const { return WiFi.status() == WL_CONNECTED; }
String NetworkService::ipAddress() const {
    if (connected()) return WiFi.localIP().toString();
    return apRunning_ ? WiFi.softAPIP().toString() : String("offline");
}
String NetworkService::ssid() const { return ssid_; }
int NetworkService::rssi() const { return connected() ? WiFi.RSSI() : 0; }
