#include "gbc.h"

#define A (state_.rawRegs8_[State::REG_INDEX_A])
#define B (state_.rawRegs8_[State::REG_INDEX_B])
#define C (state_.rawRegs8_[State::REG_INDEX_C])
#define D (state_.rawRegs8_[State::REG_INDEX_D])
#define E (state_.rawRegs8_[State::REG_INDEX_E])
#define H (state_.rawRegs8_[State::REG_INDEX_H])
#define L (state_.rawRegs8_[State::REG_INDEX_L])
#define AF (state_.rawRegs16_[State::REG_INDEX_AF])
#define BC (state_.rawRegs16_[State::REG_INDEX_BC])
#define DE (state_.rawRegs16_[State::REG_INDEX_DE])
#define HL (state_.rawRegs16_[State::REG_INDEX_HL])

#define PC (state_.pc_)
#define SP (state_.sp_)

static constexpr int val_Z = -1;
static constexpr int val_N = -1;
static constexpr int val_H = -1;
static constexpr int val_C = -1;
static constexpr int val__ = -1;
static constexpr int val_0 = 0;
static constexpr int val_1 = 1;

void GBC::loop() {
    cycles_ = 0;
    while (true) {
        uint8_t opcode = rd8(state_.pc_);
        switch (opcode) {
#define INS(OPCODE, FLAG_Z, FLAG_N, FLAG_H, FLAG_C, SIZE, CYCLES, MNEMONIC, ...) \
    case OPCODE: \
        cycles_ += CYCLES; \
        if (val_ ## FLAG_Z != -1) state_.setFlagZ(val_ ## FLAG_Z); \
        if (val_ ## FLAG_N != -1) state_.setFlagN(val_ ## FLAG_N); \
        if (val_ ## FLAG_H != -1) state_.setFlagH(val_ ## FLAG_H); \
        if (val_ ## FLAG_C != -1) state_.setFlagC(val_ ## FLAG_C); \
        __VA_ARGS__ \
        break;
#include "insns.inc.h"
           default:
                ASSERT("Unsupported opcode");
                break;
        }
    }
}