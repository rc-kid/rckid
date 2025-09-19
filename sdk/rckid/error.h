#pragma once

#include <platform.h>
#include <platform/writer.h>

#define UNIMPLEMENTED do { rckid::Error::setFatal(rckid::Error{__LINE__, __FILE__, rckid::Error::unimplemented }); } while (false)
#define UNREACHABLE do { rckid::Error::setFatal(rckid::Error{__LINE__, __FILE__, rckid::Error::unreachable }); } while (false)

#define ASSERT(...) do { if (!(__VA_ARGS__)) rckid::Error::setFatal(rckid::Error{__LINE__, __FILE__, rckid::Error::assertionFailure}); } while (false)

#define FATAL_ERROR(...) do {rckid::Error::setFatal(rckid::Error{__LINE__, __FILE__, __VA_ARGS__ }); } while (false)
#define FATAL_ERROR_IF(COND, ...) do { if (COND) FATAL_ERROR(__VA_ARGS__); } while (false)




namespace rckid {
    struct Error {
        static constexpr uint32_t success = 0;
        static constexpr uint32_t unspecified = 1;
        static constexpr uint32_t unimplemented = 2;
        static constexpr uint32_t unreachable = 3;
        static constexpr uint32_t assertionFailure = 4;
        static constexpr uint32_t outOfMemory = 5;
        static constexpr uint32_t stackProtectionFailure = 6;
        static constexpr uint32_t hardwareFailure = 7;

        uint32_t code = unspecified;
        uint32_t arg = 0;
        char const * msg = nullptr;
        uint32_t line = 0;
        char const * file = nullptr;

        Error() = default;
        
        Error(uint32_t line, char const * file, uint32_t code = unspecified, uint32_t arg = 0, char const * msg = nullptr) :
            code{code},
            arg{arg},
            msg{msg},
            line{line},
            file{file} {
        }

        Error(uint32_t line, char const * file, uint32_t code, char const * msg) :
            code{code},
            msg{msg},
            line{line},
            file{file} {
        }

        NORETURN(static void setFatal(Error err));

        static Error const & last() { return last_; }

        static void set(Error err) { last_ = err; }

        static void clear() { last_ = Error{}; }


    private:

        NORETURN(static void bsod());

        static Error last_;

    }; // rckid::error

    /** Raises fatal error. 
     
        Since exceptions are not supported, rasing errors is akin to blue screen of death, which is indeed what this function shows. The line and file can be specified for debugging purposes and will be displayed on screen. There is no recovery from fatal errors and the device should be reset afterwards. 
     */
} // namespace rckid