#include "SysInfoScene.h"
#include "display/TextRenderer.h"

namespace {
void line(DisplayDriver& d, int row, const String& s, fm::RGB c) {
    TextRenderer::drawString(d, TextRenderer::FONT_STD, 1, s, 2, 2 + row * 9, fm::to565(c));
}
}

void SysInfoScene::render(uint32_t now) {
    d_.clear();
    line(d_, 0, String("v") + version_, {90, 220, 255});
    line(d_, 1, String("R:") + reason_, bad_ ? fm::RGB{255, 80, 60} : fm::RGB{120, 230, 140});
    line(d_, 2, String("crashes ") + unstable_, unstable_ ? fm::RGB{255, 180, 60} : fm::RGB{170, 185, 200});
    line(d_, 3, String("up ") + (now / 1000) + "s", {170, 185, 200});
    line(d_, 4, network_.stateText(), {255, 255, 255});
    line(d_, 5, String("clients ") + network_.apClients(), {170, 185, 200});
    line(d_, 6, String("heap ") + (ESP.getFreeHeap() / 1024) + "K", {170, 185, 200});
}
