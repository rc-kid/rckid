#include "gbctests.h"

namespace rckid::gbcemu {

    TEST(gbcemu, jumps_jp) {
        GBCEmu gbc{"", nullptr};
        RUN(
            // 0x150
            LD_A_imm8(123),
            // 2
            JP(0x157),
            // 5
            LD_A_imm8(151),
            // 7
        );
        EXPECT(gbc.a(), 123);
        EXPECT_CYCLES(8 + 16);
    }


    TEST(gbcemu, jumps_jp_c_nc) {
        GBCEmu gbc{"", nullptr};
        RUN(
            // 0
            LD_A_imm8(123),
            // 2
            JP_C(0x157),
            // 5
            LD_A_imm8(151),
            // 7
        );
        EXPECT(gbc.a(), 151);
        EXPECT_CYCLES(8 + 12 + 8);
        RUN(
            // 0
            LD_A_imm8(123),
            // 2
            SCF,
            // 3
            JP_C(0x158),
            // 6
            LD_A_imm8(151),
            // 8
        );
        EXPECT(gbc.a(), 123);
        EXPECT_CYCLES(8 + 4 + 16);
    }

    TEST(gbcemu, jumps_jr) {
        GBCEmu gbc{"", nullptr};
        RUN(
            LD_A_imm8(123),
            JR(2),
            LD_A_imm8(151),
        );
        EXPECT(gbc.a(), 123);
        EXPECT_CYCLES(8 + 12);
    }

    TEST(gbcemu, jumps_jr_c_nc) {
        GBCEmu gbc{"", nullptr};
        RUN(
            LD_A_imm8(123),
            JR_C(2),
            LD_A_imm8(151),
        );
        EXPECT(gbc.a(), 151);
        EXPECT_CYCLES(8 + 8 + 8);
        RUN(
            LD_A_imm8(123),
            SCF,
            JR_C(2),
            LD_A_imm8(151),
        );
        EXPECT(gbc.a(), 123);
        EXPECT_CYCLES(8 + 4 + 12);
    }

} // namespace rckid::gbcemu