#include "gbctests.h"

TEST(simple, nop) {
    GBC gbc{};
    RUN(
        NOP, 
    );
    EXPECT(gbc.pc(), 3);
    EXPECT(gbc.cyclesElapsed(), 8);
}

TEST(simple, regLoads) {
    GBC gbc{};
    RUN(
        LD_A_imm8(0x12),
        LD_B_imm8(0x34),
        LD_C_imm8(0x56),
        LD_HL_imm16(0xc000),
    );
    EXPECT(gbc.a(), 0x12);
    EXPECT(gbc.b(), 0x34);
    EXPECT(gbc.c(), 0x56);
    EXPECT(gbc.bc(), 0x3456);
    EXPECT(gbc.hl(), 0xc000);
}
