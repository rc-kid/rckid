#pragma once

#include <type_traits>

#include "bitmap.h"
#include "palette.h"

namespace rckid {

    template<typename COLOR>
    class Sprite : public Bitmap<COLOR>, public PaletteOffsetHolder<COLOR> {
    public:

        Sprite(Coord width, Coord height): Bitmap<COLOR>{width, height} {}
        Sprite(Coord width, Coord height, Allocator & a): Bitmap<COLOR>{width, height, a} {}

        using Bitmap<COLOR>::width;
        using Bitmap<COLOR>::height;

        Coord x() const { return x_; }
        Coord y() const { return y_; }
        Point pos() const { return Point{x_, y_}; }
        Rect rect() const { return Rect::XYWH(x_, y_, width(), height()); }

        void setX(Coord x) { x_ = x; }
        void setY(Coord y) { y_ = y; }
        void setPos(Point pos) { x_ = pos.x; y_ = pos.y; }

        template<typename T = COLOR, std::enable_if_t<T::PALETTE, bool> = true>
        void renderColumn(Coord x, ColorRGB * buffer, Palette const & palette);

        template<typename T = COLOR, std::enable_if_t<! T::PALETTE, bool> = true>
        void renderColumn(Coord x, ColorRGB * buffer);

    private:

        Coord x_ = 0;
        Coord y_ = 0;

    }; // rckid::Sprite

    /** Column renderer from 565 color to 565 color. This is the simplest case where we just copy the memory.
     */
    template<> template<>
    void Sprite<ColorRGB565>::renderColumn<>(Coord x, ColorRGB * buffer) {
        UNIMPLEMENTED;
        /** 
        // determine the sprite column to render
        Coord rx = x - x_;
        // don't render at all if the sprite column is out of range
        if (rx < 0 || rx >= width())
            return;
        // see if we are rendeing the whole sprite
        if (y_ >= 0 && y_ <= height()) {
            renderFullColumn(rx, buffer + y_);
        } else {
            // don't start 
            UNIMPLEMENTED;
        }
            */
    }

} // namespace rckid