#pragma once

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/ui/header.h"

#include "sd.h"

namespace rckid {

    /** USB Mass Storage device.  
     
        The app has to be active in order for the mass storage attachment to work. It detaches the SD card from the RCKid async DMA routines and makes it exclusively available to the USB connected PC as a mass storage device. 

        Furthermore, it detects if DC power is applied while the app is on, and if it is, initializes the usb stack. We can'd do this immediately because the ESD protection circuit on the USB leaks the pullups on the dataline to the VCC which then prevents the USB aware devices to connect and charge the RCKid properly. 

        When the app is exitted, the USB device is turned off, which re-enables the DC voltage detection & charging. 

     */
    class USBMassStorage : public App<FrameBuffer<ColorRGB>> {
    public:

        static USBMassStorage * create() { return new USBMassStorage(); }

    protected:

        void onFocus() override;

        void onBlur() override;

        void update() override;

        void draw() override {
            driver_.fill();
            header_.drawOn(driver_, Rect::WH(320, 20));

            driver_.textMultiline(0,20) <<
                "Label:    " << label_ << "\n" <<
                "Format:   " << format_ << "\n" <<
                "Capacity: " << capacity_ << "\n" <<
                "Free:     " << free_ << "\n" <<
                "Blocks:   " << SD::numBlocks() << "\n\n" << 
                "R: " << stats::sdReadBlocks() << ", W: " << stats::sdWriteBlocks();

        }

    private:

        header::Renderer<Color> header_;

        std::string label_;
        std::string format_;
        uint64_t capacity_;
        uint64_t free_;


        
    }; // USBMassStorage

} // namespace rckid
