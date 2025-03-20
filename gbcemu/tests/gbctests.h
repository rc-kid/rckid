#pragma once

#include <platform/tests.h>

#include "../gbcemu/assembler.h"
#include "../gbcemu/gbcemu.h"

#define Z (1 << 7)
#define N (1 << 6)
#define H (1 << 5)
#define C (1 << 4)

/** Runs given test.
    
    Creates a very crude static gamepak with the given instructions starting from 0x150 and a jump at 0x100 with 32kb rom by default (at 0x148). 
 */
#define RUN(...) do { uint8_t pgm[] = { \
    /* 000 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 010 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 020 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 030 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 040 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 050 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 060 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 070 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 080 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 090 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 0a0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 0b0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 0c0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 0d0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 0e0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 0f0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 100 */ JP(0x150), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 110 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 120 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 130 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
    /* 140 */ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, \
    __VA_ARGS__ BKPT }; gbc.loadCartridge(new FlashGamePak{pgm}); gbc.run(); } while (false)

#define EXPECT_FLAGS(...) EXPECT((int)gbc.f(), (static_cast<int>(__VA_ARGS__)))

#define EXPECT_CYCLES(cycles) EXPECT(gbc.elapsedCycles(), 16 + cycles)

namespace rckid::gbcemu {

} // namespace rckid::gbcemu
