# WF1 LED Matrix Firmware

Eigene Firmware für den **Huidu WF1 (ESP32-S2)** und ein 64×64-HUB75-Panel. Das Projekt wird lokal mit PlatformIO entwickelt. Die Firmware startet jeweils genau eine ausgewählte Szene; diese bleibt aktiv, bis eine andere Szene gewählt wird. Die Auswahl und wichtige Einstellungen werden in NVS gespeichert.

> Stand `0.2.0-dev`: Alle Szenen aus dem Entwicklungsplan sind implementiert. Sie wurden im Host-Simulator (`tools/sim`) und per Syntaxprüfung getestet, **aber noch nicht auf echter Hardware und nicht mit PlatformIO gebaut** (siehe „Prüfung“).

## Funktionen

**Szenen** (immer genau eine aktiv, Auswahl wird gespeichert):

| ID | Szene | Einstellungen (Auswahl) |
|---|---|---|
| `text` | Text | 10 Schriftarten (inkl. Umlaute), Größe, Laufrichtung (←, →, ↑, ↓, statisch, pendeln), Geschwindigkeit, Effekt (blinken, pulsieren, Regenbogen, Verlauf), Farbe, Hintergrund; mehrzeilig |
| `clock` | Uhr | digital oder analog, 12/24 h, Sekunden, Farbe |
| `world` | Weltzeituhr | Weltkarte mit Tag/Nacht-Schattierung und Terminator-Kurve, Sonnenposition, Lokalzeit/UTC |
| `iss` | ISS-Tracker | Live-Position (Open-Notify-API), Bahnkurve, Tag/Nacht, Koordinaten, „über Land/Meer“ |
| `fire` | Lagerfeuer | Palette, Flammenhöhe, Holzscheite, Funken |
| `rain` | Matrix-Regen | Farbe, Geschwindigkeit |
| `stars` | Sternenhimmel | funkelnde Sterne, Milchstraße, Sternschnuppen |
| `warp` | Hyperraum | Geschwindigkeit, Schweif |
| `plasma` | Plasma | Palette, Geschwindigkeit, Skalierung |
| `ascii` | ASCII-Art | Plasma/Feuer/Tunnel aus Zeichen, Farbe |
| `mandel` | Mandelbrot | Zoom-Fahrt zu 4 Zielen, Palette |

**Demo-Modus:** wechselt automatisch zwischen frei wählbaren Szenen (Intervall einstellbar); Szenen ohne Daten (ISS ohne Netz, Uhr ohne Zeit) werden übersprungen. Eine manuell gewählte Szene beendet die Demo.

**Weiteres:** Weboberfläche aus LittleFS · WLAN-Einrichtung über die Weboberfläche mit Access-Point-Fallback `LED-Matrix-Setup` · optionales Web-Passwort · Double-Buffering (kein Flackern) · ISS-Abruf in eigenem Task (blockiert die Anzeige nicht) · Boot-Loop-Schutz (nach 3 Abstürzen in Folge startet eine sichere Szene) · alle Einstellungen in NVS · BM8563-RTC + NTP.

## Voraussetzungen

- VS Code mit PlatformIO
- Huidu WF1 (ESP32-S2)
- 64×64 HUB75-Panel
- Separate, ausreichend dimensionierte 5-V-Versorgung für das Panel; die Stromversorgung muss zur Matrix passen

## Lokal verwaltete Bibliotheken

Die vom Firmware-Code verwendeten Arduino-Bibliotheken liegen unter `lib/` und
werden nicht bei jedem Build von ihren Upstream-Git-Repositories geladen. Zum
Übergang werden die exakt aufgelösten Versionen aus dem bestehenden
PlatformIO-Cache übernommen:

```bash
bash scripts/vendor-dependencies.sh
```

Das Skript kopiert HUB75-DMA, BM8563 RTC, Adafruit GFX und Adafruit BusIO aus
`.pio/libdeps/huidu_hd_wf1` nach `lib/`. Deshalb muss mindestens ein
erfolgreicher PlatformIO-Build mit diesen Abhängigkeiten zuvor erfolgt sein.
Die Original-Manifeste, Copyright-Hinweise und Lizenzdateien werden mitkopiert.
Details stehen in `third_party/THIRD_PARTY_NOTICES.md`.

Die ESP32-Platform, das Arduino-Framework und der Compiler bleiben bewusst
separat und sind in `platformio.ini` auf den getesteten Platform-Commit
gepinnt. Das ist Build-Infrastruktur und keine zur Laufzeit geladene
Anwendungsbibliothek.

## Auf ein bestehendes geklontes Repository anwenden

Das ursprüngliche Refactor-Paket enthält die neue Projektstruktur auf der Archivebene (ohne `.git`). Im Stammverzeichnis deines bestehenden Klons entpacken und das Migrationsskript einmalig ausführen:

```bash
unzip -o wf1-project-refactor-v0.1.0.zip -d .
bash scripts/migrate-refactor.sh
```

Für nachfolgende Updates genügt es, das jeweilige Update-ZIP im Projektstamm zu entpacken. Das bestehende `.git`-Verzeichnis und deine lokale `src/secrets.h` bleiben dabei erhalten.

Das Skript übernimmt vorhandene Zugangsdaten aus `src/wf1wf2demo/secrets.h` nach `src/secrets.h`, sofern die neue Datei noch nicht existiert, und entfernt danach den alten, nicht mehr benötigten monolithischen Quellcodeordner. Die alte Hauptdatei bleibt zusätzlich unter `reference/legacy/` als nicht mitgebauter Vergleich erhalten.

## WLAN einrichten

Die Datei `src/secrets.h.example` nach `src/secrets.h` kopieren und Zugangsdaten eintragen:

```cpp
#define WIFI_SSID "dein-wlan"
#define WIFI_PASS "dein-passwort"
```

`src/secrets.h` ist in `.gitignore` eingetragen und wird nicht in Git übernommen. Ohne Zugangsdaten kompiliert die Firmware, bleibt aber offline.

## Kompilieren

```bash
pio run -e huidu_hd_wf1
```

## LittleFS-Weboberfläche übertragen

Die Webdateien liegen in `data/`. Nach der Firmware-Partitionierung müssen sie einmalig auf das LittleFS-Dateisystem übertragen werden:

```bash
pio run -e huidu_hd_wf1 -t uploadfs
```

Das Übertragen des Dateisystems ist getrennt vom Firmware-Upload. Beim lokalen Entwickeln können die Webdateien geändert und erneut mit `uploadfs` übertragen werden.

## Lokaler Zugriff

Im selben Netzwerk:

- `http://ledmatrix.local/` (mDNS; je nach Betriebssystem/Netzwerkunterstützung)
- alternativ `http://<IP-Adresse-des-WF1>/`

API (Formulardaten `application/x-www-form-urlencoded`):

- `GET /api/status` – alles: `scene`, `brightness`, `text`, `demo{on,sec,ids}`, `sys{ip,heap,rssi,up,...}`, `scenes[]` mit `opts[]`
- `GET /api/state` – leichtes Polling: `scene`, `demo`, `note`, `heap`, `rssi`, `up`
- `POST /api/scene` `id=<szene>`
- `POST /api/option` `scene`, `key`, `value`
- `POST /api/text` `text` (max. 240 Bytes, UTF-8)
- `POST /api/brightness` `value` 1–180
- `POST /api/demo` `on`, `sec`, `ids` (kommagetrennt, leer = alle)
- `GET/POST /api/wifi` (inkl. Netzliste), `POST /api/wifi/scan`

## Architektur

```text
src/
├── main.cpp                 # Zusammensetzen und Start der Anwendung
├── app/                     # SceneManager
├── board/                   # WF1-Pins und Displayparameter
├── display/                 # HUB75-DMA-Abstraktion
├── network/                 # WLAN und HTTP-Weboberfläche/API
├── scenes/                  # Display-Szenen (je begin/update/end + Optionen)
├── services/                # Zeit, ISS-Abruf/-Bahn, Sonnenstand, Weltkarte
└── storage/                 # Persistente Einstellungen (NVS)

data/
├── index.html
├── style.css
└── app.js
```

Jede Szene implementiert `begin()`, `update()` und `end()`. `SceneManager` aktiviert genau eine Szene. Neue Szenen werden registriert und müssen weder den Webserver noch die Displayinitialisierung kennen.

## Technische Hinweise

- Das Projekt zielt derzeit explizit auf den WF1. Die WF2-Pindatei bleibt als Referenz im Repository, wird aber nicht mitgebaut.
- Die Helligkeit ist bewusst auf 180/255 begrenzt. Vor einer Erhöhung muss die tatsächliche Panelversorgung sicher dimensioniert sein.
- Die Animationen sind auf 64×64 Pixel und die begrenzten Ressourcen des ESP32-S2 ausgelegt. Neue Szenen sollten zeitgesteuert und ohne blockierende `delay()`-Schleifen arbeiten.
- Die Uhrzeit wird intern als UTC geführt; `CET/CEST` wird nur für die lokale Anzeige angewendet.
- Die Weboberfläche hat in dieser Entwicklungsstufe noch keine Anmeldung. Sie ist nur für ein vertrauenswürdiges lokales Netzwerk gedacht; keine Portfreigabe ins Internet einrichten.
- Kein OTA-Update ist aktiviert. Firmware-Upload und Flashen am WF1 erfolgen erst nach gesonderter Prüfung der passenden WF1-Revision und des Backup-/Recovery-Wegs.

## Herkunft und Danksagung

Die WF1-Pinbelegung, die ursprüngliche HUB75-DMA-Integration und Teile der Geräteintegration wurden durch das Projekt [HD-WF1-WF2-LED-MatrixPanel-DMA](https://github.com/mrcodetastic/HD-WF1-WF2-LED-MatrixPanel-DMA) angeregt. Vielen Dank an MrCodetastic und die Maintainer der verwendeten Open-Source-Bibliotheken. Die frühere Demo-Datei liegt zur Referenz unter `reference/legacy/wf1wf2demo.cpp`; sie wird nicht kompiliert. Die vollständigen Drittanbieter-Hinweise, Versionen und Lizenzen stehen in `third_party/THIRD_PARTY_NOTICES.md`.

## Prüfung ohne Hardware

```bash
bash tools/sim/build.sh && build/sim selftest          # Szenen im Host-Simulator
python3 tools/sim/render_sheet.py out.png work "Feuer|fire|6000|3000"   # Bilder rendern
bash tools/fwcheck.sh                                  # Typprüfung WLAN/Web/Tasks gegen Stubs
bash tools/sim/webtest/build.sh && node tools/sim/webtest/ui_test.js   # Weboberfläche gegen echten WebController
python3 tools/gen_worldmap.py ne_110m_land.geojson src/data/WorldMap.h   # Karte neu erzeugen
```

Der Simulator kompiliert die echten Szenen-Quelldateien. Er ersetzt keinen PlatformIO-Build: Flash-Größe (App-Partition 1,25 MB, siehe `partitions_noota.csv`), Speicherbedarf und Timing auf dem ESP32-S2 sind ungeprüft.

## Erstes Flashen

Beim ersten Flashen wird die Huidu-Originalfirmware überschrieben. Vorher ein Backup ziehen (Port anpassen) und die Flash-Größe prüfen:

```bash
esptool.py --chip esp32s2 --port COMx flash_id
esptool.py --chip esp32s2 --port COMx read_flash 0 0x400000 wf1-backup.bin
```

Danach `pio run -t upload`. Die Weboberfläche ist (gzip) in die Firmware eingebettet (`scripts/embed_ui.py` läuft bei jedem Build), `uploadfs` ist nicht nötig. Zum Entwickeln direkt aus LittleFS: `-DWEB_UI_FROM_FS` in `build_flags` setzen und `pio run -t uploadfs`. Zeigt das Panel Müll, in `DisplayDriver.cpp` den Treiberchip setzen (`config.driver`, z. B. `FM6126A` oder `ICN2038S`).

Heimatstandort für die Karten-Markierung: `HOME_LAT/HOME_LON` in `src/board/BoardConfig.h` (Standard: Berlin).
