#pragma once

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"

namespace rckid {
    /** USB Mass Storage device.  
     
        The app has to be active in order for the mass storage attachment to work. 
     */
    class USBMassStorage : public App<FrameBuffer<ColorRGB>> {
    public:

        static USBMassStorage * create() { return new USBMassStorage(); }

        static bool available() { return available_; }

        static uint32_t numBlocks() { return numBlocks_; }
        static uint16_t blockSize() { return blockSize_; }
        static void addEvent() { ++numEvents_; }

    protected:

        void onFocus() override {
            App::onFocus();
            // TODO: fill in the numBlocks and blockSize, unmount SD card
            numEvents_ = 0;
            available_ = true;

        }

        void onBlur() override {
            // TODO: remount the SD card
            available_ = false;
            App::onBlur();
        }

        void update() override {
            if (pressed(Btn::B))
                exit();
        }

        void draw() override;

    private:

        static inline bool available_ = false;

        static inline uint32_t numBlocks_ = 16;
        static inline uint16_t blockSize_ = 512;

        static inline size_t numEvents_ = 0;
        
    }; // USBMassStorage

} // namespace rckid
