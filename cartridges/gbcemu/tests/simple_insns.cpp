#include "utils/tests.h"

#include "../lib/gbc.h"

TEST(simple, nop) {
    uint8_t pgm[] = { 
        0x00, // nop 
        0x10, 0x00 // stop 0
    };
    GBC gbc{};
    gbc.setROM(pgm, sizeof(pgm));
    gbc.startTest();
    EXPECT(gbc.pc(), 3);
}