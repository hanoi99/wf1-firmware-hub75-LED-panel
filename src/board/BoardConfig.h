#pragma once

#if !defined(WF1)
  #error "This firmware build currently targets the Huidu WF1. Define WF1=1."
#endif

#include <Arduino.h>
#include "hd-wf1-esp32s2-config.h"

namespace BoardConfig {
constexpr int PANEL_WIDTH = 64;
constexpr int PANEL_HEIGHT = 64;
constexpr int PANEL_CHAIN = 1;

// true = flicker-free double buffering (needs ~2x DMA RAM; falls back automatically).
constexpr bool USE_DOUBLE_BUFFER = true;
// Shift-register chip of the panel: 0 = plain SHIFTREG, 1 = FM6124, 2 = FM6126A, 3 = ICN2038S.
// If the panel stays dark or shows garbage, try 2 and then 3.
constexpr uint8_t PANEL_DRIVER = 0;

constexpr uint8_t DEFAULT_BRIGHTNESS = 96;
constexpr uint8_t MAX_BRIGHTNESS = 180;  // Conservative ceiling; raise only after power validation.

// Home marker on the world map (default Berlin). Change to your own position.
constexpr float HOME_LAT = 52.52f;
constexpr float HOME_LON = 13.40f;

constexpr const char* MDNS_HOSTNAME = "ledmatrix";
constexpr const char* TIMEZONE = "CET-1CEST,M3.5.0,M10.5.0/3";
constexpr const char* NTP_SERVER_1 = "time.cloudflare.com";
constexpr const char* NTP_SERVER_2 = "pool.ntp.org";
}
