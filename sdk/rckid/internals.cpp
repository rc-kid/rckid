#include "internals.h"
#include "graphics/bitmap.h"
#include "assets/fonts/Iosevka16.h"

namespace rckid {

    void bsod(uint32_t error, uint32_t line, char const * file, char const * extras) {
        // print the error to debug console
        LOG("Fatal error: " << error);
        if (file != nullptr) {
            LOG("Line:        " << line);
            LOG("File:        " << file);
        }
        LOG("Free heap:   " << memoryFreeHeap());
        // create the blue screen of death
        Bitmap<ColorRGB> fb{320, 240};
        fb.fill(color::Blue);
        Font f = Font::fromROM<assets::font::Iosevka16>();
        fb.text(10,10, f, color::White)
            << ":(  Error: " << error << "\n\n"
            << "    File:  " << ((file != nullptr) ? file : "???") << "\n"
            << "    Line:  " << line << "\n\n"
            << "    Extra: " << ((extras != nullptr) ? extras : "-") << "\n\n"
            << "    Long press home button to reset.";
        // render the framebuffer we have
        Renderer<Bitmap<ColorRGB>> r;
        r.initialize(fb);
        r.render(fb);
    }

} // namespace rckid