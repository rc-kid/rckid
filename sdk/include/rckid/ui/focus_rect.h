#pragma once

#include <rckid/ui/widget.h>

namespace rckid::ui {

    /** Simple focus rectangle. 
     
        A rectangle used to indicate focus. The rectangle show color with breathe animation.
     */
    class FocusRect : public Widget {
    public:

        FocusRect() {
            fg_ = Style::defaultStyle().accentFg();
            updateAnimation(Color::Black(), fg_);
        }

        Color fg() const { return fg_;}

        void setFg(Color value) { 
            fg_ = value; 
            updateAnimation(Color::Black(), fg_);
        }

        Coord padding() const { return padding_; }

        void setPadding(Coord value) { padding_ = value; }

        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            if (column == 0 || column == width() - 1) {
                memset16(reinterpret_cast<uint16_t*>(buffer), current_.toRGB565(), numPixels);
            } else {
                if (startRow == 0)
                    buffer[0] = current_;
                if (startRow + numPixels >= height())
                    buffer[numPixels - 1] = current_;
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

        void updateAnimation(Color a, Color b) {
            cancelAnimations();
            animate() 
                << (new Animation{
                    [this, a, b](FixedRatio progress) {
                        current_ = Color::RGB(
                            a.r + progress.scale(b.r - a.r),
                            a.g + progress.scale(b.g - a.g),
                            a.b + progress.scale(b.b - a.b)
                        );
                    },
                    animationSpeed()
                })->setEasingFunction(easing::inOutIn)->setRepeat(true);
        }

        Color fg_;
        Color current_;
        Coord padding_ = 3;

    private:

    }; // rckid::ui::FocusRect

} // namespace rckid::ui