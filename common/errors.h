#pragma once

#include <stdint.h>

namespace rckid {

    /** Error codes
     
        Error codes that will be displayed on the blue screen of death as arguments to panic for some basic debugging. These are not an enum so that users can add their own when necessary. 
     */
    enum class Error : uint16_t {
#define ERROR_CODE(NAME, ...) NAME, 
#include "error_codes.inc.h"
        UserError, 
    }; // rckid::Error

}