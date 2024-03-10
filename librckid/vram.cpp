#include <cstdint>
#include <cstdio>

#include "rckid/rckid.h"

#if (defined LIBRCKID_MOCK)
// fake section start & end to keep linker happy
uint8_t __vram_start__;
uint8_t __vram_end__;



namespace rckid {

    // real allocation for the mock mode
    uint8_t __vram__[RCKID_VRAM_SIZE];
    size_t __rckid_vram_size__ = RCKID_VRAM_SIZE;

    size_t freeVRAM() { return (__vram__ + RCKID_VRAM_SIZE) - Device::vramPtr_; }

    void resetVRAM() { Device::vramPtr_ = __vram__; }

    bool isVRAMPtr(void * ptr) { return (ptr >= static_cast<void*>(& __vram_start__)) && (ptr < static_cast<void*>(& __vram_end__)); }

}
#else
namespace rckid {
    // fake allocation for the VRAM of properly set size
    uint8_t __attribute__((section (".vram"))) __vram__[RCKID_VRAM_SIZE];
}
#endif
