#pragma once

#include <rckid/app.h>

#include "gamepak.h"

namespace rckid::gbcemu {

    /** GameBoy Color Emulator App
     
        The emulator is an rckid app that consists of the following components organized closely to the original hardware:

        - the cpu, including registers
        - memory (video ram, work ram, cartridge rom, cartridge ram) including bank switching 
        - ppu (pixel processing unit)
        - apu (audio processing unit)


        Most of this builds on a very good GB/GBC reference available at https://gbdev.io/pandocs/About.html
     */
    class GBCEmu : public App {
    public:

        GBCEmu(Allocator & a = Heap::allocator());

        ~GBCEmu() override;

        void run() override;

        void save([[maybe_unused]] WriteStream & into) override {
            UNIMPLEMENTED;
        }

        void load([[maybe_unused]] ReadStream & from) override {
            UNIMPLEMENTED;
        }

        /** Laods the given gamepak in the emulator. 
         */
        void loadCartridge(GamePak * game);

        /** Test interface
         */

        bool terminateAfterStop() const { return terminateAfterStop_; }
        void setTerminateAfterStop(bool value) { terminateAfterStop_ = value; }

        uint32_t elapsedCycles() const { return cycles_ + totalCycles_; }

        uint8_t a() const { return regs8_[REG_INDEX_A]; }
        uint8_t b() const { return regs8_[REG_INDEX_B]; }
        uint8_t c() const { return regs8_[REG_INDEX_C]; }
        uint8_t d() const { return regs8_[REG_INDEX_D]; }
        uint8_t e() const { return regs8_[REG_INDEX_E]; }
        uint8_t h() const { return regs8_[REG_INDEX_H]; }
        uint8_t l() const { return regs8_[REG_INDEX_L]; }
        uint8_t f() const { return regs8_[REG_INDEX_F]; }

        uint16_t af() const { return regs16_[REG_INDEX_AF]; }
        uint16_t bc() const { return regs16_[REG_INDEX_BC]; }
        uint16_t de() const { return regs16_[REG_INDEX_DE]; }
        uint16_t hl() const { return regs16_[REG_INDEX_HL]; }

        uint16_t pc() const { return pc_; }
        uint16_t sp() const { return sp_; }

        uint8_t readWRam(uint32_t offset) {
            uint32_t page = offset >> 12;
            uint32_t pageOffset = offset & 0xfff;
            return wram_[page][pageOffset];
        }

        uint8_t readOam(uint32_t offset) const {
            return oam_[offset];
        }

        /** Reads the high RAM at given offset. 
         */
        uint8_t readHRam(uint32_t offset) const {
            // note that we keep hram an io regs as a single array, so need to adjust here for the actual hram start inside hram
            return hram_[offset + 128];
        }

        /** Disassembles the given section on memory as assembly instructions. 
         */
        void disassemble(uint16_t start, uint16_t end);

    protected:

        void focus() override;
        void blur() override;
        void update() override;
        void draw() override;

    private:

        /** \name Arithmetic helpers
         
            Helper arithmetic functions called from the instruction implementations in insns.inc.h.
         */
        //@{

        FORCE_INLINE(uint8_t inc8(uint8_t x));
        FORCE_INLINE(uint8_t dec8(uint8_t x));
        FORCE_INLINE(uint8_t add8(uint8_t a, uint8_t b, uint8_t c = 0));
        FORCE_INLINE(uint8_t sub8(uint8_t a, uint8_t b, uint8_t c = 0));
        FORCE_INLINE(uint16_t add16(uint16_t a, uint16_t b));
        FORCE_INLINE(uint8_t rlc8(uint8_t a));
        FORCE_INLINE(uint8_t rl8(uint8_t a));
        FORCE_INLINE(uint8_t rrc8(uint8_t a));
        FORCE_INLINE(uint8_t rr8(uint8_t a));
        FORCE_INLINE(uint8_t sla8(uint8_t a));
        FORCE_INLINE(uint8_t sra8(uint8_t a));
        FORCE_INLINE(uint8_t srl8(uint8_t a));

        //@}

        /** The following 8 bit registers are available:

            - A (accumulator)
            - F (flags), only few bits available as the ZNHC flags. 
            - B, C, D, E, H, L general purpose 8bit registers
        
            The AF, BC, DE and HL registers are 16bit wide and share the same space with their 8bit counterparts.

            Dedicated 16bit registers are:

            - SP (stack pointer)
            - PC (program counter)
         */
        static constexpr uint32_t REG_INDEX_A = 1;
        static constexpr uint32_t REG_INDEX_F = 0;
        static constexpr uint32_t REG_INDEX_B = 3;
        static constexpr uint32_t REG_INDEX_C = 2;
        static constexpr uint32_t REG_INDEX_D = 5;
        static constexpr uint32_t REG_INDEX_E = 4;
        static constexpr uint32_t REG_INDEX_H = 7;
        static constexpr uint32_t REG_INDEX_L = 6;

        static constexpr uint32_t REG_INDEX_AF = 0;
        static constexpr uint32_t REG_INDEX_BC = 1;
        static constexpr uint32_t REG_INDEX_DE = 2;
        static constexpr uint32_t REG_INDEX_HL = 3;

        union {
            uint8_t regs8_[8];
            uint16_t regs16_[4];
        }; 

        uint16_t sp_;
        uint16_t pc_;

        /** Flags. 
         
            - `Z` (zero flag) set only if the actual result is 0
            - `N` (subtract flag) set to a constant it seems
            - `H` (half carry flag) 
            - `C` (carry flag)
         */
        static constexpr uint8_t FLAG_Z = 1 << 7;
        static constexpr uint8_t FLAG_N = 1 << 6;
        static constexpr uint8_t FLAG_H = 1 << 5;
        static constexpr uint8_t FLAG_C = 1 << 4;   

        bool flagZ() const { return regs8_[REG_INDEX_F] & FLAG_Z; }
        bool flagN() const { return regs8_[REG_INDEX_F] & FLAG_N; }
        bool flagH() const { return regs8_[REG_INDEX_F] & FLAG_H; }
        bool flagC() const { return regs8_[REG_INDEX_F] & FLAG_C; }

        void setFlagZ(bool value) { value ? regs8_[REG_INDEX_F] |= FLAG_Z : regs8_[REG_INDEX_F] &= ~FLAG_Z; }

        void setFlagN(bool value) { value ? regs8_[REG_INDEX_F] |= FLAG_N : regs8_[REG_INDEX_F] &= ~FLAG_N; }

        void setFlagH(bool value) { value ? regs8_[REG_INDEX_F] |= FLAG_H : regs8_[REG_INDEX_F] &= ~FLAG_H; }

        void setFlagC(bool value) { value ? regs8_[REG_INDEX_F] |= FLAG_C : regs8_[REG_INDEX_F] &= ~FLAG_C; }

        /** Memory.
         
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
            
            The memory is divided into 16 4KB blocks, with the last block being a bit more complex as it contains echo ram, oam memory, io regs and hram. Furthemore, all allocations of RAM memories are done in 4KB blocks as well to limit fragmentation due to possibly large allocatons in case of big memories. 
         */

        /** Reads one byte from memory. For most reads & writes this simply goes through the memmap, but there are special cases for the shorter memory blocks towards the end of the address space.
         */
        uint8_t memRd8(uint16_t addr);

        /** Reads a 16bit value. 
         */
        uint16_t memRd16(uint16_t addr);

        void memWr8(uint16_t addr, uint8_t value);
        void memWr16(uint16_t addr, uint16_t value);

        /** Sets the second ROM page to the given address. The address must point to a consecutive 16KB bytes long array. 
         */
        void setRomPage(uint32_t page);

        /** Sets the video ram page. Video ram pages are 8 KB and two of them are available for GBC. 
         */
        void setVideoRamPage(uint32_t page);

        /** Sets external ram bank. Valid values are from 0 to 16, external ram banks are 8KB in size.
         */
        void setExternalRamPage(uint32_t page); 

        /** Sets the second work ram page (first is always 0). GBC has 8 work ram banks, banks 1 to 7 can be mapped here.
         */
        void setWorkRamPage(uint32_t page);


        /** Faster memory reads for program counter where we assume the address does not belong to the 16th bank with oam and high mem. 
         */
        uint8_t mem8(uint16_t addr);
        uint16_t mem16(uint16_t addr);

        uint8_t * vram_[4];
        uint8_t * wram_[8];
        uint8_t * oam_ = nullptr;
        uint8_t * hram_ = nullptr;

        // eram of up to 128kb (32 banks of 4kb)
        uint8_t * eram_[32] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

        // memory mapping information. For fast access, the memory is divided into 16 4kb regions with pointers to beginning in the array. This is true for all but the last block, which is a bit more complex as it contains echo ram, oam memory, io regs and hram.
        uint8_t * memMap_[16];

        /** \name PPU
     
            The PPU rendering happens in dots. One dot corresponds to 1 clock cycle on Original gameboy, or 2 clock cycles on the faster GBC. There are 144 scanlines with 160 pixels per scanline. Each scanline consists of 456 dots, which are distributed as follows:

            - Mode 2: 80 dots are spent in OAM scan (OAM inaccessible)
            - Mode 3: 172-289 dots for drawing pixel data  (OAM and VRAM inaccessible)
            - Mode 0: 87-204 dots for hblank

            The vblank itself is 4560 dots (10 lines), called Mode 1, during which all memory is accessible (same as hblank). 
         */
        //@{

        /** Object Attribute Memory for a single sprite.
         
            Each sprite contains its x and y values, tile number and flags (priority, palettes, flips, etc.). Important note is that the sprite y coordinate is offset by 16 pixels, so the actual sprite y position is y + 16.
         */
        PACKED(struct OAMSprite {
            uint8_t y;
            uint8_t x;
            uint8_t tile;
            uint8_t flags;

            bool priority() const { return flags & 0x80; }
            bool yFlip() const { return flags & 0x40; }
            bool xFlip() const { return flags & 0x20; }
            uint8_t palette() const { return flags & 0x10; }
            uint8_t bank() const { return flags & 0x08; }
            uint8_t cgbPalette() const { return flags & 0x07; }
        });

        static_assert(sizeof(OAMSprite) == 4);

        /** Dots per scanline. There are 154 scanlines, 144 of which are visible on the display. 
         */
        static constexpr uint32_t DOTS_PER_LINE = 456;

        void setMode(unsigned mode);
        
        /** Sets the Y LCD coordinate (currently drawn row)
         */
        void setLY(uint8_t value);

        /** Graphics rendering for a single line.
         */
        void renderLine();
        //@}

        /** \name CPU 
         */
        void runCPU(); 

        // Current gamepak
        GamePak * gamepak_ = nullptr;

        // cycles elapsed since last display update
        uint32_t cycles_ = 0;

        uint32_t totalCycles_ = 0;

        // interrupts enabled flag (cannot be read, only set by insns)
        // TODO is this necessary or can I just use the last high mem byte?
        bool ime_ = false;

        // when true, the stop instruction terminates the program, useful for debugging & testing
        bool terminateAfterStop_ = false;

        DoubleBuffer<uint16_t> pixels_;

    }; // rckid::gbcemu::GBCEmu


} // namespace rckid::gbcemu