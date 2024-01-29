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
    PC = 0x100;


    while (true) {
        uint8_t opcode = rd8(pc_);
        switch (opcode) {
            /** nop
             
                Does nothing
             */
            case 0x00:
                break;
            /** ld bc, imm16

               Loads the next word into the `bc` register.  
             */
            case 0x01:
                BC = rd16(pc_);
                break;
            /** ld [bc], a

               Saves contents of register `a` to address in `bc`
             */
            case 0x02:
                write8(BC, A);
                break;
            /** inc bc
             
                Increments the `bc` register, does not change flags
             */
            case 0x03:
                ++BC; 
                break;
            /** inc b
             
                Increments the `b` register, sets flags accordingly. 
             */
            case 0x04:
                INC8(B); 
                break;
            /** dec b
             
                Decrements the `b` register and sets flags accordingly. 
             */
            case 0x05:
                DEC8(B);
                break;
            /** ld b, imm8
             
                Loads 8bit immediate into register `b`.
             */
            case 0x06:
                B = rd8(pc_);
                break;
            /** rlca 
             
             */
            case 0x07:
                UNIMPLEMENTED;
                break;
            /** ld [imm16], sp
             
                Take contents of the `sp` register and save it to given immediate address. 
             */
            case 0x08:
                write16(rd16(PC), SP);
                break;
            /** add hl, bc
             
                Adds `bc` to `hl` and sets flags. 
             */
            case 0x09:
                UNIMPLEMENTED;

            default:
                ASSERT("Unsupported opcode");
                break;
        }
    }
}