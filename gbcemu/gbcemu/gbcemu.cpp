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

#define IO_ADDR(REG) (& REG - hram_)

/** The joypad register. 
 
    Writing to the register's upper nibble chooses between dpad (bit 4) and buttons (bit 5), while the lower nibble contains the values of the buttons. When button is pressed, it should read 0. Internally this really is a button matrix.

    bit 5 = select buttons (write)
    bit 4 = select dpad (write)
    bit 3 = start / down (read)
    bit 2 = select / up (read)
    bit 1 = B / left (read)
    bit 0 = A / right (read)

    When any of the lower bits transition from 1 to 0, the joypad interrupt is raised.
 */
#define IO_JOYP (hram_[0x00])
static constexpr uint8_t JOYP_DPAD = 16;
static constexpr uint8_t JOYP_BUTTONS = 32;

/** When device wants to transfer a byte via the serial line, it writes it to this register. 
 */
#define IO_SB (hram_[0x01])
#define IO_SC (hram_[0x02])

/** The 16384Hz timer. This value is incremented at a steady rate. Any write to it should reset the value to 0. 
 
    The DMG runs the CPU at 4 194 304 Hz, which means it should be incremented 256 times in a second, while the CGB runs at 8 388 608 Hz, exactly double the rate, which gives 512 increments per second. 

    TODO Determine when & how to update this
 */
#define IO_DIV (hram_[0x04])

/** Timer counter. Increments the register value at rate given by IO_TAC. When it overflows 0xff, it is being reset to IO_TMA value. 
 */
#define IO_TIMA (hram_[0x05])

/** Timer modulo. When IO_TIMA overflows, it is reset to this value. 
 */
#define IO_TMA (hram_[0x06])

/** The Timer control register. 
 
    When the timer is enabled, the IO_TIMA is incremented at the rate given by the lower 2 bits of this register. Note that the IO_DIV is always counting, and unlike the IO_TIMA is not specified in M cycles, but actual Hz so will increment at the same pace in both DMG and CGB, whereas the IO_TIMA will run twice as fast on CGB for the same values. 
 */
#define IO_TAC (hram_[0x07])
static constexpr uint8_t TAC_ENABLE = 4;
static constexpr uint8_t TAC_CLOCK_SELECT_MASK = 3;
static constexpr uint8_t TAC_CLOCK_SELECT_256M = 0;
static constexpr uint8_t TAC_CLOCK_SELECT_4M = 1;
static constexpr uint8_t TAC_CLOCK_SELECT_16M = 2;
static constexpr uint8_t TAC_CLOCK_SELECT_64M = 4;

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
    bit 4 = BG & Window tile data area ( 0 = 0x8800 - 0x97ff, 1 = 0x8000 - 0x8fff)
    bit 3 = BG tilemap area ( 0 = 0x9800 - 0x9bfff, 1 = 0x9c00 - 0x9fff)
    bit 2 = OBJ size (0 = 8x8, 1 = 8x16)
    bit 1 = OBJ enable
    bit 0 = BG/Win enable / priority -- CGB Specific
 */
#define IO_LCDC (hram_[0x40])
static constexpr uint8_t LCDC_LCD_ENABLE = 1 << 7;
static constexpr uint8_t LCDC_WINDOW_TILEMAP = 1 << 6;
static constexpr uint8_t LCDC_WINDOW_ENABLE = 1 << 5;
static constexpr uint8_t LCDC_BG_WIN_TILEDATA = 1 << 4;
static constexpr uint8_t LCDC_BG_TILEMAP = 1 << 3;
static constexpr uint8_t LCDC_OBJ_SIZE = 1 << 2;
static constexpr uint8_t LCDC_OBJ_ENABLE = 1 << 1;
static constexpr uint8_t LCDC_BG_WIN_PRIORITY = 1 << 0;

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
#define IO_BGP (hram_[0x47])
#define IO_OBP0 (hram_[0x48])
#define IO_OBP1 (hram_[0x49])
#define IO_WY (hram_[0x4a])
#define IO_WX (hram_[0x4b])
#define IO_KEY1 (hram_[0x4d])
#define IO_VBK (hram_[0x4f])
#define IO_HDMA1 (hram_[0x51])
#define IO_HDMA2 (hram_[0x52])
#define IO_HDMA3 (hram_[0x53])
#define IO_HDMA4 (hram_[0x54])
#define IO_HDMA5 (hram_[0x55])
#define IO_RP (hram_[0x56])
#define IO_BCPS (hram_[0x68])
#define IO_BCPD (hram_[0x69])
#define IO_OCPS (hram_[0x6a])
#define IO_OCPD (hram_[0x6b])
#define IO_SVBK (hram_[0x70])
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
            a.alloc<uint8_t>(0x2000), 
            a.alloc<uint8_t>(0x2000), 
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
        for (uint32_t i = 0; i < 2; ++i)
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
        // and IO initial values, also from CGB
        IO_JOYP = 0xcf;
        IO_SB = 0x00;
        IO_SC = 0x7f;
        // IO_DIV = 0x18; // from DMG, can't tell on CBG
        IO_TIMA = 0x00;
        IO_TMA = 0x00;
        setIORegisterOrHRAM(IO_ADDR(IO_TAC), 0xf8);
        IO_IF = 0xe1;
        IO_NR10 = 0x80;
        IO_NR11 = 0xbf;
        IO_NR12 = 0xf3;
        IO_NR13 = 0xff;
        IO_NR14 = 0xbf;
        IO_NR21 = 0x3f;
        IO_NR22 = 0x00;
        IO_NR23 = 0xff;
        IO_NR24 = 0xbf;
        IO_NR30 = 0x7f;
        IO_NR31 = 0xff;
        IO_NR32 = 0x9f;
        IO_NR33 = 0xff;
        IO_NR34 = 0xbf;
        IO_NR41 = 0xff;
        IO_NR42 = 0x00;
        IO_NR43 = 0x00;
        IO_NR44 = 0xbf;
        IO_NR50 = 0x77;
        IO_NR51 = 0xf3;
        IO_NR52 = 0xf1;
        IO_LCDC = 0x91;
        // IO_STAT = 0; -- can't tell
        IO_SCY = 0;
        IO_SCX = 0;
        // IO_LY = 0; // can't tell
        IO_LYC = 0;
        IO_DMA = 0;
        IO_BGP = 0xfc;
        // IO_OBP0 = 0; // can't tell
        // IO_OBP1 = 0; // can't tell
        IO_WY = 0;
        IO_WY = 1;
        IO_KEY1 = 0x7e;
        IO_VBK = 0xfe;
        IO_HDMA1 = 0xff;
        IO_HDMA2 = 0xff;
        IO_HDMA3 = 0xff;
        IO_HDMA4 = 0xff;
        IO_HDMA5 = 0xff;
        IO_RP = 0x3e;
        // IO_BCPS = 0; // can't tell
        // IO_BCPD = 0; // can't tell
        // IO_OCPS = 0; // can't tell
        // IO_OCPD = 0; // can't tell
        IO_SVBK = 0xf8;
        IO_IE = 0;
        // and reset counters
        timerCycles_ = 0;
        // set the initial values for the IO registers 
        IO_LY = 0; // ensure we'll start with new frame
    #ifdef GBCEMU_INTERACTIVE_DEBUG     
        resetVisited();
    #endif
    }

    void GBCEmu::runCPU() {
        clearTilemap();
        clearTileset();
        //setBreakpoint(0xc243);
        //setBreakpoint(0xcb10);
        //setMemoryBreakpoint(0xdffd, 0xdfff);
        while (true) {
            renderLine();
            for (uint32_t cycles = 0; cycles < cyclesPerLine_; ) {
    #ifdef GBCEMU_INTERACTIVE_DEBUG     
                markAsVisited(PC);           
                if (PC == breakpoint_ || debug_) {
                    debugWrite() << "===== BREAKPOINT ===== (pc " << hex(pc_) << ")\n";
                    logDisassembly(PC, PC + 10);
                    logState();
                    debugInteractive();
                } else if (PC == overBreakpoint_) {
                    debugInteractive();
                }
    #endif
                cycles += step();
            }
        }
    }

    void GBCEmu::setState(uint16_t pc, uint16_t sp, uint16_t af, uint16_t bc, uint16_t de, uint16_t hl, bool ime, uint8_t ie) {
        PC = pc;
        SP = sp;
        AF = af;
        BC = bc;
        DE = de;
        HL = hl;
        ime_ = ime;
        IO_IE = ie;
    }

    void GBCEmu::writeMem(uint16_t address, std::initializer_list<uint8_t> values) {
        for (uint8_t value : values) {
            uint32_t page = address >> 12;
            uint32_t offset = address & 0xfff;
            if (page == 15)
                memWr8(address, value);
            else 
                memMap_[page][offset] = value;
            ++address;
        }
    }

    uint8_t GBCEmu::readMem(uint16_t address) {
        return memRd8(address);
    }

    uint32_t GBCEmu::step() {
        uint32_t usedCycles = 0;
        uint8_t opcode = mem8(PC++);
        switch (opcode) {
            #define INS(OPCODE, FLAG_Z, FLAG_N, FLAG_H, FLAG_C, SIZE, CYCLES, MNEMONIC, ...) \
            case OPCODE: \
                usedCycles = CYCLES; \
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
        timerCycles_ += usedCycles;
        updateTimer();
        return usedCycles;
    }

#ifdef GBCEMU_INTERACTIVE_DEBUG

    void GBCEmu::logDisassembly(uint16_t start, uint16_t end) {
        debugWrite() << "===== DISASSEMBLY ===== (from " << hex(start) << " to " << hex(end) << ")\n";
        for (uint16_t i = start; i < end; ) {
            i += disassembleInstruction(i);
        }
    }

    void GBCEmu::logMemory(uint16_t start, uint16_t end) {
        debugWrite() << "===== MEMORY ===== (from " << hex(start) << " to " << hex(end) << ")\n";
        debugWrite() << "      00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n";
        for (uint16_t i = start; i < end; i += 16) {
            debugWrite() << hex(i, false) << ": ";
            for (uint16_t j = 0; j < 16; ++j) {
                debugWrite() << hex(mem8(i + j), false) << ' ';
            }
            debugWrite() << '\n';
        }
    }

    void GBCEmu::logStack(uint32_t n) {
        debugWrite() << "===== STACK =====\n";
        for (uint16_t i = SP, e = SP + n; i != e; i += 2) {
            debugWrite() << hex(i, false) << ": " << hex(mem16(i), false) << '\n';
        }
    }

    void GBCEmu::logState() {
        debugWrite() << "===== CPU STATE =====\n";
        debugWrite() <<  "af:   " <<  hex(AF, false) << " bc:   " <<  hex(BC, false) << " de:   " <<  hex(DE, false) << " hl:   " <<  hex(HL, false) << " sp:   " <<  hex(SP, false) << " pc:   " <<  hex(PC, false) << '\n';

        debugWrite() << "lcdc: " << hex(IO_LCDC, false) << "   stat: " << hex(IO_STAT, false) << "   ly:   " << hex(IO_LY, false) << "   ie:   " << hex(IO_IE, false) << "   if:   " << hex(IO_IF, false) << '\n';
    }

    void GBCEmu::logVisited() {
        uint32_t total = 0;
        debugWrite() << "===== VISITED INSTRUCTIONS =====\n";
        for (uint32_t i = 0; i < 32; ++i) {
            for (uint32_t j = 0; j < 8; ++j) {
                if (visitedInstructions_[i] & (1 << j)) {
                    debugWrite() << hex<uint8_t>(i * 8 + j, false) << ' ';
                    ++total;
                } else {
                    debugWrite() << "   ";
                }
            }
            if (i & 1)
                debugWrite() << '\n';
        }
        debugWrite() << "Extended (0xcb) prefix:\n";
        for (uint32_t i = 0; i < 32; ++i) {
            for (uint32_t j = 0; j < 8; ++j) {
                if (visitedInstructions_[i + 32] & (1 << j)) {
                    debugWrite() << hex<uint8_t>(i * 8 + j, false) << ' ';
                    ++total;
                } else {
                    debugWrite() << "   ";
                }
            }
            if (i & 1)
                debugWrite() << '\n';
        }
        debugWrite() << "Total: " << total << '\n';
    }

    void GBCEmu::clearTilemap() {
        for (uint16_t i = 0x9800; i < 0x9fff; ++i)
            memWr8(i, 0);
    }

    void GBCEmu::setTilemap(uint32_t x, uint32_t y, uint8_t tile) {
        uint16_t tAddr = 0x9800 + y * 32 + x;
        memWr8(tAddr, tile);
    }

    void GBCEmu::clearTileset() {
        for (uint16_t i = 0x8000; i < 0x9800; ++i)
            memWr8(i, 0);
    }

    void GBCEmu::setTile(uint8_t index, uint8_t * data) {
        uint16_t tAddr = 0x8800 + index * 16;
        for (uint16_t i = 0; i < 16; ++i)
            memWr8(tAddr + i, data[i]);
    }

    uint32_t GBCEmu::instructionSize(uint8_t opcode) const {
        switch (opcode) {
            #define INS(OPCODE, FLAG_Z, FLAG_N, FLAG_H, FLAG_C, SIZE, CYCLES, MNEMONIC, ...) \
            case OPCODE: \
                return SIZE;
            #include "insns.inc.h"
            default:
                return 1;
        };
    }

    uint32_t GBCEmu::disassembleInstruction(uint16_t addr) {
        uint8_t opcode = mem8(addr);
        uint32_t size = instructionSize(opcode);
        debugWrite() << hex(addr, false) << ": " << hex(mem8(addr), false) << ' ';
        if (size == 1)
            debugWrite() << "          ";
        else if (size == 2) 
            debugWrite() << hex(mem8(addr + 1), false) << "        ";
        else if (size == 3)
            debugWrite() << hex(mem8(addr + 1), false) << ' ' << hex(mem8(addr + 2), false) << "     ";
        else 
            ASSERT("Invalid instruction size");
        switch (opcode) {
            #define INS(OPCODE, FLAG_Z, FLAG_N, FLAG_H, FLAG_C, SIZE, CYCLES, MNEMONIC, ...) \
            case OPCODE: \
                debugWrite() << MNEMONIC << '\n'; \
                break;
            #include "insns.inc.h"
            default:
                debugWrite() << "???\n";
        };
        return size;
    }

    void GBCEmu::markAsVisited(uint16_t pc) {
        uint32_t opcode = mem8(pc);
        if (opcode == 0xcb)
            opcode = 0xff + mem8(pc + 1);
        uint32_t addr = opcode / 8;
        uint32_t bit = opcode & 7;
        visitedInstructions_[addr] |= 1 << bit;
    }

    void GBCEmu::resetVisited() {
        for (uint32_t i = 0; i < 64; ++i)
            visitedInstructions_[i] = 0;
    }

    void GBCEmu::debugInteractive() {
        overBreakpoint_ = 0xffffff; // disable step over breakpoint
        debug_ = false;
        while (true) {
            disassembleInstruction(PC);
            uint8_t cmd = debugRead(false);
            switch (cmd) {
                // continue running uninterrupted
                case 'c':
                    debugWrite() << "> continue\n";
                    return;
                // execute single instruction
                case 'n':
                    step();
                    break;
                case 'o':
                    overBreakpoint_ = PC + instructionSize(mem8(PC));
                    return;
                // set breakpoint
                case 'b': {
                    debugWrite() << "? breakpoint address ";
                    breakpoint_ = debugReadHex16();
                    debugWrite() << '\n';
                    //logBreakpoint();
                    break;
                }
                // display cpu info (state & stuff)
                case 'i':
                    logState();
                    break;
                case 's':
                    logStack(10);
                    break;
                // display disassembly
                case 'd': {
                    debugWrite() << "? disassembly start address ";
                    uint16_t start = debugReadHex16();
                    debugWrite() << "\n? length (default 0x10) ";
                    uint16_t l = debugReadHex16();
                    if (l == 0)
                        l = 0x10;
                    logDisassembly(start, start + l);
                    break;
                }
                // display memory
                case 'm': {
                    debugWrite() << "? memory start address ";
                    uint16_t start = debugReadHex16();
                    debugWrite() << "\n? length (default 0x10) ";
                    uint16_t l = debugReadHex16();
                    if (l == 0)
                        l = 0x10;
                    logMemory(start, start + l);
                    break;
                }
                // shows visited instructions
                case 'v':
                    logVisited();
                    break;
                default:
                    debugWrite() << "! invalid command '" << cmd << "'\n";
            }
        }
        UNREACHABLE;
    }

#endif

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
        uint8_t * vram = memMap_[MEMMAP_VRAM_0];
        uint8_t * tilemapAddress = vram + ((IO_LCDC & LCDC_BG_TILEMAP) ? 0x1c00 : 0x1800);
        tilemapAddress += ty * 32 + tx; 
        // and determine the tileset address
        uint16_t * tilesetAddress = reinterpret_cast<uint16_t *>(vram + ((IO_LCDC & LCDC_BG_WIN_TILEDATA) ? 0x0000 : 0x0800));
        // and figure out the palette we will be using for the row
        uint16_t palette[4];
        // TODO fill the palette from IO_BGB
        palette[0] = ColorRGB{155, 188, 15}.raw16();
        palette[1] = ColorRGB{139, 172, 15}.raw16();    
        palette[2] = ColorRGB{48, 98, 48}.raw16();
        palette[3] = ColorRGB{15, 56, 15}.raw16();

        // render the pixels now, we keep x as the current x coordinate on the screen
        int16_t x = - (8 - (bx % 8)) & 0x7;
        while (x < 160) {
            // determine which tile we are using
            // TODO for CGB we also need to determine the tile attributes 
            uint8_t tileIndex = *tilemapAddress++;
            uint16_t tileRow = *(tilesetAddress + tileIndex * 8 + tr);
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
        memMap_[MEMMAP_VRAM_0] = vram_[page];
        memMap_[MEMMAP_VRAM_1] = vram_[page] + 4096;
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
#ifdef GBCEMU_INTERACTIVE_DEBUG
        if (addr >= memoryBreakpointStart_ && addr < memoryBreakpointEnd_) {
            debugWrite() << "===== MEMORY BREAKPOINT ===== (read address " << hex(addr) << ")\n";
            logMemory(memoryBreakpointStart_, memoryBreakpointEnd_);
            debug_ = true;
        }
#endif
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
#ifdef GBCEMU_INTERACTIVE_DEBUG
        if (addr >= memoryBreakpointStart_ && addr < memoryBreakpointEnd_) {
            debugWrite() << "===== MEMORY BREAKPOINT ===== (write address " << hex(addr) << ", value " << hex(value) << ")\n";
            logMemory(memoryBreakpointStart_, memoryBreakpointEnd_);
            debug_ = true;
        }
#endif
        uint32_t page = addr >> 12;
        uint32_t offset = addr & 0xfff;
        switch (page) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
                // UNIMPLEMENTED;
                // TODO bank switching
                break;
            case 10:
            case 11:
                // writing to eram even if there is none is technically allowed and should not break the game
                if (memMap_[page] != nullptr)
                    memMap_[page][offset] = value;
                break;
            case 8:
            case 9:
            case 12:
            case 13:
            case 14:
                // vram and wram are always there so we can do what we want, the shadow mem is implemented having the shadow pages identical to the real ones
                memMap_[page][offset] = value;
                break;
            case 15:
                if (offset >= 0xf00)
                    setIORegisterOrHRAM(offset - 0xf00, value);
                else if (offset >= 0xe00)
                    oam_[offset - 0xe00] = value;
                else
                    memMap_[page][offset] = value;
                break;
        }
    }

    void GBCEmu::setIORegisterOrHRAM(uint32_t addr, uint8_t value) {
        switch (addr) {
            case 0x00: // IO_JOYP
                // only the upper nibble is writeable, and once written, update the lower nibble accordingly
                IO_JOYP = (IO_JOYP & 0xf) | (value & 0xf0);
                updateIO_JOYP();
                break;
            case 0x01: // IO_SB
                LOG(LL_GBCEMU_SERIAL, static_cast<char>(value));
                break;
            case 0x02: // IO_SC
                break;
            case 0x04: // IO_DIV
                // this is the 16374Hz timer. Any write to the register resets the value to zero
                IO_DIV = 0;
                return; // do not perform the write
            case 0x05: // IO_TIMA
            case 0x06: // IO_TMA
                break; // no special care necessary for those registers
            case 0x07: // IO_TAC
                if (value & TAC_ENABLE == 0)
                    timerCycles_ = 0;
                else switch (value & TAC_CLOCK_SELECT_MASK) {
                    case TAC_CLOCK_SELECT_256M:
                        timerTIMAModulo_ = 255;
                        break;
                    case TAC_CLOCK_SELECT_4M:
                        timerTIMAModulo_ = 3;
                        break;
                    case TAC_CLOCK_SELECT_16M:
                        timerTIMAModulo_ = 15;
                        break;
                    case TAC_CLOCK_SELECT_64M:
                        timerTIMAModulo_ = 63;
                        break;
                }
                break;
        }
        hram_[addr] = value;
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


    void GBCEmu::updateTimer() {
        if (timerCycles_ & timerDIVModulo_ == 0)
            ++IO_DIV;
        if (timerTIMAModulo_ != 0 && timerCycles_ & timerTIMAModulo_ == 0) {
            if (++IO_TIMA == 0) {
                IO_TIMA = IO_TMA;
                IO_IF |= IF_TIMER;
            }
        }
    }

    void GBCEmu::updateIO_JOYP() {
        #undef A
        #undef B
        uint8_t value;
        if (IO_JOYP & 0x10) { // dpad
            value = (btnDown(Btn::Down)   ? 0 : 8) |
                    (btnDown(Btn::Up)     ? 0 : 4) |
                    (btnDown(Btn::Left)   ? 0 : 2) |
                    (btnDown(Btn::Right)  ? 0 : 1);
        } else { // buttons
            value = (btnDown(Btn::Start)  ? 0 : 8) |
                    (btnDown(Btn::Select) ? 0 : 4) |
                    (btnDown(Btn::B)      ? 0 : 2) |
                    (btnDown(Btn::A)      ? 0 : 1);
        }
        // check if we need an interrupt to be requested
        if ( ((value & 1) == 0 && IO_JOYP & 1) ||
             ((value & 2) == 0 && IO_JOYP & 2) ||
             ((value & 4) == 0 && IO_JOYP & 4) ||
             ((value & 8) == 0 && IO_JOYP & 8) )
            IO_IF |= IF_JOYPAD;
        // set the register
        IO_JOYP = (IO_JOYP & 0xf0) | value;
    }

} // namespace rckid::gbcemu


