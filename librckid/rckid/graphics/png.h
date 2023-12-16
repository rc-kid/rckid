#pragma once

#include <PNGdec.h>


namespace rckid {

    /** PNG image wrapper. 
     */
    class PNG {
    public:

        static PNG fromBuffer(uint8_t const * buffer, size_t numBytes);

        /** Opens a png file from the provided memory buffer. The entire file must be in the buffer. 
         */
        PNG(uint8_t const * buffer, size_t numBytes) {
            png_.openRAM(const_cast<uint8_t*>(buffer), numBytes, nullptr);
        }

        int width() const { return png_.getWidth(); }

        int height() const { return png_.getHeight(); }

        void decode()

    private:
       mutable ::PNG png_;

    }; // rckid::PNG

} // namespace rckid