#pragma once
#include <cstdlib>
#include <cstdint>

#include "rckid/rckid.h"

/** GameBoy Color Emulator.

 */
class GBC {
public:

    GBC() {}

    void setROM(uint8_t const * rom, size_t numBytes) {
        rom_ = rom;
        romSize_ = numBytes;
        memMap_[0] = const_cast<uint8_t *>(rom_);
        setROMBank(0);
    }

    void start(uint16_t pc = 0x100) {
        pc_ = pc;
        terminateAfterStop_ = false;
        loop();
    }

    void startTest(uint16_t pc = 0x0) {
        pc_ = pc;
        terminateAfterStop_ = true;
        loop();
    }


    uint16_t pc() const { return pc_; }

private:

    /** \name CPU Registers and state
     
        Flags

        - `Z` (zero flag) set only if the actual result is 0
        - `N` (subtract flag) set to a constant it seems
        - `H` (half carry flag) 
        - `C` (carry flag)
     */
    //@{

    union {
        // a f, b, c, d, e, h, l    
        uint8_t rawRegs8_[8];
        // af, bc, de, hl
        uint16_t rawRegs16_[4];
    }; 

    size_t pc_; // this really is a 16bit number only
    size_t sp_; 

    void setFlagZ(bool value) {

    }

    void setFlagN(bool value) {

    }

    void setFlagH(bool value) {

    }

    void setFlagC(bool value) {

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

        0x0000 | 0x3fff | 16KB  | ROM Bank 00, from cartridge, usually fixed bank
        0x4000 | 0x7fff | 16KB  | ROM bank 01-NN, from cartridge, switchable by mapper chip if any
        0x8000 | 0x9fff |  8KB  | Video RAM
        0xa000 | 0xbfff |  8KB  | External RAM
        0xc000 | 0xcfff |  4KB  | WRAM bank 0
        0xd000 | 0xdfff |  4KB  | WRAM bank 1..7
        0xe000 | 0xfdff |  <8KB | Echo ram, mirror of 0xc000..0xddff, prohibited
        0xfe00 | 0xfe9f |  160  | OAM memory
        0xfea0 | 0xfeff |       | Prohibited
        0xff00 | 0xff7f | 128   | IO Registers
        0xff80 | 0xfffe | 127   | High RAM 
        0xffff | 0xffff | 1     | Interrupt enable register        

        The internal memory, in case
     */
    //@{


    uint8_t * memMap_[16];
    uint8_t const * rom_;
    size_t romSize_;


    void setWorkRAMBank(size_t index) {
    }


    void setVideoRAMBank(size_t index) {
    }

    // depends on memory controller in cartridge
    void setROMBank(size_t bank) {
    }

    // depends on memory controller in cartridge
    void setExternalRAMBank(size_t index) {
    }

    uint8_t read8(size_t address) {
        if (address < 0xfe00) {
            return memMap_[address >> 12][address & 0xfff];
        }
        return 0;
    }

    uint16_t read16(size_t address) {
        return 0;
    }

    void write8(size_t address, uint8_t value) {
        UNIMPLEMENTED;
    }

    void write16(size_t address, uint16_t value) {
        UNIMPLEMENTED;
    }

    uint8_t rd8(size_t & address) {
        uint8_t result = memMap_[address >> 12][address & 0xfff];
        ++address;
        return result;
    }

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


    void loop();


    // when true, the stop instruction terminates the program, useful for debugging & testing
    bool terminateAfterStop_ = false;

}; // GBC