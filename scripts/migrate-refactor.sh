#!/usr/bin/env bash
set -euo pipefail

# Run from the repository root after extracting the refactor archive.
# Keep any existing local Wi-Fi credentials before removing the old source tree.
OLD_SECRETS="src/wf1wf2demo/secrets.h"
NEW_SECRETS="src/secrets.h"

if [[ -f "$OLD_SECRETS" && ! -f "$NEW_SECRETS" ]]; then
  cp "$OLD_SECRETS" "$NEW_SECRETS"
  echo "Copied existing Wi-Fi credentials to $NEW_SECRETS"
fi

if [[ -d "src/wf1wf2demo" ]]; then
  rm -rf "src/wf1wf2demo"
  echo "Removed obsolete monolithic source directory src/wf1wf2demo"
fi

echo "Migration complete. Check $NEW_SECRETS and compile with: pio run -e huidu_hd_wf1"
