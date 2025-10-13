#include "rckid.h"
#include "graphics/canvas.h"
#include "assets/fonts/Iosevka16.h"

namespace rckid {

    Error Error::last_;

    void Error::bsod() {
        // print the error to debug console
        LOG(LL_ERROR, "Fatal error: " << last_.code << " (arg " << last_.arg << ")");
        if (last_.file != nullptr) {
            LOG(LL_ERROR, "Line:        " << last_.line);
            LOG(LL_ERROR, "File:        " << last_.file);
        }
        LOG(LL_ERROR, "  heap: " << RAMHeap::usedBytes() << " bytes used, " << RAMHeap::freeBytes() << " bytes free");
        LOG(LL_ERROR, "  stack: " << StackProtection::currentSize() << " bytes used, max " << StackProtection::maxSize() << " bytes");
        // stop
        while (true) {

        }
        // and draw the debug console
        RenderableCanvas<ColorRGB> fb{RCKID_DISPLAY_WIDTH, RCKID_DISPLAY_HEIGHT};
        fb.fill(ColorRGB::RGB(0, 0, 255));
        Font f = Font::fromROM<assets::Iosevka16>();
        fb.text(10, 10, f, ColorRGB::RGB(255, 255, 255))
            << ":(  Error: " << last_.code << "\n"
            << "    Arg:   " << last_.arg << "\n\n"
            << "    File:  " << ((last_.file != nullptr) ? last_.file : "???") << "\n"
            << "    Line:  " << last_.line << "\n\n"
            << "    Long press home button to reset.";
        fb.initialize();
        fb.render();
        while (true) {
            yield();
#ifdef RCKID_BACKEND_FANTASY
            tick();
#endif
        }
    }
}