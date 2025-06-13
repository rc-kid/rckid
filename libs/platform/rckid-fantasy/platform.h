#pragma once
/** Platform for RCKid fantasy console. 
 
    The fantasy console is essentially just a pc platform with a few exceptions, that are listed below. Notably, rckid provides its own string implementation. 
 */

#define PLATFORM_NO_STDSTRING

#include "../pc/platform.h"

// generally useful 
#include <utility>

// particularly for the host filesystem
#include <fstream>
#include <filesystem>
