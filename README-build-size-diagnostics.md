# Build-size diagnostics

The official PlatformIO Espressif32 8.0.0 environment is the current production
candidate. Its Arduino-ESP32 Release configuration already uses `-Os`.
`CORE_DEBUG_LEVEL` is no longer overridden by this project, avoiding duplicate
macro definitions and leaving the platform default in effect.

The `huidu_hd_wf1_arduino334_compare` environment is not the default. It pins a
separate pioarduino platform release that supplies Arduino Core 3.3.4 (ESP-IDF
5.5.1), so its binary can be compared against the official 3.3.12 build. This is
a diagnostic A/B test, not a decision to adopt that platform.

Both builds emit a linker map. Compare their `.text`, `.rodata`, and major object
contributions before changing optimization or removing functionality. The
partition CSV pins the previously observed 4 MiB flash layout exactly.
