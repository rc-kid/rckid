#pragma once

#include "ST7789.h"

#include "ST7789_rgb.pio.h"
#include "ST7789_rgb_double.pio.h"
#include "ST7789_rgba.pio.h"
#include "ST7789_rgba_double.pio.h"
#include "ST7789_picosystem.pio.h"
#include "ST7789_picosystem_double.pio.h"

#include "graphics/color.h"

namespace rckid::display_profile {

    class RGB {
    public:
        using Color = ColorRGB;
        static constexpr int Width = 320;
        static constexpr int Height = 240;
        static constexpr bool NativeMode = true;
        static constexpr bool Double = false;

        static void configureDisplay() {
            ST7789::leaveContinuousMode();
            ST7789::setColumnRange(0, 239);
            ST7789::setRowRange(0, 319);
            ST7789::setColorMode(ST7789::ColorMode::RGB565);
            ST7789::setDisplayMode(ST7789::DisplayMode::Native);
            ST7789::enterContinuousMode();
            ST7789::loadPIODriver(ST7789_rgb_program, ST7789_rgb_program_init);
            ST7789::startPIODriver();
        }
    }; 

    class RGBDouble {
    public:
        using Color = ColorRGB;
        static constexpr int Width = 160;
        static constexpr int Height = 120;
        static constexpr bool NativeMode = true;
        static constexpr bool Double = false;

        static void configureDisplay() {
            ST7789::leaveContinuousMode();
            ST7789::setColumnRange(0, 239);
            ST7789::setRowRange(0, 319);
            ST7789::setColorMode(ST7789::ColorMode::RGB565);
            ST7789::setDisplayMode(ST7789::DisplayMode::Native);
            ST7789::enterContinuousMode();
            ST7789::loadPIODriver(ST7789_rgb_double_program, ST7789_rgb_double_program_init);
            ST7789::startPIODriver();
        }
    }; 

    class RGBA {
    public:
        using Color = ColorRGBA;
        static constexpr int Width = 320;
        static constexpr int Height = 240;
        static constexpr bool NativeMode = true;
        static constexpr bool Double = false;

        static void configureDisplay() {
            ST7789::leaveContinuousMode();
            ST7789::setColumnRange(0, 239);
            ST7789::setRowRange(0, 319);
            ST7789::setColorMode(ST7789::ColorMode::RGB666);
            ST7789::setDisplayMode(ST7789::DisplayMode::Native);
            ST7789::enterContinuousMode();
            ST7789::loadPIODriver(ST7789_rgba_program, ST7789_rgba_program_init);
            ST7789::startPIODriver();
        }
    };

    class RGBADouble {
    public:
        using Color = ColorRGBA;
        static constexpr int Width = 160;
        static constexpr int Height = 120;
        static constexpr bool NativeMode = true;
        static constexpr bool Double = true;

        static void configureDisplay() {
            ST7789::leaveContinuousMode();
            ST7789::setColumnRange(0, 239);
            ST7789::setRowRange(0, 319);
            ST7789::setColorMode(ST7789::ColorMode::RGB666);
            ST7789::setDisplayMode(ST7789::DisplayMode::Native);
            ST7789::enterContinuousMode();
            ST7789::loadPIODriver(ST7789_rgba_double_program, ST7789_rgba_double_program_init);
            ST7789::startPIODriver();
        }
    };

    class Picosystem {
        using Color = ColorRGBA;
        static constexpr int Width = 240;
        static constexpr int Height = 240;
        static constexpr bool NativeMode = false;
        static constexpr bool Double = false;

        static void configureDisplay() {
            ST7789::leaveContinuousMode();
            ST7789::setDisplayMode(ST7789::DisplayMode::Natural);
            ST7789::setRowRange(0, 239);
            ST7789::setColumnRange(40, 279);
            ST7789::setColorMode(ST7789::ColorMode::RGB666);
            ST7789::enterContinuousMode();
            ST7789::loadPIODriver(ST7789_picosystem_program, ST7789_picosystem_program_init);
            ST7789::startPIODriver();
        }
    };

    class PicosystemDouble {
        using Color = ColorRGBA;
        static constexpr int Width = 120;
        static constexpr int Height = 120;
        static constexpr bool NativeMode = false;
        static constexpr bool Double = true;
        static void configureDisplay() {
            ST7789::leaveContinuousMode();
            ST7789::setDisplayMode(ST7789::DisplayMode::Natural);
            ST7789::setRowRange(0, 239);
            ST7789::setColumnRange(40, 279);
            ST7789::setColorMode(ST7789::ColorMode::RGB666);
            ST7789::enterContinuousMode();
            ST7789::loadPIODriver(ST7789_picosystem_double_program, ST7789_picosystem_double_program_init);
            ST7789::startPIODriver();
        }
    };


} // namespace rckid::display_profile