#pragma once

#include <Arduino.h>
#include "board/BoardConfig.h"
#include "storage/SettingsStore.h"

// HUB75 timing parameters. They are stored in NVS and applied at boot, because the I2S/LCD
// DMA engine can only be configured before it starts. Changing them from the web UI restarts the board.
struct DisplayTuning {
    uint8_t speedMHz = 8;       // pixel clock: 8, 16 or 20 MHz (lower = more tolerant of long cables)
    uint8_t latch = 12;         // extra blanking clocks around the latch (0 = library auto, 1..32)
    bool clkphase = true;       // sample data on the other clock edge when false
    bool doubleBuf = BoardConfig::USE_DOUBLE_BUFFER;
    uint8_t bits = 8;           // colour depth per channel (4..8): fewer bits = less RAM
    uint8_t minHz = 100;        // minimum refresh rate; higher = fewer DMA descriptors (less RAM), small loss in dark gradients
    uint8_t driver = BoardConfig::PANEL_DRIVER;   // fixed: the driver chip option made no difference on this panel

    static bool validSpeed(long v) { return v == 8 || v == 16 || v == 20; }

    // Latch 0 (library auto) together with a fast clock left the panel dead on real hardware.
    static bool combinationOk(uint8_t speedMHz, uint8_t latch) { return !(latch == 0 && speedMHz > 8); }

    static DisplayTuning load(SettingsStore& s) {
        DisplayTuning t;
        // A timing change is a "trial" until the board ran 30 s without trouble (confirm()).
        // If the previous boot never got that far, the new values are suspected: back to defaults.
        if (s.getInt("dsp.try", 0) != 0) {
            Serial.println("[disp] previous timing change never ran stable -> default timing restored");
            DisplayTuning().save(s, false);
            return t;
        }
        const int32_t spd = s.getInt("dsp.spd", t.speedMHz);
        const int32_t lat = s.getInt("dsp.lat", t.latch);
        if (validSpeed(spd)) t.speedMHz = (uint8_t)spd;
        if (lat >= 0 && lat <= 32) t.latch = (uint8_t)lat;
        if (!combinationOk(t.speedMHz, t.latch)) t.latch = DisplayTuning().latch;
        t.clkphase = s.getInt("dsp.clk", t.clkphase ? 1 : 0) != 0;
        t.doubleBuf = s.getInt("dsp.db", t.doubleBuf ? 1 : 0) != 0;
        const int32_t bt = s.getInt("dsp.bits", t.bits);
        if (bt >= 4 && bt <= 8) t.bits = (uint8_t)bt;
        const int32_t hz = s.getInt("dsp.hz", t.minHz);
        if (hz >= 60 && hz <= 200) t.minHz = (uint8_t)hz;
        return t;
    }
    void confirm(SettingsStore& s) const { if (s.getInt("dsp.try", 0) != 0) s.putInt("dsp.try", 0); }
    void save(SettingsStore& s, bool trial = true) const {
        s.putInt("dsp.try", trial ? 1 : 0);
        s.putInt("dsp.spd", speedMHz);
        s.putInt("dsp.lat", latch);
        s.putInt("dsp.clk", clkphase ? 1 : 0);
        s.putInt("dsp.db", doubleBuf ? 1 : 0);
        s.putInt("dsp.hz", minHz);
        s.putInt("dsp.bits", bits);
    }
};
