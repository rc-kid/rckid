#pragma once

#include "canvas.h"

#include "../ST7789.h"
#include "../display_profiles.h"


namespace rckid {

    /** A framebuffer is nothing more than a canvas that knows how to display itself on the display. 
     */
    template<typename DISPLAY_PROFILE> 
    class Framebuffer : public Canvas<typename DISPLAY_PROFILE::Color> {
    public:

        using DisplayProfile = DISPLAY_PROFILE;

        Framebuffer(): Canvas<typename DisplayProfile::Color>{DISPLAY_PROFILE::Width, DISPLAY_PROFILE::Height} {}

        void startRendering();

        uint16_t updateLine_ = 0;
    }; // Framebuffer


    template<>
    inline void Framebuffer<display_profile::RGB>::startRendering() {
        using namespace display_profile;
        ST7789::updatePixels(rawPixels(), DisplayProfile::Width * DisplayProfile::Height);
    }

    template<>
    inline void Framebuffer<display_profile::RGBDouble>::startRendering() {
        using namespace display_profile;
        updateLine_ = 0;
        ST7789::updatePixelsPartial(rawPixels(), DisplayProfile::Height, [this](){
            if (updateLine_ == DisplayProfile::Width - 1)
                ST7789::updatePixels(rawPixels() + DisplayProfile::Height * (DisplayProfile::Width - 1), DisplayProfile::Height);
            else 
               ST7789::updatePixelsPartial(rawPixels() + DisplayProfile::Height * updateLine_++, DisplayProfile::Height * 2);
        }); 
    }

    template<>
    inline void Framebuffer<display_profile::RGBA>::startRendering() {
        using namespace display_profile;
        ST7789::updatePixels(rawPixels(), DisplayProfile::Width * DisplayProfile::Height);
    }

    template<>
    inline void Framebuffer<display_profile::RGBADouble>::startRendering() {
        using namespace display_profile;
        updateLine_ = 0;
        ST7789::updatePixelsPartial(rawPixels(), DisplayProfile::Height, [this](){
            if (updateLine_ == DisplayProfile::Width - 1)
                ST7789::updatePixels(rawPixels() + DisplayProfile::Height * (RGBDouble::Width - 1), DisplayProfile::Height);
            else 
               ST7789::updatePixelsPartial(rawPixels() + DisplayProfile::Height * updateLine_++, DisplayProfile::Height * 2);
        }); 
    }


} // namespace rckid