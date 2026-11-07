# WF1 Firmware – size and platform diagnostics 0.1.4-dev

Changes:
- Removed the project-level `CORE_DEBUG_LEVEL=3` override. The official Arduino
  platform defines its own default (`0`); the override caused repeated macro
  redefinition warnings. Application `Serial.print/printf` diagnostics remain.
- Changed clock time/date formatting to `strftime`, eliminating the compiler's
  format-truncation warning without changing the visible formats.
- Pinned the existing 4 MiB flash partition layout in `partitions.csv`.
- Added a pre-build script that writes `firmware.map` for each build environment.
- Added a separate, non-default pioarduino Arduino Core 3.3.4 environment strictly
  for A/B binary-size comparison against official PlatformIO Espressif32 8.0.0.

Important findings:
- Official Arduino-ESP32 3.3.12 Release builds already use `-Os`; blindly adding
  another `-Os` flag is not a justified size fix.
- The 86.7% app usage needs symbol/object-level attribution. The map files from
  both environments make that possible.
- No firmware has been flashed by these changes.

Builds to run:

```bash
pio run -e huidu_hd_wf1_official_test -t clean
pio run -e huidu_hd_wf1_official_test
pio run -e huidu_hd_wf1_official_test -t buildfs
pio run -e huidu_hd_wf1_arduino334_compare -t clean
pio run -e huidu_hd_wf1_arduino334_compare
```

Map files:
- `.pio/build/huidu_hd_wf1_official_test/firmware.map`
- `.pio/build/huidu_hd_wf1_arduino334_compare/firmware.map`

Do not upload either environment to the WF1 until the board-specific hardware
and recovery path is verified.
