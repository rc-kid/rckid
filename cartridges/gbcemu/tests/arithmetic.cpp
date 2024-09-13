#include "gbctests.h"

TEST(gbcemu, arithmetic_add8) {
    GBC gbc{};
    RUN(
        LD_A_imm8(0x12),
        LD_B_imm8(0x05),
        ADD_A_B,
    );
    EXPECT(gbc.state().a(), 0x17);
    EXPECT_FLAGS(0);
    RUN(
        LD_A_imm8(0xff),
        LD_B_imm8(0x01),
        ADD_A_B,
    );
    EXPECT(gbc.state().a(), 0x0);
    EXPECT_FLAGS(Z | H | C);
    RUN(
        LD_A_imm8(0xf0),
        LD_B_imm8(0x10),
        ADD_A_B,
    );
    EXPECT(gbc.state().a(), 0x0);
    EXPECT_FLAGS(Z | C);
}