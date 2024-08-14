#pragma once
#include <type_traits>

#include "../rckid.h"
#include "drawing.h"
namespace rckid {


    /** Pixel bitmap, templated by the underlying color type. 

        The bitmap manages the pixel data and provides access to it in two different modes - slow per pixel manipulation and faster blitting of regions.      
     */
    template<typename COLOR>
    class Bitmap {
    public:
        using Color = COLOR;
        static constexpr uint8_t BPP = Color::BPP;

        static_assert(BPP == 16 || BPP ==8 || BPP == 4);

        Bitmap() = default;
        Bitmap(Coord width, Coord height): w_{width}, h_{height}, buffer_{allocateBuffer(width, height) } {}

        ~Bitmap() {
            delete [] buffer_;
        }

        Coord width() const { return w_; }
        Coord height() const { return h_; }

        /** \name Per-pixel interface 

            A very slow interface that provides per pixel access to the bitmap. On a 16BPP color, filling up the 320x240 screen using the per-pixel interface on RP200 takes around 12ms, which is way too slow for 60fps.  
         */
        //@{

        Color pixelAt(Coord x, Coord y) const { return rckid::pixelAt<COLOR>(buffer_, x, y, w_, h_); }

        void setPixelAt(Coord x, Coord y, Color c) { rckid::setPixelAt<COLOR>(buffer_, x, y, c, w_, h_); }
        //@}

        const typename Color::RawBufferType rawBuffer() const {
            return reinterpret_cast<const typename Color::RawBufferType>(buffer_);
        }

    private:

        constexpr uint8_t * allocateBuffer(int w, int h) {
            if (w == 0 || h == 0)
                return nullptr;
            return new uint8_t[(w * h) * BPP / 8];
        } 

        constexpr __force_inline size_t map(Coord x, Coord y) const { return pixelOffset(x, y, w_, h_); }

        Coord w_ = 0;
        Coord h_ = 0;

        uint8_t * buffer_ = nullptr;



    }; // rckid::Bitmap

} // namespace rckid