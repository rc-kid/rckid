#pragma once

#include "container.h"

namespace rckid::ui {

    class Panel : public Container {
    public:

        Panel() = default;

        void setBg(ColorRGB bg) { bg_ = bg; }

        void renderColumn(Coord column, Pixel * buffer, Coord starty, Coord numPixels) override {
            for (Coord i = 0; i < numPixels; ++i)
                buffer[i] = bg_.raw16();
            Container::renderColumn(column, buffer, starty, numPixels);
        }

    protected:
        ColorRGB bg_;

    }; // rckid::ui::Panel

} // namespace rckid::ui