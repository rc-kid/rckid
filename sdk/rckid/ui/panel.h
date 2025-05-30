#pragma once

#include "container.h"

namespace rckid::ui {

    class Panel : public Container {
    public:

        Panel() = default;

        Panel(Rect rect): Container{rect} {}

        void setBg(ColorRGB bg) { bg_ = bg; }

    protected:

        void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
            for (Coord i = 0; i < numPixels; ++i)
                buffer[i] = bg_.raw16();
            Container::renderColumn(column, buffer, starty, numPixels);
        }

    private:

        ColorRGB bg_;

    }; // rckid::ui::Panel

} // namespace rckid::ui