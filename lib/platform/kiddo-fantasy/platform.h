#pragma once
/** Platform for KidDo fantasy console. 
 
    The fantasy console is essentially just a pc platform with a few exceptions, that are listed below. Notably, kiddo provides its own string implementation. 
 */

#define PLATFORM_NO_STDSTRING

#include "../pc/platform.h"

// particularly for the host filesystem
#include <fstream>
#include <filesystem>
