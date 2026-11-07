# Changelog

All notable changes to the WF1 Firmware project are documented here.

## [Unreleased]

### Validation – 2026-11-07

- Host simulator build and selftest completed successfully.
- Firmware syntax check completed successfully using tools/fwcheck.sh.
- Full PlatformIO firmware build completed successfully for huidu_hd_wf1.
- LittleFS filesystem image generated successfully.
- Firmware image size: approximately 727 KB.
- Application flash usage: 55.5%.
- Static RAM usage: 18.5%.

### Pending

- Original WF1 firmware backup.
- Hardware validation on the Huidu HD-WF1.
- HUB75 display timing and driver validation.
- Real-world Wi-Fi and web interface testing.

## [0.2.0-dev]

### Added

- Generic scene options with persistent NVS storage.
- Text renderer with 10 fonts, UTF-8 support and multiple scrolling directions.
- World clock with day/night visualization.
- ISS tracker with orbital visualization.
- Fire, digital rain, starfield, hyperspace and plasma scenes.
- ASCII art and Mandelbrot scenes.
- Configurable demo mode.
- Wi-Fi setup with Access Point fallback.
- Optional web interface password.
- Boot-loop protection.
- Double buffering for smoother display rendering.

### Improved

- Modular scene architecture.
- Centralized scene management.
- Separated display, network, storage and service components.
- Web-based scene configuration.

## [0.1.4-dev]

### Changed

- Removed the project-level CORE_DEBUG_LEVEL override.
- Changed clock formatting to strftime.
- Explicitly configured the 4 MB flash partition layout.

### Added

- Firmware linker map generation for build diagnostics.
- Separate build environment for toolchain comparison.

## [0.1.3-dev]

### Changed

- Replaced remote Arduino library dependencies with locally vendored libraries.
- Pinned the ESP32 platform to a specific Git commit.

### Added

- Dependency vendoring script.
- Third-party license and attribution documentation.

## [0.1.2-dev]

### Fixed

- Added the missing hexColor(uint32_t) helper in WebController.
- Corrected RGB color formatting for the web color input.
