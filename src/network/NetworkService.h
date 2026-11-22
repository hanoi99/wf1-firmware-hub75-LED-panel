#pragma once

#include <Arduino.h>
#include "storage/SettingsStore.h"

// Station-mode WiFi with credentials from NVS (or secrets.h as a default) and an
// automatic setup access point when no network can be reached.
class NetworkService {
public:
    explicit NetworkService(SettingsStore& settings) : settings_(settings) {}
    void begin();
    void update();

    bool connected() const;
    bool apActive() const { return apRunning_; }
    int apClients() const;
    String stateText() const;       // short status for the on-panel diagnostics
    void forgetCredentials();       // delete stored WiFi credentials
    String ipAddress() const;
    String ssid() const;
    int rssi() const;

    // Stores new credentials and reconnects. Returns false for invalid input.
    bool saveCredentials(const String& ssid, const String& pass);
    void startScan();
    // JSON array of networks, or "null" while a scan is still running.
    String scanJson();

    static constexpr const char* AP_SSID = "LED-Matrix-Setup";

private:
    void loadCredentials();
    void startAp();
    void stopAp();
    void connectStation();

    SettingsStore& settings_;
    String ssid_, pass_;
    unsigned long lastReconnectAttemptMs_ = 0;
    unsigned long lastMdnsAttemptMs_ = 0;
    unsigned long connectStartMs_ = 0;
    unsigned long connectedSinceMs_ = 0;
    bool mdnsStarted_ = false;
    bool apRunning_ = false;
};
