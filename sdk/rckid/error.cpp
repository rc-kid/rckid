#include "rckid.h"
#include "graphics/bitmap.h"
#include "assets/fonts/Iosevka16.h"

namespace rckid {

    void bsod(uint32_t error, uint32_t line, char const * file) {
        // print the error to debug console
        LOG(LL_ERROR, "Fatal error: " << error << " (arg " << error::arg() << ")");
        if (file != nullptr) {
            LOG(LL_ERROR, "Line:        " << line);
            LOG(LL_ERROR, "File:        " << file);
        }
        if (error::extras() != nullptr)
            LOG(LL_ERROR, "Extras:      " << error::extras());
        // and draw the debug console
        RenderableBitmap<ColorRGB> fb{RCKID_DISPLAY_WIDTH, RCKID_DISPLAY_HEIGHT};
        fb.fill(ColorRGB::RGB(0, 0, 255));
        Font f = Font::fromROM<assets::Iosevka16>();
        fb.text(10, 10, f, ColorRGB::RGB(255, 255, 255))
            << ":(  Error: " << error << "\n"
            << "    Arg:   " << error::arg() << "\n\n"
            << "    File:  " << ((file != nullptr) ? file : "???") << "\n"
            << "    Line:  " << line << "\n\n"
            << "    Extra: " << ((error::extras() != nullptr) ? error::extras() : "-") << "\n\n"
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