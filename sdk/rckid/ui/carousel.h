#pragma once

#include "menu.h"

namespace rckid {

    /** Holds menu, manages resources and draws carousel, but does not deal with the application aspects. */
    template<typename COLOR>
    class Carousel {


    private:
        Menu * menu_;
        uint32_t i_;

    }; // rckid::Carousel

} // namespace rckid