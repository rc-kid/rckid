#pragma once

/* The actual neopixel update is platform specific and this peripheral header only dispatches to the appropriate platform architecture. 
 */
#if (defined PLATFORM_AVR_MEGATINY)
#include "../attiny_series_1/neopixel_impl.inc.h"
#else
#error "Neopixels are not supported for current platform"
#endif

