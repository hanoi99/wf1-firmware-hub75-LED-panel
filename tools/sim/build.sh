#!/bin/bash
# Baut den Host-Simulator (g++). Aufruf: bash tools/sim/build.sh  -> build/sim
set -e
cd "$(dirname "$0")/../.."
mkdir -p build
G=lib/Adafruit-GFX-Library
g++ -std=gnu++17 -DARDUINO=100 -DWF1=1 -O1 -Wall -Wno-unused-variable -Wno-sign-compare -Wno-unused-function -o build/sim \
  -include tools/sim/stubs/freertos_stub.h -Itools/sim/shim -Isrc -I$G -I$G/Fonts \
  tools/sim/sim_main.cpp tools/sim/mock/*.cpp $G/Adafruit_GFX.cpp \
  src/app/SceneManager.cpp src/display/DisplayDriver.cpp src/display/TextRenderer.cpp \
  $(ls src/scenes/*.cpp | grep -v SysInfoScene) src/services/SolarCalc.cpp src/storage/SettingsStore.cpp
echo "built build/sim"
