#pragma once

#include "../graphics/color.h"
#include "widget.h"

namespace rckid::ui {

    class HLine : public Widget {
    public:

    }; // rckid::ui::HLine

    class VLine : public Widget {
    public:

        VLine(Rect rect): Widget{rect} {}

        void setColor(ColorRGB color) {
            color_ = color;
        }

        void setLineWidth(Coord width) {
            lineWidth_ = width;
        }   

    protected:

        /** Renders the given column, 
         */
        void renderColumn(Coord column, uint16_t * buffer, [[maybe_unused]] Coord starty, Coord numPixels) override {
            if (column < lineWidth_) {
                // just fill with the color
                while (numPixels-- > 0) {
                    *buffer = color_.toRaw();
                    ++buffer;
                }
            } 
        }

    private:
        ColorRGB color_ = ColorRGB::Red();
        Coord lineWidth_ = 1;
        
    }; // rckid::ui::VLine

    class Rectangle : public Widget {
    public:

        Rectangle(Rect rect): Widget{rect} {}

        ColorRGB color() const { return color_; }

        void setColor(ColorRGB color) { color_ = color; }

        void setLineWidth(Coord width) { lineWidth_ = width; }   

        bool fill() const { return fill_; }
        void setFill(bool fill) { fill_ = fill; }

    protected:
        /** Renders the given column, 
         */
        void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
            if (fill_ || (column < lineWidth_) || (column + lineWidth_ >= width())) {
                // just fill with the color
                while (numPixels-- > 0) {
                    *buffer = color_.toRaw();
                    ++buffer;
                }
            } else {
                // fill the line
                for (Coord i = starty, e = starty + numPixels; i != e; ++i) {
                    if (i < lineWidth_ || i + lineWidth_ >= height())
                        *buffer = color_.toRaw();
                    ++buffer;
                }
            }
        }

    private:
        ColorRGB color_ = ColorRGB::Red();
        Coord lineWidth_ = 1;
        bool fill_ = false;

    }; // rckid::ui::Rect`


} // namespace rckid::ui