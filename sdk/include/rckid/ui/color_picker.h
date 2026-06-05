#pragma once

#include <rckid/ui/widget.h>
#include <rckid/ui/focus_rect.h>
#include <rckid/graphics/color.h>

namespace rckid::ui {

    /** Simple color picker from given palette. 
     
        Displays squares of colors and a focus rectangle around one of them.
     */
    class ColorPicker : public Widget {
    public:

        ColorPicker() {
            with(focusRect_)
                << SetPadding(0);
            placeFocusRect(pos_);
        }

        Color selected() const { 
            Coord cols = width() / colorSize_;
            uint32_t idx = pos_.y * cols + pos_.x;
            ASSERT(idx < paletteSize_);
            return palette_[idx];
        }

        void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
            // TODO this is extremely ineffective as we go over entire color in the buffer, instead we should go just over the colors in the current column
            Coord cols = width() / colorSize_;
            for (uint32_t i = 0; i < paletteSize_; ++i) {
                Coord cx = (i % cols) * colorSize_ + 1;
                if (column < cx || column >= (cx + colorSize_))
                    continue;
                Coord cy = (i / cols) * colorSize_ + 1;
                Color::RGB565 c = palette_[i];
                // TODO do not draw the last line
                for (Coord i = 0; i < colorSize_; ++i)
                    renderPixelInBuffer(cy + i, c, startRow, buffer, numPixels);
            }
            if (focused())
                renderChildColumn(& focusRect_, column, startRow, buffer, numPixels);
            Widget::renderColumn(column, startRow, buffer, numPixels);
        }

        void processEvents() override {
            if (btnPressed(Btn::Up))
                placeFocusRect(pos_ + Point{0, -1});
            if (btnPressed(Btn::Down))
                placeFocusRect(pos_ + Point{0, 1});
            if (btnPressed(Btn::Left))
                placeFocusRect(pos_ + Point{-1, 0});
            if (btnPressed(Btn::Right))
                placeFocusRect(pos_ + Point{1, 0});
        }

    protected:
    
        void onChange() override {
            Widget::onChange();
            placeFocusRect(pos_);
        }

        void placeFocusRect(Point pos) {
            if (paletteSize_ == 0)
                return;
            Coord cols = width() / colorSize_;
            if (cols == 0)
                cols = 1;
            Coord rows = paletteSize_ / cols + (paletteSize_ % cols != 0);
            if (pos.y < 0)
                pos.y = rows - 1;
            if (pos.y >= rows)
                pos.y = 0;
            if (pos.y == rows - 1)
                cols = paletteSize_ % cols;
            if (pos.x < 0)
                pos.x = cols - 1;
            if (pos.x >= cols)
                pos.x = 0;

            focusRect_.setRect(Rect::XYWH(pos.x * colorSize_, pos.y * colorSize_, colorSize_ + 2, colorSize_ + 2));
            pos_ = pos;
        }


    private:
        Point pos_{0,0};
        FocusRect focusRect_;

        // size of every color square
        Coord colorSize_ = 10;

        Color::RGB565 const * palette_ = Palette16;
        // 
        uint32_t paletteSize_ = sizeof(Palette16) / sizeof(Color::RGB565);

    }; // ColorPicker

} // namespace rckid::ui