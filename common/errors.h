#pragma once

namespace rckid {

    /** Error codes
     
        Error codes that will be displayed on the blue screen of death as arguments to panic for some basic debugging. These are not an enum so that users can add their own when necessary. 
     */
    enum class Error {
#define ERROR_CODE(NAME, ...) NAME, 
#include "error_codes.inc.h"
        UserError, 
    }; // rckid::Error

}