#include "rckid.h"
#include "graphics/canvas.h"
#include "assets/fonts/Iosevka16.h"

namespace rckid {

    void bsod(uint32_t error, uint32_t arg, uint32_t line, char const * file) {
        // print the error to debug console
        LOG(LL_ERROR, "Fatal error: " << error << " (arg " << arg << ")");
        if (file != nullptr) {
            LOG(LL_ERROR, "Line:        " << line);
            LOG(LL_ERROR, "File:        " << file);
        }
        // and draw the debug console
        RenderableCanvas<ColorRGB> fb{RCKID_DISPLAY_WIDTH, RCKID_DISPLAY_HEIGHT};
        fb.fill(ColorRGB::RGB(0, 0, 255));
        Font f = Font::fromROM<assets::Iosevka16>();
        fb.text(10, 10, f, ColorRGB::RGB(255, 255, 255))
            << ":(  Error: " << error << "\n"
            << "    Arg:   " << arg << "\n\n"
            << "    File:  " << ((file != nullptr) ? file : "???") << "\n"
            << "    Line:  " << line << "\n\n"
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