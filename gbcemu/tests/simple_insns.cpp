#include "gbctests.h"

namespace rckid::gbcemu {

    TEST(gbcemu, simple_nop) {
        GBCEmu gbc{};
        RUN(
            NOP, 
        );
        EXPECT_CYCLES(8);
    }

    TEST(gbcemu, simple_scf) {
        GBCEmu gbc{};
        RUN(
            SCF,
        );
        // sets the carry flag, but zero flag exists from the console startup
        EXPECT_FLAGS(C | Z);
    }
    
    TEST(gbcemu, simple_regLoads) {
        GBCEmu gbc{};
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

    TEST(gbcemu, opcode_test) {
        GBCEmu gbc{};
        uint8_t cartridge[0x8000];
        gbc.loadCartridge(new FlashGamePak(cartridge));
        // 01 0000
        gbc.setState(0xe5fe, 0x3246, 0x51f0, 0x64a2, 0xa5bd, 0x9570, 0x0, 0x0);
        gbc.writeMem(0xe5fe, { 0x1, 0xbb, 0x2d });
        gbc.step();
        EXPECT(gbc.a(),  0x51);
        EXPECT(gbc.b(),  0x2d);
        EXPECT(gbc.c(),  0xbb);
        EXPECT(gbc.d(),  0xa5);
        EXPECT(gbc.e(),  0xbd);
        EXPECT(gbc.h(),  0x95);
        EXPECT(gbc.l(),  0x70);
        EXPECT(gbc.sp(), 0x3246);
        EXPECT(gbc.pc(), 0xe601);
        EXPECT(gbc.ime(), 0x0);
        EXPECT(gbc.flagZ(),  true);
        EXPECT(gbc.flagN(),  true);
        EXPECT(gbc.flagH(),  true);
        EXPECT(gbc.flagC(),  true);
        EXPECT(gbc.readMem(0xe5fe), 0x1);
        EXPECT(gbc.readMem(0xe5ff), 0xbb);
        EXPECT(gbc.readMem(0xe600), 0x2d);
    }
}