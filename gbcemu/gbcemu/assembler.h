#pragma once

// basic instructions

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
#define STOP                    0x10, 0x00
#define LD_DE_imm16(IMM)        0x11, (IMM & 0xff), (IMM >> 8)
#define LD_ptrDE_A              0x12
#define INC_DE                  0x13
#define INC_D                   0x14
#define DEC_D                   0x15
#define LD_D_imm8(IMM)          0x16, IMM
#define RLA                     0x17
#define JR(REL)                 0x18, static_cast<uint8_t>(REL)
#define ADD_HL_DE               0x19
#define LD_A_ptrDE              0x1a
#define DEC_DE                  0x1b
#define INC_E                   0x1c
#define DEC_E                   0x1d
#define LD_E_imm8(IMM)          0x1e, IMM
#define RRA                     0x1f
#define JR_NZ(REL)              0x20, static_cast<uint8_t>(REL)
#define LD_HL_imm16(IMM)        0x21, (IMM & 0xff), (IMM >> 8)
#define LD_incHL_A              0x22
#define INC_HL                  0x23
#define INC_H                   0x24
#define DEC_H                   0x25
#define LD_H_imm8(IMM)          0x26, IMM
#define DAA                     0x27
#define JR_Z(REL)               0x28, static_cast<uint8_t>(REL)
#define ADD_HL_HL               0x29
#define LD_A_incHL              0x2a
#define DEC_HL                  0x2b
#define INC_L                   0x2c
#define DEC_L                   0x2d
#define LD_L_imm8(IMM)          0x2e, IMM
#define CPL                     0x2f
#define JR_NC(REL)              0x30, static_cast<uint8_t>(REL)
#define LD_SP_imm16(IMM)        0x31, (IMM & 0xff), (IMM >> 8)
#define LD_decHL_A              0x32
#define INC_SP                  0x33
#define INC_ptrHL               0x34
#define DEC_ptrHL               0x35
#define LD_ptrHL_imm8(IMM)      0x36, IMM
#define SCF                     0x37
#define JR_C(REL)               0x38, static_cast<uint8_t>(REL)
#define ADD_HL_SP               0x39
#define LD_A_decHL              0x3a
#define DEC_SP                  0x3b
#define INC_A                   0x3c
#define DEC_A                   0x3d
#define LD_A_imm8(IMM)          0x3e, IMM
#define CCF                     0x3f
#define LD_B_B                  0x40
#define LD_B_C                  0x41
#define LD_B_D                  0x42
#define LD_B_E                  0x43
#define LD_B_H                  0x44
#define LD_B_L                  0x45
#define LD_B_ptrHL              0x46
#define LD_B_A                  0x47
#define LD_C_B                  0x48
#define LD_C_C                  0x49
#define LD_C_D                  0x4a
#define LD_C_E                  0x4b
#define LD_C_H                  0x4c
#define LD_C_L                  0x4d
#define LD_C_ptrHL              0x4e
#define LD_C_A                  0x4f
#define LD_D_B                  0x50
#define LD_D_C                  0x51
#define LD_D_D                  0x52
#define LD_D_E                  0x53
#define LD_D_H                  0x54
#define LD_D_L                  0x55
#define LD_D_ptrHL              0x56
#define LD_D_A                  0x57
#define LD_E_B                  0x58
#define LD_E_C                  0x59
#define LD_E_D                  0x5a
#define LD_E_E                  0x5b
#define LD_E_H                  0x5c
#define LD_E_L                  0x5d
#define LD_E_ptrHL              0x5e
#define LD_E_A                  0x5f
#define LD_H_B                  0x60
#define LD_H_C                  0x61
#define LD_H_D                  0x62
#define LD_H_E                  0x63
#define LD_H_H                  0x64
#define LD_H_L                  0x65
#define LD_H_ptrHL              0x66
#define LD_H_A                  0x67
#define LD_L_B                  0x68
#define LD_L_C                  0x69
#define LD_L_D                  0x6a
#define LD_L_E                  0x6b
#define LD_L_H                  0x6c
#define LD_L_L                  0x6d
#define LD_L_ptrHL              0x6e
#define LD_L_A                  0x6f
#define LD_ptrHL_B              0x70
#define LD_ptrHL_C              0x71
#define LD_ptrHL_D              0x72
#define LD_ptrHL_E              0x73
#define LD_ptrHL_H              0x74
#define LD_ptrHL_L              0x75
#define HALT                    0x76
#define LD_ptrHL_A              0x77
#define LD_A_B                  0x78
#define LD_A_C                  0x79
#define LD_A_D                  0x7a
#define LD_A_E                  0x7b
#define LD_A_H                  0x7c
#define LD_A_L                  0x7d
#define LD_A_ptrHL              0x7e
#define LD_A_A                  0x7f
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
#define CP_A_B                  0xb8
#define CP_A_C                  0xb9
#define CP_A_D                  0xba
#define CP_A_E                  0xbb
#define CP_A_H                  0xbc
#define CP_A_L                  0xbd
#define CP_A_ptrHL              0xbe
#define CP_A_A                  0xbf
#define RET_NZ                  0xc0
#define POP_BC                  0xc1
#define JP_NZ(ABS)              0xc2, (ABS & 0xff), (ABS >> 8)
#define JP(ABS)                 0xc3, (ABS & 0xff), (ABS >> 8)
#define CALL_NZ(ABS)            0xc4, (ABS & 0xff), (ABS >> 8)
#define PUSH_BC                 0xc5
#define ADD_A_imm8(IMM)         0xc6, IMM
#define RST_00                  0xc7
#define RET_Z                   0xc8
#define RET                     0xc9
#define JP_Z(ABS)               0xca, (ABS & 0xff), (ABS >> 8)
// prefix 0xcb
#define CALL_Z(ABS)             0xcc, (ABS & 0xff), (ABS >> 8)
#define CALL(ABS)               0xcd, (ABS & 0xff), (ABS >> 8)
#define ADC_A_imm8(IMM)         0xce, IMM
#define RST_08                  0xcf
#define RET_NC                  0xd0
#define POP_DE                  0xd1
#define JP_NC(ABS)              0xd2, (ABS & 0xff), (ABS >> 8)       
// 0xd3 not used
#define CALL_NC(ABS)            0xd4, (ABS & 0xff), (ABS >> 8)
#define PUSH_DE                 0xd5
#define SUB_A_imm8(IMM)         0xd6, IMM
#define RST_10                  0xd7
#define RET_C                   0xd8
#define RETI                    0xd9
#define JP_C(ABS)               0xda, (ABS & 0xff), (ABS >> 8)
// 0xdb not used
#define CALL_C(ABS)             0xdc, (ABS & 0xff), (ABS >> 8)
// 0xdd not used
#define SBC_A_imm8(IMM)         0xde, IMM
#define RST_18                  0xdf
#define LDH_ptr8_A(IMM)         0xe0, IMM
#define POP_HL                  0xe1
#define LD_ptrC_A               0xe2
// 0xe3 not used
// 0xe4 not used
#define PUSH_HL                 0xe5
#define AND_A_imm8(IMM)         0xe6, IMM
#define RST_20                  0xe7
#define ADD_SP_imm8(IMM)        0xe8, static_cast<uint8_t>(IMM)   
#define JP_HL                   0xe9
#define LD_ptr16_A(ABS)         0xea, (ABS & 0xff), (ABS >> 8)
// 0xeb not used
// 0xec not used
// 0xed not used
#define XOR_A_imm8(IMM)         0xee, IMM
#define RST_28                  0xef
#define LDH_A_ptr8(IMM)         0xf0, IMM
#define POP_AF                  0xf1
#define LD_A_ptrC               0xf2
#define DI                      0xf3
// 0xf4 not used
#define PUSH_AF                 0xf5
#define OR_A_imm8(IMM)          0xf6, IMM
#define RST_30                  0xf7
#define LD_HL_SP_imm8(IMM)      0xf8, IMM
#define LD_SP_HL                0xf9
#define LD_A_ptr16(ABS)         0xfa, (ABS & 0xff), (ABS >> 8)
#define EI                      0xfb
// 0xfc not used
// 0xfd not used
#define BKPT                    0xfd
#define CP_A_imm8(IMM)          0xfe, IMM
#define RST_38                  0xff

// extended instructions

#define RLC_B                   0xcb, 0x00
#define RLC_C                   0xcb, 0x01
#define RLC_D                   0xcb, 0x02
#define RLC_E                   0xcb, 0x03
#define RLC_H                   0xcb, 0x04
#define RLC_L                   0xcb, 0x05
#define RLC_ptrHL               0xcb, 0x06
#define RLC_A                   0xcb, 0x07
#define RRC_B                   0xcb, 0x08
#define RRC_C                   0xcb, 0x09
#define RRC_D                   0xcb, 0x0a
#define RRC_E                   0xcb, 0x0b
#define RRC_H                   0xcb, 0x0c
#define RRC_L                   0xcb, 0x0d
#define RRC_ptrHL               0xcb, 0x0e
#define RRC_A                   0xcb, 0x0f
#define RL_B                    0xcb, 0x10
#define RL_C                    0xcb, 0x11
#define RL_D                    0xcb, 0x12
#define RL_E                    0xcb, 0x13
#define RL_H                    0xcb, 0x14
#define RL_L                    0xcb, 0x15
#define RL_ptrHL                0xcb, 0x16
#define RL_A                    0xcb, 0x17
#define RR_B                    0xcb, 0x18
#define RR_C                    0xcb, 0x19
#define RR_D                    0xcb, 0x1a
#define RR_E                    0xcb, 0x1b
#define RR_H                    0xcb, 0x1c
#define RR_L                    0xcb, 0x1d
#define RR_ptrHL                0xcb, 0x1e
#define RR_A                    0xcb, 0x1f
#define SLA_B                   0xcb, 0x20
#define SLA_C                   0xcb, 0x21
#define SLA_D                   0xcb, 0x22
#define SLA_E                   0xcb, 0x23
#define SLA_H                   0xcb, 0x24
#define SLA_L                   0xcb, 0x25
#define SLA_ptrHL               0xcb, 0x26
#define SLA_A                   0xcb, 0x27
#define STA_B                   0xcb, 0x28
#define STA_C                   0xcb, 0x29
#define STA_D                   0xcb, 0x2a
#define STA_E                   0xcb, 0x2b
#define STA_H                   0xcb, 0x2c
#define STA_L                   0xcb, 0x2d
#define STA_ptrHL               0xcb, 0x2e
#define STA_A                   0xcb, 0x2f
#define SWAP_B                  0xcb, 0x30
#define SWAP_C                  0xcb, 0x31
#define SWAP_D                  0xcb, 0x32
#define SWAP_E                  0xcb, 0x33
#define SWAP_H                  0xcb, 0x34
#define SWAP_L                  0xcb, 0x35
#define SWAP_ptrHL              0xcb, 0x36
#define SWAP_A                  0xcb, 0x37
#define SRL_B                   0xcb, 0x38
#define SRL_C                   0xcb, 0x39
#define SRL_D                   0xcb, 0x3a
#define SRL_E                   0xcb, 0x3b
#define SRL_H                   0xcb, 0x3c
#define SRL_L                   0xcb, 0x3d
#define SRL_ptrHL               0xcb, 0x3e
#define SRL_A                   0xcb, 0x3f
#define BIT_B(BI)               0xcb, (0x40 + BI * 8) 
#define BIT_C(BI)               0xcb, (0x41 + BI * 8) 
#define BIT_D(BI)               0xcb, (0x42 + BI * 8) 
#define BIT_E(BI)               0xcb, (0x43 + BI * 8) 
#define BIT_H(BI)               0xcb, (0x44 + BI * 8) 
#define BIT_L(BI)               0xcb, (0x45 + BI * 8) 
#define BIT_ptrHL(BI)           0xcb, (0x46 + BI * 8) 
#define BIT_A(BI)               0xcb, (0x47 + BI * 8) 
#define RES_B(BI)               0xcb, (0x80 + BI * 8) 
#define RES_C(BI)               0xcb, (0x81 + BI * 8) 
#define RES_D(BI)               0xcb, (0x82 + BI * 8) 
#define RES_E(BI)               0xcb, (0x83 + BI * 8) 
#define RES_H(BI)               0xcb, (0x84 + BI * 8) 
#define RES_L(BI)               0xcb, (0x85 + BI * 8) 
#define RES_ptrHL(BI)           0xcb, (0x86 + BI * 8) 
#define RES_A(BI)               0xcb, (0x87 + BI * 8) 
#define SET_B(BI)               0xcb, (0xc0 + BI * 8) 
#define SET_C(BI)               0xcb, (0xc1 + BI * 8) 
#define SET_D(BI)               0xcb, (0xc2 + BI * 8) 
#define SET_E(BI)               0xcb, (0xc3 + BI * 8) 
#define SET_H(BI)               0xcb, (0xc4 + BI * 8) 
#define SET_L(BI)               0xcb, (0xc5 + BI * 8) 
#define SET_ptrHL(BI)           0xcb, (0xc6 + BI * 8) 
#define SET_A(BI)               0xcb, (0xc7 + BI * 8) 
