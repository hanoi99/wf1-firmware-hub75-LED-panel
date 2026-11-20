#!/bin/bash
# Syntax-/Typpruefung der Firmware-Dateien, die der Simulator nicht abdeckt (WLAN, Webserver,
# FreeRTOS-Task, RTC), gegen kleine Stub-Header. Ersetzt KEINEN echten PlatformIO-Build,
# faengt aber Tippfehler und falsche Schnittstellen zwischen den eigenen Klassen ab.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
rc=0
python3 "$ROOT/tools/check_includes.py" || rc=1
for f in main.cpp network/WebController.cpp network/NetworkService.cpp services/IssService.cpp \
         services/TimeService.cpp services/SolarCalc.cpp services/IssOrbit.cpp storage/SettingsStore.cpp; do
  out=$(g++ -std=c++17 -fsyntax-only -Wall -Wextra -Wno-unused-parameter -DARDUINO=100 -DWF1=1 \
      -include "$ROOT/tools/sim/stubs/freertos_stub.h" \
      -I"$ROOT/src" -I"$ROOT/tools/sim/stubs" -I"$ROOT/tools/sim/shim" -I"$ROOT/lib/Adafruit-GFX-Library" \
      -I"$ROOT/lib/I2C_BM8563/src" "$ROOT/src/$f" 2>&1 | grep -E "error|warning")
  if [ -n "$out" ]; then echo "== $f"; echo "$out" | head -10; rc=1; fi
done
[ $rc -eq 0 ] && echo "fwcheck: OK"
exit $rc
