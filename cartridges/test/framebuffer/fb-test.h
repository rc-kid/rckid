#pragma once

#include "common/config.h"
#include "rckid/rckid.h"
#include "rckid/graphics/ST7789.h"
#include "rckid/graphics/framebuffer.h"

template<typename COLOR, rckid::DisplayMode DISPLAY_MODE>
void helloWorld() {
    using namespace rckid;
    size_t afterInit = getUsedHeap();
    rckid::initialize();
    FrameBuffer<COLOR, DISPLAY_MODE> fb{};
    fb.enable();
    fb.setFg(ColorRGB::White());
    fb.setFont(Iosevka_Mono6pt7b);
    fb.setBg(ColorRGB::RGB(0, 0, 0));
    unsigned t = 0;
    while (true) {
        fb.fill();
        fb.textMultiline(0,0) << "Hello world! " << t << "\n"
            << "Heap:    " << afterInit << ", now: " << getUsedHeap() << "\n"
            << "Calls:   " << getMallocCalls() << ", " << getFreeCalls() << "\n"
            << "BPP:     " << COLOR::BPP << "\n"
            << "Draw:    " << stats::drawUs() << "\n"
            << "Display: " << stats::displayUpdateUs() << "\n";
        fb.render();
        cpu::delayMs(1000);
        ++t;
    }
}