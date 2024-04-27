#pragma once

#include "rckid.h"

namespace rckid {

    class SD {
    public:

        static bool usbMscAvailable() { return usbMscAvailable_; }

        static uint32_t numBlocks() { return numBlocks_; }

        static uint16_t blockSize() { return blockSize_; }

    private:
        // usb storage configuration
        static inline bool usbMscAvailable_ = false;
        static inline uint32_t numBlocks_ = 16;
        static inline uint16_t blockSize_ = 512;
        static inline size_t usbMscNumEvents_ = 0;  


    }; // rckid::SD


} // namespace rckid