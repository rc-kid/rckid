#pragma once

#include "f_util.h"
#include "ff.h"
#include "rtc.h"

#include "rckid/app.h"
#include "rckid/audio.h"
#include "rckid/graphics/framebuffer.h"
#include "fonts/Iosevka_Mono6pt7b.h"

struct sd_card_t;

namespace rckid {

    class SD {
    public:

        static bool mount();
        static void unmount();
        static bool mounted() { return card_ != nullptr; }

        /** Returns the total number of bytes available on the card. 
         */
        static uint64_t totalBytes(); 

        /** Returns the free number of bytes available on the card. Note that on larger cards, this function can actually take multiple seconds. 
         */
        static uint64_t freeBytes();

        /** Initializes the SD card in SPI mode. 
         
            This is to be executed once when the RCKid starts. 
         */
        static void initialize();

        static void processEvents();

    private:

        friend class BaseApp;

        static constexpr uint8_t CMD0 = 0; // reset the card, when sent with CS low, switches the card to SPI mode


        static constexpr size_t BYTES_PER_SECTOR = 512;

        static inline sd_card_t * card_ = nullptr;

    }; // rckid::SD


    class SDBlockingTest : public App<FrameBuffer<ColorRGB, DisplayMode::Native_2X_RGB565>> {
    public:

        SDBlockingTest() = default;

    protected:

        void onFocus() override {
            App::onFocus();
            SD::mount();
            sdSize_ = SD::totalBytes() / 1000000_u64;
        }


        void update() override {
        }

        void draw() override {
            driver_.setFg(Color::White());
            driver_.setFont(Iosevka_Mono6pt7b);
            driver_.setBg(Color::RGB(0, 0, 0));
            driver_.fill();
            driver_.text(0,0) << "Total: " << sdSize_ << "\n";
        }

    private:

        size_t sdSize_; // SD card size in megabytes

    }; 


}; // namespace rckid::sd