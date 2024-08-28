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

/** 
    \section rckid__mk2_backend RCKid mk II Backend 
    \addtogroup backends
 
    NOTE: This is a temporary backend that uses the older V2 revision (RP2040 and ATTiny) to allow running the basic SDK on the previous RCKid hardware version. Once the V3 hardware is built and tested, this code will be obsoleted and removed from the repository. 

    The V2 backend is rather complicated because of the small number of RP2040 IO pins required an additional MCU - ATTiny3217 that controls power management and input/output (buttons, LEDs, rumbler). 
 */

namespace rckid {

    namespace {
        DisplayMode displayMode_ = DisplayMode::Off;
        DisplayUpdateCallback displayCallback_;

    }

    void __not_in_flash_func(irqDMADone_)() {
        //gpio::outputHigh(GPIO21);
        unsigned irqs = dma_hw->ints0;
        dma_hw->ints0 = irqs;
        // for audio, reset the DMA start address to the beginning of the buffer and tell the stream to refill
//        if (irqs & (1u << audio::dma0_))
//            audio::irqHandler1();
//        if (irqs & (1u << audio::dma1_))
//            audio::irqHandler2();
        // display
        if (irqs & ( 1u << ST7789::dma_))
            ST7789::irqHandler();
        //gpio::outputLow(GPIO21);
    }



    void initialize() {
        ST7789::initialize();

    }

    void tick() {

    }

    void yield() {
        tight_loop_contents();
        tud_task();
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

    DisplayMode displayMode() { 
        return ST7789::displayMode();
     }

    void displaySetMode(DisplayMode mode) {
        ST7789::setDisplayMode(mode);
    }

    uint8_t displayBrightness() { 
        // TODO Read from state
    }

    void displaySetBrightness(uint8_t value) {  
        // TODO send I2C command to AVR
    }

    Rect displayUpdateRegion() {    
        return ST7789::updateRegion();
    }

    void displaySetUpdateRegion(Rect region) { 
        ST7789::setUpdateRegion(region);
    }

    bool displayUpdateActive() {
        return ST7789::dmaUpdateInProgress();
    }

    void displayWaitVSync() { 
        ST7789::waitVSync();
    }

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels) {
        ST7789::dmaUpdateAsync(pixels, numPixels);
    }

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels, DisplayUpdateCallback callback) {
        ST7789::dmaUpdateAsync(pixels, numPixels, callback);
    }

    // accelerated functions
    #include "rckid/accelerated.inc.h"
    MEM_FILL_8
    MEM_FILL_16
    MEM_FILL_32

}