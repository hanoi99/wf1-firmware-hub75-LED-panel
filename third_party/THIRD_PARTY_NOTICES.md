# Third-party software notices

The project vendors the following libraries under `lib/`. Their source files
are retained from the exact PlatformIO-resolved packages used for the WF1
firmware build. The stated versions are taken from the successful PlatformIO
build/dependency output dated 2026-11-01.

| Component | Vendored version | Upstream | License |
|---|---:|---|---|
| ESP32 HUB75 MatrixPanel DMA | 3.1.0 (`4ffd6f0` commit prefix) | https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA | MIT; see `lib/ESP32-HUB75-MatrixPanel-DMA/` |
| I2C BM8563 RTC | 1.0.4 | https://github.com/tanakamasayuki/I2C_BM8563 | CC0-1.0; see `lib/I2C_BM8563/` |
| Adafruit GFX Library | 1.13.0 | https://github.com/adafruit/Adafruit-GFX-Library | BSD license; see `lib/Adafruit-GFX-Library/` |
| Adafruit BusIO | 1.17.4 | https://github.com/adafruit/Adafruit_BusIO | MIT; see `lib/Adafruit_BusIO/` |

## Build platform (not vendored)

The Espressif32 PlatformIO platform, Arduino-ESP32 framework, compiler and
related build tools are not Arduino libraries and are not copied into `lib/`.
The project pins the tested PlatformIO platform commit in `platformio.ini`:

`Jason2866/platform-espressif32` — commit
`66389b3fdfbdc29716b8cddfd80cbf11315af82c`.

This is intentionally kept separate from the four vendored source libraries.
Do not change the platform/toolchain as part of a library update; test such a
change independently.

## Original WF1/WF2 project

The initial WF1/WF2 demonstration and device-specific integration were based
on the project `mrcodetastic/HD-WF1-WF2-LED-MatrixPanel-DMA`. Its attribution,
license and retained legacy reference are documented in the root README and
`reference/legacy/`. This project retains the root GPL-3.0 license and the
upstream copyright/license notices. Acknowledgment does not replace any
applicable license obligations.
