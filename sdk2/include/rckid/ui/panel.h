#pragma once

#include <rckid/ui/widget.h>

namespace rckid::ui {

    class Panel : public Widget {
    public:

        Panel() = default;

        Panel(Rect rect, Color bg = Color::Black()): Widget{rect}, bg_{bg} {}

        Color bg() const { return bg_; }

        void setBg(Color value) { bg_ = value; }

        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            memset16(reinterpret_cast<uint16_t*>(buffer), bg_.toRGB565(), numPixels);
            Widget::renderColumn(column, startRow, buffer, numPixels);
        }

    protected:

        Color bg_ = Color::RGB(0, 0, 0);

    }; // ui::Panel


    struct SetBg {
        Color value;

        SetBg(Color value): value{value} {}

        void operator () (Panel * w) const { 
            w->setBg(value); 
        }
    };

} // namespace rckid::ui