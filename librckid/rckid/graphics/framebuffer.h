#pragma once

#include "canvas.h"

#include "../ST7789.h"
#include "../display_profiles.h"
#include "ST7789_rgb.pio.h"
#include "ST7789_rgb_double.pio.h"
#include "ST7789_rgba.pio.h"
#include "ST7789_rgba_double.pio.h"


namespace rckid {

    /** A framebuffer is nothing more than a canvas that knows how to display itself on the display. 
     */
    template<typename DISPLAY_PROFILE> 
    class Framebuffer : public Canvas<typename DISPLAY_PROFILE::Color> {
    public:

        using DisplayProfile = DISPLAY_PROFILE;

        Framebuffer(): Canvas<typename DisplayProfile::Color>{DISPLAY_PROFILE::Width, DISPLAY_PROFILE::Height} {}

        void configureDisplay();

        void startRendering();

        uint16_t updateLine_ = 0;
    }; // Framebuffer


    template<>
    inline void Framebuffer<display_profile::RGB>::configureDisplay() {
        ST7789::leaveContinuousMode();
        ST7789::setColumnRange(0, 239);
        ST7789::setRowRange(0, 319);
        ST7789::setColorMode(ST7789::ColorMode::RGB565);
        ST7789::setDisplayMode(ST7789::DisplayMode::Native);
        ST7789::enterContinuousMode();
        ST7789::loadPIODriver(ST7789_rgb_program, ST7789_rgb_program_init);
        ST7789::startPIODriver();
    }

    template<>
    inline void Framebuffer<display_profile::RGB>::startRendering() {
        using namespace display_profile;
        ST7789::waitVSync();
        ST7789::updatePixels(rawPixels(), DisplayProfile::Width * DisplayProfile::Height);
    }

    template<>
    inline void Framebuffer<display_profile::RGBDouble>::configureDisplay() {
        ST7789::leaveContinuousMode();
        ST7789::setColumnRange(0, 239);
        ST7789::setRowRange(0, 319);
        ST7789::setColorMode(ST7789::ColorMode::RGB565);
        ST7789::setDisplayMode(ST7789::DisplayMode::Native);
        ST7789::enterContinuousMode();
        ST7789::loadPIODriver(ST7789_rgb_double_program, ST7789_rgb_double_program_init);
        ST7789::startPIODriver();
    }

    template<>
    inline void Framebuffer<display_profile::RGBDouble>::startRendering() {
        using namespace display_profile;
        updateLine_ = 0;
        ST7789::waitVSync();
        ST7789::updatePixelsPartial(rawPixels(), RGBDouble::Height, [this](){
            if (updateLine_ == RGBDouble::Width - 1)
                ST7789::updatePixels(rawPixels() + RGBDouble::Height * (RGBDouble::Width - 1), RGBDouble::Height);
            else 
               ST7789::updatePixelsPartial(rawPixels() + RGBDouble::Height * updateLine_++, RGBDouble::Height * 2);
        }); 
    }

} // namespace rckid