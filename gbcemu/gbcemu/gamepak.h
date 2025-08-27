#pragma once

#include <cstdint>

#ifdef RCKID_BACKEND_FANTASY
#include <fstream>
#endif

#include <rckid/utils/string.h>
#include <rckid/utils/stream.h>

namespace rckid::gbcemu {

    /** Memory bank controller types. 
     
        0000 RAM Enable
        1fff
        2000 ROM Bank (0x01 - 0x1f)
        3fff 
        4000 RAM Bank or ROM 
        5fff
     */
    enum class MBC {
        None, 
        Type1,
        Type2,
        Type3,
        Type5, 
        Type6,
        Type7,
        MMM01,
        M161,
        HuC1,
        HuC3,
        Other,
    }; 

    /** GB(C) Cartridge 
     
        The gamepak is a wrapper around the ROM of the cartridge. Everything else, i.e. RAM, memory controller, or extra hardware is provided by gbcemu. The GamePak provides the basic interface while the actual implementation is provided by the subclasses below. 
     */
    class GamePak {
    public:

        /** Gamepak type. For details see the documentation in gamepak_type.inc.h 
         */
        enum class Type : uint8_t {
    #define CTYPE(ID, NAME, MBC_NAME) NAME = ID,         
    #include "gamepak_type.inc.h"
        }; // GamePak::Type 

        virtual ~GamePak() = default;

        /** Returns address at which the given 16KB ROM page can be held. 
         
            The gamepak itself is responsible for managing the ROM pages and guarantees that the first page (index 0) is always available, and any other page is available at least until the next call of the getPage function. This corresponds to the two pages always mapped by the gameboy wherethe 0th page is always fixed.
         */
        uint8_t const * getPage(uint32_t page) const {
            if (page == 0)
                return doGetPage(0);
            else
                return doGetPage(page & (cartridgeROMPages() - 1));
        }

        /** Returns the type of the loaded cartridge. 
         */
        Type type() const { return static_cast<Type>(getPage(0)[HEADER_CARTRIDGE_TYPE]); }

        /** Returns the size of cartridge's ROM in bytes.
         */
        uint32_t cartridgeROMSize() const { return (1 << getPage(0)[HEADER_ROM_SIZE]) * 32 * 1024; }

        /** Returns the number of ROM pages available in the cartridge. 
         
            This could only be 2,4,8,16,32,64,128,256 or 512 for sizes total sizes from 32kb to 8Mb.
         */
        uint32_t cartridgeROMPages() const { return (2 << getPage(0)[HEADER_ROM_SIZE]); }

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

        /** Returns the MBC used in the cartridge.
         */
        MBC cartridgeMBC() {
            switch (getPage(0)[HEADER_CARTRIDGE_TYPE]) {
            #define CTYPE(ID, NAME, MBC_TYPE) case ID: return MBC_TYPE;
            #include "gamepak_type.inc.h"
                default:
                    return MBC::Other;
            }
        }

    protected:
        static constexpr uint32_t PAGE_SIZE = 16 * 1024;

        virtual uint8_t const * doGetPage(uint32_t page) const = 0;

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

    protected:

        uint8_t const * doGetPage(uint32_t page) const override {
            return rom_ + page * PAGE_SIZE;
        }

    private:

        uint8_t const * rom_;
    }; 

    /** Gamepak with ROM page caching. 
     
        As the RCKid RAM size is much smaller than the max size of GBC cartridges, the cached gamepak employs a simple LRU caching scheme to keep the most recently used pages in RAM (arena). 
     */
    template<typename T>
    class CachedGamePak : public GamePak {
    public:

        CachedGamePak(T && s):
            s_{std::move(s)} {
        }

        ~CachedGamePak() override {
            PageInfo * p = cache_;
            while (p != nullptr) {
                PageInfo * next = p->last;
                delete p;
                p = next;
            }
            delete page0_;
        }

    protected:

        struct PageInfo {
            static constexpr uint32_t EMPTY = 0xffffffff;

            uint8_t * buffer;
            PageInfo * last = nullptr;
            uint32_t page = EMPTY;

            PageInfo(uint8_t * buffer):
                buffer{buffer} {
            }

            ~PageInfo() {
                delete [] buffer;
            }

            void detach() {
                last = nullptr;
            }

            void reset() {
                page = EMPTY;
                detach();
            }

        };

        uint8_t const * doGetPage(uint32_t page) const override {
            // page0 must always be available and hence is removed from the caching
            if (page == 0) {
                if (page0_ == nullptr) {
                    page0_ = new PageInfo{new uint8_t[PAGE_SIZE]};
                    fetchPage(0, page0_);
                }
                return page0_->buffer;
            // rest is LRU cached for as long as there is memory available
            } else {
                PageInfo * p = getPage(page);
                if (p->page != page)
                    fetchPage(page, p);
                p->last = cache_;
                cache_ = p;
                return p->buffer;
            }
        }

        PageInfo * getPage(uint32_t page) const {
            PageInfo * p = cache_;
            PageInfo * prev = nullptr;
            while (p != nullptr) {
                // if we found the page, we need to remove it from the cache for the LRU to work
                if (p->page == page) {
                    if (prev != nullptr)
                        prev->last = p->last;
                    else
                        cache_ = p->last;
                    p->detach();
                    return p;
                }
                if (p->last == nullptr)
                    break;
                prev = p;
                p = p->last;
            }
            // haven't found the page, try creating one first
            uint8_t * buffer = new uint8_t[PAGE_SIZE];
            if (buffer != nullptr) {
                p = new PageInfo{buffer};
                return p;
            }
            prev->last = nullptr;
            p->reset();
            return p;
        }

        void fetchPage(uint32_t page, PageInfo * p) const {
            uint32_t offset = page * PAGE_SIZE;
            s_.seek(offset);
            s_.read(p->buffer, PAGE_SIZE);
            p->page = page;
        }

    private:

        mutable PageInfo * cache_ = nullptr;
        mutable PageInfo * page0_ = nullptr;

        mutable T s_;
    }; 

#ifdef RCKID_BACKEND_FANTASY

    /** Convenience gamepak for testing in fantasy mode where entire file can be loaded to memory and then used similarly to a flash gamepak. Not available on the device because of rather small RAM size not allowing full cartridge to fit.
     */
    class FileGamePak : public GamePak {
    public:

        FileGamePak(String const & filename);

        ~FileGamePak() override {
            delete [] rom_;
        }

    protected:
            
        uint8_t const * doGetPage(uint32_t page) const override {
            return rom_ + page * PAGE_SIZE;
        }
    
    private:
        uint8_t * rom_;
    }; // rckid::gbcemu::FileGamePak
    #endif

} // namespace rckid::gbcemu