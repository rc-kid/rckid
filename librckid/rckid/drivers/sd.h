#pragma once
#ifdef HAHA

#include "f_util.h"
#include "ff.h"
#include "rtc.h"

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

    private:

        friend class BaseApp;

        static constexpr uint8_t CMD0 = 0; // reset the card, when sent with CS low, switches the card to SPI mode

        /** Initializes the SD card in SPI mode. 
         
            This is to be executed once when the RCKid starts. 
         */
        static void initialize();

        static void processEvents();

        static constexpr size_t BYTES_PER_SECTOR = 512;

        static inline sd_card_t * card_ = nullptr;

    }; // rckid::SD

}; // namespace rckid::sd

#endif // HAHA