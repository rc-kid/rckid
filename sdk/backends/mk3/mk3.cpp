/** \page backend_mk3 Mk III 

    Mark III, currently in development whose hardware specifications are still in progress.
*/

#ifndef RCKID_BACKEND_MK3
#error "You are building fantasy (RayLib) backend without the indicator macro"
#endif

#include "rckid/rckid.h"

namespace rckid {

    void initialize() {

    }

    void initialize() {
    }

    void fatalError(uint32_t error, uint32_t line, char const * file) {
        // clear all memory arenas to clean up space, this is guarenteed to succeed as the SDK creates memory arena when it finishes initialization    
        /**
        memoryReset();
        bsod(error, line, file, nullptr);
        systemMalloc_ = true;
        if (sdIso_.good())
            sdIso_.close();
        if (flashIso_.good())
            flashIso_.close();
        while (! WindowShouldClose())
            PollInputEvents();
        systemMalloc_ = false;
        */
        std::exit(EXIT_FAILURE);
    }




}