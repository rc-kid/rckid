#pragma once

#include <kiddo/kiddo.h>

namespace kiddo {

    class Pedometer {
    public:

        static Pedometer * instance();

        uint32_t count() { return count_; }

        void reset(); 

    private:
        uint32_t count_ = 0;

    }; // kiddo::Pedometer

} // namespace kiddo