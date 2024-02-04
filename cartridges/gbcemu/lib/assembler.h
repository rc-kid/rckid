#pragma once

#define NOP                     0x00
#define LD_BC_imm16(IMM)        0x01, (IMM & 0xff), (IMM >> 8)
#define LD_ptrBC_A              0x02
#define INC_BC                  0x03
#define INC_B                   0x04
#define DEC_B                   0x05
#define LD_B_imm8(IMM)          0x06, IMM
#define RLCA                    0x07
#define LD_ptr_SP(PTR)          0x08, (PTR & 0xff), (PTR >> 8)
#define ADD_HL_BC               0x09
#define LD_A_ptrBC              0x0a
#define DEC_BC                  0x0b
#define INC_C                   0x0c
#define DEC_C                   0x0d
#define LD_C_imm8(IMM)          0x0e, IMM 
#define RRCA                    0x0f
#define STOP(X)                 0x10, X
#define LD_DE_imm16(IMM)        0x11, (IMM & 0xff), (IMM >> 8)
#define LD_ptrDE_A              0x12
#define INC_DE                  0x13
#define INC_D                   0x14
#define DEC_D                   0x15
#define LD_D_imm8(IMM)          0x16, IMM
#define RLA                     0x17
#define JR(REL)                 0x18, REL
#define ADD_HL_DE               0x19
#define LD_A_ptrDE              0x1a
#define DEC_DE                  0x1b
#define INC_E                   0x1c
#define DEC_E                   0x1d
#define LD_E_imm8(IMM)          0x1e, IMM
#define RRA                     0x1f
#define JR_NZ(REL)              0x20, REL
#define LD_HL_imm16(IMM)        0x21, (IMM & 0xff), (IMM >> 8)
#define LD_ptrHL_A              0x22
#define INC_HL                  0x23
#define INC_H                   0x24
#define DEC_H                   0x25
#define LD_H_imm8(IMM)          0x26, IMM
#define DAA                     0x27
#define JR_Z(REL)               0x28, REL
#define ADD_HL_HL               0x29
#define LD_A_ptrHL              0x2a
#define DEC_HL                  0x2b
#define INC_L                   0x2c
#define DEC_L                   0x2d
#define LD_L_imm8(IMM)          0x2e, IMM
#define CPL                     0x2f
#define JR_NC(REL)              0x30, REL
#define LD_SP_imm16(IMM)        0x31, (IMM & 0xff), (IMM >> 8)

#define INC_A                   0x3c

#define LD_A_imm8(IMM)          0x3e, IMM

#define ADD_A_B                 0x80
#define ADD_A_C                 0x81
#define ADD_A_D                 0x82
#define ADD_A_E                 0x83
#define ADD_A_H                 0x84
#define ADD_A_L                 0x85
#define ADD_A_ptrHL             0x86
#define ADD_A_A                 0x87
#define ADC_A_B                 0x88
#define ADC_A_C                 0x89
#define ADC_A_D                 0x8a
#define ADC_A_E                 0x8b
#define ADC_A_H                 0x8c
#define ADC_A_L                 0x8d
#define ADC_A_ptrHL             0x8e
#define ADC_A_A                 0x8f
#define SUB_A_B                 0x90
#define SUB_A_C                 0x91
#define SUB_A_D                 0x92
#define SUB_A_E                 0x93
#define SUB_A_H                 0x94
#define SUB_A_L                 0x95
#define SUB_A_ptrHL             0x96
#define SUB_A_A                 0x97
#define SBC_A_B                 0x98
#define SBC_A_C                 0x99
#define SBC_A_D                 0x9a
#define SBC_A_E                 0x9b
#define SBC_A_H                 0x9c
#define SBC_A_L                 0x9d
#define SBC_A_ptrHL             0x9e
#define SBC_A_A                 0x9f
#define AND_A_B                 0xa0
#define AND_A_C                 0xa1
#define AND_A_D                 0xa2
#define AND_A_E                 0xa3
#define AND_A_H                 0xa4
#define AND_A_L                 0xa5
#define AND_A_ptrHL             0xa6
#define AND_A_A                 0xa7
#define XOR_A_B                 0xa8
#define XOR_A_C                 0xa9
#define XOR_A_D                 0xaa
#define XOR_A_E                 0xab
#define XOR_A_H                 0xac
#define XOR_A_L                 0xad
#define XOR_A_ptrHL             0xae
#define XOR_A_A                 0xaf
#define OR_A_B                  0xb0
#define OR_A_C                  0xb1
#define OR_A_D                  0xb2
#define OR_A_E                  0xb3
#define OR_A_H                  0xb4
#define OR_A_L                  0xb5
#define OR_A_ptrHL              0xb6
#define OR_A_A                  0xb7
#define cp_A_B                  0xb8
#define cp_A_C                  0xb9
#define cp_A_D                  0xba
#define cp_A_E                  0xbb
#define cp_A_H                  0xbc
#define cp_A_L                  0xbd
#define cp_A_ptrHL              0xbe
#define cp_A_A                  0xbf

