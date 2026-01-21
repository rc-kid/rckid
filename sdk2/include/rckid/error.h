#pragma once

#include <platform.h>
#include <rckid/hal.h>

#define UNIMPLEMENTED do { rckid::hal::device::fatalError(__FILE__, __LINE__, "UNIMPLEMENTED"); } while (false)
#define UNREACHABLE do { rckid::hal::device::fatalError(__FILE__, __LINE__,  "UNREACHABLE"); } while (false)

#define ASSERT(...) do { if (!(__VA_ARGS__)) rckid::hal::device::fatalError(__FILE__, __LINE__, "ASSERT"); } while (false)

#define FATAL_ERROR(...) do {rckid::hal::device::fatalError(__FILE__, __LINE__, __VA_ARGS__ ); } while (false)

extern "C" {
    /** printf-like wrapper around the debugWrite() rckid function so that third party libraries can be used seamlessly.
     */
    void debug_printf(char const * fmt, ...);
}
