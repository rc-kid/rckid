#pragma once

#include <platform.h>
#include <platform/writer.h>

#define UNIMPLEMENTED do { rckid::fatalError(rckid::error::Unimplemented, __LINE__, __FILE__); } while (false)
#define UNREACHABLE do { rckid::fatalError(rckid::error::Unreachable, __LINE__, __FILE__); } while (false)

#define ASSERT(...) do { if (!(__VA_ARGS__)) rckid::fatalError(rckid::error::Assert, __LINE__, __FILE__); } while (false)

#define ERROR(ERRNO) do { rckid::fatalError(ERRNO, __LINE__, __FILE__); } while (false)
#define ERROR_IF(ERRNO, ...) do { if ((__VA_ARGS__)) ERROR(ERRNO); } while (false)

namespace rckid {
    namespace error {
        static constexpr uint32_t Success = 0;
        static constexpr uint32_t Unimplemented = 1;
        static constexpr uint32_t Unreachable = 2;
        static constexpr uint32_t Assert = 3;
        static constexpr uint32_t OutOfMemory = 4;
        static constexpr uint32_t StackProtectionFailure = 5;
        static constexpr uint32_t USBMSCRead = 6;
        static constexpr uint32_t USBMSCWrite = 7;
    }; // rckid::error

    /** Raises fatal error. 
     
        Since exceptions are not supported, rasing errors is akin to blue screen of death, which is indeed what this function shows. The line and file can be specified for debugging purposes and will be displayed on screen. There is no recovery from fatal errors and the device should be reset afterwards. 

     */

    NORETURN(void fatalError(uint32_t error, uint32_t arg, uint32_t line = 0, char const * file = nullptr));

    NORETURN(void fatalError(uint32_t error, uint32_t line = 0, char const * file = nullptr));

    inline void fatalError(uint32_t error, uint32_t line, char const * file) {
        fatalError(error, 0, line, file);
    }

    /** Displays the blue screen of death and hangs indefinitely. This is the ultimate end of all fatal errors, but has to be called safely from the fatal error handler after all hardware is correctly torn down.
     */
    NORETURN(void bsod(uint32_t error, uint32_t arg, uint32_t line = 0, char const * file = nullptr, char const * extras = nullptr));

} // namespace rckid