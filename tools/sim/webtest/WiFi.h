#pragma once
#include <Arduino.h>
typedef enum { WL_IDLE_STATUS = 0, WL_NO_SSID_AVAIL = 1, WL_CONNECTED = 3, WL_DISCONNECTED = 6 } wl_status_t;
typedef enum { WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3 } wifi_mode_t;
typedef enum { WIFI_AUTH_OPEN = 0 } wifi_auth_mode_t;
#define WIFI_SCAN_RUNNING (-1)
#define WIFI_SCAN_FAILED (-2)
typedef enum { WIFI_POWER_11dBm = 44, WIFI_POWER_19_5dBm = 78 } wifi_power_t;
class IPAddress { public: String toString() const { return String(); } };
class WiFiClient { public: };
class WiFiClass {
public:
    bool mode(wifi_mode_t m) { return true; }
    bool setSleep(bool e) { return true; }
    bool setHostname(const char* h) { return true; }
    String macAddress() { return String(); }
    void persistent(bool p) {}
    void setAutoReconnect(bool a) {}
    wl_status_t begin(const char* ssid, const char* pass = nullptr) { return WL_IDLE_STATUS; }
    bool disconnect(bool wifioff = false, bool eraseap = false) { return true; }
    wl_status_t status() { return WL_CONNECTED; }
    IPAddress localIP() { return IPAddress(); }
    String SSID() const { return String("Heimnetz"); }
    String SSID(uint8_t i) { return i ? String("Gast \"WLAN\"") : String("Heimnetz"); }
    int32_t RSSI() { return 0; }
    int32_t RSSI(uint8_t i) { return -50 - 10 * i; }
    wifi_auth_mode_t encryptionType(uint8_t i) { return WIFI_AUTH_OPEN; }
    bool softAP(const char* ssid, const char* pass = nullptr, int ch = 1, int hidden = 0, int max = 4) { return true; }
    IPAddress softAPIP() { return IPAddress(); }
    bool softAPdisconnect(bool wifioff = false) { return true; }
    int softAPgetStationNum() { return 0; }
    bool setTxPower(wifi_power_t p) { return true; }
    int16_t scanNetworks(bool async = false, bool show_hidden = false) { return 0; }
    int16_t scanComplete() { return 2; }
    void scanDelete() {}
};
extern WiFiClass WiFi;
