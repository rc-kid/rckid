#include "gbc.h"

#define IO_JOYP (state_.highMem_[ADDR_IO_JOYP])
#define IO_SB (state_.highMem_[ADDR_IO_SB])
#define IO_SC (state_.highMem_[ADDR_IO_SC])
#define IO_DIV (state_.highMem_[ADDR_IO_DIV])
#define IO_TIMA (state_.highMem_[ADDR_IO_TIMA])
#define IO_TMA (state_.highMem_[ADDR_IO_TMA])
#define IO_TAC (state_.highMem_[ADDR_IO_TAC])
#define IO_IF (state_.highMem_[ADDR_IO_IF])
#define IO_NR10 (state_.highMem_[ADDR_IO_NR10])
#define IO_NR11 (state_.highMem_[ADDR_IO_NR11])
#define IO_NR12 (state_.highMem_[ADDR_IO_NR12])
#define IO_NR13 (state_.highMem_[ADDR_IO_NR13])
#define IO_NR14 (state_.highMem_[ADDR_IO_NR14])
#define IO_NR21 (state_.highMem_[ADDR_IO_NR21])
#define IO_NR22 (state_.highMem_[ADDR_IO_NR22])
#define IO_NR23 (state_.highMem_[ADDR_IO_NR23])
#define IO_NR24 (state_.highMem_[ADDR_IO_NR24])
#define IO_NR30 (state_.highMem_[ADDR_IO_NR30])
#define IO_NR31 (state_.highMem_[ADDR_IO_NR31])
#define IO_NR32 (state_.highMem_[ADDR_IO_NR32])
#define IO_NR33 (state_.highMem_[ADDR_IO_NR33])
#define IO_NR34 (state_.highMem_[ADDR_IO_NR34])
#define IO_NR41 (state_.highMem_[ADDR_IO_NR41])
#define IO_NR42 (state_.highMem_[ADDR_IO_NR42])
#define IO_NR43 (state_.highMem_[ADDR_IO_NR43])
#define IO_NR44 (state_.highMem_[ADDR_IO_NR44])
#define IO_NR50 (state_.highMem_[ADDR_IO_NR50])
#define IO_NR51 (state_.highMem_[ADDR_IO_NR51])
#define IO_NR52 (state_.highMem_[ADDR_IO_NR52])
#define IO_WAVE_RAM_0 (state_.highMem_[ADDR_IO_WAVE_RAM_0])
#define IO_LCDC (state_.highMem_[ADDR_IO_LCDC])
#define IO_STAT (state_.highMem_[ADDR_IO_STAT])
#define IO_SCY (state_.highMem_[ADDR_IO_SCY])
#define IO_SCX (state_.highMem_[ADDR_IO_SCX])
#define IO_LY (state_.highMem_[ADDR_IO_LY])
#define IO_LYC (state_.highMem_[ADDR_IO_LYC])
#define IO_DMA (state_.highMem_[ADDR_IO_DMA])
#define IO_BGB (state_.highMem_[ADDR_IO_BGB])
#define IO_OBP0 (state_.highMem_[ADDR_IO_OBP0])
#define IO_OBP1 (state_.highMem_[ADDR_IO_OBP1])
#define IO_WY (state_.highMem_[ADDR_IO_WY])
#define IO_WX (state_.highMem_[ADDR_IO_WX])
#define IO_KEY1 (state_.highMem_[ADDR_IO_KEY1])
#define IO_WBK (state_.highMem_[ADDR_IO_WBK])
#define IO_HDMA1 (state_.highMem_[ADDR_IO_HDMA1])
#define IO_HDMA2 (state_.highMem_[ADDR_IO_HDMA2])
#define IO_HDMA3 (state_.highMem_[ADDR_IO_HDMA3])
#define IO_HDMA4 (state_.highMem_[ADDR_IO_HDMA4])
#define IO_HDMA5 (state_.highMem_[ADDR_IO_HDMA5])
#define IO_RP (state_.highMem_[ADDR_IO_RP])
#define IO_BCPS_BGPI (state_.highMem_[ADDR_IO_BCPS_BGPI])
#define IO_BCPD_BGPD (state_.highMem_[ADDR_IO_BCPD_BGPD])
#define IO_OCPS_OCPI (state_.highMem_[ADDR_IO_OCPS_OCPI])
#define IO_OCPD_OBPD (state_.highMem_[ADDR_IO_OCPD_OBPD])
#define IO_PCM12 (state_.highMem_[ADDR_IO_PCM12])
#define IO_PCM34 (state_.highMem_[ADDR_IO_PCM34])
#define IO_IE (state_.highMem_[ADDR_IO_IE])

static constexpr int val_Z = -1;
static constexpr int val_N = -1;
static constexpr int val_H = -1;
static constexpr int val_C = -1;
static constexpr int val__ = -1;
static constexpr int val_0 = 0;
static constexpr int val_1 = 1;


uint8_t GBC::read8(uint16_t address) {
    using namespace rckid;
    if (address < 0xfe00) {
        return state_.memMap_[address >> 12][address & 0xfff];
    } else if (address < 0xfea0) {
        return state_.oam_[address - 0xfe00];
    } else if (address >= 0xff00) {
        switch (address & 0xff) {
            case ADDR_IO_JOYP: 
                if (IO_JOYP & JOYP_DPAD)
                    return JOYP_DPAD | (btnDown(Btn::Down) ? 0 : 8) | (btnDown(Btn::Up) ? 0 : 4) | (btnDown(Btn::Left) ? 0 : 2) | (btnDown(Btn::Right) ? 0 : 1);
                else 
                   return JOYP_BUTTONS | (btnDown(Btn::Start) ? 0 : 8) | (btnDown(Btn::Select) ? 0 : 4) | (btnDown(Btn::B) ? 0 : 2) | (btnDown(Btn::A) ? 0 : 1);
                    
            default:
                return state_.highMem_[address & 0xff];
        }
    } else {
        // reading from elsewhere should not be allowed, but to be on the safe side, just return 0
        return 0;
    }
}

uint16_t GBC::read16(uint16_t address) {
    return read8(address) | read8(address + 1) * 256;
}

void GBC::write8(uint16_t address, uint8_t value) {
    // TODO order according to the most likely outcomes
    if (address < 0x8000) { // rom
        // do nothing 
        // TODO this actually changes the rom bank & eram bank via cartridge mapper
    } else if (address >= 0x8000 && address < 0xa000) { // vram
        state_.memMap_[address >> 12][address & 0xfff] = value;
    } else if (address >= 0xa000 && address < 0xc000) { // eram
        UNIMPLEMENTED; 
    } else if (address < 0xfe00) { // wram & echo ram
        state_.memMap_[address >> 12][address & 0xfff] = value;
    } else if (address < 0xfea0) { // oam
        state_.oam_[address - 0xfe00] = value;
    } else if (address >= 0xff00) { // hram & io
        // since some of the IO regs are readonly (or their portions), we have to ensure that those bits won't get overwritten
        size_t offset = address & 0xff;
        switch (offset) {
            case ADDR_IO_JOYP:
                IO_JOYP = value;
                break;
            case ADDR_IO_STAT:
                 IO_STAT &= STAT_WRITE_MASK;
                 IO_STAT |= value & STAT_WRITE_MASK;
                 break;
            case ADDR_IO_LY: 
            case ADDR_IO_PCM12:
            case ADDR_IO_PCM34:
                // read only
                break;
                
            default:
                state_.highMem_[offset] = value;
                break;
        }
    } else {
        // technically this is not allowed so ignore it and do nothing
    }
}

void GBC::write16(uint16_t address, uint16_t value) {
    write8(address, value & 0xff);
    write8(address + 1, value >> 8);
}

/** Faster 8bit read with address increment. Does not work on IO registers, but very useful for PC. 
 */
uint8_t GBC::rd8(uint16_t & address) {
    uint8_t result = state_.memMap_[address >> 12][address & 0xfff];
    ++address;
    return result;
}

/** Faster 16bit read with address increment. Does not work on IO registers, but very useful for PC. 
 */
uint16_t GBC::rd16(uint16_t & address) {
    uint16_t result = * reinterpret_cast<uint16_t*>(state_.memMap_[address >> 12] + (address & 0xfff));
    address += 2;
    return result;
}

// arithmetics

uint8_t GBC:: inc8(uint8_t x) {
    ++x;
    state_.setFlagZ(x == 0);
    state_.setFlagH((x & 0xf) == 0);
    return x;
}

uint8_t GBC:: dec8(uint8_t x) {
    --x;
    state_.setFlagZ(x == 0);
    state_.setFlagH((x & 0xf) == 0xf);
    return x;
}

/** Adds two 8bit numbers, optinally including a carry flag and sets the Z, H and C flags accordingly. 

    The Z flag is set if the 8bit result is 0. The C flag is set if the result is greater than 256. Finally, the H flag is set if during 
*/
uint8_t GBC:: add8(uint8_t a, uint8_t b, uint8_t c) {
    unsigned r = a + b + c;
    state_.setFlagZ((r & 0xff) == 0);
    state_.setFlagH(((a ^ b ^ r) & 0x10) != 0);
    state_.setFlagC(r > 0xff);
    return static_cast<uint8_t>(r);
}

uint8_t GBC:: sub8(uint8_t a, uint8_t b, uint8_t c) {
    unsigned r = a - (b + c);
    state_.setFlagZ((r & 0xff) == 0);
    state_.setFlagH(((a ^ b ^ r) & 0x10) != 0);
    state_.setFlagC(r > 0xff);
    return static_cast<uint8_t>(r);
}

uint16_t GBC:: add16(uint16_t a, uint16_t b) {
    uint32_t r = a + b;
    state_.setFlagC(r > 0xffff);
    state_.setFlagH(((r ^ a ^ b) & 0x1000) != 0);
    return static_cast<uint16_t>(r);
}

/** Rotate left, set carry
 */
uint8_t GBC:: rlc8(uint8_t a) {
    uint16_t r = a << 1;
    state_.setFlagC(r & 256);
    r = r | state().flagC();
    return (r & 0xff);
}

/** Rotate left through carry. 
 */
uint8_t GBC:: rl8(uint8_t a) {
    uint16_t r = a << 1;
    r = r | state().flagC();
    state_.setFlagC(r & 256);
    return (r & 0xff);
}

/** Rotate right, set carry. 
 */
uint8_t GBC:: rrc8(uint8_t a) {
    state_.setFlagC(a & 1);
    a = a >> 1;
    a = a | (state().flagC() ? 128 : 0);
    return a;
}

/** Rotate right, through carry. 
 */
uint8_t GBC:: rr8(uint8_t a) {
    bool cf = state().flagC();
    state_.setFlagC(a & 1);
    a = a >> 1;
    a = a | (cf ? 128 : 0);
    return a;
}

/** Shift left, overflow to carry.
 */
uint8_t GBC:: sla8(uint8_t a) {
    uint16_t r = a * 2;
    state_.setFlagC(r & 256);
    return r & 0xff;
}

/** Shift right, arithmetically, i.e. keep msb intact*/
uint8_t GBC:: sra8(uint8_t a) {
    state_.setFlagC(a & 1);
    a = a >> 1;
    a |= (a & 64) ? 128 : 0;
    return a;
}

/** Shift right, logically, i.e.msb set to 0. 
 */
uint8_t GBC:: srl8(uint8_t a) {
    state_.setFlagC(a & 1);
    a = a >> 1;
    return a;
}

// rendering 

void GBC::setMode(unsigned mode) {
    IO_STAT &= ~ STAT_PPU_MODE; 
    IO_STAT |= (mode & STAT_PPU_MODE);
    // check the interrupts
    switch (mode) {
        case 0:
        case 1:
        case 2:
            break;
    }
}

void GBC::setLY(uint8_t value) {
    IO_LY = value;
    if (value == 144)
        IO_IF |= IF_VBLANK;
    if (value == IO_LYC) {
        IO_STAT |= STAT_LYC_EQ_LY;
        // check LYC LY interrupt
        if (IO_STAT & STAT_INT_LYC)
            IO_IF |= IF_LCD;
    } else {
        IO_STAT &= ~STAT_LYC_EQ_LY;
    }
}

void GBC::render(size_t & dots) {
    while (dots >= DOTS_PER_LINE) {
        /*
        size_t y = IO_LY;
        for (size_t x = 0; x < 160; ++x) {

        }
        */
    }
}

// main loop

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
