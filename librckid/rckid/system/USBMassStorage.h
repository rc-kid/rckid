#pragma once

#include "rckid/app.h"
#include "rckid/sd.h"
#include "rckid/graphics/framebuffer.h"

namespace rckid {
    /** USB Mass Storage device.  
     
        The app has to be active in order for the mass storage attachment to work. 
     */
    class USBMassStorage : public App<FrameBuffer<ColorRGB>> {
    public:

        static USBMassStorage * create() { return new USBMassStorage(); }

    protected:

        void onFocus() override {
            App::onFocus();
            SD::enableUsbMsc(true);

        }

        void onBlur() override {
            // TODO: remount the SD card
            App::onBlur();
            SD::enableUsbMsc(false);
        }

        void update() override {
            if (pressed(Btn::B))
                exit();
        }

        void draw() override {
            driver_.fill();
            driver_.text(0,0) << "R: " << SD::numMscReads() << ", W: " << SD::numMscWrites();
        }

    private:

        
    }; // USBMassStorage

} // namespace rckid
