#pragma once

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

    private:

        static inline sd_card_t * card_ = nullptr;

    }; // rckid::SD

}; // namespace rckid::sd