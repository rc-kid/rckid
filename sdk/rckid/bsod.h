#pragma once

#include "rckid.h"
#include "graphics/bitmap.h"
#include "graphics/renderer.h"
#include "assets/fonts/Iosevka16.h"

namespace rckid {

    /** Displays a blue screen of death with extra information about the error. 
     
        The function uses the simplest 16 bpp color fullscreen framebuffer, so callers must ensure there is enough free RAM to allocate the required bitmap. This can usually be done by leaving all opened memory areas as the BSOD is not expected to return to normal app code. 
     */
    inline void bsod(uint32_t error, uint32_t line, char const * file, char const * extras = nullptr) {
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