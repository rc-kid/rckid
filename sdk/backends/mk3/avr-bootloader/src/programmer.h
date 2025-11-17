#pragma once

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <functional>

#include "platform/platform.h"
#include "utils/utils.h"
#include "utils/intelhex.h"

#include "bootloader_config.h"


class ProgrammerError : public std::runtime_error {
public:
    ProgrammerError(std::string const & what): std::runtime_error{what} {}
}; // ProgrammerError


/** List of chips supported by the bootloader. 
 
    Each chip has a MCU name, signature, family identifier, memory size and page size specified.  
*/
#define SUPPORTED_CHIPS \
    CHIP(ATTiny1614, 0x1e9422, TinySeries1, 16384, 64) \
    CHIP(ATTiny1616, 0x1e9421, TinySeries1, 16384, 64) \
    CHIP(ATTiny1617, 0x1e9420, TinySeries1, 16384, 64) \
    CHIP(ATTiny3216, 0x1e9521, TinySeries1, 32768, 128)

/** Structure that holds the information about the attached chip. 
 
    This info comes either from the chip itself (such as fuses, clock control and address & last command), or is calculated from the detected part and SUPPORTED_CHIPS configuration. 
*/
struct ChipInfo {
public:
    enum class Family {
        TinySeries1,
    }; // Family

    enum class MCU {
        #define CHIP(ID, ...) ID, 
        SUPPORTED_CHIPS
        #undef CHIP
    }; // Chip

    Family family;
    MCU mcu;
    uint16_t memsize;
    uint16_t pagesize;
    uint16_t progstart;
    uint16_t mappedProgOffset;
    bootloader::State state;

    ChipInfo(bootloader::State state):
        state{state} {
        uint32_t signature = state.deviceId0 << 16 | state.deviceId1 << 8 | state.deviceId2;
        switch (signature) {
            #define CHIP(ID, SIG, FAMILY, MEMSIZE, PAGESIZE) case SIG: mcu = MCU::ID; family = Family::FAMILY; memsize = MEMSIZE; pagesize = PAGESIZE; break;
            SUPPORTED_CHIPS
            #undef CHIP
            default:
                throw ProgrammerError{"Unknown device signature"};
        }
        switch (family) {
            case Family::TinySeries1:
                progstart = state.fuses[8] * 256;
                mappedProgOffset = 0x8000;
                break;
        }
    }

    friend std::ostream & operator << (std::ostream & s, Family family) {
        switch (family) {
            case Family::TinySeries1:
                s << "TinySeries1";
                break;
        }
        return s;
    }

    friend std::ostream & operator << (std::ostream & s, MCU mcu) {
        switch (mcu) {
            #define CHIP(ID, ...) case MCU::ID: s << # ID; break;
            SUPPORTED_CHIPS
            #undef CHIP
        }
        return s;
    }

    friend std::ostream & operator << (std::ostream & s, ChipInfo const & info);
}; // ChipInfo    

class Programmer {
public:

    static constexpr size_t LOG_TRACE = 2;
    static constexpr size_t LOG_VERBOSE = 1;
    static constexpr size_t LOG_INFO = 0;

    Programmer(uint8_t address, platform::gpio::Pin irq): address_{address}, avrIrq_{irq} {}
    Programmer(uint8_t address, platform::gpio::Pin irq, std::function<void(std::string const&)> f): address_{address}, avrIrq_{irq}, log_{f} {}

    void setLogLevel(size_t value) { verbosity_ = value; }
    void setTimeout(size_t value) { timeout_ = value; }
    void setDryRun(bool value) { dry_ = value; }
    uint8_t i2cAddress() const { return address_; }
    void setI2CAddress(uint8_t value) { address_ = value; }
    void setLog(std::function<void(std::string const &)> f) { log_ = f; }

    /** Sets the conditions for entering the bootloader mode and sends the reset command. 
     
        Special method since reset should not wait for the AVR_IRQ busy flag (this would lead in an infinite loop when rebooting to the program)
    */
    bool resetToBootloader();

    /** Sets the conditions for entering the app and sends the reset command. 
     */
    bool resetToApp();

    /** Enters the bootloader mode.
     
        This is a bit trickier since we can't HW reset the device, nor can we assume that there is a valid application that will respon to the SW reset case.
    */
    void enterBootloader();

    /** Switches the AVR chip to the bootloader mode and returns the chip info. 
     
        The command should work in both bootloader and app mode and will always use a 10ms timer instead of IRQ. 
     */
    ChipInfo getChipInfo();

    /** Performs a chip erase operation. 
     */
    void erase();

    /** Programs the chip with provided hex file.
     */
    void writeProgram(hex::Program program);

private:   


    void setAddress(uint16_t addr);

    uint16_t getAddress(uint8_t & err);


    void writeProgram(ChipInfo const & info, uint8_t const * pgm, uint16_t start, uint16_t end);

    void verifyProgram(ChipInfo & info, uint8_t const * pgm, uint16_t start, uint16_t end);

    void waitForDevice();

    /** Sends the given command to the AVR. Second argument indicates whether the method will also wait for the busy flag to be cleared by the AVR after the command is completed. 
     */
    void sendCommand(uint8_t cmd, bool wait = true);
    void readBuffer(uint8_t * buffer, size_t numBytes);
    void writeBuffer(uint8_t const * buffer, size_t numBytes);

    uint8_t address_;
    platform::gpio::Pin avrIrq_;
    size_t timeout_ = 0;
    bool dry_ = false;

    std::function<void(std::string const &)> log_;
    size_t verbosity_ = LOG_INFO;

    friend std::ostream & operator << (std::ostream & s, Programmer const & p) {
        s << "I2C Addess: " << (int)p.address_ << std::endl;
        s << "IRQ pin:    " << p.avrIrq_ << std::endl;
        s << "Timeout:    " << p.timeout_ << std::endl;
        s << "Dry:        " << p.dry_ << std::endl;
        s << "Verbosity:  " << p.verbosity_ << std::endl;
        return s;
    }

}; // Programmer


