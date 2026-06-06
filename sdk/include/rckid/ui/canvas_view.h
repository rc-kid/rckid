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
        void setCanvas(Canvas * canvas) { 
            canvas_ = canvas; 
            onChange();
        }

        Coord zoom() const { return zoom_; }
        void setZoom(Coord value) { 
            ASSERT(value > 0);
            if (zoom_ != value) {
                zoom_ = value; 
                onChange();
            }
        }

        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            drawCanvasContents(column, startRow, buffer, numPixels);
            Widget::renderColumn(column, startRow, buffer, numPixels);
        }

        HAlign hAlign() const { return hAlign_; }

        void setHAlign(HAlign value) {
            hAlign_ = value;
            onChange();
        }

        VAlign vAlign() const { return vAlign_; }

        void setVAlign(VAlign value) {
            vAlign_ = value;
            onChange();
        }

    protected:

        void onChange() override {
            Widget::onChange();
            Coord x = 0;
            Coord y = 0;
            if (canvas_ != nullptr) {
                switch (hAlign_) {
                    case HAlign::Left:
                        x = 0;
                        break;
                    case HAlign::Center:
                        x = (width() - canvas_->width() * zoom_ - 2) / 2;
                        break;
                    case HAlign::Right:
                        x = width() - canvas_->width() * zoom_ - 2;
                        break;
                    case HAlign::Manual: // do not change
                        break;
                    default:
                        UNREACHABLE;
                }
                switch (vAlign_) {
                    case VAlign::Top:
                        y = 0;
                        break;
                    case VAlign::Center:
                        y = (height() - canvas_->height() * zoom_ - 2) / 2;
                        break;
                    case VAlign::Bottom:
                        y = height() - canvas_->height() * zoom_ - 2;
                        break;
                    case VAlign::Manual: // do not change
                        break;
                    default:
                        UNREACHABLE;
                }
            }
            offset_ = Point{x, y};
        }

        void drawCanvasContents(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) {
            if (canvas_ == nullptr)
                return;
            // first and last column are reserved for the focus rect border
            if (column < 0 || column >= width() - 1)
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
            // adjust rendering accordingly
            adjustRenderParams(offset_, column, startRow, buffer, numPixels);
            if (column < 0)
                return;
            Coord cx = column / zoom_;
            if (cx < 0 || cx >= canvas_->width())
                return;
            for (Coord y = startRow, ye = startRow + numPixels; y < ye; ++y) {
                Coord cy = y / zoom_;
                if (cy < 0) {
                    ++buffer;
                    continue;
                }
                if (cy >= canvas_->height())
                    break;
                *(buffer++) = canvas_->getPixel(cx, cy);
            }
        }

        // pointer to the canvas we are showing
        Canvas * canvas_ = nullptr;
        // zoom level
        Coord zoom_ = 1;

        HAlign hAlign_ = HAlign::Center;
        VAlign vAlign_ = VAlign::Center;

        Point offset_{0,0};


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

        Point pos() const { return pos_; }

        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            drawCanvasContents(column, startRow, buffer, numPixels);
            if (focused())
                renderChildColumn(& focusRect_, column, startRow, buffer, numPixels);
            Widget::renderColumn(column, startRow, buffer, numPixels);
        }

        void processEvents() override {
            if (canvas_ == nullptr)
                return;
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
                if (zoom_ < 24)
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
            Point pos = Point{
                pos_.x * zoom_ + offset_.x, 
                pos_.y * zoom_ + offset_.y
            };
            // update the offset if we can't fit the rectangle in the view area
            if (pos.x < 0) {
                offset_.x -= pos.x;
                pos.x = 0;
            }
            if (pos.y < 0) {
                offset_.y -= pos.y;
                pos.y = 0;
            }
            Coord posRight = pos.x + focusRect_.width();
            if (posRight > width()) {
                offset_.x -= posRight - width();
                pos.x -= posRight - width(); 
            }
            Coord posBottom = pos.y + focusRect_.height();
            if (posBottom > height()) {
                offset_.y -= posBottom - height();
                pos.y -= posBottom - height();
            }
            with(focusRect_) << SetPosition(pos);
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