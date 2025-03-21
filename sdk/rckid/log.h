#pragma once

#include <platform.h>
#include <platform/writer.h>

#define IS_LOGLEVEL_ENABLED_HELPER(X) #X
#define IS_LOGLEVEL_ENABLED(X) (IS_LOGLEVEL_ENABLED_HELPER(X)[0] == '1')
#define LOG(LOGLEVEL,...) do { if (IS_LOGLEVEL_ENABLED(LOGLEVEL)) rckid::debugWrite() << #LOGLEVEL << ": " << __VA_ARGS__ << '\n'; } while (false)

#define LL_HEAP 0
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

#define LL_GBCEMU_INS 1
//#define LL_GBCEMU_SERIAL 1

namespace rckid {

    /** Returns writer intended for debugging purposes.
     
        This is usually the USB to UART bridge on the device, or dedicated UART port via cartridge GPIO or in the fantasy console a standard stdout.
     */
    Writer debugWrite();

    uint8_t debugRead(bool echo = true);

    inline bool debugReadAndAddHexDigit(uint32_t & value) {
        uint8_t x = debugRead();
        if (x >= '0' && x <= '9')
            x = x - '0';
        else if (x >= 'a' && x <= 'f')
            x = x - 'a' + 10;
        else if (x >= 'A' && x <= 'F')
            x = x - 'A' + 10;
        else 
            return false;
        value = (value << 4) | x;
        return true;
    }

    inline uint8_t debugReadHex8() {
        uint32_t result = 0;
        if (!debugReadAndAddHexDigit(result))
            return result;
        debugReadAndAddHexDigit(result);
        return result;
    }

    inline uint16_t debugReadHex16() {
        uint32_t result = 0;
        if (!debugReadAndAddHexDigit(result))
            return result;
        if (!debugReadAndAddHexDigit(result))
            return result;
        if (!debugReadAndAddHexDigit(result))
            return result;
        debugReadAndAddHexDigit(result);
        return result;
    }

}