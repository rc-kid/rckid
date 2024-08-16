#pragma once

#include "bitmap.h"
#include "canvas.h"

namespace rckid {

    template<typename T>
    class Renderer;

    /** The simplest display renderer from a RGB color bitmap. 
     */
    template<>
    class Renderer<Bitmap<ColorRGB>> {
    public:
        void initialize(Bitmap<ColorRGB> const & bitmap) {
            displaySetMode(DisplayMode::Native);
            displaySetUpdateRegion(bitmap.width(), bitmap.height());
        }

        void finalize() {
            // nothing to finalize
        }

        void render(Bitmap<ColorRGB> const & bitmap) {
            displayUpdate(bitmap.buffer(), bitmap.numPixels());
        }

    }; // rckid::Renderer<Bitmap<ColorRGB>>

    template<typename T>
    class Renderer<Canvas<T>> : public Renderer<Bitmap<T>> {};


} // namespace rckid