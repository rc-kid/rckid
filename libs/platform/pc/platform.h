#pragma once

#include <sstream>
#include <string>

#include "../definitions.h"

#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()