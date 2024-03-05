#include <cstdint>
#include <cstdio>

#if (defined LIBRCKID_MOCK)
// real allocation for the mock mode
uint8_t  vram_[256 * 1024 /*RCKID_VRAM_SIZE*/];
size_t rckid_vram_size_ = RCKID_VRAM_SIZE;
#else
// fake allocation for the VRAM of properly set size
uint8_t __attribute__((section (".vram"))) vram_[RCKID_VRAM_SIZE];
#endif

