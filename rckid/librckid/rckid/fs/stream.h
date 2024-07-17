#pragma once

#include "rckid/rckid.h"

namespace rckid {

    /** Input stream. 
     */
    class InStream {
    public:
        virtual ~InStream() = default;

        virtual uint32_t size() const = 0;

        virtual uint32_t read(uint8_t * buffer, uint32_t bufferSize) = 0; 

        virtual uint32_t seek(uint32_t pos) = 0;

    }; // rckid::InStream

} // namespace rckid