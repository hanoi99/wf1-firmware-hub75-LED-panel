#!/bin/bash
# Baut tools/sim/webtest -> build/web_contract (echter WebController + Szenen auf dem Host)
set -e
cd "$(dirname "$0")/../../.."
mkdir -p build
G=lib/Adafruit-GFX-Library
g++ -std=gnu++17 -DARDUINO=100 -DWF1=1 -O1 -Wno-unused-variable -Wno-sign-compare -Wno-unused-function \
  -include tools/sim/stubs/freertos_stub.h -Itools/sim/webtest -Itools/sim/stubs -Itools/sim/shim -Isrc -I$G -I$G/Fonts \
  tools/sim/webtest/web_contract.cpp tools/sim/mock/*.cpp $G/Adafruit_GFX.cpp \
  src/app/SceneManager.cpp src/display/DisplayDriver.cpp src/display/TextRenderer.cpp \
  src/scenes/*.cpp src/services/SolarCalc.cpp src/storage/SettingsStore.cpp \
  src/network/NetworkService.cpp src/network/WebController.cpp -o build/web_contract
echo "built build/web_contract"
