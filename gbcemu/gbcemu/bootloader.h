#pragma once
#include <cstdint>
#include "assembler.h"

namespace rckid::gbcemu {

    /** Gameboy bootloader  
     
        This is the copy of the gameboy bootloader, which is a very simple test program that can be used to verify the emulator's absolute basics.
     */
    constexpr uint8_t DMGBootloader[] = {
        LD_SP_imm16(0xfffe),    // $0000  Setup Stack so that we can call functions

        // now we are going to zero the memory from 0x8000 to 0x9ffff (VRAM)
        XOR_A_A,                // $0003  set a to 0
        LD_HL_imm16(0x9fff),    // $0004 load the upper address to HL
        // 0x0007
        LD_decHL_A,             // $0007 store A (0) to HL, and decrement HL
        BIT_H(7),               // $0008 check if HL is below 0x8000
        JR_NZ(-5 /*0x0007*/),   // $000a

        // audio setup
        LD_HL_imm16(0xff26),    // $000c
        LD_C_imm8(0x11),        // $000f
        LD_A_imm8(0x80),        // $0011
        LD_decHL_A,             // $0013
        LD_ptrC_A,              // $0014
        INC_C,                  // $0015
        LD_A_imm8(0xf3),        // $0016
        LD_ptrC_A,              // $0018
        LD_decHL_A,             // $0019
        LD_A_imm8(0x77),        // $001a
        LD_ptrHL_A,             // $001c

        // sets up palette for the bacground layer
        LD_A_imm8(0xfc),        // $001d  Setup BG palette
        LDH_ptr8_A(0x47),       // $001f

        LD_DE_imm16(0x0104),    // $0021  Convert and load logo data from cart into Video RAM
        
        LD_HL_imm16(0x8010),    // $0024
        // 0x0027
        LD_A_ptrDE,             // $0027
        CALL(0x0095),           // $0028
        CALL(0x0096),           // $002b
        INC_DE,                 // $002e
        LD_A_E,                 // $002f
        CP_A_imm8(0x34),        // $0030
        JR_NZ(-13 /*0x0027*/),  // $0032

        LD_DE_imm16(0x00d8),    // $0034  Load 8 additional bytes into Video RAM
        LD_B_imm8(0x08),        // $0037
        // 0x0039
        LD_A_ptrDE,             // $0039
        INC_DE,                 // $003a
        LD_incHL_A,             // $003b
        INC_HL,                 // $003c
        DEC_B,                  // $003d
        JR_NZ(-7 /*0x0039*/),   // $003e

        LD_A_imm8(0x19),        // $0040  Setup background tilemap
        LD_ptr16_A(0x9910),     // $0042
        LD_HL_imm16(0x992f),    // $0045
        // 0x0048
        LD_C_imm8(0x0c),        // $0048
        // 0x004a
        DEC_A,                  // $004a
        JR_Z(8 /*0x0055*/),     // $004b
        LD_decHL_A,             // $004d
        DEC_C,                  // $004e
        JR_NZ(-7 /*0x004a*/),   // $004f
        LD_L_imm8(0x0f),        // $0051
        JR(-13 /*0x0048*/),     // $0053

        // Scroll logo on screen, and play logo sound
        // 0x0055
        LD_H_A,                 // $0055  Initialize scroll count, H=0
        LD_A_imm8(0x64),        // $0056
        LD_D_A,                 // $0058  set loop count, D=$64
        LDH_ptr8_A(0x42),       // $0059  Set vertical scroll register
        LD_A_imm8(0x91),        // $005b
        LDH_ptr8_A(0x40),       // $005d  Turn on LCD, showing Background
        INC_B,                  // $005f  Set B=1
        // 0x0060
        LD_E_imm8(0x02),        // $0060
        // 0x0062
        LD_C_imm8(0x0c),        // $0062
        // 0x0064
        LDH_A_ptr8(0x44),       // $0064  wait for screen frame
        CP_A_imm8(0x90),        // $0066
        JR_NZ(-6 /*0x0064*/),   // $0068
        DEC_C,                  // $006a
        JR_NZ(-9 /*0x0064*/),   // $006b
        DEC_E,                  // $006d
        JR_NZ(-14 /*0x0062*/),  // $006e    

        LD_C_imm8(0x13),        // $0070
        INC_H,                  // $0072  increment scroll count
        LD_A_H,                 // $0073
        LD_E_imm8(0x83),        // $0074
        CP_A_imm8(0x62),        // $0076  $62 counts in, play sound #1
        JR_Z(6 /*0x0080*/),     // $0078
        LD_E_imm8(0xc1),        // $007a
        CP_A_imm8(0x64),        // $007c
        JR_NZ(6 /*0x0086*/),    // $007e  $64 counts in, play sound #2
        // 0x0080
        LD_A_E,                 // $0080  play sound
        LD_ptrC_A,              // $0081
        INC_C,                  // $0082
        LD_A_imm8(0x87),        // $0083
        LD_ptrC_A,              // $0085
        // 0x0086
        LDH_A_ptr8(0x42),       // $0086
        SUB_A_B,                // $0088
        LDH_ptr8_A(0x42),       // $0089  scroll logo up if B=1
        DEC_D,                  // $008b
        JR_NZ(-46 /*0x0060*/),  // $008c

        DEC_B,                  // $008e  set B=0 first time
        JR_NZ(79 /*0x00e0*/),   // $008f    ... next time, cause jump to "Nintendo Logo check"

        LD_D_imm8(0x20),        // $0091  use scrolling loop to pause
        JR(-53 /*0x0060*/),     // $0093

        // Graphic routine
        LD_C_A,                 // $0095  "Double up" all the bits of the graphics data
        LD_B_imm8(0x04),        // $0096     and store in Video RAM
        // 0x0098
        PUSH_BC,                // $0098
        RL_C,                   // $0099
        RLA,                    // $009b
        POP_BC,                 // $009c
        RL_C,                   // $009d
        RLA,                    // $009f
        DEC_B,                  // $00a0
        JR_NZ(-11 /*0x0098*/),  // $00a1
        LD_incHL_A,             // $00a3
        INC_HL,                 // $00a4
        LD_incHL_A,             // $00a5
        INC_HL,                 // $00a6
        RET,                    // $00a7


        // 0x00a8
        // Nintendo Logo
        0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0c, 0x00, 0x0d,
        0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e, 0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99,
        0xbb, 0xbb, 0x67, 0x63, 0x6e, 0x0e, 0xec, 0xcc, 0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e,
        // 0x00d8
        // More video data
        0x3c, 0x42, 0xb9, 0xa5, 0xb9, 0xa5, 0x42, 0x3c,

        // Nintendo logo comparison routine
        // 0x00e0
        LD_HL_imm16(0x0104),    // $00e0  point HL to Nintendo logo in cart
        LD_DE_imm16(0x00a8),    // $00e3  point DE to Nintendo logo in DMG rom
        // 0x00e6
        LD_A_ptrDE,             // $00e6
        INC_DE,                 // $00e7
        CP_A_ptrHL,             // $00e8  compare logo data in cart to DMG rom
        JR_NZ(19 /*0xfe*/),     // $00e9  if not a match, lock up here
        INC_HL,                 // $00eb
        LD_A_L,                 // $00ec
        CP_A_imm8(0x34),        // $00ed  do this for $30 bytes
        JR_NZ(-11 /*0xe6*/),    // $00ef    

        LD_B_imm8(0x19),        // $00f1
        LD_A_B,                 // $00f3
        // 0x00f4
        ADD_A_ptrHL,            // $00f4
        INC_HL,                 // $00f5
        DEC_B,                  // $00f6
        JR_NZ(-5 /*0x00f4*/),   // $00f7
        ADD_A_ptrHL,            // $00f9
        JR_NZ(2 /*0x00fe*/),    // $00fa  if $19 + bytes from $0134-$014D  don't add to $00
                                //  ... lock up 

        LD_A_imm8(0x01),        // $00fc
        LDH_ptr8_A(0x50),       // $00fe  turn off DMG rom
                                // and continue to the game    
        // 0x0100
        JP(0x0000),             // $0100 (the default cartridge start, jump to the beginning of the bootstrap)
        0x00,                   // $1003 // padding
        // 0x0104 
        // Nintendo Logo - cartridge version
        0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0c, 0x00, 0x0d,
        0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e, 0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99,
        0xbb, 0xbb, 0x67, 0x63, 0x6e, 0x0e, 0xec, 0xcc, 0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e,
    };
} // namespace rckid::gbcemu
