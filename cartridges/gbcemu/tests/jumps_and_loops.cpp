#include "gbctests.h"

TEST(jumps, jp) {
    GBC gbc{};
    RUN(
        // 0
        LD_A_imm8(123),
        // 2
        JP(7),
        // 5
        LD_A_imm8(151),
        // 7
    );
    EXPECT(gbc.a(), 123);
    EXPECT(gbc.cyclesElapsed(), 8 + 16 + 4);
}

TEST(jumps, jp_c_nc) {
    GBC gbc{};
    RUN(
        // 0
        LD_A_imm8(123),
        // 2
        JP_C(7),
        // 5
        LD_A_imm8(151),
        // 7
    );
    EXPECT(gbc.a(), 151);
    EXPECT(gbc.cyclesElapsed(), 8 + 12 + 8 + 4);
    RUN(
        // 0
        LD_A_imm8(123),
        // 2
        SCF,
        // 3
        JP_C(8),
        // 6
        LD_A_imm8(151),
        // 8
    );
    EXPECT(gbc.a(), 123);
    EXPECT(gbc.cyclesElapsed(), 8 + 4 + 16 + 4);
}

TEST(jumps, jr) {
    GBC gbc{};
    RUN(
        LD_A_imm8(123),
        JR(2),
        LD_A_imm8(151),
    );
    EXPECT(gbc.a(), 123);
    EXPECT(gbc.cyclesElapsed(), 8 + 12 + 4);
}

TEST(jumps, jr_c_nc) {
    GBC gbc{};
    RUN(
        LD_A_imm8(123),
        JR_C(2),
        LD_A_imm8(151),
    );
    EXPECT(gbc.a(), 151);
    EXPECT(gbc.cyclesElapsed(), 8 + 8 + 8 + 4);
    RUN(
        LD_A_imm8(123),
        SCF,
        JR_C(2),
        LD_A_imm8(151),
    );
    EXPECT(gbc.a(), 123);
    EXPECT(gbc.cyclesElapsed(), 8 + 4 + 12 + 4);
}
