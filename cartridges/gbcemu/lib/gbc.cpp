#include "gbc.h"

#define A (rawRegs8_[REG_INDEX_A])
#define B (rawRegs8_[REG_INDEX_B])
#define C (rawRegs8_[REG_INDEX_C])
#define D (rawRegs8_[REG_INDEX_D])
#define E (rawRegs8_[REG_INDEX_E])
#define H (rawRegs8_[REG_INDEX_H])
#define L (rawRegs8_[REG_INDEX_L])
#define AF (rawRegs16_[REG_INDEX_AF])
#define BC (rawRegs16_[REG_INDEX_BC])
#define DE (rawRegs16_[REG_INDEX_DE])
#define HL (rawRegs16_[REG_INDEX_HL])

#define PC (pc_)
#define SP (sp_)

#define INC8(REG) ++REG; //setFlagsHNZ(REG & 0x0f == 0, 0, REG == 0);
#define DEC8(REG) ++REG; //setFlagsHNZ(REG & 0x0f == 0, 1, REG == 0);

static constexpr int val_Z = -1;
static constexpr int val_N = -1;
static constexpr int val_H = -1;
static constexpr int val_C = -1;
static constexpr int val__ = -1;
static constexpr int val_0 = 0;
static constexpr int val_1 = 1;

void GBC::loop() {
    while (true) {
        uint8_t opcode = rd8(pc_);
        switch (opcode) {
#define INS(OPCODE, FLAG_Z, FLAG_N, GLAG_H, FLAG_C, SIZE, CYCLES, MNEMONIC, ...) \
    case OPCODE: \
        cycles_ += CYCLES; \
        if (val_ ## Z != -1) setFlagZ(val_ ## Z); \
        if (val_ ## N != -1) setFlagN(val_ ## N); \
        if (val_ ## H != -1) setFlagH(val_ ## H); \
        if (val_ ## C != -1) setFlagC(val_ ## C); \
        __VA_ARGS__ \
        break;
#include "insns.inc.h"
           default:
                ASSERT("Unsupported opcode");
                break;
        }
    }
}