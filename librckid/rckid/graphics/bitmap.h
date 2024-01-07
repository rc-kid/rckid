#pragma once

#include "rckid/rckid.h"

namespace rckid {

    template<typename COLOR>
    class Bitmap {
    public:
        using Color = COLOR;

        Bitmap(int w, int h) {
            allocateBuffer(w, h);
        }

        ~Bitmap() { delete [] buffer_; }

        Color pixelAt(int x, int y) const {
            switch (Color::BPP) {
                // getting the right pixel is simple as we only need to cast the buffer to the underlying type and then map the coordinates                
                case 16:
                case 8:
                    return reinterpret_cast<Color const *>(buffer_)[map(x, y)];
                case 4:
                case 2:
                case 1:
                    NOT_IMPLEMENTED;
            }
        }

        void setPixelAt(int x, int y, Color c) {
            switch (Color::BPP) {
                case 16:
                case 8:
                    reinterpret_cast<Color*>(buffer_)[map(x, y)] = c;
                    break;
                case 4:
                case 2:
                case 1:
                    NOT_IMPLEMENTED;
            }
        }


    private:

        constexpr size_t map(int x, int y) const __attribute__((always_inline)) { return (w_ - x - 1) * h_ + y; }

        void allocateBuffer(int w, int h) {
            if (w * h == 0) {
                buffer_ = nullptr;
            } else {
                ASSERT((w * h) % 4 == 0); 
                buffer_ = new uint32_t[w * h * 32 / Color::BPP];
            }
            w_ = w;
            h_ = h;
        }

        uint32_t * buffer_;
        int w_;
        int h_;
    }; 


} // namespace rckid