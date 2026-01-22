#pragma once

#include <rckid/ui/widget.h>

namespace rckid::ui {

    class Panel : public Widget {
    public:

        Color bg() const { return bg_; }

        void setBg(Color value) { bg_ = value; }

        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            memset16(reinterpret_cast<uint16_t*>(buffer), bg_.toRGB565(), numPixels);
            Widget::renderColumn(column, startRow, buffer, numPixels);
        }

    protected:

        Color bg_ = Color::RGB(0, 0, 0);

    }; // ui::Panel

    /** Sets panel's background color.
     */
    inline auto SetBg(Color value) {
        return [=](Panel * p) { p->setBg(value); };
    }

} // namespace rckid::ui