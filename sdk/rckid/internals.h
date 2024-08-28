#pragma once

#include "rckid.h"

namespace rckid {

    /** Displays a blue screen of death with extra information about the error. 
     
        The function uses the simplest 16 bpp color fullscreen framebuffer, so callers must ensure there is enough free RAM to allocate the required bitmap. This can usually be done by leaving all opened memory areas as the BSOD is not expected to return to normal app code. 

        The function is expected to be called from the 
     */
    void bsod(uint32_t error, uint32_t line, char const * file, char const * extras = nullptr);

} // namespace rckid
