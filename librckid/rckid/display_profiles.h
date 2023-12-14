#pragma once

#include "ST7789.h"
#include "graphics/color.h"

namespace rckid::display_profile {

    class RGB {
    public:
        using Color = ColorRGB;
        static constexpr int Width = 320;
        static constexpr int Height = 240;
        static constexpr bool NativeMode = true;
        static constexpr bool Double = false;
    }; 

    class RGBDouble {
    public:
        using Color = ColorRGB;
        static constexpr int Width = 160;
        static constexpr int Height = 120;
        static constexpr bool NativeMode = true;
        static constexpr bool Double = false;
    }; 

    class RGBA {
    public:
        using Color = ColorRGBA;
        static constexpr int Width = 320;
        static constexpr int Height = 240;
        static constexpr bool NativeMode = true;
        static constexpr bool Double = false;
    };

    class RGBADouble {
    public:
        using Color = ColorRGBA;
        static constexpr int Width = 160;
        static constexpr int Height = 120;
        static constexpr bool NativeMode = true;
        static constexpr bool Double = true;
    };

    class Picosystem {
        using Color = ColorRGBA;
        static constexpr int Width = 240;
        static constexpr int Height = 240;
        static constexpr bool NativeMode = false;
        static constexpr bool Double = false;
    };

    class PicosystemDouble {
        using Color = ColorRGBA;
        static constexpr int Width = 120;
        static constexpr int Height = 120;
        static constexpr bool NativeMode = false;
        static constexpr bool Double = true;
    };


} // namespace rckid::display_profile