#pragma once

#include <rckid/graphics/canvas.h>
#include <rckid/ui/widget.h>

namespace rckid::ui {

    /** Canvas viewer. 

        The viewer attaches to an existing canvas and displays its contents
    */
    class CanvasView : public Widget {
    public:

        Canvas * canvas() const { return canvas_; }
        void setSanvas(Canvas * canvas) { canvas_ = canvas; }

        uint32_t zoom() const { return zoom_; }
        void setZoom(uint32_t value) { zoom_ = value; }

        virtual void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) {
            [&](){
                if (canvas_ == nullptr)
                    return;
                Coord cx = column / zoom_;
                if (cx < 0 || cx > canvas_->width())
                    return;
                for (Coord y = startRow, ye = startRow + numPixels; y < ye; ++y) {
                    Coord cy = y / zoom_;
                    if (cy < 0) {
                        ++buffer;
                        continue;
                    }
                    if (cy > canvas_->height())
                        break;
                    *(buffer++) = canvas_->at(cx, cy);
                }
            }();
            Widget::renderColumn(column, startRow, buffer, numPixels);
        }

    private:
        // pointer to the canvas we are showing
        Canvas * canvas_ = nullptr;
        // zoom level
        uint32_t zoom_ = 1;
    }; // rckid::ui::CanvasEdit

    /** Sets canvas.
     */
    struct SetCanvas {
        Canvas * canvas;
        SetCanvas(Canvas * value): canvas{value} {}
    };
    template<typename T>
    inline with<T> operator << (with<T> w, SetCanvas x) {
        w->setCanvas(x.canvas);
        return w;
    }

} // namespace rckid::ui