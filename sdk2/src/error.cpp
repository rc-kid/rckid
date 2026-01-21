#include <cstdarg>

#include <rckid/rckid.h>

extern "C" {
    void debug_printf(char const * fmt, ...) {
        char buf[512]; // just to be sure
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        rckid::debugWrite() << buf;
    }
}
