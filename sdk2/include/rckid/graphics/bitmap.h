#pragma once

#include <rckid/memory.h>
#include <rckid/graphics/geometry.h>
#include <rckid/graphics/color.h>
#include <rckid/graphics/blit.h>
#include <rckid/graphics/image_source.h>

namespace rckid {

    /** Bitmap representation
     */
    class Bitmap {
    public:

        Bitmap() = default;

        Bitmap(ImageSource && src);

        Bitmap(Coord w, Coord h, Color::Representation colorRep, mutable_ptr<uint8_t> pixels, mutable_ptr<Color::RGB565> palette = nullptr) :
            w_{w}, h_{h}, colorRepresentation_{colorRep}, pixels_{std::move(pixels)}, palette_{std::move(palette)} {
        }

        Bitmap(Coord w, Coord h, Color::Representation colorRep) :
            w_{w}, h_{h}, colorRepresentation_{colorRep} 
        {
            uint32_t size = Color::getPixelArraySize(colorRep, w, h);
            pixels_ = mutable_ptr<uint8_t>{new uint8_t[size], size};
            // TODO use standard palette? 
        }

        Bitmap(Bitmap const &) = delete;
        Bitmap & operator = (Bitmap const &) = delete;

        Bitmap(Bitmap && other) noexcept :
            w_{other.w_},
            h_{other.h_},
            colorRepresentation_{other.colorRepresentation_},
            pixels_{std::move(other.pixels_)},
            palette_{std::move(other.palette_)},
            transparentColor_{other.transparentColor_}
        {
            other.w_ = 0;
            other.h_ = 0;
        }

        Bitmap & operator = (Bitmap && other) {
            if (this == & other)
                return *this;
            w_ = other.w_;
            h_ = other.h_;
            colorRepresentation_ = other.colorRepresentation_;
            pixels_ = std::move(other.pixels_);
            palette_ = std::move(other.palette_);
            transparentColor_ = other.transparentColor_;
            other.w_ = 0;
            other.h_ = 0;
            return *this;
        }

        Coord width() const { return w_; }

        Coord height() const { return h_;}

        bool empty() const { return w_ == 0 || h_ == 0; }

        Color::Representation colorRepresentation() const { return colorRepresentation_; }

        uint32_t bpp() const { return colorRepresentationBpp(colorRepresentation_); }

        Color::RGB565 const * palette() const { return palette_.ptr(); }

        void setPalette(mutable_ptr<Color::RGB565> palette) {
            palette_ = std::move(palette);
        }

        std::optional<uint32_t> transparentColor() const { 
            if (transparentColor_ == NO_TRANSPARENCY)
                return std::nullopt;
            return transparentColor_;
        }

        /** Sets or clears transparent color for the rendering. 
         
            Transparent color is unsigned number that is interpreted as the raw value of the corresponding pixel format in the bitmap. When set, all pixels matching the transparent color will not be rendered.
         */
        void setTransparentColor(std::optional<uint32_t> value) {
            transparentColor_ = value.has_value() ? value.value() : NO_TRANSPARENCY;
        }

        uint16_t getPixel(Coord x, Coord y) const {
            return Color::getPixel(colorRepresentation_, pixels_.ptr(), w_, h_, x, y);
        }

        void setPixel(Coord x, Coord y, uint16_t color) {
            Color::setPixel(colorRepresentation_, pixels_.mut(), w_, h_, x, y, color);
        }

        uint8_t const * rawPixelArray(Coord column = 0) const {
            return pixels_.ptr() + mapIndexColumnFirst(column, 0, w_, h_) * bpp() / 8;
        }

        void renderColumn(Coord column, Coord startRow, Coord numPixels, Color::RGB565 * buffer) {
            ASSERT(column < width());
            ASSERT(startRow + numPixels <= height());
            // if using less than 8 bpp, we must ensure that the coordinates & number of pixels to render are aligned properly
            if (colorRepresentation_ == Color::Representation::Index16) {
                ASSERT(startRow % 2 == 0);
            } else {
                ASSERT(bpp() >= 8);
            }
            // get source start pointer
            uint8_t const * start = rawPixelArray(column) + startRow * bpp() / 8;
            if (transparentColor_ != NO_TRANSPARENCY) {
                switch (colorRepresentation_) {
                    case Color::Representation::RGB565:
                        return blit_rgb565(start, buffer, numPixels, transparentColor_);
                    case Color::Representation::RGB332:
                        return blit_rgb332(start, buffer, numPixels, transparentColor_);
                    case Color::Representation::Index256:
                        return blit_index256(start, buffer, numPixels, palette_.ptr(), transparentColor_);
                    case Color::Representation::Index16:
                        return blit_index16(start, buffer, numPixels, palette_.ptr(), transparentColor_);
                }
            } else {
                switch (colorRepresentation_) {
                    case Color::Representation::RGB565:
                        return blit_rgb565(start, buffer, numPixels);
                    case Color::Representation::RGB332:
                        return blit_rgb332(start, buffer, numPixels);
                    case Color::Representation::Index256:
                        return blit_index256(start, buffer, numPixels, palette_.ptr());
                    case Color::Representation::Index16:
                        return blit_index16(start, buffer, numPixels, palette_.ptr());
                }
            }
            UNREACHABLE;
        }

    private:
        Coord w_ = 0;
        Coord h_ = 0;
        Color::Representation colorRepresentation_ = Color::Representation::RGB565;
        mutable_ptr<uint8_t> pixels_;
        mutable_ptr<Color::RGB565> palette_;

        static constexpr uint32_t NO_TRANSPARENCY = 0xFFFFFFFF;
        uint32_t transparentColor_ = 0;
    }; 


} // namespace rckid