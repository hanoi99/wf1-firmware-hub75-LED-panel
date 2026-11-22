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

## 0.2.4
- ISS: API-`timestamp` wird ausgewertet; veraltete Positionen werden auf ihren wahren Zeitpunkt zurückdatiert.
- ISS: Ohne bekannte Flugrichtung keine Hochrechnung mehr (Marker bleibt auf der letzten echten Position, Status „ALT“).
- ISS: Serial-Log `[iss] fix lat=… lon=… api_age=… s` und `[iss] HTTP <code>`; Alter der Daten in `/api/state` (`note`).
- Bahnmodell gegen unabhängige Rechnung geprüft (Abweichung 0,00°).

## 0.2.5
- ISS: Hauptquelle jetzt wheretheiss.at (HTTPS, `setInsecure()`), Open Notify nur noch Rückfall. `-DISS_USE_HTTPS=0` schaltet auf reines HTTP zurück (spart Flash/RAM).
- Alle Texte auf dem Panel auf Englisch (Wochentage/Monate, WAIT/EST/OLD, Over land/sea, Locating, Need WiFi, ISO-Datum). Weboberfläche bleibt deutsch.
- Standardtext „Hello World“ (gespeicherte Texte bleiben unverändert).

## 0.2.6-dev - Flicker tuning

- New web UI card "Panel-Timing (Flackern)": pixel clock (8/16/20 MHz), latch blanking, clock phase, double buffer on/off. Stored in NVS (`dsp.*`), applied at boot, the board restarts after saving.
- Defaults changed: 16 MHz pixel clock and latch blanking 8 (was 20 MHz / 4). Overall refresh stays above ~100 Hz.
- Status/serial now show whether double buffering is really active, the requested vs. actual mode and the refresh rate calculated by the library.
- Boot fallback chain: requested settings -> single buffer -> conservative defaults, so the panel always comes up.
- `POST /api/display` (spd, lat, clk, db, reset=1).

## 0.2.7-dev

- Build fix: Arduino core 3.3.4 has no `WiFiClientSecure.h`; IssService now uses `NetworkClientSecure.h` / `NetworkClient`.

## 0.2.8-dev

- platformio.ini: `lib_deps = NetworkClientSecure` (the LDF did not pull the TLS library in on its own). Fallback: remove it and build with `-DISS_USE_HTTPS=0`.

## 0.2.9-dev

- ISS: default is Open Notify over plain HTTP only (no TLS library => no build dependency). HTTPS source is optional via -DISS_USE_HTTPS=1 + lib_deps NetworkClientSecure.

## 0.2.10-dev

- WiFi: setup AP "LED-Matrix-Setup" (fixed channel 1) now reliably appears ~25 s after a failed connect. Auto-reconnect of the driver is off; while the AP is up the home network is retried only every 60 s and never while a client is connected to the AP.
- Display tuning: new "panel driver chip" option (Standard / FM6126A / FM6124 / ICN2038S), `drv` in /api/display. Wrong driver is a common cause of a misbehaving outermost LED column.

## 0.2.11-dev

- WiFi: placeholder SSID "your-ssid" from secrets.h is ignored (setup AP starts immediately). Station connect attempts are stopped before the setup AP starts (channel hopping blocked client joins). Serial log prints the AP password.

## 0.2.12-dev

- WiFi: while the setup AP is up the radio runs in pure AP mode (no station interface, so no channel hopping). The home network is tried in 20 s trials every 90 s, only if nobody is connected to the AP. Saving credentials from the AP starts such a trial. Reverted the fixed-channel / auto-reconnect experiments of 0.2.10.

## 0.2.13-dev

- WiFi: setup AP stays untouched once it is up. The home network is retried only every 10 min in a 15 s trial and only if nobody used the AP for 10 min (before: every 90 s, which broke joins).
- New persistent "setup-only" mode (button "Nur Setup-WLAN nutzen" in the WLAN card, `POST /api/wifi/mode m=ap|sta`): the home network is not tried at all, also after reboot. Saving new credentials switches it off again.
- Scanning from the setup AP temporarily enables the station interface for the scan only.

## 0.2.14-dev

- New "System-Info" scene (also shown automatically after every boot for up to 45 s, or 8 s once WiFi is connected): version, reset reason (POWER-ON / SOFTWARE / CRASH / WATCHDOG / BROWNOUT), unstable boots, uptime, WiFi state, AP clients, free heap. Diagnostics without a serial monitor. Never part of demo mode.

## 0.2.15-dev

- Setup AP runs with reduced TX power (11 dBm) to lower current peaks.

## 0.2.16-dev

- WiFi: NetworkService is back to the code of 0.2.3/0.2.5 (setup AP in AP+STA mode, which is known to work) with only small additions: placeholder SSID ignored, persistent "setup only" mode, radio reset (WIFI_OFF) before the AP starts after a failed station attempt, and no automatic home-network retries while the AP is up. Removed the pure-AP mode, reduced TX power and 10-minute trials of 0.2.12-0.2.15.

## 0.2.17-dev

- WiFi: NetworkService is the unmodified code of v0.2.3 (identical to 0.2.5). Only additions: password in the AP log line, read-only helpers for the on-panel info, and `forgetCredentials()`.
- New button "WLAN vergessen & neu starten" (`POST /api/wifi/forget`): deletes the stored credentials and restarts. Removed the setup-only mode.

## 0.2.18-dev - WiFi fix: RAM

- Root cause of the unreachable setup AP: the display driver left only ~6 KB of heap (8 bit colour + double buffer = ~170 KB of DMA buffers/descriptors), so WiFi/lwIP could not accept clients. Default colour depth is now 6 bit (double buffer still on, ~80 KB less RAM), configurable 4-8 bit in the Panel-Timing card.
- Display init checks the remaining heap (must stay >= 80 KB) and otherwise falls back: single buffer, then 5 and 4 bit. Serial/Panel/UI show colour depth and free heap.
- Placeholder SSID from secrets.h.example is ignored again.

## 0.2.19-dev - more free RAM

- Big scene buffers (Fire/Plasma/Starfield/Mandelbrot, ~15 KB) now share one 5 KB block (only one scene runs at a time): ~10 KB more heap.
- New option "min. refresh rate" (default 100 Hz): the HUB75 library then uses a higher LSB/MSB transition and far fewer DMA descriptors (~12 KB less at 8 MHz) with no visible loss at 8 bit.
- LittleFS is no longer mounted (the web UI is embedded): a few KB.
- Defaults are now the values that worked on the real panel: 8 MHz, latch 12, 8 bit, double buffer. Panel driver option removed (no effect on this panel).

## 0.2.20-dev - scenes

- Removed: ISS tracker (service, orbit model, API task) and ASCII-art scene.
- World map: new view "Nur Karte (groß)" (default): map stretched vertically to 52 rows, smooth terminator curve, sun and home marker, no clock. The old view with clock is still selectable ("Ansicht").
- Campfire: flames now start on top of the wood, not below it. New wood option "Holzstapel" (two base logs, tipi of two leaning logs, one on top, embers); "Zwei Scheite" and "Kein Holz" remain.
- New scene "Herbstbaum am See" (autumn tree on a headland, mirror image with ripples, falling leaves; moods: sunset / morning / sunny day).
- New scene "Mondinsel" (island with palm, moon in the corner with glitter path, stars, clouds, waves).
- Both new scenes use almost no RAM (shared 5 KB scratch block / fully procedural).

## 0.2.21-dev
- Neue Szene "Hund mit Ball" (id `dog`): kleiner blonder Hund mit spitzen Ohren holt auf einer Wiese einen Ball, bringt ihn zurück, wird erneut geworfen (Endlosschleife). Optionen: Wolken, Tempo.
- Neue Szene "Nachtfahrt" (id `drive`): dunkelgrüne Limousine diagonal von vorn, Straße und Mittelstreifen laufen, Straßenlaternen ziehen vorbei. Optionen: Tempo, Laternen.
- Beide Szenen rein prozedural, kein zusätzlicher RAM-Bedarf (kein Markenlogo; generische Limousine, kein exakter Octavia-Nachbau).

## 0.2.22-dev
- Nachtfahrt: Auto neu gezeichnet nach klassischer Limousinen-Form (flache breite Haube, schräge Frontscheibe, große Doppelscheinwerfer, breiter Grill, Stoßfänger mit Lufteinlass und Kennzeichen), ohne Markenlogo.

## 0.2.23-dev
- Nachtfahrt: neue Option "Ansicht" (Standard: Seitenansicht). Dunkelgrüne Limousine im Profil, Straße/Mittelstreifen laufen von links nach rechts, Räder drehen mit passender Geschwindigkeit, Skyline und Hecke mit Parallaxe, Laternen mit Lichtkegel. Frontansicht bleibt wählbar.

## 0.2.24-dev
- Nachtfahrt Seitenansicht: Räder und Felgen kleiner (bessere Proportionen).

## 0.2.25-dev
- Nachtfahrt Seitenansicht: Raeder wieder in der groesseren Variante, Motorhaube hoeher gezogen, sodass sie ueber dem Vorderrad klar sichtbar bleibt.

## 0.2.26-dev
- Nachtfahrt Seitenansicht: Motorhaube eine Pixelreihe höher.

## 0.2.27-dev
- Nachtfahrt Seitenansicht: Motorhaube steigt sanft bis zur Frontscheibe an (natürlichere Silhouette).

## 0.2.28-dev
- Lagerfeuer: Bereich unter dem Holz wird jetzt jedes Bild geleert (keine Reste der vorherigen Szene mehr beim Szenenwechsel).
- Panel-Timing Schutz: Jede Änderung ist ein "Probelauf" (`dsp.try`). Läuft das Board danach keine 30 s stabil (Absturz/Hänger/Strom weg), werden beim nächsten Start automatisch die Standardwerte (8 MHz, Latch 12, ...) geladen – kein Flash-Erase mehr nötig.
- Latch 0 zusammen mit 16/20 MHz wird abgelehnt (web) bzw. beim Laden auf Latch 12 korrigiert.
