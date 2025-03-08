#pragma once

#include <cstdint>

namespace rckid::gbcemu {

    /** GB(C) Cartridge 
     
        The gamepak is a wrapper around the ROM of the cartridge. Everything else, i.e. RAM, memory controller, or extra hardware is provided by gbcemu. 

        TODO in the future, the gamepak is also responsible for loading the cartridge ROM into RP2350 RAM from various sources, but for now this is simplified by having the entire ROM present in either RAM or ROM.  
     */
    class GamePak {
    public:

        /** Gamepak type. For details see the documentation in gamepak_type.inc.h 
         */
        enum class Type : uint8_t {
    #define CTYPE(ID, NAME) NAME = ID,         
    #include "gamepak_type.inc.h"
        }; // GamePak::Type 

        virtual ~GamePak() = default;

        /** Returns address at which the given 16KB ROM page can be held. 
         
            The gamepak itself is responsible for managing the ROM pages and guarantees that the first page (index 0) is always available, and any other page is available at least until the next call of the getPage function. This corresponds to the two pages always mapped by the gameboy wherethe 0th page is always fixed.
         */
        virtual uint8_t const * getPage(uint32_t page) const = 0;

        /** Returns the type of the loaded cartridge. 
         */
        Type type() const { return static_cast<Type>(getPage(0)[HEADER_CARTRIDGE_TYPE]); }

        /** Returns the size of cartridge's ROM in bytes.
         */
        uint32_t cartridgeROMSize() const { return (1 << getPage(0)[HEADER_ROM_SIZE]) * 32 * 1024; }

        /** Returns the RAM size of the cartridge in bytes. 
         */
        uint32_t cartridgeRAMSize() const { 
            switch (getPage(0)[HEADER_RAM_SIZE]) {
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
        static constexpr uint32_t HEADER_CBG_FLAG = 0x0143;
        static constexpr uint32_t HEADER_CARTRIDGE_TYPE = 0x0147;
        static constexpr uint32_t HEADER_ROM_SIZE = 0x0148;
        static constexpr uint32_t HEADER_RAM_SIZE = 0x0149;
    }; // GamePak

    /** Gamepak that stores the entire ROM in flash. 
     
        This is the simplest gamepak as all the ROM is always available and no resource management is necessary. The gamepak does not need to know the size of the ROM as for any valid cartridges the size can be determined from the contents of the ROM itself.
     */
    class FlashGamePak : public GamePak {
    public:
    
        FlashGamePak(uint8_t const * rom) : rom_(rom){}

        uint8_t const * getPage(uint32_t page) const override {
            return rom_ + page * 16 * 1024;
        }

    private:

        uint8_t const * rom_;
    }; 

} // namespace rckid::gbcemu