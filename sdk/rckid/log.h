#pragma once

#include <platform.h>
#include <platform/writer.h>

#define IS_LOGLEVEL_ENABLED_HELPER(X) #X
#define IS_LOGLEVEL_ENABLED(X) (IS_LOGLEVEL_ENABLED_HELPER(X)[0] == '1')
#define LOG(LOGLEVEL,...) do { if (IS_LOGLEVEL_ENABLED(LOGLEVEL)) rckid::debugWrite() << #LOGLEVEL << ": " << __VA_ARGS__ << '\n'; } while (false)

#define LL_HEAP 1
#define LL_INFO 1

#ifndef LL_ERROR
#define LL_ERROR 1
#endif

#ifndef LL_WARN
#define LL_WARN 1
#endif

#ifndef NDEBUG
#ifndef LL_INFO
#define LL_INFO 1
#endif
#endif

namespace rckid {

    /** Returns writer intended for debugging purposes.
     
        This is usually the USB to UART bridge on the device, or dedicated UART port via cartridge GPIO or in the fantasy console a standard stdout.
     */
    Writer debugWrite();

}