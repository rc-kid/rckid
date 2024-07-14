#pragma once

#include "rckid/common/config.h"
#include "rckid/rckid.h"
#include "rckid/graphics/ST7789.h"
#include "rckid/graphics/framebuffer.h"

template<typename COLOR, rckid::DisplayMode DISPLAY_MODE>
void helloWorld() {
    using namespace rckid;
    size_t afterInit = getUsedHeap();
    size_t afterInitMalloc = getMallocCalls();
    size_t afterInitFree = getFreeCalls();
    rckid::initialize();
    FrameBuffer<COLOR, DISPLAY_MODE> fb{};
    fb.enable();
    fb.setBg(COLOR::RGB(0, 0, 0));
    unsigned t = 0;
    while (true) {
        fb.fill();
        fb.textMultiline(0,0) << "Hello world! " << t << "\n"
            << "Init:    " << afterInit << " - " << afterInitMalloc << " - " << afterInitFree << "\n"
            << "Heap:    " << getUsedHeap() << " - " << getMallocCalls() << " - " << getFreeCalls() << "\n"
            << "BPP:     " << COLOR::BPP << "\n"
            << "Draw:    " << stats::drawUs() << "\n"
            << "Display: " << stats::displayUpdateUs() << "\n";
        fb.render();
        cpu::delayMs(500);
        ++t;
    }
}