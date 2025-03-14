#include "gbcemu.h"

#define A (regs8_[REG_INDEX_A])
#define F (regs8_[REG_INDEX_F])
#define B (regs8_[REG_INDEX_B])
#define C (regs8_[REG_INDEX_C])
#define D (regs8_[REG_INDEX_D])
#define E (regs8_[REG_INDEX_E])
#define H (regs8_[REG_INDEX_H])
#define L (regs8_[REG_INDEX_L])
#define AF (regs16_[REG_INDEX_AF])
#define BC (regs16_[REG_INDEX_BC])
#define DE (regs16_[REG_INDEX_DE])
#define HL (regs16_[REG_INDEX_HL])

#define PC (pc_)
#define SP (sp_)

/** The joypad register. 
 
    Writing to the register's upper nibble chooses between dpad (bit 4) and buttons (bit 5), while the lower nibble contains the values of the buttons. When button is pressed, it should read 0. 

    bit 5 = select buttons (write)
    bit 4 = select dpad (write)
    bit 3 = start / down (read)
    bit 2 = select / up (read)
    bit 1 = B / left (read)
    bit 0 = A / right (read)
 */
#define IO_JOYP (hram_[0x00])
static constexpr uint8_t JOYP_DPAD = 16;
static constexpr uint8_t JOYP_BUTTONS = 32;

#define IO_SB (hram_[0x01])
#define IO_SC (hram_[0x02])
#define IO_DIV (hram_[0x04])
#define IO_TIMA (hram_[0x05])
#define IO_TMA (hram_[0x06])
#define IO_TAC (hram_[0x07])

/** The Interrupt Flag Register
 
    bit 4 = Joypad
    bit 3 = Serial
    bit 2 = Timer
    bit 1 = LCD
    bit 0 = VBLANK
 */
#define IO_IF (hram_[0x0f])
static constexpr uint8_t IF_JOYPAD = 1 << 4;
static constexpr uint8_t IF_SERIAL = 1 << 3;
static constexpr uint8_t IF_TIMER = 1 << 2;
static constexpr uint8_t IF_LCD = 1 << 1;
static constexpr uint8_t IF_VBLANK = 1 << 0;

#define IO_NR10 (hram_[0x10])
#define IO_NR11 (hram_[0x11])
#define IO_NR12 (hram_[0x12])
#define IO_NR13 (hram_[0x13])
#define IO_NR14 (hram_[0x14])
//static constexpr size_t IO_NR20 = 0x15; // not used
#define IO_NR21 (hram_[0x16])
#define IO_NR22 (hram_[0x17])
#define IO_NR23 (hram_[0x18])
#define IO_NR24 (hram_[0x19])
#define IO_NR30 (hram_[0x1a])
#define IO_NR31 (hram_[0x1b])
#define IO_NR32 (hram_[0x1c])
#define IO_NR33 (hram_[0x1d])
#define IO_NR34 (hram_[0x1e])
//static constexpr size_t IO_NR40 = 0x1f; // not used
#define IO_NR41 (hram_[0x20])
#define IO_NR42 (hram_[0x21])
#define IO_NR43 (hram_[0x22])
#define IO_NR44 (hram_[0x23])
#define IO_NR50 (hram_[0x24])
#define IO_NR51 (hram_[0x25])
#define IO_NR52 (hram_[0x26])
// TODO the next is 16 bytes? should be changed accordingly?
#define IO_WAVE_RAM_0 (hram_[0x30]) // 16 bytes

/** LCD Control Register

    bit 7 = LCD & PPU enable / disable
    bit 6 = window tilemap area ( 0 == 0x9800 - 9bff, 1 = 0x9c00 - 0x9fff)
    bit 5 = window enable 
    bit 4 = BG & Window tilemap area ( 0 = 0x9800 - 0x9bfff, 1 = 0x9c00 - 0x9fff)
    bit 3 = BG tilemap area ( 0 = 0x9800 - 0x9bfff, 1 = 0x9c00 - 0x9fff)
    bit 2 = OBJ size (0 = 8x8, 1 = 8x16)
    bit 1 = OBJ enable
    bit 0 = BG/Win enable / priority -- CGB Specific
 */
#define IO_LCDC (hram_[0x40])

/** Status and interrupts for the LCD driver
 
    bit 6 = LYC int select
    bit 5 = mode 2 int select 
    bit 4 = mode 1 int select
    bit 3 = mode 0 int select
    bit 2 = LYC == LY
    bits 0 & 1 = PPU mode 
*/
#define IO_STAT (hram_[0x41])
static constexpr uint8_t STAT_WRITE_MASK = 0b01111100;
static constexpr uint8_t STAT_PPU_MODE = 3;
static constexpr uint8_t STAT_LYC_EQ_LY = 1 << 2;
static constexpr uint8_t STAT_INT_MODE0 = 1 << 3;
static constexpr uint8_t STAT_INT_MODE1 = 1 << 4;
static constexpr uint8_t STAT_INT_MODE2 = 1 << 5;
static constexpr uint8_t STAT_INT_LYC = 1 << 6;


/** Scroll X and scroll Y values for the background layer. 
 
    The values can be from 0 to 255, which corresponds to 32x32 tilemap of 8x8 tile sizes. Both SCX and SCY wrap around their limits independently. 
 */
#define IO_SCY (hram_[0x42])
#define IO_SCX (hram_[0x43])

/** LCD Y Coordinate
     
    Contains the current coordinate of the LCD renderer. 0 to 143 is active renderering, 144 to 153 is VBLANK.  
 */
#define IO_LY (hram_[0x44])

/** LY compare
 
    When the LY value becomes identical to the LYC value, the STAT bit 2 (LYC == LY) is set and an interrupt can be triggered if enabled. 
 */
#define IO_LYC (hram_[0x45])
#define IO_DMA (hram_[0x46])

/** DMG palette register. Contains color indices for the 4 available colors. 
 
    bits:            7 6 | 5 4 | 3 2 | 1 0
    pallete indices: -3- | -2- | -1- | -0-
    
    The values in the palette indices correspond to the following colors:

    00 = White 
    01 = Light Gray (green really)
    02 = Dark Gray
    03 = Black
 */
#define IO_BGB (hram_[0x47])
#define IO_OBP0 (hram_[0x48])
#define IO_OBP1 (hram_[0x49])
#define IO_WY (hram_[0x4a])
#define IO_WX (hram_[0x4b])
#define IO_KEY1 (hram_[0x4d])
#define IO_WBK (hram_[0x4f])
#define IO_HDMA1 (hram_[0x51])
#define IO_HDMA2 (hram_[0x52])
#define IO_HDMA3 (hram_[0x53])
#define IO_HDMA4 (hram_[0x54])
#define IO_HDMA5 (hram_[0x55])
#define IO_RP (hram_[0x56])
#define IO_BCPS_BGPI (hram_[0x68])
#define IO_BCPD_BGPD (hram_[0x69])
#define IO_OCPS_OCPI (hram_[0x6a])
#define IO_OCPD_OBPD (hram_[0x6b])
#define IO_PCM12 (hram_[0x76])
#define IO_PCM34 (hram_[0x77])
#define IO_IE (hram_[0xff])

/** Flag values to be used with the instruction macro expansion. The ins macro stores the flag values as either of:
 
    - 0 if the instructions always clears the flag
    - 1 if the instruction always sets the flag
    - -1 if the instruction does not affect the flag
    - Z, N, H C (the flag name) if the instruction updates the flag depending on its values in which case this is done explicitly in the instruction code 
 */
static constexpr int val_Z = -1;
static constexpr int val_N = -1;
static constexpr int val_H = -1;
static constexpr int val_C = -1;
static constexpr int val__ = -1;
static constexpr int val_0 = 0;
static constexpr int val_1 = 1;

namespace rckid::gbcemu {

    GBCEmu::GBCEmu(Allocator & a):
        vram_{
            a.alloc<uint8_t>(0x1000), 
            a.alloc<uint8_t>(0x1000), 
            a.alloc<uint8_t>(0x1000), 
            a.alloc<uint8_t>(0x1000)
        },
        wram_{
            a.alloc<uint8_t>(0x1000),
            a.alloc<uint8_t>(0x1000), 
            a.alloc<uint8_t>(0x1000), 
            a.alloc<uint8_t>(0x1000), 
            a.alloc<uint8_t>(0x1000), 
            a.alloc<uint8_t>(0x1000), 
            a.alloc<uint8_t>(0x1000), 
            a.alloc<uint8_t>(0x1000)
        },
        oam_{a.alloc<uint8_t>(160)},
        hram_{a.alloc<uint8_t>(256)},
        pixels_{320,a} {
    }

    GBCEmu::~GBCEmu() {
        Heap::tryFree(gamepak_);
        for (uint32_t i = 0; i < 4; ++i)
            Heap::tryFree(vram_[i]);
        for (uint32_t i = 0; i < 8; ++i)
            Heap::tryFree(wram_[i]);
        Heap::tryFree(oam_);
        Heap::tryFree(hram_);
        for (uint32_t i = 0; i < 32; ++i)
            Heap::tryFree(eram_[i]);
        // TODO some more cleanup would be good here
    }

    void GBCEmu::run() {
        // set the current app in focus. If there is previous app, it will be blurred. The focus method also updates the parent app so that we can go back with the apps
        focus();

        runCPU();
        /*
        // now run the app
        while (app_ == this) {
            tick();
            update();
            displayWaitUpdateDone();
            draw();
        }
        */
        // we are done, should blur ourselves, and refocus parent (if any)
        blur();
    }

    void GBCEmu::focus() {
        App::focus();
        // set the display to row-first mode, which is what gameboy is expecting and set the resolution to 160x144
        // TODO this should be changed to some scaling ideally
        displaySetRefreshDirection(DisplayRefreshDirection::RowFirst);
        displaySetUpdateRegion(Rect::Centered(160, 144, RCKID_DISPLAY_WIDTH, RCKID_DISPLAY_HEIGHT));
    }

    void GBCEmu::blur() {
        App::blur();
    }


    void GBCEmu::update() {
        // TODO what to do with update? 
    }

    void GBCEmu::draw() {
        // TODO do noithing for now as drawing is done as part of the update function ATM
    }

    void GBCEmu::loadCartridge(GamePak * game) {
        gamepak_ = game;
        // figure out the size of the external RAM and allocate accordingly
        for (uint32_t i = 0; i < 32; ++i) {
            Heap::tryFree(eram_[i]);
            eram_[i] = nullptr;
        }
        uint32_t eramSize = gamepak_->cartridgeRAMSize() / 4096;
        for (uint32_t i = 0; i < eramSize; ++i)
            eram_[i] = Heap::alloc<uint8_t>(0x1000);
        // initialize memory mapping defaults
        memMap_[0] = const_cast<uint8_t *>(gamepak_->getPage(0));
        memMap_[1] = memMap_[0] + 0x1000;
        memMap_[2] = memMap_[0] + 0x2000;
        memMap_[3] = memMap_[0] + 0x3000;
        setRomPage(1);
        setVideoRamPage(0);
        setExternalRamPage(0);
        memMap_[12] = wram_[0];
        memMap_[14] = wram_[0];
        setWorkRamPage(1);
        // register initial values as if this is CGB
        // from https://gbdev.io/pandocs/Power_Up_Sequence.html
        A = 0x11;
        F = FLAG_Z;
        B = 0;
        C = 0;
        D = 0;
        E = 0x08;
        H = 0x00;
        L = 0x7c;
        PC = 0x100;
        SP = 0xfffe;
        // and reset counters
        cycles_ = 0;
        totalCycles_ = 0;
        // set the initial values for the IO registers
        IO_LY = 0; // ensure we'll start with new frame
    }

    void GBCEmu::runCPU() {
        disassemble(0, 0x100);
        PC = 0;
        while (true) {
            renderLine();
            while (cycles_ < 456) {
                if (PC > 16)
                    totalCycles_ += 1;
                uint8_t opcode = mem8(PC++);
                switch (opcode) {
                    #define INS(OPCODE, FLAG_Z, FLAG_N, FLAG_H, FLAG_C, SIZE, CYCLES, MNEMONIC, ...) \
                    case OPCODE: \
                        LOG(LL_ERROR, pc_ << ": " << MNEMONIC); \
                        cycles_ += CYCLES; \
                        if (val_ ## FLAG_Z != -1) setFlagZ(val_ ## FLAG_Z); \
                        if (val_ ## FLAG_N != -1) setFlagN(val_ ## FLAG_N); \
                        if (val_ ## FLAG_H != -1) setFlagH(val_ ## FLAG_H); \
                        if (val_ ## FLAG_C != -1) setFlagC(val_ ## FLAG_C); \
                        __VA_ARGS__ \
                        break;
                    #include "insns.inc.h"
                    default:
                        ASSERT("Unsupported opcode");
                        break;
                };
            }
            cycles_ -= 456;
        }
        /*
        for (int i = 0; i < 144; ++i)
            renderLine();
        while (true) {};
        return;
        while (true) {
            uint8_t opcode = mem8(PC++);
            switch (opcode) {
                #define INS(OPCODE, FLAG_Z, FLAG_N, FLAG_H, FLAG_C, SIZE, CYCLES, MNEMONIC, ...) \
                case OPCODE: \
                    cycles_ += CYCLES; \
                    if (val_ ## FLAG_Z != -1) setFlagZ(val_ ## FLAG_Z); \
                    if (val_ ## FLAG_N != -1) setFlagN(val_ ## FLAG_N); \
                    if (val_ ## FLAG_H != -1) setFlagH(val_ ## FLAG_H); \
                    if (val_ ## FLAG_C != -1) setFlagC(val_ ## FLAG_C); \
                    __VA_ARGS__ \
                    break;
                #include "insns.inc.h"
                default:
                    ASSERT("Unsupported opcode");
                    break;
            };
        }
        */
    }

    void GBCEmu::disassemble(uint16_t start, uint16_t end) {
        for (uint16_t i = start; i < end; ) {
            uint8_t opcode = mem8(i);
            switch (opcode) {
                #define INS(OPCODE, FLAG_Z, FLAG_N, FLAG_H, FLAG_C, SIZE, CYCLES, MNEMONIC, ...) \
                case OPCODE: \
                    LOG(LL_ERROR, i << ": " << MNEMONIC); \
                    i += SIZE; \
                    break;
                #include "insns.inc.h"
                default:
                    LOG(LL_ERROR, i << ": ??? " << opcode);
                    ASSERT("Unsupported opcode");
                    i += 1;
                    break;
            };
        }
    }

    void GBCEmu::setMode(unsigned mode) {
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
    
    /** Special function for setting the LY  */
    void GBCEmu::setLY(uint8_t value) {
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
    
    /** TODO this is the simplest rendering possible where we just render the entire line. 
     */
    void GBCEmu::renderLine() {
        // get the line we will be drawing now
        uint8_t ly = IO_LY;
        setLY(ly == 153 ? 0 : ly + 1);
        // don't do anything in VBlank
        if (ly >= 144)
            return;

        // TODO determine which sprites to use

        // calculate the background position we will be drawing. This is the position to the 256x256 background map created by 32x32 tiles. Using the uint8_t values for the coordinates gives us the automatic wraparound
        uint8_t by = ly + IO_SCY;
        uint8_t bx = IO_SCX;
        // determine the row of tiles we will be using and the row of inside the tile (this stays the same for the entire line)
        uint32_t ty = by / 8;
        uint32_t tr = by % 8;
        uint8_t tx = bx / 8;
        // determine tilemap address, which too stays the same for the entire line, we can also add the line offset
        uint16_t tilemapAddress = (IO_LCDC & 0x08) ? 0x9c00 : 0x9800;
        tilemapAddress += ty * 32; 
        // and determine the tileset address
        uint16_t tilesetAddress = (IO_LCDC & 0x10) ? 0x8000 : 0x9000;

        // and figure out the palette we will be using for the row
        uint16_t palette[4];
        // TODO fill the palette from IO_BGB
        palette[0] = 0xf0f0;
        palette[1] = 0x6666;
        palette[2] = 0xaaaa;
        palette[3] = 0xffff;

        // render the pixels now, we keep x as the current x coordinate on the screen
        int16_t x = - (8 - (bx % 8)) & 0x7;
        while (x < 160) {
            // determine which tile we are using
            // TODO for CGB we also need to determine the tile attributes 
            uint8_t tileIndex = mem8(tilemapAddress + tx);
            uint16_t tileRow = mem16(tilesetAddress + tileIndex * 16 + tr * 2);
            // we have the tile pixels, figure out the palette indices, the tile pixels are 2 bits each in 2 panes so we need to first put them together
            uint8_t upper = tileRow >> 8;
            uint8_t lower = tileRow & 0xff;
            for (int i = 7; i >= 0; --i) {
                uint8_t colorIndex = ((upper >> i) & 1) | (((lower >> i) & 1) << 1);
                // TODO render the pixel
                // displaySetPixel(x, ly, palette[colorIndex]);
                if (x >= 0 && x < 160)
                    pixels_.front()[x] = palette[colorIndex];
                ++x;
            }
        }
        displayUpdate(pixels_.front(), 160);
        pixels_.swap();

    }

    void GBCEmu::setRomPage(uint32_t page) {
        memMap_[4] = const_cast<uint8_t *>(gamepak_->getPage(page));
        memMap_[5] = memMap_[4] + 0x1000;
        memMap_[6] = memMap_[4] + 0x2000;
        memMap_[7] = memMap_[4] + 0x3000;
    }

    void GBCEmu::setVideoRamPage(uint32_t page) {
        ASSERT(page < 2);
        memMap_[8] = vram_[page * 2];
        memMap_[9] = vram_[page * 2 + 1];
    }

    void GBCEmu::setExternalRamPage(uint32_t page) {
        ASSERT(page < 16);
        memMap_[10] = eram_[page * 2];
        memMap_[11] = eram_[page * 2 + 1];
    }

    void GBCEmu::setWorkRamPage(uint32_t page) {
        ASSERT(page > 0 && page < 8);
        memMap_[13] = wram_[page];
        // don't forget to set the echo ram as well here
        memMap_[15] = wram_[page];
    }

    // arithmetics

    uint8_t GBCEmu::inc8(uint8_t x) {
        ++x;
        setFlagZ(x == 0);
        setFlagH((x & 0xf) == 0);
        return x;
    }
    
    uint8_t GBCEmu::dec8(uint8_t x) {
        --x;
        setFlagZ(x == 0);
        setFlagH((x & 0xf) == 0xf);
        return x;
    }
    
    /** Adds two 8bit numbers, optinally including a carry flag and sets the Z, H and C flags accordingly. 

        The Z flag is set if the 8bit result is 0. The C flag is set if the result is greater than 256. Finally, the H flag is set if during 
     */
    uint8_t GBCEmu::add8(uint8_t a, uint8_t b, uint8_t c) {
        unsigned r = a + b + c;
        setFlagZ((r & 0xff) == 0);
        setFlagH(((a ^ b ^ r) & 0x10) != 0);
        setFlagC(r > 0xff);
        return static_cast<uint8_t>(r);
    }

    uint8_t GBCEmu::sub8(uint8_t a, uint8_t b, uint8_t c) {
        unsigned r = a - (b + c);
        setFlagZ((r & 0xff) == 0);
        setFlagH(((a ^ b ^ r) & 0x10) != 0);
        setFlagC(r > 0xff);
        return static_cast<uint8_t>(r);
    }

    uint16_t GBCEmu::add16(uint16_t a, uint16_t b) {
        uint32_t r = a + b;
        setFlagC(r > 0xffff);
        setFlagH(((r ^ a ^ b) & 0x1000) != 0);
        return static_cast<uint16_t>(r);
    }

    /** Rotate left, set carry
     */
    uint8_t GBCEmu::rlc8(uint8_t a) {
        uint16_t r = a << 1;
        setFlagC(r & 256);
        r = r | flagC();
        return (r & 0xff);
    }

    /** Rotate left through carry. 
     */
    uint8_t GBCEmu::rl8(uint8_t a) {
        uint16_t r = a << 1;
        r = r | flagC();
        setFlagC(r & 256);
        return (r & 0xff);
    }

    /** Rotate right, set carry. 
     */
    uint8_t GBCEmu::rrc8(uint8_t a) {
        setFlagC(a & 1);
        a = a >> 1;
        a = a | (flagC() ? 128 : 0);
        return a;
    }

    /** Rotate right, through carry. 
     */
    uint8_t GBCEmu::rr8(uint8_t a) {
        bool cf = flagC();
        setFlagC(a & 1);
        a = a >> 1;
        a = a | (cf ? 128 : 0);
        return a;
    }

    /** Shift left, overflow to carry.
     */
    uint8_t GBCEmu::sla8(uint8_t a) {
        uint16_t r = a * 2;
        setFlagC(r & 256);
        return r & 0xff;
    }

    /** Shift right, arithmetically, i.e. keep msb intact*/
    uint8_t GBCEmu::sra8(uint8_t a) {
        setFlagC(a & 1);
        a = a >> 1;
        a |= (a & 64) ? 128 : 0;
        return a;
    }

    /** Shift right, logically, i.e.msb set to 0. 
     */
    uint8_t GBCEmu::srl8(uint8_t a) {
        setFlagC(a & 1);
        a = a >> 1;
        return a;
    }

    // memory 

    uint8_t GBCEmu::memRd8(uint16_t addr) {
        uint32_t page = addr >> 12;
        uint32_t offset = addr & 0xfff;
        if (page < 15)
            return memMap_[page][offset];
        // if the offset is last 256 bytes, return the hram contents
        if (offset >= 0xf00)
            return hram_[offset - 0xf00];
        // otherwise return OAM if in range
        if (offset >= 0xe00)
            return oam_[offset - 0xe00];
        // otherwise this is the echo ram, return the wram mirror contents
        return memMap_[page][offset];
    }

    uint16_t GBCEmu::memRd16(uint16_t addr) {
        // TODO this is the naive implementation where we read two bytes and combine them. However, when reading from the lower pages and the read does not cross page boundary, we can optimize this into a single 16bit read
        return memRd8(addr) | (memRd8(addr + 1) << 8);
    }

    void GBCEmu::memWr8(uint16_t addr, uint8_t value) {
        uint32_t page = addr >> 12;
        uint32_t offset = addr & 0xfff;
        if (page < 8) {
            //UNIMPLEMENTED;
            // TODO bank switching
        } else if (page != 15) {
            memMap_[page][offset] = value;
        } else if (offset >= 0xf00) {
            hram_[offset - 0xf00] = value;
            // TODO special handling for ioregs and stuff
        } else if (offset >= 0xe00) {
            oam_[offset - 0xe00] = value;
        } else {
            memMap_[page][offset] = value;
        }
    }

    void GBCEmu::memWr16(uint16_t addr, uint16_t value) {
        memWr8(addr, value & 0xff);
        memWr8(addr + 1, value >> 8);
    }

    uint8_t GBCEmu::mem8(uint16_t addr) {
        uint32_t page = addr >> 12;
        uint32_t offset = addr & 0xfff;
        return memMap_[page][offset];
    }

    uint16_t GBCEmu::mem16(uint16_t addr) {
        // TODO this is the naive implementation where we read two bytes and combine them. However, when reading from the lower pages and the read does not cross page boundary, we can optimize this into a single 16bit read
        return mem8(addr) | (mem8(addr + 1) << 8);
    }

} // namespace rckid::gbcemu
