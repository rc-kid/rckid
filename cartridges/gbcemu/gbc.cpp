#include "gbc.h"

void GBC::loop() {


    uint8_t opcode;
    switch (opcode) {
        // NOP - don't do anything, no change in state
        case 0x00:
            break;
        // LD BC, imm16 - loads the next immediate value into the BC register, no change in flags
        case 0x01:
        // LD (BC), A - stores value of A to address in BC. No change in flags
        case 0x02:
        // INC BC - increments BC
        case 0x03:
            ++bc_;
            break;
        // INC B
        case 0x04:
            ++b_;
            break;
        // DEC B
        case 0x05:
            --b_;
            break;
        // LD B, imm8
        case 0x06:
        // RLCA 
        case 0x07:
        // LD (imm16), SP
        case 0x08:
        // ADD HL, BC - HL = HL + BC
        case 0x09:
        // LD A, (BC) - load contents at address in BC to A
        case 0x0a:
        

            
    }

}