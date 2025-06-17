#include <platform.h>
#include <platform/string_utils.h>
#include "gamepak.h"

#ifdef RCKID_BACKEND_FANTASY

namespace rckid::gbcemu {
    FileGamePak::FileGamePak(String const & filename) {
        SystemMallocGuard g;
        try {
            std::ifstream input(filename.c_str(), std::ios::binary | std::ios::ate);
            std::streamsize fileSize = input.tellg();
            input.seekg(0, std::ios::beg);
            rom_ = new uint8_t[fileSize];
            if (!input.read(reinterpret_cast<char*>(rom_), fileSize))
                throw std::runtime_error(STR("Error reading file " << filename).c_str());
            input.close();
        } catch (...) {
            throw;
        }
    }
}

#endif