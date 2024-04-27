#pragma once

#include <platform.h>

namespace rckid {

    /** \name Cartridge pins. 
     */
    //@{
    constexpr gpio::Pin GPIO14 = gpio::Pin{14};
    constexpr gpio::Pin GPIO15 = gpio::Pin{15};
    constexpr gpio::Pin GPIO16 = gpio::Pin{16};
    constexpr gpio::Pin GPIO17 = gpio::Pin{17};
    constexpr gpio::Pin GPIO18 = gpio::Pin{18};
    constexpr gpio::Pin GPIO19 = gpio::Pin{19};
    constexpr gpio::Pin GPIO20 = gpio::Pin{20};
    constexpr gpio::Pin GPIO21 = gpio::Pin{21};
    //@}

    /** Buttons. 
    */
    enum class Btn {
        Left, 
        Right,
        Up, 
        Down, 
        A, 
        B, 
        Select, 
        Start,
        Home, 
        VolumeUp, 
        VolumeDown,
    }; // rckid::Btn

    /** \name Error codes
     
        Error codes that will be displayed on the blue screen of death as arguments to panic for some basic debugging. These are not an enum so that users can add their own when necessary. 
     */
    //@{
    constexpr int INTERNAL_ERROR = 1;
    constexpr int ASSERTION_ERROR = 2;
    constexpr int VRAM_OUT_OF_MEMORY = 3;
    constexpr int HEAP_OUT_OF_MEMORY = 4;

    constexpr int NOT_IMPLEMENTED_ERROR = 256;
    constexpr int UNREACHABLE_ERROR = 257;
    //@}

}