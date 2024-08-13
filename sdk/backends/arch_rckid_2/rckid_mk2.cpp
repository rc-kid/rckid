/** \page mk2_backend RCKid mk II backend 
 
    NOTE: This is a temporary backend that uses the older V2 revision (RP2040 and ATTiny) to allow running the basic SDK on the previous RCKid hardware version. Once the V3 hardware is built and tested, this code will be obsoleted and removed from the repository. 
 */

#ifndef ARCH_RCKID_2
#error "You are building RCKid mk II backend without the indicator macro"
#endif

#include "bsp/board.h"
#include "tusb_config.h"
#include "tusb.h"
#include "hardware/structs/usb.h"


#include "rckid/rckid.h"


namespace rckid {

    void initialize() {

    }

    void tick() {

    }

    Writer debugWrite() {
        return Writer{[](char x) {
            tud_cdc_write(& x, 1);
            if (x == '\n')
                tud_cdc_write_flush();            
        }};
    }    

}