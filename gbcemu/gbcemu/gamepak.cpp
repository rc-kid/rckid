#include <platform.h>
#include "gamepak.h"

#ifdef RCKID_BACKEND_FANTASY

extern thread_local bool systemMalloc_;

namespace rckid::gbcemu {
    FileGamePak::FileGamePak(std::string const & filename) {
        systemMalloc_ = true;
        try {
            std::ifstream input(filename, std::ios::binary | std::ios::ate);
            std::streamsize fileSize = input.tellg();
            input.seekg(0, std::ios::beg);
            rom_ = new uint8_t[fileSize];
            if (!input.read(reinterpret_cast<char*>(rom_), fileSize))
                throw std::runtime_error(STR("Error reading file " << filename));
            input.close();
            systemMalloc_ = false;
        } catch (...) {
            systemMalloc_ = false;
            throw;
        }
    }
}

#endif