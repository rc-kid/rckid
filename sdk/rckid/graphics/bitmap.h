#pragma once

#include "../rckid.h"
#include "pixel_surface.h"
#include "image_decoder.h"

namespace rckid {

    /** Bitmap is a simple renderable container that holds both pixel data (surface), and a palette, where appropriate. 
     
        The bitmap works with any bit depth and provides a simple rendering interface. 
     */
    class Bitmap {
    public:

        Bitmap() = default;

        Bitmap(Coord w, Coord h, uint32_t bpp = 16):
            w_{w}, h_{h}, bpp_{bpp} {
            ASSERT(bpp == 16 || bpp == 8 || bpp == 4 || bpp == 2 || bpp == 1);
            pixels_ = new uint16_t[numHalfWords()];
            if (bpp != 16)
                palette_ = new uint16_t[1 << bpp];
        }

        Bitmap(Bitmap const &) = delete;
        
        Bitmap(Bitmap && other) noexcept:
            w_{other.w_}, 
            h_{other.h_}, 
            bpp_{other.bpp_}, 
            pixels_{other.pixels_}, 
            palette_{other.palette_} {
            other.pixels_ = nullptr;
            other.palette_ = nullptr;
            other.clear(); // reset the w & h & bpp too
        }

        Bitmap(ImageDecoder && decoder) {
            loadImage(std::move(decoder));
        }

        ~Bitmap() {
            clear();
        }

        Bitmap & operator = (Bitmap const &) = delete;

        Bitmap & operator = (Bitmap && other) noexcept {
            if (this != &other) {
                clear();
                pixels_ = other.pixels_;
                palette_ = other.palette_;
                w_ = other.w_;
                h_ = other.h_;
                bpp_ = other.bpp_;
                other.pixels_ = nullptr;
                other.palette_ = nullptr;
                other.clear();
            }
            return *this;
        }

        bool empty() const { return pixels_ == nullptr; }
        Coord width() const { return w_; }
        Coord height() const { return h_; }
        uint32_t numBytes() const { return w_ * h_ * bpp_ / 8; }
        uint32_t numPixels() const { return w_ * h_; }
        uint32_t numHalfWords() const { return numBytes() / 2; }

        uint32_t bpp() const { return bpp_; }

        ColorRGB colorAt(Coord x, Coord y) const {
            switch (bpp_) {
                case 16:
                    return ColorRGB::fromRaw(PixelSurface<16>::pixelAt(x, y, w_, h_, pixels_));
                case 8:
                    return ColorRGB::fromRaw(palette_[PixelSurface<8>::pixelAt(x, y, w_, h_, pixels_)]);
                case 4:
                    return ColorRGB::fromRaw(palette_[PixelSurface<4>::pixelAt(x, y, w_, h_, pixels_)]);
                case 2:
                    return ColorRGB::fromRaw(palette_[PixelSurface<2>::pixelAt(x, y, w_, h_, pixels_)]);
                case 1:
                    return ColorRGB::fromRaw(palette_[PixelSurface<1>::pixelAt(x, y, w_, h_, pixels_)]);
                default:
                    UNREACHABLE; // invalid bpp
            }
        }

        void setPixelAt(Coord x, Coord y, uint32_t rawValue) {
            switch (bpp_) {
                case 16:
                    PixelSurface<16>::setPixelAt(x, y, w_, h_, pixels_, rawValue);
                    break;
                case 8:
                    PixelSurface<8>::setPixelAt(x, y, w_, h_, pixels_, rawValue);
                    break;
                case 4:
                    PixelSurface<4>::setPixelAt(x, y, w_, h_, pixels_, rawValue);
                    break;
                case 2:
                    PixelSurface<2>::setPixelAt(x, y, w_, h_, pixels_, rawValue);
                    break;
                case 1:
                    PixelSurface<1>::setPixelAt(x, y, w_, h_, pixels_, rawValue);
                    break;
                default:
                    UNREACHABLE; // invalid bpp
            }
        }

        // 285 kb left with 16bpp only
        void loadImage(ImageDecoder && decoder) {
            clear();
            w_ = decoder.width();
            h_ = decoder.height();
            bpp_ = decoder.bpp();
            pixels_ = new uint16_t[numHalfWords()];
            if (bpp_ == 16) {
                palette_ = nullptr; // no palette for 16bpp
                decoder.decodeRGB([this](uint16_t * rgb, int lineNum, int lineWidth) {
                    for (int i = 0; i < lineWidth; ++i)
                        PixelSurface<16>::setPixelAt(i, lineNum, w_, h_, pixels_, rgb[i]);
                });
            } else {
                decoder.decode([this](uint16_t * pixels, int lineNum, int lineWidth) {
                    switch (bpp_) {
                        case 8:
                            for (int i = 0; i < lineWidth; ++i)
                                PixelSurface<8>::setPixelAt(i, lineNum, w_, h_, pixels_, pixels[i]);
                            break;
                        case 4:
                            for (int i = 0; i < lineWidth; ++i)
                                PixelSurface<4>::setPixelAt(i, lineNum, w_, h_, pixels_, pixels[i]);
                            break;
                        case 2:
                            for (int i = 0; i < lineWidth; ++i)
                                PixelSurface<2>::setPixelAt(i, lineNum, w_, h_, pixels_, pixels[i]);
                            break;
                        case 1:
                            for (int i = 0; i < lineWidth; ++i)
                                PixelSurface<1>::setPixelAt(i, lineNum, w_, h_, pixels_, pixels[i]);
                            break;
                        default:
                            UNREACHABLE; // invalid bpp
                    }
                });
                palette_ = decoder.palette();
            }
        }

        void clear() {
            delete [] pixels_;
            delete [] palette_;
            pixels_ = nullptr;
            palette_ = nullptr;
            w_ = 0;
            h_ = 0;
            bpp_ = 16; // why not
        }
        
        uint32_t renderColumn(Coord column, Coord startRow, Coord numPixels, uint16_t * buffer) {
            switch (bpp_) {
                case 16:
                    ASSERT(palette_ == nullptr);
                    return PixelSurface<16>::renderColumn(pixels_, column, startRow, numPixels, w_, h_, buffer, nullptr);
                case 8:
                    ASSERT(palette_ != nullptr);
                    return PixelSurface<8>::renderColumn(pixels_, column, startRow, numPixels, w_, h_, buffer, palette_);
                case 4:
                    ASSERT(palette_ != nullptr);
                    return PixelSurface<4>::renderColumn(pixels_, column, startRow, numPixels, w_, h_, buffer, palette_);
                case 2:
                    ASSERT(palette_ != nullptr);
                    return PixelSurface<2>::renderColumn(pixels_, column, startRow, numPixels, w_, h_, buffer, palette_);
                case 1:
                    ASSERT(palette_ != nullptr);
                    return PixelSurface<1>::renderColumn(pixels_, column, startRow, numPixels, w_, h_, buffer, palette_);
                default:
                    UNREACHABLE; // invalid bpp
            }    
        }

        uint32_t renderColumn(Coord column, Coord startRow, Coord numPixels, uint16_t * buffer, uint32_t transparent) const {
            switch (bpp_) {
                case 16:
                    ASSERT(palette_ == nullptr);
                    return PixelSurface<16>::renderColumn(pixels_, column, startRow, numPixels, w_, h_, buffer, transparent);
                case 8:
                    ASSERT(palette_ != nullptr);
                    return PixelSurface<8>::renderColumn(pixels_, column, startRow, numPixels, w_, h_, buffer, transparent, palette_);
                case 4:
                    ASSERT(palette_ != nullptr);
                    return PixelSurface<4>::renderColumn(pixels_, column, startRow, numPixels, w_, h_, buffer, transparent, palette_);
                case 2:
                    ASSERT(palette_ != nullptr);
                    return PixelSurface<2>::renderColumn(pixels_, column, startRow, numPixels, w_, h_, buffer, transparent, palette_);
                case 1:
                    ASSERT(palette_ != nullptr);
                    return PixelSurface<1>::renderColumn(pixels_, column, startRow, numPixels, w_, h_, buffer, transparent, palette_);
                default:
                    UNREACHABLE; // invalid bpp
            }    
        }

    private:
        Coord w_ = 0;
        Coord h_ = 0;
        uint32_t bpp_ = 16;

        uint16_t * pixels_ = nullptr;
        uint16_t * palette_ = nullptr;
    }; // rckid::Bitmap

} // namespace rckid