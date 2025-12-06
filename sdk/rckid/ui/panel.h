#pragma once

#include "widget.h"
#include "style.h"

namespace rckid::ui {

    class Panel : public Widget {
    public:

        Panel() = default;

        Panel(Rect rect): Widget{rect} {}

        void setBg(ColorRGB bg) { bg_ = bg; }

        ColorRGB bg() const { return bg_;}

    protected:

        Panel(Rect rect, FormWidget * form):
            Widget{rect, form} {
        }

        void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
            for (Coord i = 0; i < numPixels; ++i)
                buffer[i] = bg_.raw16();
            Widget::renderColumn(column, buffer, starty, numPixels);
        }

    private:

        ColorRGB bg_{Style::bg()};

    }; // rckid::ui::Panel

} // namespace rckid::ui