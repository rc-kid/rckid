#pragma once

#include <platform.h>

namespace rckid {

    class ID3 {
    public:

        static uint32_t getID3Size(uint8_t const * buffer) {
            if (buffer[0] == 'I' && buffer[1] == 'D' && buffer[2] == '3')
                return synchSafeInt(buffer + 6);
            return 0;
        }

    private:

        static uint32_t synchSafeInt(uint8_t const * buffer) {
            return (buffer[0] << 21) | (buffer[1] << 14) | (buffer[2] << 7) | buffer[3];
        }

    }; // rckid::ID3

} // namespace rckid
