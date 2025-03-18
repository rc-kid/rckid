#include "gbctests.h"

namespace rckid::gbcemu {
    TEST(gbcemu, arithmetic_add8) {
        GBCEmu gbc{};
        RUN(
            LD_A_imm8(0x12),
            LD_B_imm8(0x05),
            ADD_A_B,
        );
        EXPECT(gbc.a(), 0x17);
        EXPECT_FLAGS(0);
        RUN(
            LD_A_imm8(0xff),
            LD_B_imm8(0x01),
            ADD_A_B,
        );
        EXPECT(gbc.a(), 0x0);
        EXPECT_FLAGS(Z | H | C);
        RUN(
            LD_A_imm8(0xf0),
            LD_B_imm8(0x10),
            ADD_A_B,
        );
        EXPECT(gbc.a(), 0x0);
        EXPECT_FLAGS(Z | C);
    }

    TEST(gbcemu, arithmetic_inc) {
        GBCEmu gbc{};
        RUN(
            LD_A_imm8(0x12),
            INC_A,
        );
        EXPECT(gbc.a(), 0x13);
        EXPECT_FLAGS(0);
        RUN(
            LD_A_imm8(0xff),
            INC_A,
        );
        EXPECT(gbc.a(), 0x0);
        EXPECT_FLAGS(Z | H);
        RUN(
            LD_A_imm8(0x0),
            INC_A,
        );
        EXPECT(gbc.a(), 0x1);
        EXPECT_FLAGS(0);
        RUN(
            LD_A_imm8(0x0f),
            INC_A,
        );
        EXPECT(gbc.a(), 0x10);
        EXPECT_FLAGS(H);
    }

    TEST(gbcemu, arithmetic_dec) {
        GBCEmu gbc{};
        RUN(
            LD_A_imm8(0x12),
            DEC_A,
        );
        EXPECT(gbc.a(), 0x11);
        EXPECT_FLAGS(N);
        RUN(
            LD_A_imm8(0x0),
            DEC_A,
        );
        EXPECT(gbc.a(), 0xff);
        EXPECT_FLAGS(N | H);
    }

    TEST(gbcemu, clear_flags) {
        GBCEmu gbc{};
        RUN(
            LD_A_imm8(0x01), 
            ADD_A_A,
        );
        EXPECT(gbc.a(), 0x02);
        EXPECT_FLAGS(0);
    }

    TEST(gbcemu, arithmetic_add16) {
        GBCEmu gbc{};
        RUN(
            LD_A_imm8(0x01), // clear Z flag
            ADD_A_A,
            LD_HL_imm16(0x1234),
            LD_DE_imm16(0x5678),
            ADD_HL_DE,
        );
        EXPECT(gbc.hl(), 0x68ac);
        EXPECT_FLAGS(0);
        RUN(
            LD_HL_imm16(0xffff),
            LD_DE_imm16(0x0001),
            ADD_HL_DE,
        );
        EXPECT(gbc.hl(), 0x0000);
        EXPECT_FLAGS(Z | H | C);
    }

    TEST(gbcemu, add_a_n8) {
        GBCEmu gbc{};
        RUN(
            LD_A_imm8(0x01),
            ADD_A_imm8(0x02),
        );
        EXPECT(gbc.a(), 0x03);
        EXPECT_FLAGS(0);
        RUN(
            LD_A_imm8(0xff),
            ADD_A_imm8(0x01),
        );
        EXPECT(gbc.a(), 0x00);
        EXPECT_FLAGS(Z | H | C);
        RUN(
            LD_A_imm8(0xf0),
            ADD_A_imm8(0x10),
        );
        EXPECT(gbc.a(), 0x00);
        EXPECT_FLAGS(Z | C);
        RUN(
            LD_A_imm8(0x0f),
            ADD_A_imm8(0x01),
        );
        EXPECT(gbc.a(), 0x10);
        EXPECT_FLAGS(H);
        RUN(
            LD_A_imm8(0x0f),
            ADD_A_imm8(0x10),
        );
        EXPECT(gbc.a(), 0x1f);
        EXPECT_FLAGS(0);
    }

    TEST(gbcemu, adc_a_n8) {
        GBCEmu gbc{};
        RUN(
            LD_A_imm8(0x01),
            ADC_A_imm8(0x02),
        );
        EXPECT(gbc.a(), 0x03);
        EXPECT_FLAGS(0);
        RUN(
            LD_A_imm8(0xff),
            ADC_A_imm8(0x01),
        );
        EXPECT(gbc.a(), 0x00);
        EXPECT_FLAGS(Z | H | C);
        RUN(
            LD_A_imm8(0xf0),
            ADC_A_imm8(0x10),
        );
        EXPECT(gbc.a(), 0x00);
        EXPECT_FLAGS(Z | C);
    }

    TEST(gbcemu, sub_a_n8) {
        GBCEmu gbc{};
        RUN(
            LD_A_imm8(0x01),
            SUB_A_imm8(0x02),
        );
        EXPECT(gbc.a(), 0xff);
        EXPECT_FLAGS(N | H |C);
        RUN(
            LD_A_imm8(0x00),
            SUB_A_imm8(0x01),
        );
        EXPECT(gbc.a(), 0xff);
        EXPECT_FLAGS(N | H | C);
        RUN(
            LD_A_imm8(0x10),
            SUB_A_imm8(0x10),
        );
        EXPECT(gbc.a(), 0x00);
        EXPECT_FLAGS(Z | N);
    }

    TEST(gbcemu, cp_a_n8) {
        GBCEmu gbc{};
        RUN(
            LD_A_imm8(0x01),
            CP_A_imm8(0x02),
        );
        EXPECT_FLAGS(N | H | C);
        RUN(
            LD_A_imm8(0x00),
            CP_A_imm8(0x01),
        );
        EXPECT_FLAGS(N | H | C);
        RUN(
            LD_A_imm8(0x10),
            CP_A_imm8(0x10),
        );
        EXPECT_FLAGS(Z | N);
    }

    TEST(gbcemu, rra) {
        GBCEmu gbc{};
        RUN(
            LD_A_imm8(0x01),
            RRA,
        );
        EXPECT(gbc.a(), 0x00);
        EXPECT_FLAGS(C);
        RUN(
            LD_A_imm8(0x02),
            RRA,
        );
        EXPECT(gbc.a(), 0x01);
        EXPECT_FLAGS(0);
        // ensure it goes through carry
        RUN(
            LD_A_imm8(0x03),
            RRA,
            RRA,
        );
        EXPECT(gbc.a(), 0x80);
        EXPECT_FLAGS(C);
    }


    
} // namespace rckid::gbcemu