#pragma once

#include "widget.h"

namespace rckid::ui {

    class Panel : public Widget {
    public:

        Panel(Rect rect): Widget{rect} {}

        Color bg() const { return bg_; }

        void renderColumn(Coord column, Color::RGB565 * buffer, Coord starty, Coord numPixels) override {
            memset16(reinterpret_cast<uint16_t*>(buffer), bg_.toRGB565(), numPixels);
            Widget::renderColumn(column, buffer, starty, numPixels);
        }

    protected:

        Color bg_ = Color::RGB(0, 0, 0);

    }; // ui::Panel

} // namespace rckid::ui