#pragma once
#include <cstdlib>
#include <cstdint>

#include "rckid/rckid.h"

/** GameBoy Color Emulator.

 */
class GBC {
public:

    GBC() {}

    ~GBC() {
        delete [] vram_;
        delete [] wram_;
        delete [] eram_;
    }

    void start(uint16_t pc = 0x100) {
        pc_ = pc;
        terminateAfterStop_ = false;
        loop();
    }

    void initializeState() {
        for (size_t i = 0; i < sizeof(rawRegs8_); ++i)
            rawRegs8_[i] = 0;
        for (size_t i = 0; i < 16; ++i)
            memMap_[i] = nullptr;
        delete [] vram_;
        delete [] wram_;
        delete [] eram_;
        vram_ = new uint8_t[VRAM_SIZE];
        wram_ = new uint8_t[WRAM_SIZE]; 
        eram_ = nullptr;
        setVideoRAMBank(0);
        memMap_[MEMMAP_REGION_WRAM] = wram_;
        memMap_[MEMMAP_REGION_ECHO_RAM] = wram_;
        setWorkRAMBank(1);
        // start with interrupts enabled
        ime_ = true;
    }

    /** \name Debugging functions
      
        These methods are intended for running tests and observing GBC's state. 
     */
    //@{

    /** Runs a test and returns when the `stop` instruction with any argument is encountered. 
     */
    void runTest(uint8_t const * rom, size_t numBytes, uint16_t pc = 0x0) {
        initializeState();
        // set the rom & set bank to 0
        rom_ = rom;
        romSize_ = numBytes;
        for (size_t i = 0; i < 4; ++i)
            memMap_[i] = const_cast<uint8_t *>(rom_ + MEMMAP_REGION_SIZE * i);
        setROMBank(1);
        // set pc, enable stop termination and run the emulator loop
        pc_ = pc;
        cycles_ = 0;
        terminateAfterStop_ = true;
        loop();
    }
    /** Number of cycles the emulator executed since last timed event. 
     */
    size_t cyclesElapsed() const { return cycles_; }

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

    bool flagZ() const { return rawRegs8_[REG_INDEX_F] & FLAG_Z; }
    bool flagN() const { return rawRegs8_[REG_INDEX_F] & FLAG_N; }
    bool flagH() const { return rawRegs8_[REG_INDEX_F] & FLAG_H; }
    bool flagC() const { return rawRegs8_[REG_INDEX_F] & FLAG_C; }

    uint8_t const * const * memMap() const { return memMap_; }
    uint8_t const * vram() const { return vram_; }
    uint8_t const * wram() const { return wram_; }

    uint8_t readWRAM(size_t index) { return wram_[index]; }
    uint8_t readVRAM(size_t index) { return vram_[index]; }


    //@}
private:

    /** \name CPU Registers
     
        Flags

        - `Z` (zero flag) set only if the actual result is 0
        - `N` (subtract flag) set to a constant it seems
        - `H` (half carry flag) 
        - `C` (carry flag)
     */
    //@{
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

    union {
        // a f, b, c, d, e, h, l    
        uint8_t rawRegs8_[8];
        // af, bc, de, hl
        uint16_t rawRegs16_[4];
    }; 

    size_t pc_; // this really is a 16bit number only
    size_t sp_; 

    void setFlagZ(bool value) { value ? rawRegs8_[REG_INDEX_F] |= FLAG_Z : rawRegs8_[REG_INDEX_F] &= ~FLAG_Z; }

    void setFlagN(bool value) { value ? rawRegs8_[REG_INDEX_F] |= FLAG_N : rawRegs8_[REG_INDEX_F] &= ~FLAG_N; }

    void setFlagH(bool value) { value ? rawRegs8_[REG_INDEX_F] |= FLAG_H : rawRegs8_[REG_INDEX_F] &= ~FLAG_H; }

    void setFlagC(bool value) { value ? rawRegs8_[REG_INDEX_F] |= FLAG_C : rawRegs8_[REG_INDEX_F] &= ~FLAG_C; }

    uint8_t setFlagZFrom(uint8_t value) {
        setFlagZ(value == 0);
        return value;
    }

    //@}

    /** \name Memory (RAM & ROM)
     
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

        The internal memory, in case
     */
    //@{

    static constexpr size_t VRAM_SIZE = 16 * 1024;
    static constexpr size_t WRAM_SIZE = 32 * 1024;

    static constexpr size_t ROM_BANK_SIZE = 16 * 1024;
    static constexpr size_t VRAM_BANK_SIZE = 8 * 1024;

    static constexpr size_t MEMMAP_REGION_SIZE = 4 * 1024;
    static constexpr size_t MEMMAP_REGION_ROM = 4;
    static constexpr size_t MEMMAP_REGION_WRAM = 12;
    static constexpr size_t MEMMAP_REGION_VRAM = 8;
    static constexpr size_t MEMMAP_REGION_ECHO_RAM = 14;

    uint8_t * memMap_[16];
    uint8_t const * rom_;
    size_t romSize_;
    uint8_t * vram_ = nullptr;
    uint8_t * wram_ = nullptr;
    uint8_t * eram_ = nullptr;
    size_t eramSize_;

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
    void setExternalRAMBank(size_t index) {
    }

    uint8_t read8(uint16_t address) {
        if (address < 0xfe00) {
            return memMap_[address >> 12][address & 0xfff];
        }
        UNIMPLEMENTED;
        return 0;
    }

    uint16_t read16(size_t address) {
        return read8(address) | read8(address + 1) * 256;
    }

    void write8(uint16_t address, uint8_t value) {
        // TODO order according to the most likely outcomes
        if (address < 0x8000) { // rom
            // do nothing 
            // TODO this actually changes the rom bank & eram bank via cartridge mapper
        } else if (address >= 0x8000 && address < 0xa000) { // vram
            memMap_[address >> 12][address & 0xfff] = value;
        } else if (address >= 0xa000 && address < 0xc000) { // eram
            UNIMPLEMENTED; 
        } else if (address < 0xfe00) { // wram & echo ram
            memMap_[address >> 12][address & 0xfff] = value;
        } else {
            UNIMPLEMENTED;
        }
    }

    void write16(size_t address, uint16_t value) {
        write8(address, value & 0xff);
        write8(address + 1, value >> 8);
    }

    /** Faster 8bit read with address increment. Does not work on IO registers, but very useful for PC. 
     */
    uint8_t rd8(size_t & address) {
        uint8_t result = memMap_[address >> 12][address & 0xfff];
        ++address;
        return result;
    }

    /** Faster 16bit read with address increment. Does not work on IO registers, but very useful for PC. 
     */
    uint16_t rd16(size_t & address) {
        uint16_t result = * reinterpret_cast<uint16_t*>(memMap_[address >> 12] + (address & 0xfff));
        address += 2;
        return result;
    }

    //@}

    /** \name IO Registers 
     */
    //@{

    //@}

    /** \name Arithmetics
     */
    //@{

    uint8_t inc8(uint8_t x) {
        ++x;
        setFlagZ(x == 0);
        setFlagH((x & 0xf) == 0);
        return x;
    }

    uint8_t dec8(uint8_t x) {
        --x;
        setFlagZ(x == 0);
        setFlagH((x & 0xf) == 0xf);
        return x;
    }

    /** Adds two 8bit numbers, optinally including a carry flag and sets the Z, H and C flags accordingly. 

        The Z flag is set if the 8bit result is 0. The C flag is set if the result is greater than 256. Finally, the H flag is set if during 
    */
    uint8_t add8(uint8_t a, uint8_t b, uint8_t c = 0) {
        unsigned r = a + b + c;
        setFlagZ((r & 0xff) == 0);
        setFlagH(((a ^ b ^ r) & 0x10) != 0);
        setFlagC(r > 0xff);
        return static_cast<uint8_t>(r);
    }

    uint8_t sub8(uint8_t a, uint8_t b, uint8_t c = 0) {
        unsigned r = a - (b + c);
        setFlagZ((r & 0xff) == 0);
        setFlagH(((a ^ b ^ r) & 0x10) != 0);
        setFlagC(r > 0xff);
        return static_cast<uint8_t>(r);
    }

    uint16_t add16(uint16_t a, uint16_t b) {
        uint32_t r = a + b;
        setFlagC(r > 0xffff);
        setFlagH(((r ^ a ^ b) & 0x1000) != 0);
        return static_cast<uint16_t>(r);
    }

    /** Rotate left, set carry
     */
    uint8_t rlc8(uint8_t a) {
        uint16_t r = a << 1;
        setFlagC(r & 256);
        r = r | flagC();
        return (r & 0xff);
    }

    /** Rotate left through carry. 
     */
    uint8_t rl8(uint8_t a) {
        uint16_t r = a << 1;
        r = r | flagC();
        setFlagC(r & 256);
        return (r & 0xff);
    }

    /** Rotate right, set carry. 
     */
    uint8_t rrc8(uint8_t a) {
        setFlagC(a & 1);
        a = a >> 1;
        a = a | (flagC() ? 128 : 0);
        return a;
    }

    /** Rotate right, through carry. 
     */
    uint8_t rr8(uint8_t a) {
        bool cf = flagC();
        setFlagC(a & 1);
        a = a >> 1;
        a = a | (cf ? 128 : 0);
        return a;
    }

    /** Shift left, overflow to carry.
     */
    uint8_t sla8(uint8_t a) {
        uint16_t r = a * 2;
        setFlagC(r & 256);
        return r & 0xff;
    }

    /** Shift right, arithmetically, i.e. keep msb intact*/
    uint8_t sra8(uint8_t a) {
        setFlagC(a & 1);
        a = a >> 1;
        a |= (a & 64) ? 128 : 0;
        return a;
    }

    /** Shift right, logically, i.e.msb set to 0. 
     */
    uint8_t srl8(uint8_t a) {
        setFlagC(a & 1);
        a = a >> 1;
        return a;
    }

    //@}


    void loop();



    // number of cycles ellapsed since last timed event
    size_t cycles_;

    // interrupts enabled flag (cannot be read, only set by insns)
    bool ime_;

    // when true, the stop instruction terminates the program, useful for debugging & testing
    bool terminateAfterStop_ = false;


}; // GBC