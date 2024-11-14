#pragma once

#include <platform.h>

namespace rckid {

    /** Error enum. 
     
        To be used with the fatalError() function. The following error codes are reserved for the SDK, while any value equal or larger to Error::User can be used by the application itself. 
     */
    enum class Error : uint32_t {
        NoError = 0, 
        Unimplemented = 1,
        Unreachable, 
        Assert, 
        User, 
    }; // rckid::Error

    NORETURN(void fatalError(uint32_t error, uint32_t line = 0, char const * file = nullptr));

    NORETURN(void fatalError(Error error, uint32_t line = 0, char const * file = nullptr));

}