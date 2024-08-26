/** 
    \section rckid__mk2_backend RCKid mk II Backend 
    \addtogroup backends
 
    NOTE: This is a temporary backend that uses the older V2 revision (RP2040 and ATTiny) to allow running the basic SDK on the previous RCKid hardware version. Once the V3 hardware is built and tested, this code will be obsoleted and removed from the repository. 
 */

#ifndef ARCH_RCKID_2
#error "You are building RCKid mk II backend without the indicator macro"
#endif

#include "pico/rand.h"
#include "bsp/board.h"
#include "tusb_config.h"
#include "tusb.h"
#include "hardware/structs/usb.h"


#include "screen/ST7789.h"

#include "rckid/rckid.h"




namespace rckid {

    namespace {
        DisplayMode displayMode_ = DisplayMode::Off;
        DisplayUpdateCallback displayCallback_;

    }

    void initialize() {

    }

    void tick() {

    }

    void yield() {

    }

    void fatalError(uint32_t error, uint32_t line, char const * file) {
    }

    void fatalError(Error error, uint32_t line, char const * file) {
        fatalError(static_cast<uint32_t>(error), line, file);
    }

    uint32_t uptimeUs() {

    }

    uint32_t random() { return get_rand_32(); }

    Writer debugWrite() {
        return Writer{[](char x) {
            tud_cdc_write(& x, 1);
            if (x == '\n')
                tud_cdc_write_flush();            
        }};
    }    

    // io

    bool btnDown(Btn b) {

    }

    bool btnPressed(Btn b) {

    }

    bool btnReleased(Btn b) {

    }

    // display

    DisplayMode displayMode() { return displayMode_; }

    void displaySetMode(DisplayMode mode) {
        if (displayMode_ == mode)
            return;
        displayMode_ = mode;
        switch (displayMode_) {
            case DisplayMode::Native:
            case DisplayMode::NativeDouble:
            case DisplayMode::Natural:
            case DisplayMode::NaturalDouble:
            case DisplayMode::Off:
                break;            
        }
    }

    uint8_t displayBrightness() { 

    }

    void displaySetBrightness(uint8_t value) {  }

    Rect displayUpdateRegion() { 

    }

    void displaySetUpdateRegion(Rect region) { 
    }

    bool displayUpdateActive() { 

    }

    void displayWaitVSync() { 
        ST7789::waitVSync();
    }

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels) {
    }

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels, DisplayUpdateCallback callback) {
    }


    // accelerated functions
    #include "rckid/accelerated.inc.h"
    MEM_FILL_8
    MEM_FILL_16
    MEM_FILL_32

}