#include <cstdint>

#if (defined LIBRCKID_MOCK)
// real allocation for the mock mode
uint8_t  _vram[RCKID_VRAM_SIZE];
#else
// fake allocation for the VRAM of properly set size
uint8_t __attribute__((section (".vram"))) _vram[RCKID_VRAM_SIZE];
#endif

