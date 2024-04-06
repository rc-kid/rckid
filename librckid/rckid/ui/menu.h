#pragma once

#include "../rckid.h"

namespace rckid {

    /** Menu item, designed for minimal overhead. 
     */
    class MenuItem {
    public:
        char const * text; 
        uint8_t const * icon;
        size_t iconBytes;
    }; 



    class Menu {

    }; 


}; // namespace rckid