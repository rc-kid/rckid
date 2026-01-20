#pragma once

#include <platform.h>

namespace rckid {

    template<typename T>
    using unique_ptr = std::unique_ptr<T>;
    
} // namespace rckid