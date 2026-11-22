#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "app/SceneManager.h"
#include "display/DisplayDriver.h"
#include "network/NetworkService.h"
#include "scenes/ManualTextScene.h"
#include "services/TimeService.h"
#include "storage/SettingsStore.h"

class WebController {
public:
    WebController(SceneManager& scenes, ManualTextScene& textScene, DisplayDriver& display,
                  SettingsStore& settings, NetworkService& network, TimeService& time)
        : scenes_(scenes), textScene_(textScene), display_(display),
          settings_(settings), network_(network), time_(time) {}
    void begin();
    void update();

private:
    WebServer server_{80};
    SceneManager& scenes_;
    ManualTextScene& textScene_;
    DisplayDriver& display_;
    SettingsStore& settings_;
    NetworkService& network_;
    TimeService& time_;

    bool authorized();
    void route(const char* uri, HTTPMethod method, void (WebController::*handler)());
    void sendJson(int statusCode, const String& body);
    void sendError(int statusCode, const char* message);
    void serveFile();
    void sendStatus();
    void sendState();
    void setScene();
    void setOption();
    void setText();
    void setBrightness();
    void setDemo();
    void postDisplay();
    void getWifi();
    void postWifi();
    void postWifiScan();
    void postWifiForget();
    void getWifiScan();
    void appendScene(String& json, IScene& scene);
    void wifiJson(String& json);

    uint32_t restartAt_ = 0;
    bool scanning_ = false;
    String networks_ = "[]";
};
