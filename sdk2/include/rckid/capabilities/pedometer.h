#pragma once

#include <rckid/rckid.h>

namespace rckid {

    class Pedometer {
    public:

        static Pedometer * instance();

        uint32_t count() { return count_; }

        void reset(); 

    private:
        uint32_t count_ = 0;

    }; // rckid::Pedometer

} // namespace rckid