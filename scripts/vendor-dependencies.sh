#!/usr/bin/env bash
set -euo pipefail

# Copy the exact PlatformIO-resolved library versions from the existing build
# cache into the project's local lib/ directory. This avoids downloading a
# possibly newer upstream version during the vendoring step.

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ENV_NAME="huidu_hd_wf1"
CACHE="$ROOT/.pio/libdeps/$ENV_NAME"
LIB_DIR="$ROOT/lib"
STAGE="$ROOT/.pio/vendor-staging"

if [[ ! -d "$CACHE" ]]; then
  echo "ERROR: PlatformIO dependency cache not found: $CACHE" >&2
  echo "Run 'pio run -e $ENV_NAME' once before vendoring." >&2
  exit 1
fi

# Source folder in .pio/libdeps : destination folder under lib/
PACKAGES=(
  "ESP32 HUB75 LED MATRIX PANEL DMA Display|ESP32-HUB75-MatrixPanel-DMA"
  "I2C BM8563 RTC|I2C_BM8563"
  "Adafruit GFX Library|Adafruit-GFX-Library"
  "Adafruit BusIO|Adafruit_BusIO"
)

mkdir -p "$LIB_DIR" "$ROOT/.pio"
rm -rf "$STAGE"
mkdir -p "$STAGE"

for package in "${PACKAGES[@]}"; do
  IFS='|' read -r source_name target_name <<< "$package"
  source_path="$CACHE/$source_name"
  target_path="$STAGE/$target_name"

  if [[ ! -d "$source_path" ]]; then
    echo "ERROR: Required cached library missing: $source_path" >&2
    echo "No project files were replaced. Check 'pio pkg list -e $ENV_NAME'." >&2
    rm -rf "$STAGE"
    exit 1
  fi

  mkdir -p "$target_path"
  # Copy hidden files too; retain each package's original source and license.
  cp -R "$source_path"/. "$target_path"/
  rm -rf "$target_path/.git" "$target_path/.pio"
  echo "Staged: $source_name -> lib/$target_name"
done

# Replace only the four specifically named vendored-library folders.
for package in "${PACKAGES[@]}"; do
  IFS='|' read -r _ target_name <<< "$package"
  rm -rf "$LIB_DIR/$target_name"
  mv "$STAGE/$target_name" "$LIB_DIR/$target_name"
done
rm -rf "$STAGE"

# Ensure project configuration no longer declares remote lib_deps.
if grep -Eq '^[[:space:]]*lib_deps[[:space:]]*=' "$ROOT/platformio.ini"; then
  echo "ERROR: platformio.ini still contains lib_deps." >&2
  echo "Vendored sources were copied, but inspect platformio.ini manually." >&2
  exit 1
fi

# Verify that the copied packages retain at least one recognizable manifest
# and one license/copyright file. Report rather than guessing license names.
for target_name in \
  ESP32-HUB75-MatrixPanel-DMA \
  I2C_BM8563 \
  Adafruit-GFX-Library \
  Adafruit_BusIO; do
  package_dir="$LIB_DIR/$target_name"
  if [[ ! -d "$package_dir" ]]; then
    echo "ERROR: Vendor folder not found: $package_dir" >&2
    exit 1
  fi
  if ! find "$package_dir" -maxdepth 2 -type f \
      \( -iname 'library.json' -o -iname 'library.properties' \) | grep -q .; then
    echo "WARNING: No PlatformIO/Arduino library manifest found in $target_name" >&2
  fi
  if ! find "$package_dir" -maxdepth 2 -type f \
      \( -iname 'license' -o -iname 'license.*' -o -iname 'copying*' \) | grep -q .; then
    echo "WARNING: No obvious license file found in $target_name; review before publishing." >&2
  fi
done

echo
echo "Vendoring complete. Local libraries are now under ./lib/."
echo "Next run: pio run -e $ENV_NAME"
echo "Then run: pio run -e $ENV_NAME -t buildfs"
