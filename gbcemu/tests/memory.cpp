#include "gbctests.h"

/*

TEST(gbcemu, memory_initial_memmap) {
    uint8_t const pgm[] = { STOP(0) };
    GBC gbc{};
    gbc.runTest(pgm, sizeof(pgm));
    uint8_t const * const * mmap = gbc.state().memMap();
    // first 16kb set to bank 0 of rom
    EXPECT(mmap[0], pgm);
    EXPECT(mmap[1], pgm + 1 * 4096);
    EXPECT(mmap[2], pgm + 2 * 4096);
    EXPECT(mmap[3], pgm + 3 * 4096);
    // second 16kb set to bank 1 of rom
    EXPECT(mmap[4], pgm + 4 * 4096);
    EXPECT(mmap[5], pgm + 5 * 4096);
    EXPECT(mmap[6], pgm + 6 * 4096);
    EXPECT(mmap[7], pgm + 7 * 4096);
    // video ram is set to bank 0
    EXPECT(mmap[8], gbc.state().vram());
    EXPECT(mmap[9], gbc.state().vram() + 4096);
    // external ram is not present
    EXPECT(mmap[10], nullptr);
    EXPECT(mmap[11], nullptr);
    // wram bank 0 fixed
    EXPECT(mmap[12], gbc.state().wram());
    // wram bank 1 set to 1
    EXPECT(mmap[13], gbc.state().wram() + 4096);
    // echo ram set to wram
    EXPECT(mmap[14], gbc.state().wram());
    EXPECT(mmap[15], gbc.state().wram() + 4096);
}

TEST(gbcemu, memory_storeload8) {
    GBC gbc{};
    RUN(
        LD_HL_imm16(0xc000),
        LD_A_imm8(0x12),
        LD_incHL_A,
        INC_A,
        LD_incHL_A,
    );
    //uint8_t const * const * mmap = gbc.memMap();
    EXPECT(gbc.state().hl(), 0xc002);
    EXPECT(gbc.state().wram()[0], 0x12);
    EXPECT(gbc.state().wram()[1], 0x13);
}

TEST(gbcemu, memory_stack) {
    GBC gbc{};
    RUN(
        LD_SP_imm16(0xd000),
        LD_HL_imm16(0xcffe),
        LD_B_imm8(0x12),
        LD_C_imm8(0x34),
        PUSH_BC,
        POP_DE,
        LD_A_incHL,
    );
    EXPECT(gbc.state().sp(), 0xd000);
    EXPECT(gbc.state().b(), 0x12);
    EXPECT(gbc.state().c(), 0x34);
    EXPECT(gbc.state().de(), 0x1234);
    EXPECT(gbc.state().a(), 0x34);
    EXPECT(gbc.state().hl(), 0xcfff);
}

TEST(gbcemu, memory_oam) {
    GBC gbc{};
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
    EXPECT(gbc.state().hl(), 0xfe03);
    EXPECT(gbc.state().oam()[0], 0);
    EXPECT(gbc.state().oam()[1], 1);
    EXPECT(gbc.state().oam()[2], 2);
    EXPECT(gbc.state().b(), 0);
    EXPECT(gbc.state().c(), 1);
    EXPECT(gbc.state().a(), 2);
}

TEST(gbcemu, memory_hram) {
    GBC gbc{};
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
    EXPECT(gbc.state().hl(), 0xff83);
    EXPECT(gbc.state().hram()[0], 10);
    EXPECT(gbc.state().hram()[1], 11);
    EXPECT(gbc.state().hram()[2], 12);
    EXPECT(gbc.state().b(), 10);
    EXPECT(gbc.state().c(), 11);
    EXPECT(gbc.state().a(), 12);
}

*/