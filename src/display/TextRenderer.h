#pragma once

#include <Arduino.h>
#include "DisplayDriver.h"

// Font handling for the 64x64 panel: font table, UTF-8 -> single byte text
// (German umlauts), measuring, word wrap and glyph drawing.
namespace TextRenderer {

constexpr uint8_t FONT_STD = 0;   // Adafruit 5x7 (CP437, has umlauts), integer scaling
constexpr uint8_t FONT_MINI = 1;  // TomThumb 3x5

// Option choices string for the web UI, same order as the font table.
extern const char* const FONT_CHOICES;
uint8_t fontCount();

// UTF-8 -> bytes understood by drawChar()/width() for the given font.
String prepare(const String& utf8, uint8_t font);

int advance(uint8_t font, uint8_t size, uint8_t c);
int width(uint8_t font, uint8_t size, const String& s);
int lineHeight(uint8_t font, uint8_t size);

// Draws one glyph with its top-left corner at (x, yTop). Text is transparent.
void drawChar(DisplayDriver& d, uint8_t font, uint8_t size, uint8_t c, int x, int yTop, uint16_t color565);
void drawString(DisplayDriver& d, uint8_t font, uint8_t size, const String& s, int x, int yTop, uint16_t color565);

// Greedy word wrap into at most maxLines lines. Returns number of lines.
uint8_t wrap(uint8_t font, uint8_t size, const String& s, int maxWidth, String* lines, uint8_t maxLines);

}  // namespace TextRenderer
