#include "TextRenderer.h"

#include <Fonts/TomThumb.h>
#include <Fonts/Picopixel.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>

namespace TextRenderer {

namespace {
// Index = value of the "font" option. nullptr = built-in 5x7 font.
const GFXfont* const FONTS[] = {
    nullptr, &TomThumb, &Picopixel, &FreeSans9pt7b, &FreeSansBold9pt7b,
    &FreeSans12pt7b, &FreeSansBold12pt7b, &FreeSerif9pt7b, &FreeMono9pt7b,
    &FreeSansBold18pt7b,
};
constexpr uint8_t FONT_COUNT = sizeof(FONTS) / sizeof(FONTS[0]);

// Single-byte codes used internally for characters outside ASCII (CP437 values,
// so the built-in font can draw them directly).
constexpr uint8_t C_AE = 0x84, C_OE = 0x94, C_UE = 0x81;
constexpr uint8_t C_AE_UP = 0x8E, C_OE_UP = 0x99, C_UE_UP = 0x9A;
constexpr uint8_t C_SZ = 0xE1, C_DEG = 0xF8;

const GFXfont* gfx(uint8_t font) { return font < FONT_COUNT ? FONTS[font] : nullptr; }

bool isUmlaut(uint8_t c, uint8_t& base) {
    switch (c) {
        case C_AE: base = 'a'; return true;
        case C_OE: base = 'o'; return true;
        case C_UE: base = 'u'; return true;
        case C_AE_UP: base = 'A'; return true;
        case C_OE_UP: base = 'O'; return true;
        case C_UE_UP: base = 'U'; return true;
        default: return false;
    }
}

const GFXglyph* glyphFor(const GFXfont* f, uint8_t c) {
    if (c < f->first || c > f->last) return nullptr;
    return &f->glyph[c - f->first];
}

int ascent(const GFXfont* f) {
    const GFXglyph* h = glyphFor(f, 'H');
    return h ? -h->yOffset : f->yAdvance * 3 / 4;
}

// Fonts with a 3x5 grid and no lowercase need a bit more clearance for dots.
}  // namespace

const char* const FONT_CHOICES =
    "Standard 5x7|Mini 3x5|Pico|Sans 9|Sans Fett 9|Sans 12|Sans Fett 12|Serif 9|Mono 9|Sans Fett 18";

uint8_t fontCount() { return FONT_COUNT; }

String prepare(const String& in, uint8_t font) {
    String out;
    out.reserve(in.length());
    const bool builtin = gfx(font) == nullptr;
    const size_t n = in.length();
    for (size_t i = 0; i < n; ++i) {
        const uint8_t c = (uint8_t)in[i];
        if (c < 0x80) {
            if (c == '\r') continue;
            if (c == '\t') { out += ' '; continue; }
            if (c == '\n' || c >= 0x20) out += (char)c;
            continue;
        }
        uint8_t mapped = '?';
        if (c == 0xC3 && i + 1 < n) {  // Latin-1 supplement
            const uint8_t d = (uint8_t)in[++i];
            switch (d) {
                case 0xA4: mapped = C_AE; break;      // ä
                case 0xB6: mapped = C_OE; break;      // ö
                case 0xBC: mapped = C_UE; break;      // ü
                case 0x84: mapped = C_AE_UP; break;   // Ä
                case 0x96: mapped = C_OE_UP; break;   // Ö
                case 0x9C: mapped = C_UE_UP; break;   // Ü
                case 0x9F: mapped = C_SZ; break;      // ß
                case 0xA9: case 0xA8: case 0xAA: mapped = 'e'; break;
                case 0xA0: case 0xA1: case 0xA2: mapped = 'a'; break;
                default: mapped = '?'; break;
            }
        } else if (c == 0xC2 && i + 1 < n) {
            const uint8_t d = (uint8_t)in[++i];
            mapped = (d == 0xB0) ? C_DEG : (d == 0xA0 ? ' ' : '?');
        } else if (c == 0xE2 && i + 2 < n) {  // € and typographic punctuation
            const uint8_t d = (uint8_t)in[i + 1], e = (uint8_t)in[i + 2];
            i += 2;
            if (d == 0x82 && e == 0xAC) mapped = 'E';
            else if (d == 0x80 && (e == 0x93 || e == 0x94)) mapped = '-';
            else if (d == 0x80 && (e == 0x98 || e == 0x99)) mapped = '\'';
            else if (d == 0x80 && (e == 0x9C || e == 0x9D)) mapped = '"';
            else mapped = '?';
        } else {
            while (i + 1 < n && ((uint8_t)in[i + 1] & 0xC0) == 0x80) ++i;  // skip continuation bytes
        }
        if (!builtin) {
            if (mapped == C_SZ) { out += "ss"; continue; }
            if (mapped == C_DEG) mapped = '*';
        }
        out += (char)mapped;
    }
    return out;
}

int advance(uint8_t font, uint8_t size, uint8_t c) {
    const GFXfont* f = gfx(font);
    if (!f) return 6 * size;
    uint8_t base;
    if (isUmlaut(c, base)) c = base;
    const GFXglyph* g = glyphFor(f, c);
    if (!g) g = glyphFor(f, '?');
    return g ? g->xAdvance * size : 0;
}

int width(uint8_t font, uint8_t size, const String& s) {
    int w = 0;
    for (size_t i = 0; i < s.length(); ++i) w += advance(font, size, (uint8_t)s[i]);
    return w;
}

int lineHeight(uint8_t font, uint8_t size) {
    const GFXfont* f = gfx(font);
    return f ? f->yAdvance * size : 8 * size;
}

void drawChar(DisplayDriver& d, uint8_t font, uint8_t size, uint8_t c, int x, int yTop, uint16_t color) {
    auto& m = d.matrix();
    const GFXfont* f = gfx(font);
    if (!f) {
        if (c == ' ') return;
        m.setFont(nullptr);
        m.drawChar((int16_t)x, (int16_t)yTop, c, color, color, size);
        return;
    }
    if (c == ' ') return;
    uint8_t base = 0;
    const bool umlaut = isUmlaut(c, base);
    const uint8_t code = umlaut ? base : c;
    const GFXglyph* g = glyphFor(f, code);
    if (!g) { g = glyphFor(f, '?'); if (!g) return; }
    const int baseline = yTop + ascent(f) * size;
    m.setFont(f);
    m.drawChar((int16_t)x, (int16_t)baseline, glyphFor(f, code) ? code : '?', color, color, size);
    m.setFont(nullptr);
    if (umlaut) {
        // Two dots above the glyph (the GFX fonts are ASCII only).
        const int top = baseline + g->yOffset * size;
        const int gx = x + g->xOffset * size;
        const int dotY = top - 2 * size;
        const int w = g->width * size;
        const int left = gx + (w >= 4 * size ? size : 0);
        const int right = gx + w - size - (w >= 4 * size ? size : 0);
        m.fillRect((int16_t)left, (int16_t)dotY, size, size, color);
        m.fillRect((int16_t)right, (int16_t)dotY, size, size, color);
    }
}

void drawString(DisplayDriver& d, uint8_t font, uint8_t size, const String& s, int x, int yTop, uint16_t color) {
    for (size_t i = 0; i < s.length(); ++i) {
        const uint8_t c = (uint8_t)s[i];
        if (c == '\n') continue;
        drawChar(d, font, size, c, x, yTop, color);
        x += advance(font, size, c);
        if (x >= DisplayDriver::W) break;
    }
}

uint8_t wrap(uint8_t font, uint8_t size, const String& s, int maxWidth, String* lines, uint8_t maxLines) {
    uint8_t count = 0;
    String cur;
    int curW = 0;
    String word;
    int wordW = 0;

    auto pushLine = [&]() {
        if (count < maxLines) lines[count++] = cur;
        cur = "";
        curW = 0;
    };
    auto flushWord = [&]() {
        if (word.length() == 0) return;
        const int spaceW = cur.length() ? advance(font, size, ' ') : 0;
        if (curW + spaceW + wordW > maxWidth && cur.length()) pushLine();
        if (cur.length()) { cur += ' '; curW += advance(font, size, ' '); }
        cur += word;
        curW += wordW;
        word = "";
        wordW = 0;
    };

    for (size_t i = 0; i < s.length() && count < maxLines; ++i) {
        const uint8_t c = (uint8_t)s[i];
        if (c == '\n') { flushWord(); pushLine(); continue; }
        if (c == ' ') { flushWord(); continue; }
        const int cw = advance(font, size, c);
        if (wordW + cw > maxWidth && word.length()) {  // overlong word: break it
            flushWord();
            if (cur.length()) pushLine();
        }
        word += (char)c;
        wordW += cw;
    }
    flushWord();
    if (cur.length() && count < maxLines) pushLine();
    return count;
}

}  // namespace TextRenderer
