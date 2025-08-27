#include "gbctests.h"

namespace rckid::gbcemu {

    TEST(gbcemu, memory_storeload8) {
        GBCEmu gbc{"", nullptr};
        RUN(
            LD_HL_imm16(0xc000),
            LD_A_imm8(0x12),
            LD_incHL_A,
            INC_A,
            LD_incHL_A,
        );
        EXPECT(gbc.hl(), 0xc002);
        EXPECT(gbc.readMem(0xc000), 0x12);
        EXPECT(gbc.readMem(0xc001), 0x13);
    }

    TEST(gbcemu, memory_stack) {
        GBCEmu gbc{"", nullptr};
        RUN(
            LD_SP_imm16(0xd000),
            LD_HL_imm16(0xcffe),
            LD_B_imm8(0x12),
            LD_C_imm8(0x34),
            PUSH_BC,
            POP_DE,
            LD_A_incHL,
        );
        EXPECT(gbc.sp(), 0xd000);
        EXPECT(gbc.b(), 0x12);
        EXPECT(gbc.c(), 0x34);
        EXPECT(gbc.de(), 0x1234);
        EXPECT(gbc.a(), 0x34);
        EXPECT(gbc.hl(), 0xcfff);
    }

    TEST(gbcemu, memory_oam) {
        GBCEmu gbc{"", nullptr};
        RUN(
            LD_HL_imm16(0xfe00),
            LD_A_L,
            LD_incHL_A,        
            LD_A_L,
            LD_incHL_A,        
            LD_A_L,
            LD_incHL_A,        
            LD_HL_imm16(0xfe00),
            LD_A_incHL,
            LD_B_A,
            LD_A_incHL,
            LD_C_A,
            LD_A_incHL,
        );
        EXPECT(gbc.hl(), 0xfe03);
        EXPECT(gbc.readMem(0xfe00), 0);
        EXPECT(gbc.readMem(0xfe01), 1);
        EXPECT(gbc.readMem(0xfe02), 2);
        EXPECT(gbc.b(), 0);
        EXPECT(gbc.c(), 1);
        EXPECT(gbc.a(), 2);
    }

    TEST(gbcemu, memory_hram) {
        GBCEmu gbc{"", nullptr};
        RUN(
            LD_HL_imm16(0xff80),
            LD_A_imm8(10),
            LD_incHL_A,        
            INC_A,
            LD_incHL_A,        
            INC_A,
            LD_incHL_A,        
            LD_HL_imm16(0xff80),
            LD_A_incHL,
            LD_B_A,
            LD_A_incHL,
            LD_C_A,
            LD_A_incHL,
        );
        EXPECT(gbc.hl(), 0xff83);
        EXPECT(gbc.readMem(0xff80), 10);
        EXPECT(gbc.readMem(0xff81), 11);
        EXPECT(gbc.readMem(0xff82), 12);
        EXPECT(gbc.b(), 10);
        EXPECT(gbc.c(), 11);
        EXPECT(gbc.a(), 12);
    }

} // namespace rckid::gbcemu