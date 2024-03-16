#pragma once
#if (defined DEBUG_DISPLAY)

#include "peripherals/ssd1306.h"

namespace platform {

    class DebugDisplay {
    public:
        static inline SSD1306 oled;

        static void initialize() {
            oled.initialize128x32();
            oled.normalMode();
            oled.clear32();
        }

    }; // DebugDisplay

#define DDISP_INITIALIZE() DebugDisplay::initialize()

#define DDISP(...) DebugDisplay::oled.write(__VA_ARGS__)

#else

#define DDISP_INITIALIZE()
#define DDISP(...)

#endif

} // namespace platform