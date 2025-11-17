#include "platform/platform.h"
#include "utils/utils.h"
#include "utils/intelhex.h"

#define I2C_ADDRESS 0x43
//#define PIN_AVR_IRQ 25


#include "programmer.h"

using namespace platform;

static std::string command;
static std::string hexFile;

/** \name Commands
 */


/** Simply checks if there is an I2C device at the given address. 
 */
void check(Programmer & p) {
    if (!i2c::transmit(p.i2cAddress(), nullptr, 0, nullptr, 0))
        throw std::runtime_error{STR("Device not detected at I2C address " << p.i2cAddress())};
    std::cout << "Device detected at I2C address " << p.i2cAddress() << std::endl;
}


/** Actually attempts to connect to the device, and obtain the chip information, which is available in both app and bootloader mode. 
*/
void info(Programmer & p) {
    ChipInfo info{p.getChipInfo()};
    std::cout << std::endl << "Device Info:" << std::endl;
    std::cout << info << std::endl;
}

/** Restarts the device in bootloader mode and writes the given program. 
 */
void write(Programmer & p) {
    hex::Program pgm{hex::Program::parseFile(hexFile.c_str())};
    p.writeProgram(pgm);
}

void erase(Programmer & p) {
    p.erase();
}

void reset(Programmer & p) {
    p.resetToApp();
}

void help() {
    std::cout << "I2C Bootloader for AVR ATTiny series 1 chips" << std::endl << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "    i2c-programmer COMMAND [ HEX_FILE] { OPTIONS }" << std::endl << std::endl;
    std::cout << "Available commands:" << std::endl << std::endl;
    std::cout << "    help - prints this help" << std::endl;
    std::cout << "    check - checks if the AVR is present" << std::endl;
    std::cout << "    info - displays the chip info obtained from the AVR" << std::endl;
    std::cout << "    write FILE - flashes the given hex file to chip's memory" << std::endl;
    //std::cout << "    read FILE - reads the entire chip memory into given hex file" << std::endl;
    std::cout << "    verify FILE - verifies that the memory contains the given hex file" << std::endl;
    std::cout << "    erase - erases the application memory of the chip" << std::endl;
    std::cout << "    reset - resets the AVR chip into the app mode" << std::endl;
    std::cout << "Options" << std::endl << std::endl;
    std::cout << "    --verbose - enables verbose oiutput" << std::endl;
    // can be used without the IRQ pin, but the AVR has to leave the IRQ pin alone
    std::cout << "    --timeout - instead of relying on the IRQ pin, waits 10ms after each command" << std::endl;
    std::cout << "    --trace - enables even more verbose outputs (namely all read & written buffers)" << std::endl;
    std::cout << "    --dry - enables dry run, i.e. no pages will be written" << std::endl;
}



Programmer parseArguments(int argc, char * argv[]) {
    Programmer result{I2C_ADDRESS, RPI_PIN_AVR_IRQ, [](std::string const & str) { std::cout << str << std::flush; }};
    try {
        int i = 1;
        if (i > argc)
            throw STR("No command specified!");
        command = argv[i++];
        if (command == "write" || command == "read" || command == "verify") {
            if (i >= argc)
                throw STR("No hex file specified");
            hexFile = argv[i++];
        }
        while (i < argc) {
            std::string arg{argv[i++]};
            if (arg == "--verbose")
                result.setLogLevel(Programmer::LOG_VERBOSE);
            else if (arg == "--timeout")
                result.setTimeout(10);
            else if (str::startsWith(arg, "--timeout="))
                result.setTimeout(static_cast<size_t>(std::atoi(arg.c_str() + 10)));
            else if (arg == "--trace")
                result.setLogLevel(Programmer::LOG_TRACE);
            else if (arg == "--dry")
                result.setDryRun(true);
            else if (str::startsWith(arg, "--addr=")) 
                result.setI2CAddress(static_cast<uint8_t>(std::atoi(arg.c_str() + 7)));
            else 
                throw STR("Invalid argument " << arg);
        }
        return result;
    } catch (std::string const & e) {
        std::cout << "ERROR: Invalid command line usage: " << e << std::endl;
        help();
        throw;
    }
}



int main(int argc, char * argv[]) {
    try {
        // initialize gpio and i2c
        gpio::initialize();
        i2c::initializeMaster();
        Programmer p{parseArguments(argc, argv)};
        std::cout << "Programmer configuration:" << std::endl << p << std::endl;
        if (command == "help")
            help();
        else if (command == "check")
            check(p);
        else if (command == "info")
            info(p);
        else if (command == "write")
            write(p);
        else if (command == "verify") {
            p.setDryRun(true); // force dry run for the verification only
            write(p);
        } else if (command == "erase")
            erase(p);
        else if (command == "reset")
            reset(p);
        else
            throw std::runtime_error{STR("Invalid command " << command)};
        std::cout << std::endl << "ALL DONE" << std::endl;
    } catch (hex::Error & e) {
        std::cout << std::endl << "ERROR in parsing the HEX file: " << e << std::endl;
    } catch (std::exception const & e) {
        std::cout << std::endl << "ERROR: " << e.what() << std::endl;
    } catch (...) {
        std::cout << std::endl << "UNKNOWN ERROR. This should not happen" << std::endl;
    }
    return EXIT_FAILURE;
}



#ifdef FOO



#include <cstring>
#include <iostream>
#include <iomanip>
#include <unordered_map>

#include "platform/platform.h"
#include "utils/utils.h"
#include "utils/intelhex.h"


#include "config.h"

using namespace platform;

static uint8_t i2cAddress = 0x43; 
static bool verbose = false;
static bool trace = false;
static bool dry = false;
static unsigned timeout = 0;
static std::string command;
static std::string hexFile;

/** Prints macro. 
 */
#define INFO(...) std::cout << __VA_ARGS__
#define VERBOSE(...) if (verbose) std::cout << __VA_ARGS__
#define TRACE(...) if (trace) std::cout << __VA_ARGS__

#define ERROR(...) throw std::runtime_error{STR(__VA_ARGS__)}


/** List of chips supported by the bootloader. 
 
    Each chip has a MCU name, signature, family identifier, memory size and page size specified.  
*/
#define SUPPORTED_CHIPS \
    CHIP(ATTiny1614, 0x1e9422, TinySeries1, 16384, 64) \
    CHIP(ATTiny1616, 0x1e9421, TinySeries1, 16384, 64) \
    CHIP(ATTiny1617, 0x1e9420, TinySeries1, 16384, 64) \
    CHIP(ATTiny3216, 0x1e9521, TinySeries1, 32768, 128)



/** Displays given uint8_t buffer in hex digits together with size. Useful for printing & comparing byte buffers. 
 */
std::string printBuffer(uint8_t const * buffer, size_t numBytes) {
    std::stringstream ss;
    for (size_t i = 0; i < numBytes; ++i) 
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)(buffer[i]);
    ss << " (" << std::dec << numBytes << " bytes)" << std::setfill(' ');
    return ss.str();
}

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
    State state;

    ChipInfo(State state):
        state{state} {
        uint32_t signature = state.deviceId0 << 16 | state.deviceId1 << 8 | state.deviceId2;
        switch (signature) {
            #define CHIP(ID, SIG, FAMILY, MEMSIZE, PAGESIZE) case SIG: mcu = MCU::ID; family = Family::FAMILY; memsize = MEMSIZE; pagesize = PAGESIZE; break;
            SUPPORTED_CHIPS
            #undef CHIP
            default:
                ERROR("Unknown device signature");
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

    friend std::ostream & operator << (std::ostream & s, ChipInfo const & info) {
        s << std::setw(30) << "signature: " << printBuffer(& info.state.deviceId0, 3) << std::endl;
        s << std::setw(30) << "mode: " << ((info.state.status & 0x7 == 0x7) ? "bootloader" : "app") << std::endl;
        s << std::setw(30) << "mcu: " << info.mcu << std::endl;
        s << std::setw(30) << "family: " << info.family << std::endl;
        s << std::setw(30) << "memory: " << std::dec << info.memsize << std::endl;
        s << std::setw(30) << "pagesize: " << info.pagesize << std::endl;
        s << std::setw(30) << "program start: " << "0x" << std::hex <<info.progstart << std::endl;
        s << std::setw(30) << "mapped program offset: " << "0x" << std::hex <<info.mappedProgOffset << std::endl;
        switch (info.family) {
            case Family::TinySeries1:
                s << std::setw(30) << "FUSE.WDTCFG: " << std::hex << (int)info.state.fuses[0] << std::endl;
                s << std::setw(30) << "FUSE.BODCFG: " << std::hex << (int)info.state.fuses[1] << std::endl;
                s << std::setw(30) << "FUSE.OSCCFG: " << std::hex << (int)info.state.fuses[2] << std::endl;
                s << std::setw(30) << "FUSE.TCD0CFG: " << std::hex << (int)info.state.fuses[4] << std::endl;
                s << std::setw(30) << "FUSE.SYSCFG0: " << std::hex << (int)info.state.fuses[5] << std::endl;
                s << std::setw(30) << "FUSE.SYSCFG1: " << std::hex << (int)info.state.fuses[6] << std::endl;
                s << std::setw(30) << "FUSE.APPEND: " << std::hex << (int)info.state.fuses[7] << std::endl;
                s << std::setw(30) << "FUSE.BOOTEND: " << std::hex << (int)info.state.fuses[8] << std::endl;
                s << std::setw(30) << "FUSE.LOCKBIT: " << std::hex << (int)info.state.fuses[10] << std::endl;
                s << std::setw(30) << "CLKCTRL.MCLKCTRLA: " << std::hex << (int)info.state.mclkCtrlA << std::endl;
                s << std::setw(30) << "CLKCTRL.MCLKCTRLB: " << std::hex << (int)info.state.mclkCtrlB << std::endl;
                s << std::setw(30) << "CLKCTRL.MCLKLOCK: " << std::hex << (int)info.state.mclkLock << std::endl;
                s << std::setw(30) << "CLKCTRL.MCLKSTATUS: " << std::hex << (int)info.state.mclkStatus << std::endl;
                break;
        }
        s << std::setw(30) << "address: " << std::hex << info.state.address << std::endl;
        s << std::setw(30) << "nvm last address: " << std::hex << info.state.nvmAddress << std::endl;
        s << std::dec;
        return s;
    }

}; // ChipInfo    

void waitForDevice() {
    if (timeout != 0)
        cpu::delayMs(timeout);
    size_t i = 0;
    while (gpio::read(PIN_AVR_IRQ) == false) {
        if (++i > 1000) {
            cpu::delayMs(1);
            if (i > 2000)
                ERROR("Waiting for command timed out");
        }
    };
}

void sendCommand(uint8_t cmd) {
    VERBOSE("  cmd: " << (int)cmd << std::endl);
    if (! i2c::transmit(I2C_ADDRESS, & cmd, 1, nullptr, 0))
        ERROR("Cannot send command" << (int)cmd);
    waitForDevice();        
}

void readBuffer(uint8_t * buffer, size_t numBytes) {
    if (!i2c::transmit(I2C_ADDRESS, nullptr, 0, buffer, numBytes))
       ERROR("Cannot read buffer of length " << numBytes);
    TRACE("  read: " << printBuffer(buffer, numBytes) << std::endl);
}

void writeBuffer(uint8_t const * buffer, size_t numBytes) {
    std::unique_ptr<uint8_t> data{new uint8_t[numBytes + 1]};
    data.get()[0] = CMD_WRITE_BUFFER;
    memcpy(data.get() + 1, buffer, numBytes);
    if (!i2c::transmit(I2C_ADDRESS, data.get(), numBytes + 1, nullptr, 0))
       ERROR("Cannot write buffer of length " << numBytes);
    TRACE("  write: " << printBuffer(buffer, numBytes) << std::endl);
}

/** Sends the reset command. 
 
    Special method since reset should not wait for the AVR_IRQ busy flag (this would lead in an infinite loop when rebooting to the program)
*/
bool resetAvr() {
    VERBOSE("  resetting the avr" << std::endl);
    uint8_t cmd = CMD_RESET;
    return i2c::transmit(I2C_ADDRESS, & cmd, 1, nullptr, 0);
}

void setAddress(uint16_t addr) {
    VERBOSE("  setting address to " << std::hex << addr << std::endl);
    uint8_t cmd[3] = { CMD_SET_ADDRESS, (uint8_t)(addr & 0xff),  (uint8_t)((addr >> 8) & 0xff)};
    if (! i2c::transmit(I2C_ADDRESS, cmd, 3, nullptr, 0))
        ERROR("Cannot set address to " << addr);
    waitForDevice();        
}

uint16_t getAddress(uint8_t & err) {
    sendCommand(CMD_INFO);
    State state;
    readBuffer((uint8_t *) & state, sizeof(State));
    err = state.lastError;
    TRACE("Last address: " << std::hex << state.nvmAddress);
    return state.address;
}

/** Enters the bootloader mode.
 
    This is a bit trickier since we can't HW reset the device, nor can we assume that there is a valid application that will respon to the SW reset case.
 */
void enterBootloader() {
    INFO("Entering bootloader..." << std::endl);
    // set the IRQ pin low to enter bootloader upon device reset
    gpio::output(PIN_AVR_IRQ);
    gpio::low(PIN_AVR_IRQ);
    // attempt to SW reset the device
    resetAvr();
    // wait for th I2C comms to start
    size_t i = 0;
    while (true) {
        cpu::delayMs(250);
        if (i2c::transmit(I2C_ADDRESS, nullptr, 0, nullptr, 0))
            break;
        if (i++ == 0) 
            VERBOSE(std::endl << "  device not detected, waiting" << std::flush);
        VERBOSE("." << std::flush);
        if (i > 40) 
            ERROR("Unable to connect to device when entering bootloader mode");
    }
    // set the pin to input pullup so that AVR can pull low when busy
    gpio::inputPullup(PIN_AVR_IRQ);
}

ChipInfo getChipInfo() {
    enterBootloader();
    sendCommand(CMD_INFO);
    State state;
    readBuffer((uint8_t *) & state, sizeof(State));
    return ChipInfo{state};
}

void writeProgram(ChipInfo const & info, uint8_t const * pgm, uint16_t start, uint16_t end) {
    INFO("Writing " << std::dec << (end - start) << " bytes in " << (end - start) / info.pagesize << " pages" << std::endl);
    INFO("    from " << std::hex << "0x" << start << " to " << std::hex << "0x" << end << std::endl);
    uint16_t address = start;
    INFO(std::setw(5) << std::hex << address << ": " << std::flush);
    setAddress(address + info.mappedProgOffset);
    for (; address < end; address += info.pagesize) {
        setAddress(address + info.mappedProgOffset);
        writeBuffer(pgm + (address - start), info.pagesize);
        INFO("." << std::flush);
        if (!dry)
            sendCommand(CMD_WRITE_PAGE);
    }
    INFO(std::endl);
}

void verifyProgram(ChipInfo & info, uint8_t const * pgm, uint16_t start, uint16_t end) {
    INFO("Verifying " << std::dec << (end - start) << " bytes in " << (end - start) / info.pagesize << " pages" << std::endl);
    INFO("    from " << std::hex << "0x" << start << " to " << std::hex << "0x" << end << std::endl);
    std::unique_ptr<uint8_t> buffer{new uint8_t[info.pagesize]};
    uint16_t address = start;
    INFO(std::setw(5) << std::hex << address << ": " << std::flush);
    for (; address < end; address += info.pagesize) {
        setAddress(address + info.mappedProgOffset);
        readBuffer(buffer.get(), info.pagesize);
        std::string expected = printBuffer(pgm + address - start, info.pagesize);
        std::string actual = printBuffer(buffer.get(), info.pagesize);
        if (expected != actual) {
            INFO("Error at address block " << std::hex << address << ": " << std::endl);
            INFO("  expected: " << expected << std::endl);
            INFO("  actual:   " << actual << std::endl);
            ERROR("Program verification failure");
        }
        INFO("." << std::flush);
    }
    INFO(std::endl);
}


/** \name Commands
 */


/** Simply checks if there is an I2C device at the given address. 
 */
void check() {
    if (!i2c::transmit(I2C_ADDRESS, nullptr, 0, nullptr, 0))
        ERROR("Device not detected at I2C address " << I2C_ADDRESS);
    INFO("Device detected at I2C address " << I2C_ADDRESS << std::endl);
}


/** Actually attempts to connect to the device, restarts it into bootloader and displays the information. 
*/
void info(ChipInfo & info) {
    INFO(std::endl << "Device Info:" << std::endl);
    INFO(info << std::endl);
}


void write(ChipInfo & info, bool actuallyWrite) {
    VERBOSE("Reading hex file in " << hexFile << std::endl);
    hex::Program pgm{hex::Program::parseFile(hexFile.c_str())};
    VERBOSE(pgm << std::endl);
    if (info.progstart != pgm.start())
        ERROR("Incompatible program start detected (expected 0x" << std::hex << info.progstart << ", found 0x" << pgm.start() << ")");
    if (info.memsize <= pgm.end())
       ERROR("Program too large (available: " << std::dec << info.memsize << ", required " << pgm.end() << ")");
    VERBOSE("Padding to page size (" << info.pagesize << ")..." << std::endl);
    pgm.padToPage(info.pagesize, 0xff);
    VERBOSE(pgm << std::endl);
    if (actuallyWrite)
        writeProgram(info, pgm.data(), pgm.start(), pgm.end());
    verifyProgram(info, pgm.data(), pgm.start(), pgm.end());
}

void read(ChipInfo & info) {
    uint16_t addr = 0;
    while (addr < 16384) {
        setAddress(addr + info.mappedProgOffset);
        uint8_t buffer[64];
        readBuffer(buffer, 64);
        addr += 64;
    }
}

void erase(ChipInfo & info) {
    INFO("Erasing all memory after bootloader..." << std::endl);
    size_t memsize = info.memsize - info.progstart;
    VERBOSE("  erasing " << memsize << " bytes" << std::endl);
    std::unique_ptr<uint8_t> mem{new uint8_t[memsize]};
    memset(mem.get(), 0xff, memsize);
    writeProgram(info, mem.get(), info.progstart, info.memsize);
    verifyProgram(info, mem.get(), info.progstart, info.memsize);
}

void reset() {
    gpio::inputPullup(PIN_AVR_IRQ);
}

void keepalive(ChipInfo & info) {
    INFO("Entering bootloader and stayiling indefinitely..." << std::endl);
    while (true) {
        setAddress(0x8000);
        cpu::delayMs(200);
    }
}

void help() {
    std::cout << "I2C Bootloader for AVR ATTiny series 1 chips" << std::endl << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "    i2c-programmer COMMAND [ HEX_FILE] { OPTIONS }" << std::endl << std::endl;
    std::cout << "Available commands:" << std::endl << std::endl;
    std::cout << "    help - prints this help" << std::endl;
    std::cout << "    check - checks if the AVR is present" << std::endl;
    std::cout << "    info - resets the AVR to bootloader and displays the chip info" << std::endl;
    std::cout << "    write FILE - flashes the given hex file to chip's memory" << std::endl;
    std::cout << "    read FILE - reads the entire chip memory into given hex file" << std::endl;
    std::cout << "    verify FILE - verifies that the memory contains the given hex file" << std::endl;
    std::cout << "    erase - erases the application memory of the chip" << std::endl;
    std::cout << "    reset - resets the AVR chip into the app mode" << std::endl;
    std::cout << "    keepalive - keeps the AVR in the bootloader state indefinitely" << std::endl;
    std::cout << "Options" << std::endl << std::endl;
    std::cout << "    --verbose - enables verbose oiutput" << std::endl;
    std::cout << "    --timeout - instead of relying on the IRQ pin, waits 10ms after each command" << std::endl;
    std::cout << "    --trace - enables even more verbose outputs (namely all read & written buffers)" << std::endl;
    std::cout << "    --dry - enables dry run, i.e. no pages will be written" << std::endl;
}



void parseArguments(int argc, char * argv[]) {
    try {
        int i = 1;
        if (i > argc)
            throw STR("No command specified!");
        command = argv[i++];
        if (command == "write" || command == "read" || command == "verify") {
            if (i >= argc)
                throw STR("No hex file specified");
            hexFile = argv[i++];
        }
        while (i < argc) {
            std::string arg{argv[i++]};
            if (arg == "--verbose")
                verbose = true;
            else if (arg == "--timeout")
                timeout = 10;
            else if (str::startsWith(arg, "--timeout="))
                timeout = static_cast<unsigned int>(std::atoi(arg.c_str() + 10));
            else if (arg == "--trace")
                trace = true;
            else if (arg == "--dry")
                dry = true;
            else if (str::startsWith(arg, "--addr=")) 
                i2cAddress = static_cast<uint8_t>(std::atoi(arg.c_str() + 7));
            else 
                throw STR("Invalid argument " << arg);
        }
    } catch (std::string const & e) {
        std::cout << "ERROR: Invalid command line usage: " << e << std::endl;
        help();
        throw;
    }
}



int main(int argc, char * argv[]) {
    try {
        // initialize gpio and i2c
        gpio::initialize();
        i2c::initializeMaster();
        parseArguments(argc, argv);
        if (command == "help")
            help();
        else if (command == "check")
            check();
        else {
            ChipInfo ci{getChipInfo()};
            if (command == "info")
                info(ci);
            else if (command == "write")
                write(ci, true);
            else if (command == "read")
                read(ci);
            else if (command == "verify")
                write(ci, false);
            else if (command == "erase")
                erase(ci);
            else if (command == "keepalive")
                keepalive(ci);
            else
                ERROR("Invalid command " << command);
        }
        INFO(std::endl << "ALL DONE" << std::endl);
        return EXIT_SUCCESS;
    } catch (hex::Error & e) {
        std::cout << std::endl << "ERROR in parsing the HEX file: " << e << std::endl;
    } catch (std::exception const & e) {
        std::cout << std::endl << "ERROR: " << e.what() << std::endl;
    } catch (...) {
        std::cout << std::endl << "UNKNOWN ERROR. This should not happen" << std::endl;
    }
    return EXIT_FAILURE;
}



#endif