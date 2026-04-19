#pragma once

#include <rckid/ui/widget.h>

namespace rckid::ui {

    /** TODO add pulsing color
     */
    class FocusRect : public Widget {
    public:

        Color fg() const { return fg_;}

        void setFg(Color value) { fg_ = value; }

        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            if (column == 0 || column == width() - 1) {
                memset16(reinterpret_cast<uint16_t*>(buffer), fg_.toRGB565(), numPixels);
            } else {
                if (startRow == 0)
                    buffer[0] = fg_;
                if (startRow + numPixels >= height())
                    buffer[numPixels - 1] = fg_;
            }
            Widget::renderColumn(column, startRow, buffer, numPixels);
        }

        void showAround(Widget * w) {
            // TODO this should be animation
            setRect(Rect::XYWH(w->position().x - 3, w->position().y - 3, w->width() + 6, w->height() + 6));
        }

    protected:

        Color fg_;

    private:

    }; // rckid::ui::FocusRect

} // namespace rckid::ui