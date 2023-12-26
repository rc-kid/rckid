#pragma once

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"

namespace rckid {

    class USBMassStorage : public App<Framebuffer<display_profile::RGB>> {
    public:

        USBMassStorage()  = default;

        USBMassStorage(App * parent): App{parent} { }


        static uint32_t numBlocks() { return numBlocks_; }
        static uint16_t blockSize() { return blockSize_; }
        static void addEvent() { ++numEvents_; }

    protected:

        void onFocus() override {
            App::onFocus();
            // TODO: fill in the numBlocks and blockSize, unmount SD card
            numEvents_ = 0;

        }

        void onBlur() override {
            // TODO: remount the SD card
        }

        void update() override;

        void draw() override;

    private:

        static inline uint32_t numBlocks_ = 16;
        static inline uint16_t blockSize_ = 512;

        static inline size_t numEvents_ = 0;
        
    }; // USBMassStorage

} // namespace rckid
