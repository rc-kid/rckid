#pragma once

#include <rckid/ui/widget.h>

namespace rckid::ui {

    /** TODO add pulsing color
     */
    class FocusRect : public Widget {
    public:

        Color fg() const { return fg_;}

        void setFg(Color value) { fg_ = value; }

        Coord padding() const { return padding_; }

        void setPadding(Coord value) { padding_ = value; }

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

        void showAround(Widget * w, bool animation = true) {
            Rect rect = Rect::XYWH(w->position().x - padding_, w->position().y - padding_, w->width() + padding_ * 2, w->height() + padding_ * 2);
            if (animation)
                animate() << MoveAndResize(this, rect);
            else
                setRect(rect);
        }

    protected:

        Color fg_;
        Coord padding_ = 3;

    private:

    }; // rckid::ui::FocusRect

} // namespace rckid::ui