# WF1 LED-Matrix-Firmware

Eigene Firmware für den **Huidu HD-WF1 (ESP32-S2)** mit einem 64×64-HUB75-LED-Panel. Das Projekt wird mit PlatformIO entwickelt. Die Firmware ist modular aufgebaut: `SceneManager` verwaltet die Szenen, Display-, Netzwerk-, Zeit- und Speicherfunktionen sind voneinander getrennt.

> **Projektstatus (Stand: 07.11.2026):** Host-Simulator, Firmware-Syntaxprüfung, vollständiger PlatformIO-Build und LittleFS-Build waren erfolgreich. Ein Test auf dem tatsächlichen WF1-Board steht noch aus. Vor dem ersten Flashen muss die Originalfirmware gesichert und der Recovery-Weg geprüft werden.

## Funktionen

Es ist jeweils eine Szene aktiv. Die Auswahl und wesentliche Einstellungen werden in NVS gespeichert.

| ID | Szene | Beschreibung |
|---|---|---|
| `text` | Text | Mehrzeiliger Text, verschiedene Schriftarten, Größe, Farbe, Hintergrund, Laufrichtung und Effekte |
| `clock` | Uhr | Digitale/analoge Uhr mit konfigurierbarer Darstellung |
| `world` | Weltzeituhr | Weltkarte mit Tag-/Nacht-Darstellung und Sonnen-Terminator |
| `iss` | ISS-Tracker | Darstellung der aktuellen ISS-Position und ihrer Umlaufbahn; benötigt Netzwerkzugriff |
| `fire` | Feuer | Animierte Flammen mit konfigurierbarer Darstellung |
| `rain` | Digital Rain | Matrix-Regen mit Farbe und Geschwindigkeit |
| `stars` | Sternenhimmel | Sterne, Milchstraße und Sternschnuppen |
| `warp` | Warp | Hyperraum-/Warp-Animation |
| `plasma` | Plasma | Animiertes Plasma mit Paletten und Parametern |
| `ascii` | ASCII-Art | Grafikeffekte als ASCII-Zeichen |
| `mandel` | Mandelbrot | Animierte Zoomfahrt durch die Mandelbrot-Menge |

Zusätzlich gibt es einen Demo-Modus, eine Weboberfläche, WLAN-Konfiguration, persistente Einstellungen sowie eine BM8563-RTC-/NTP-Zeitversorgung.

## Hardware und Voraussetzungen

- Huidu HD-WF1 mit ESP32-S2 und 4 MB Flash
- Ein 64×64-HUB75-Panel (die Pin- und Displayparameter sind in `src/board/` konfiguriert)
- Separate, ausreichend dimensionierte 5-V-Versorgung für das LED-Panel
- VS Code mit PlatformIO oder PlatformIO Core CLI
- Python 3 für die Entwicklungswerkzeuge
- C++-Compiler (z. B. `g++`) für den Host-Simulator und `fwcheck`

Die PlatformIO-Konfiguration verwendet `esp32-s2-saola-1` als ESP32-S2-Boardprofil und aktiviert die WF1-Konfiguration über `-DWF1=1`. Die tatsächliche WF1-Pinbelegung ist im Projekt hinterlegt. Ein erfolgreicher Compile bestätigt noch nicht die elektrische oder zeitliche Kompatibilität mit dem konkreten WF1-Board und Panel.

## Projekt einrichten und bauen

Repository klonen und in den Projektordner wechseln. Danach im Projektstamm:

```bash
pio run -e huidu_hd_wf1
```

Damit wird die Firmware kompiliert und `firmware.bin` erzeugt.

Die Weboberfläche liegt in `data/` und wird separat als LittleFS-Dateisystem-Image gebaut:

```bash
pio run -e huidu_hd_wf1 -t buildfs
```

`buildfs` erstellt nur das Dateisystem-Image. Es überträgt noch nichts auf den Controller. Das Übertragen von Firmware und Dateisystem (`upload` bzw. `uploadfs`) schreibt Flash-Speicher und darf erst nach Backup und bestätigter Hardware-/Recovery-Prozedur erfolgen.

### Build-Konfiguration

- PlatformIO-Umgebung: `huidu_hd_wf1`
- ESP32-Plattform: auf einen konkreten Git-Commit in `platformio.ini` festgelegt
- Framework: Arduino
- Dateisystem: LittleFS
- Partitionslayout: `partitions.csv` (4-MB-Layout mit zwei OTA-App-Slots)
- Alternative ohne OTA: `partitions_noota.csv` (größerer App-Bereich, kein zweiter OTA-Slot)
- Lokal eingebundene Arduino-Bibliotheken: `lib/`

Die Bibliotheken liegen bereits im Repository. Das Skript `scripts/vendor-dependencies.sh` ist ein Wartungswerkzeug zum Übernehmen zuvor aufgelöster Bibliotheken aus dem PlatformIO-Cache; es gehört nicht zum normalen Build-Ablauf.

## WLAN und Zugangsdaten

WLAN-Zugangsdaten können auf zwei Arten bereitgestellt werden:

1. **Über die Weboberfläche:** Ohne gespeicherte WLAN-Zugangsdaten startet das Gerät den Setup-Access-Point `LED-Matrix-Setup`. Die Zugangsdaten können anschließend über die Weboberfläche gespeichert werden. Sie werden in NVS abgelegt.
2. **Beim Kompilieren:** `src/secrets.h.example` nach `src/secrets.h` kopieren und dort optional `WIFI_SSID` und `WIFI_PASS` eintragen.

```bash
cp src/secrets.h.example src/secrets.h
```

`src/secrets.h` ist durch `.gitignore` vom Git-Tracking ausgeschlossen. **Echte WLAN-Passwörter oder andere Zugangsdaten niemals committen oder in Issues/Logs veröffentlichen.**

Optionale Sicherheitsparameter in `src/secrets.h`:

```cpp
#define AP_PASS  "eigenes-setup-passwort"  // Passwort des Setup-AP (mind. 8 Zeichen)
#define WEB_PASS "eigenes-web-passwort"    // HTTP-Basic-Auth, Benutzername: admin
```

Ohne gesetztes `WEB_PASS` ist die Weboberfläche nicht durch eine Anmeldung geschützt. Vor einem regulären Betrieb sollten ein eigenes `AP_PASS` und `WEB_PASS` gesetzt werden. Das Gerät nicht per Portfreigabe direkt aus dem Internet erreichbar machen.

## Weboberfläche und API

Die Weboberfläche wird aus LittleFS ausgeliefert. Im lokalen Netzwerk ist sie – sofern mDNS unterstützt wird – unter `http://ledmatrix.local/` oder alternativ über die IP-Adresse des WF1 erreichbar.

Die Firmware stellt unter anderem folgende HTTP-Endpunkte bereit. POST-Daten werden als `application/x-www-form-urlencoded` gesendet.

| Methode | Endpunkt | Zweck |
|---|---|---|
| `GET` | `/api/status` | Status, aktive Szene, Szenenoptionen, Helligkeit, Demo- und Systemdaten |
| `POST` | `/api/scene` | Szene wählen (`id`) |
| `POST` | `/api/option` | Szenenoption ändern (`scene`, `key`, `value`) |
| `POST` | `/api/text` | Anzeigetext setzen (`text`, maximal 240 UTF-8-Bytes) |
| `POST` | `/api/brightness` | Helligkeit setzen (`value`, 1 bis zum konfigurierten Maximum) |
| `POST` | `/api/demo` | Demo-Modus konfigurieren (`enabled`, `interval`, `ids`) |
| `GET` / `POST` | `/api/wifi` | WLAN-Status abrufen bzw. Zugangsdaten speichern (`ssid`, `pass`) |
| `POST` | `/api/wifi/scan` | WLAN-Suche starten |
| `GET` | `/api/wifi/scan` | Ergebnis der WLAN-Suche abrufen |

## Tests und Entwicklung ohne Hardware

### Host-Simulator

Der Host-Simulator kompiliert echte Szenen-Quelldateien mit kleinen Hardware-Stubs. Er ist für Szenen- und Logiktests gedacht, ersetzt aber weder den ESP32-Build noch einen Test am echten Panel.

```bash
bash tools/sim/build.sh
build/sim selftest
```

Optional können mit `tools/sim/render_sheet.py` Szenenbilder gerendert werden. Die genauen Argumente stehen im Skript bzw. in dessen Hilfe.

### Firmware-Syntaxprüfung

```bash
bash tools/fwcheck.sh
```

Diese Prüfung untersucht Firmware-Dateien, die der Host-Simulator nicht vollständig abdeckt, anhand von Stub-Headern. Sie ersetzt keinen echten PlatformIO-Build.

### UI-Mockserver

Unter `tools/ui_mock/` liegt ein Mockserver für die Weboberfläche. Die aktuelle Mockserver-Version ist noch nicht vollständig mit der Firmware-API bzw. dem aktuellen Simulator synchronisiert und gilt daher **nicht als bestandener Integrationstest**. Vor einer Verwendung muss sie angepasst und separat getestet werden.

### Zuletzt bekannte Prüfergebnisse

Am 07.11.2026 wurden folgende Ergebnisse erzielt:

| Prüfung | Ergebnis |
|---|---|
| Host-Simulator-Build und `selftest` | Erfolgreich |
| `tools/fwcheck.sh` | `fwcheck: OK` |
| `pio run -e huidu_hd_wf1` | `SUCCESS` |
| `pio run -e huidu_hd_wf1 -t buildfs` | `SUCCESS` |

Der PlatformIO-Build meldete 726.991 Bytes von 1.310.720 Bytes App-Partitionsgröße (55,5 %) und 60.544 Bytes von 327.680 Bytes RAM (18,5 %). Das erzeugte LittleFS-Image war 1.441.792 Bytes groß – entsprechend der Größe der dafür vorgesehenen Partition. Die Imagegröße ist nicht mit der tatsächlichen Belegung durch Webdateien gleichzusetzen.

**Noch nicht geprüft:** Start der Firmware auf dem WF1, tatsächliche HUB75-Ausgabe, Display-Timing/Treiberchip, WLAN-Verhalten am Gerät, Weboberfläche am Gerät und Recovery nach einem fehlgeschlagenen Flash.

## Hardwarehinweise und erstes Flashen

- Vor dem ersten Flashen die Originalfirmware des WF1 vollständig sichern.
- Flash-ID und tatsächliche Flashgröße auslesen, bevor ein Schreibvorgang erfolgt.
- Pinbelegung, Panel-Scanrate, Shift-Register-Treiber und Stromversorgung am konkreten Board/Panel verifizieren.
- Die Helligkeit ist in `src/board/BoardConfig.h` konservativ begrenzt. Diese Grenze nicht erhöhen, bevor die Panelversorgung geprüft wurde.
- `PANEL_DRIVER` steht derzeit auf dem generischen Treiberwert. Bei dunklem oder fehlerhaftem Bild muss der tatsächliche Shift-Register-Typ des Panels ermittelt werden; nicht blind verschiedene Werte flashen.

Ein Flash-Vorgang überschreibt Speicher auf dem Controller. Deshalb erst nach geprüftem Backup und geklärtem Recovery-Weg fortfahren. Die passenden Verbindungs- und Boot-Pads hängen von der konkreten WF1-Hardware-Revision ab.

## Projektstruktur

```text
wf1/
├── data/                 Weboberfläche (LittleFS)
├── include/              optionale öffentliche Header
├── lib/                  lokal eingebundene Drittanbieter-Bibliotheken
├── scripts/              Wartungs- und Build-Hilfsskripte
├── specification/        Huidu-Dokumentation
├── src/
│   ├── app/              Szenenverwaltung
│   ├── board/            Board-, Pin- und Displaykonfiguration
│   ├── display/          HUB75-Treiberabstraktion und Text-Rendering
│   ├── network/          WLAN und HTTP-API
│   ├── scenes/           einzelne Display-Szenen
│   ├── services/         Zeit, ISS und Sonnenberechnung
│   ├── storage/          persistente Einstellungen (NVS)
│   └── main.cpp          Einstiegspunkt und Zusammensetzen der Anwendung
├── test/                 PlatformIO-Testbereich / Hardwaretests
├── third_party/          Drittanbieter-Hinweise und Lizenzen
└── tools/
    ├── fwcheck.sh        Firmware-Syntaxprüfung
    ├── sim/              Host-Simulator und Stubs
    └── ui_mock/          Mockserver (derzeit in Überarbeitung)
```

## Weltkarten-Konfiguration

Der Heimatmarker und die Zeitzone werden in `src/board/BoardConfig.h` konfiguriert (`HOME_LAT`, `HOME_LON`, `TIMEZONE`). Vor dem Einsatz die Standardkoordinaten anpassen.

## Lizenzen und Drittanbieter

Projektlizenz und Drittanbieter-Lizenzen sind in `LICENSE`, `third_party/THIRD_PARTY_NOTICES.md` sowie den jeweiligen Bibliotheksverzeichnissen unter `lib/` dokumentiert. Copyright- und Lizenzdateien der eingebundenen Bibliotheken beim Aktualisieren beibehalten.
