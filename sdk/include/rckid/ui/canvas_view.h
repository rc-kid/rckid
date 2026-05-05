#pragma once

#include <rckid/graphics/canvas.h>
#include <rckid/ui/widget.h>
#include <rckid/ui/focus_rect.h>

namespace rckid::ui {

    /** Canvas viewer. 

        The viewer attaches to an existing canvas and displays its contents
    */
    class CanvasView : public Widget {
    public:

        Canvas * canvas() const { return canvas_; }
        void setCanvas(Canvas * canvas) { canvas_ = canvas; }

        uint32_t zoom() const { return zoom_; }
        void setZoom(uint32_t value) { 
            if (zoom_ != value) {
                zoom_ = value; 
                onChange();
            }
        }

        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            drawCanvasContents(column, startRow, buffer, numPixels);
            Widget::renderColumn(column, startRow, buffer, numPixels);
        }

    protected:

        void drawCanvasContents(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) {
            if (canvas_ == nullptr)
                return;
            // first and last column are reserved for the focus rect border
            if (column == 0 || column == width() - 1)
                return;
            --column;
            // last and first row are reserved for the focus rect border
            if (startRow + numPixels == height()) {
                --numPixels;
            }
            if (startRow == 0) {
                ++buffer;
                --numPixels;
            }
            Coord cx = column / zoom_;
            if (cx < 0 || cx >= canvas_->width())
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
        }

        // pointer to the canvas we are showing
        Canvas * canvas_ = nullptr;
        // zoom level
        uint32_t zoom_ = 1;

    }; // rckid::ui::CanvasView


    /** Like canvas view, but allows editing as well. 
    */
    class CanvasEdit : public CanvasView {
    public:

        CanvasEdit() {
            with(focusRect_)
                << SetRect(Rect::XYWH(0,0, zoom_, zoom_))
                << SetPadding(0);
        }

        Color fg() const { return fg_; }
        Color bg() const { return bg_; }
        void setFg(Color value) { fg_ = value; }
        void setBg(Color value) { bg_ = value; }

        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            drawCanvasContents(column, startRow, buffer, numPixels);
            renderChildColumn(& focusRect_, column, startRow, buffer, numPixels);
            Widget::renderColumn(column, startRow, buffer, numPixels);
        }

        void processEvents() override {
            if (canvas_ == nullptr)
                return;
            if (btnPressed(Btn::A))
                canvas_->at(pos_) = fg_;
            if (btnPressed(Btn::B))
                canvas_->at(pos_) = bg_;
            if (btnPressed(Btn::Left)) {
                if (--pos_.x < 0)
                    pos_.x = canvas_->width() - 1;
                repositionFocusRect();
            }
            if (btnPressed(Btn::Right)) {
                if (++pos_.x >= canvas_->width())
                    pos_.x = 0;
                repositionFocusRect();
            }
            if (btnPressed(Btn::Up)) {
                if (--pos_.y < 0)
                    pos_.y = canvas_->height() - 1;
                repositionFocusRect();
            }
            if (btnPressed(Btn::Down)) {
                if (++pos_.y >= canvas_->height())
                    pos_.y = 0;
                repositionFocusRect();
            }
            if (btnPressed(Btn::VolumeDown)) {
                if (zoom_ > 1)
                    setZoom(zoom_ - 1);
            }
            if (btnPressed(Btn::VolumeUp)) {
                if (zoom_ < 10)
                    setZoom(zoom_ + 1);
            }
        }

    protected:
        void onChange() override {
            CanvasView::onChange();
            repositionFocusRect();
            with(focusRect_)
                << SetWidth(zoom_ + 2)
                << SetHeight(zoom_ + 2);
        }

        void repositionFocusRect() {
            // don't do +1 for the border here because the contents itself is shifted
            with(focusRect_) << SetPosition(pos_.x * zoom_, pos_.y * zoom_);
        }

    private:
        Point pos_{0,0};
        FocusRect focusRect_;

        Color fg_ = Color::White();
        Color bg_ = Color::Black();


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