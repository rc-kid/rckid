#pragma once
#include <cstdlib>
#include <cstdint>

#include "rckid/rckid.h"

/** GameBoy Color Emulator.

 */
class GBC {
public:

    /** Emulator state. 
     
        Contains all information necessary to preserve the gameboy's state, such as registers, RAM memory, IO registers, OAM and other flags. 
     */
    class State {
    public:

        ~State() {
            delete [] vram_;
            delete [] wram_;
            delete [] eram_;
            delete [] oam_;
            delete [] highMem_;
        }
    
        /** \name Registers. 
         
            The following 8 bit registers are available:

            - A (accumulator)
            - F (flags), only few bits available as the ZNHC flags. 
            - B, C, D, E, H, L general purpose 8bit registers
            - BC, DE, HL general purpose 16bit registers
            - SP (stack pointer)
            - PC (program counter)

            The AF, BC, DE, HL 16bit registers share the same space with their 8bit counterparts. 
         */
        //@{
        uint8_t a() const { return rawRegs8_[REG_INDEX_A]; }
        uint8_t f() const { return rawRegs8_[REG_INDEX_F]; }
        uint8_t b() const { return rawRegs8_[REG_INDEX_B]; }
        uint8_t c() const { return rawRegs8_[REG_INDEX_C]; }
        uint8_t d() const { return rawRegs8_[REG_INDEX_D]; }
        uint8_t e() const { return rawRegs8_[REG_INDEX_E]; }
        uint8_t h() const { return rawRegs8_[REG_INDEX_H]; }
        uint8_t l() const { return rawRegs8_[REG_INDEX_L]; }

        uint16_t af() const { return rawRegs16_[REG_INDEX_AF]; }
        uint16_t bc() const { return rawRegs16_[REG_INDEX_BC]; }
        uint16_t de() const { return rawRegs16_[REG_INDEX_DE]; }
        uint16_t hl() const { return rawRegs16_[REG_INDEX_HL]; }

        uint16_t pc() const { return pc_; }
        uint16_t sp() const { return sp_; }
        //@}

        /** \name Flags
          
            - `Z` (zero flag) set only if the actual result is 0
            - `N` (subtract flag) set to a constant it seems
            - `H` (half carry flag) 
            - `C` (carry flag)
         
        */
        //@{
        bool flagZ() const { return rawRegs8_[REG_INDEX_F] & FLAG_Z; }
        bool flagN() const { return rawRegs8_[REG_INDEX_F] & FLAG_N; }
        bool flagH() const { return rawRegs8_[REG_INDEX_F] & FLAG_H; }
        bool flagC() const { return rawRegs8_[REG_INDEX_F] & FLAG_C; }
        //@}

        /** \name Memory 
         
            GBC provides the following memories:

            - 16KB video ram, banked by 8KB
            - 32KB work ram, banked by 4KB
            - cartridge ROM, up to 8MB, banked by 16KB
            - cartridge RAM, up to 128KB, banked by 8KB

            Since only 64KB can be addressed at time, memory mapping is a must. To access cartridge data larger than 32KB, a memory mapper chip must be present in the cartridge. 

            # Memory Map

            0x0000 | 0x3fff | 16KB  | 0..3   | ROM Bank 00, from cartridge, usually fixed bank
            0x4000 | 0x7fff | 16KB  | 4..7   | ROM bank 01-NN, from cartridge, switchable by mapper chip if any
            0x8000 | 0x9fff |  8KB  | 8,9    | Video RAM
            0xa000 | 0xbfff |  8KB  | 10, 11 | External RAM
            0xc000 | 0xcfff |  4KB  | 12     | WRAM bank 0
            0xd000 | 0xdfff |  4KB  | 13     | WRAM bank 1..7
            0xe000 | 0xfdff |  <8KB | 14, 15 | Echo ram, mirror of 0xc000..0xddff, prohibited
            0xfe00 | 0xfe9f |  160  |        | OAM memory
            0xfea0 | 0xfeff |       |        | Prohibited
            0xff00 | 0xff7f | 128   |        | IO Registers
            0xff80 | 0xfffe | 127   |        | High RAM 
            0xffff | 0xffff | 1     |        | Interrupt enable register        

         */
        //@{

        void setRom(uint8_t const * rom, size_t numBytes) {
            rom_ = rom;
            romSize_ = numBytes;
            for (size_t i = 0; i < 4; ++i)
                memMap_[i] = const_cast<uint8_t *>(rom_ + MEMMAP_REGION_SIZE * i);
            setROMBank(1);
        }

        uint8_t const * const * memMap() const { return memMap_; }

        uint8_t * vram() const { return vram_; }
        size_t vramSize() const { return 16 * 1024; }
        
        uint8_t * wram() const { return wram_; };
        size_t wramSize() const { return 32 * 1024; }
        
        uint8_t * eram() const { return eram_; };
        size_t eramSize() const { return eramSize_; }
        
        uint8_t * oam() const { return oam_; }
        size_t oamSize() const { return 160; }
        
        uint8_t * ioRegs() const { return highMem_; }
        
        uint8_t * hram() const { return highMem_ + 0x80; }
        size_t hramSize() const { return 127; }
        
        uint8_t & ieReg() const { return highMem_[0xff]; }

        //@}

    private:

        friend class GBC;

        static constexpr size_t REG_INDEX_A = 1;
        static constexpr size_t REG_INDEX_F = 0;
        static constexpr size_t REG_INDEX_B = 3;
        static constexpr size_t REG_INDEX_C = 2;
        static constexpr size_t REG_INDEX_D = 5;
        static constexpr size_t REG_INDEX_E = 4;
        static constexpr size_t REG_INDEX_H = 7;
        static constexpr size_t REG_INDEX_L = 6;

        static constexpr size_t REG_INDEX_AF = 0;
        static constexpr size_t REG_INDEX_BC = 1;
        static constexpr size_t REG_INDEX_DE = 2;
        static constexpr size_t REG_INDEX_HL = 3;

        static constexpr uint8_t FLAG_Z = 1 << 7;
        static constexpr uint8_t FLAG_N = 1 << 6;
        static constexpr uint8_t FLAG_H = 1 << 5;
        static constexpr uint8_t FLAG_C = 1 << 4;   

        static constexpr size_t VRAM_SIZE = 16 * 1024;
        static constexpr size_t WRAM_SIZE = 32 * 1024;

        static constexpr size_t ROM_BANK_SIZE = 16 * 1024;
        static constexpr size_t VRAM_BANK_SIZE = 8 * 1024;

        static constexpr size_t MEMMAP_REGION_SIZE = 4 * 1024;
        static constexpr size_t MEMMAP_REGION_ROM = 4;
        static constexpr size_t MEMMAP_REGION_WRAM = 12;
        static constexpr size_t MEMMAP_REGION_VRAM = 8;
        static constexpr size_t MEMMAP_REGION_ECHO_RAM = 14;

        void initialize() {
            for (size_t i = 0; i < sizeof(rawRegs8_); ++i)
                rawRegs8_[i] = 0;
            for (size_t i = 0; i < 16; ++i)
                memMap_[i] = nullptr;
            delete [] vram_;
            delete [] wram_;
            delete [] eram_;
            delete [] oam_;
            delete [] highMem_;
            vram_ = new uint8_t[VRAM_SIZE];
            wram_ = new uint8_t[WRAM_SIZE]; 
            eram_ = nullptr;
            eramSize_ = 0;
            oam_ = new uint8_t[oamSize()];
            highMem_ = new uint8_t[256]; 
            memMap_[MEMMAP_REGION_WRAM] = wram_;
            memMap_[MEMMAP_REGION_ECHO_RAM] = wram_;
            setVideoRAMBank(0);
            setWorkRAMBank(1);
            ime_ = true;
        } 

        void setFlagZ(bool value) { value ? rawRegs8_[REG_INDEX_F] |= FLAG_Z : rawRegs8_[REG_INDEX_F] &= ~FLAG_Z; }

        void setFlagN(bool value) { value ? rawRegs8_[REG_INDEX_F] |= FLAG_N : rawRegs8_[REG_INDEX_F] &= ~FLAG_N; }

        void setFlagH(bool value) { value ? rawRegs8_[REG_INDEX_F] |= FLAG_H : rawRegs8_[REG_INDEX_F] &= ~FLAG_H; }

        void setFlagC(bool value) { value ? rawRegs8_[REG_INDEX_F] |= FLAG_C : rawRegs8_[REG_INDEX_F] &= ~FLAG_C; }

        uint8_t setFlagZFrom(uint8_t value) {
            setFlagZ(value == 0);
            return value;
        }

        void setVideoRAMBank(size_t index) {
            memMap_[MEMMAP_REGION_VRAM] = vram_ + (index * VRAM_BANK_SIZE);
            memMap_[MEMMAP_REGION_VRAM + 1] = vram_ + (index * VRAM_BANK_SIZE) + MEMMAP_REGION_SIZE;
        }

        void setWorkRAMBank(size_t index) {
            memMap_[MEMMAP_REGION_WRAM + 1] = wram_ + (index * MEMMAP_REGION_SIZE);
            memMap_[MEMMAP_REGION_ECHO_RAM + 1] = wram_ + (index * MEMMAP_REGION_SIZE);
        }

        // depends on memory controller in cartridge
        void setROMBank(size_t bank) {
            for (size_t i = 0; i < 4; ++i)
                memMap_[MEMMAP_REGION_ROM + i] = const_cast<uint8_t *>(rom_ + (ROM_BANK_SIZE * bank) + (MEMMAP_REGION_SIZE * i));
        }

        // depends on memory controller in cartridge
        void setExternalRAMBank([[maybe_unused]] size_t index) {
        }


        union {
            uint8_t rawRegs8_[8];
            uint16_t rawRegs16_[4];
        }; 

        uint16_t sp_;
        uint16_t pc_;

        uint8_t * memMap_[16];

        uint8_t const * rom_ = nullptr;
        size_t romSize_ = 0;

        uint8_t * vram_ = nullptr;
        uint8_t * wram_ = nullptr;
        uint8_t * eram_ = nullptr;
        uint8_t * oam_ = nullptr;
        uint8_t * highMem_ = nullptr;
        size_t eramSize_ = 0;

        bool ime_ = false;


        unsigned rendererDots_ = 0;

    }; // GBC::State


    GBC() {}

    void start(uint16_t pc = 0x100) {
        state_.pc_ = pc;
        terminateAfterStop_ = false;
        loop();
    }

    /** \name Debugging functions
      
        These methods are intended for running tests and observing GBC's state. 
     */
    //@{

    /** Runs a test and returns when the `stop` instruction with any argument is encountered. 
     */
    void runTest(uint8_t const * rom, size_t numBytes, uint16_t pc = 0x0) {
        state_.initialize();
        // set the rom & set bank to 0
        state_.setRom(rom, numBytes);
        // set pc, enable stop termination and run the emulator loop
        state_.pc_ = pc;
        cycles_ = 0;
        terminateAfterStop_ = true;
        loop();
    }
    /** Number of cycles the emulator executed since last timed event. 
     */
    size_t cyclesElapsed() const { return cycles_; }

    State const & state() const { return state_; }

    //@}
private:

    /** The joypad register. 
     
        Writing to the register's upper nibble chooses between dpad (bit 4) and buttons (bit 5), while the lower nibble contains the values of the buttons. When button is pressed, it should read 0. 

        bit 5 = select buttons (write)
        bit 4 = select dpad (write)
        bit 3 = start / down (read)
        bit 2 = select / up (read)
        bit 1 = B / left (read)
        bit 0 = A / right (read)
     */
    static constexpr size_t ADDR_IO_JOYP = 0x00;
    static constexpr uint8_t JOYP_DPAD = 16;
    static constexpr uint8_t JOYP_BUTTONS = 32;

    static constexpr size_t ADDR_IO_SB = 0x01;
    static constexpr size_t ADDR_IO_SC = 0x02;
    static constexpr size_t ADDR_IO_DIV = 0x04;
    static constexpr size_t ADDR_IO_TIMA = 0x05;
    static constexpr size_t ADDR_IO_TMA = 0x06;
    static constexpr size_t ADDR_IO_TAC = 0x07;

    /** The Interrupt Flag Register
     
        bit 4 = Joypad
        bit 3 = Serial
        bit 2 = Timer
        bit 1 = LCD
        bit 0 = VBLANK
    */
    static constexpr size_t ADDR_IO_IF = 0x0f;
    static constexpr uint8_t IF_JOYPAD = 1 << 4;
    static constexpr uint8_t IF_SERIAL = 1 << 3;
    static constexpr uint8_t IF_TIMER = 1 << 2;
    static constexpr uint8_t IF_LCD = 1 << 1;
    static constexpr uint8_t IF_VBLANK = 1 << 0;
    static constexpr size_t ADDR_IO_NR10 = 0x10;
    static constexpr size_t ADDR_IO_NR11 = 0x11;
    static constexpr size_t ADDR_IO_NR12 = 0x12;
    static constexpr size_t ADDR_IO_NR13 = 0x13;
    static constexpr size_t ADDR_IO_NR14 = 0x14;
    //static constexpr size_t IO_NR20 = 0x15; // not used
    static constexpr size_t ADDR_IO_NR21 = 0x16;
    static constexpr size_t ADDR_IO_NR22 = 0x17;
    static constexpr size_t ADDR_IO_NR23 = 0x18;
    static constexpr size_t ADDR_IO_NR24 = 0x19;
    static constexpr size_t ADDR_IO_NR30 = 0x1a;
    static constexpr size_t ADDR_IO_NR31 = 0x1b;
    static constexpr size_t ADDR_IO_NR32 = 0x1c;
    static constexpr size_t ADDR_IO_NR33 = 0x1d;
    static constexpr size_t ADDR_IO_NR34 = 0x1e;
    //static constexpr size_t IO_NR40 = 0x1f; // not used
    static constexpr size_t ADDR_IO_NR41 = 0x20;
    static constexpr size_t ADDR_IO_NR42 = 0x21;
    static constexpr size_t ADDR_IO_NR43 = 0x22;
    static constexpr size_t ADDR_IO_NR44 = 0x23;
    static constexpr size_t ADDR_IO_NR50 = 0x24;
    static constexpr size_t ADDR_IO_NR51 = 0x25;
    static constexpr size_t ADDR_IO_NR52 = 0x26;
    static constexpr size_t ADDR_IO_WAVE_RAM_0 = 0x30; // 16 bytes
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
    static constexpr size_t ADDR_IO_LCDC = 0x40;
    /** Status and interrupts for the LCD driver
     
        bit 6 = LYC int select
        bit 5 = mode 2 int select 
        bit 4 = mode 1 int select
        bit 3 = mode 0 int select
        bit 2 = LYC == LY
        bits 0 & 1 = PPU mode 
    */
    static constexpr size_t ADDR_IO_STAT = 0x41;
    static constexpr uint8_t STAT_WRITE_MASK = 0b01111100;
    static constexpr uint8_t STAT_PPU_MODE = 3;
    static constexpr uint8_t STAT_LYC_EQ_LY = 1 << 2;
    static constexpr uint8_t STAT_INT_MODE0 = 1 << 3;
    static constexpr uint8_t STAT_INT_MODE1 = 1 << 4;
    static constexpr uint8_t STAT_INT_MODE2 = 1 << 5;
    static constexpr uint8_t STAT_INT_LYC = 1 << 6;
    static constexpr size_t ADDR_IO_SCY = 0x42;
    static constexpr size_t ADDR_IO_SCX = 0x43;
    /** LCD Y Coordinate
     
        Contains the current coordinate of the LCD renderer. 0 to 143 is active renderering, 144 to 153 is VBLANK.  
     */
    static constexpr size_t ADDR_IO_LY = 0x44;
    static constexpr size_t ADDR_IO_LYC = 0x45;
    static constexpr size_t ADDR_IO_DMA = 0x46;
    static constexpr size_t ADDR_IO_BGB = 0x47;
    static constexpr size_t ADDR_IO_OBP0 = 0x48;
    static constexpr size_t ADDR_IO_OBP1 = 0x49;
    static constexpr size_t ADDR_IO_WY = 0x4a;
    static constexpr size_t ADDR_IO_WX = 0x4b;
    static constexpr size_t ADDR_IO_KEY1 = 0x4d;
    static constexpr size_t ADDR_IO_WBK = 0x4f;
    static constexpr size_t ADDR_IO_HDMA1 = 0x51;
    static constexpr size_t ADDR_IO_HDMA2 = 0x52;
    static constexpr size_t ADDR_IO_HDMA3 = 0x53;
    static constexpr size_t ADDR_IO_HDMA4 = 0x54;
    static constexpr size_t ADDR_IO_HDMA5 = 0x55;
    static constexpr size_t ADDR_IO_RP = 0x56;
    static constexpr size_t ADDR_IO_BCPS_BGPI = 0x68;
    static constexpr size_t ADDR_IO_BCPD_BGPD = 0x69;
    static constexpr size_t ADDR_IO_OCPS_OCPI = 0x6a;
    static constexpr size_t ADDR_IO_OCPD_OBPD = 0x6b;
    static constexpr size_t ADDR_IO_PCM12 = 0x76;
    static constexpr size_t ADDR_IO_PCM34 = 0x77;
    static constexpr size_t ADDR_IO_IE = 0xff;

    State state_;

    /** \name Memory Reads and Writes
     */
    //@{

    uint8_t read8(uint16_t address);
    uint16_t read16(uint16_t address);
    void write8(uint16_t address, uint8_t value);
    void write16(uint16_t address, uint16_t value);
    FORCE_INLINE uint8_t rd8(uint16_t & address);
    FORCE_INLINE uint16_t rd16(uint16_t & address);

    //@}

    /** \name Arithmetics
     */
    //@{

    FORCE_INLINE uint8_t inc8(uint8_t x);
    FORCE_INLINE uint8_t dec8(uint8_t x);
    FORCE_INLINE uint8_t add8(uint8_t a, uint8_t b, uint8_t c = 0);
    FORCE_INLINE uint8_t sub8(uint8_t a, uint8_t b, uint8_t c = 0);
    FORCE_INLINE uint16_t add16(uint16_t a, uint16_t b);
    FORCE_INLINE uint8_t rlc8(uint8_t a);
    FORCE_INLINE uint8_t rl8(uint8_t a);
    FORCE_INLINE uint8_t rrc8(uint8_t a);
    FORCE_INLINE uint8_t rr8(uint8_t a);
    FORCE_INLINE uint8_t sla8(uint8_t a);
    FORCE_INLINE uint8_t sra8(uint8_t a);
    FORCE_INLINE uint8_t srl8(uint8_t a);

    //@}

    /** \name PPU
     
        The PPU rendering happens in dots. One dot corresponds to 1 clock cycle on Original gameboy, or 2 clock cycles on the faster GBC. There are 144 scanlines with 160 pixels per scanline. Each scanline consists of 456 dots, which are distributed as follows:

        - Mode 2: 80 dots are spent in OAM scan (OAM inaccessible)
        - Mode 3: 172-289 dots for drawing pixel data  (OAM and VRAM inaccessible)
        - Mode 0: 87-204 dots for hblank

        The vblank itself is 4560 dots (10 lines), called Mode 1, during which all memory is accessible (same as hblank). 

     */
    //@{

    static constexpr size_t DOTS_PER_LINE = 456;

    void setMode(unsigned mode);
    /** Sets the Y LCD coordinate (currently drawn row)*/
    void setLY(uint8_t value);
    void render(size_t & dots);

    //@}

    /** The interpreter loop. 
     */
    void loop();

    // number of cycles ellapsed since last timed event
    size_t cycles_;

    // interrupts enabled flag (cannot be read, only set by insns)
    bool ime_;

    // when true, the stop instruction terminates the program, useful for debugging & testing
    bool terminateAfterStop_ = false;

}; // GBC