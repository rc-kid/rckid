#pragma once

#include <cstdint>

/** GB(C) Cartridge 
 */
class GamePak {
public:

    enum class CartridgeType : uint8_t {
#define CTYPE(ID, NAME) NAME = ID,         
#include "cartridge_type.inc.h"
    }; // GamePak::CartridgeType 




    /** Returns the type of the loaded cartridge. */
    CartridgeType cartridgeType() const { return static_cast<CartridgeType>(rom_[HEADER_CARTRIDGE_TYPE]); }

    /** Returns the size of cartridge's ROM in bytes. */
    size_t cartridgeROMSize() const { return (1 << rom_[HEADER_ROM_SIZE]) * 32 * 1024; }

    /** Returns the RAM size of the cartridge in bytes. */
    size_t cartridgeRAMSize() const { 
        switch rom_[HEADER_RAM_SIZE] {
            case 0x02:
                return 8 * 1024;
            case 0x03:
                return 32 * 1024;
            case 0x04:
                return 128 * 1024;
            case 0x05:
                return 64 * 1024;
            default:
                return 0;
        }
    }

private:
    static constexpr size_t HEADER_CBG_FLAG = 0x0143;
    static constexpr size_t HEADER_CARTRIDGE_TYPE = 0x0147;
    static constexpr size_t HEADER_ROM_SIZE = 0x0148;
    static constexpr size_t HEADER_RAM_SIZE = 0x0149;


    uint8_t const * rom_;
}; // GamePak