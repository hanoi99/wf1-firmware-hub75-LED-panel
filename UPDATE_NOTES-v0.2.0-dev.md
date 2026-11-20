# 0.2.0-dev

Neu: Szenen-Optionen (generisch, in NVS gespeichert), Text-Renderer mit 10 Schriften/Umlauten/Laufrichtungen,
Weltzeituhr, ISS-Tracker, Lagerfeuer, Matrix-Regen, Sternenhimmel, Hyperraum, Plasma, ASCII-Art, Mandelbrot,
Demo-Modus, WLAN-Einrichtung mit AP-Fallback, optionales Web-Passwort, Boot-Loop-Schutz, Double-Buffering.

Geändert: `platformio.ini` nur minimal (Partitionstabelle explizit, `CORE_DEBUG_LEVEL`-Override entfernt).
Die in UPDATE_NOTES 0.1.4 erwähnten Zusatz-Environments standen nicht in der platformio.ini des ZIPs.

Nicht getestet: echter PlatformIO-Build und Hardware. Kein OTA aktiviert.

## 0.2.1 / 0.2.2
- Weboberfläche (gzip) in die Firmware eingebettet (`scripts/embed_ui.py`), LittleFS optional (`-DWEB_UI_FROM_FS`).
- Boot-Diagnose `[fs] ...`, `fs`-Feld in `/api/status`.
- Build-Fix: nicht benutzte `services/WorldMapView.*` entfernt (fehlerhafter Include `WorldMapData.h`).
- `tools/check_includes.py` (in `tools/fwcheck.sh`) löst alle `#include "..."` wie PlatformIO auf.
- Hinweis: `tools/gen_worldmap.py` erzeugt eine minimal andere Karte als die mitgelieferte `src/data/WorldMap.h`.

## 0.2.3
- Fix: Weboberfläche zeigte „Nicht erreichbar“, weil `data/app.js` und die Firmware-API unterschiedliche JSON-Formate nutzten
  (`sys.*`, `scene`, `demo.on/sec`, `opts` mit `k/l/t/v/c`, `/api/state`, WLAN-Liste in `/api/wifi`). Firmware angepasst.
- Demo-Liste: leer = alle Szenen, die UI zeigt jetzt alle Haken gesetzt.
- Neuer Test `tools/sim/webtest/` (`build.sh`, `node ui_test.js`): echter WebController + app.js gegen Fake-DOM.
