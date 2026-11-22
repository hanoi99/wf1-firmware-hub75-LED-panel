#pragma once

#include <stdint.h>
#include "display/FastMath.h"

// Only one scene runs at a time, so the scenes with big working buffers (Fire, Plasma, Starfield,
// Mandelbrot) share one block of RAM instead of owning ~15 KB each. A scene must claim the block
// and (re)initialise its contents in begin(); it may only touch it while it is the owner.
namespace scratch {
struct Block {
    uint8_t bytes[64 * 64];
    fm::RGB palette[256];
};
inline Block& block() { static Block b; return b; }
inline const void*& owner() { static const void* o = nullptr; return o; }
inline void claim(const void* who) { owner() = who; }
inline bool owns(const void* who) { return owner() == who; }
}
