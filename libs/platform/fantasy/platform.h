#pragma once

#include <stdint.h>

#include "../definitions.h"


#define UNIMPLEMENTED do {} while (false)
#define UNREACHABLE do {} while (false)

#define ASSERT(...)

#define LOG(...) rckid::debugWrite() << __VA_ARGS__ << '\n';
