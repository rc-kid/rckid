#include "gbc.h"

#define A (rawRegs8_[0])
#define B (rawRegs8_[2])
#define C (rawRegs8_[3])
#define D (rawRegs8_[4])
#define E (rawRegs8_[5])
#define H (rawRegs8_[6])
#define L (rawRegs8_[7])
#define AF (rawRegs16_[0])
#define BC (rawRegs16_[1])
#define DE (rawRegs16_[2])
#define HL (rawRegs16_[3])

#define PC (pc_)
#define SP (sp_)

#define INC8(REG) ++REG; //setFlagsHNZ(REG & 0x0f == 0, 0, REG == 0);
#define DEC8(REG) ++REG; //setFlagsHNZ(REG & 0x0f == 0, 1, REG == 0);

void GBC::loop() {
    while (true) {
        uint8_t opcode = rd8(pc_);
        switch (opcode) {
#define INS(OPCODE, Z, N, H, C, SIZE, CYCLES, MNEMONIC, ...) \
    case OPCODE: { __VA_ARGS__ } break;
#include "insns.inc.h"
           default:
                ASSERT("Unsupported opcode");
                break;
        }
    }
}