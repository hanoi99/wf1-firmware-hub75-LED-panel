# PlatformIO pre-build script: create a linker map for size attribution.
# This does not alter firmware behaviour or optimization settings.
Import("env")

import os

build_dir = env.subst("$BUILD_DIR")
os.makedirs(build_dir, exist_ok=True)
map_path = os.path.join(build_dir, "firmware.map")
env.Append(LINKFLAGS=[f"-Wl,-Map={map_path}"])
print(f"[size] Linker map: {map_path}")
