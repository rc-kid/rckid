#include "programmer.h"

using namespace bootloader;
using namespace platform;

#define TRACE(...) if (log_ && verbosity_ >= LOG_TRACE) log_(STR(__VA_ARGS__))
#define VERBOSE(...) if (log_ && verbosity_ >= LOG_VERBOSE) log_(STR(__VA_ARGS__))
#define INFO(...) if (log_ && verbosity_ >= LOG_INFO) log_(STR(__VA_ARGS__))
#define ERROR(...) throw ProgrammerError{STR(__VA_ARGS__)}


namespace {
    /** Displays given uint8_t buffer in hex digits together with size. Useful for printing & comparing byte buffers. 
     */
    std::string printBuffer(uint8_t const * buffer, size_t numBytes) {
        std::stringstream ss;
        for (size_t i = 0; i < numBytes; ++i) 
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)(buffer[i]);
        ss << " (" << std::dec << numBytes << " bytes)" << std::setfill(' ');
        return ss.str();
    }
}

std::ostream & operator << (std::ostream & s, ChipInfo const & info) {
    s << std::setw(30) << "signature: " << printBuffer(& info.state.deviceId0, 3) << std::endl;
    s << std::setw(30) << "mode: " << ((info.state.status & 0x7 == 0x7) ? "bootloader" : "app") << " (" << std::hex << (int) info.state.status << ")" << std::endl;
    s << std::setw(30) << "mcu: " << info.mcu << std::endl;
    s << std::setw(30) << "family: " << info.family << std::endl;
    s << std::setw(30) << "memory: " << std::dec << info.memsize << std::endl;
    s << std::setw(30) << "pagesize: " << info.pagesize << std::endl;
    s << std::setw(30) << "program start: " << "0x" << std::hex <<info.progstart << std::endl;
    s << std::setw(30) << "mapped program offset: " << "0x" << std::hex <<info.mappedProgOffset << std::endl;
    switch (info.family) {
        case ChipInfo::Family::TinySeries1:
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

bool Programmer::resetToBootloader() {
    VERBOSE("  resetting the avr to bootloader" << std::endl);
    // set the IRQ pin low to enter bootloader upon device reset
    gpio::output(avrIrq_);
    gpio::low(avrIrq_);
    uint8_t cmd = CMD_RESET;
    return i2c::transmit(address_, & cmd, 1, nullptr, 0);
}

bool Programmer::resetToApp() {
    VERBOSE("  resetting the avr to app mode" << std::endl);
    gpio::inputPullup(avrIrq_);
    uint8_t cmd = CMD_RESET;
    return i2c::transmit(address_, & cmd, 1, nullptr, 0);
}

ChipInfo Programmer::getChipInfo() {
    sendCommand(bootloader::CMD_INFO, /* wait */ false);
    // explicit delay of 10 ms to ensure that the AVR had plenty of time to finish the command
    cpu::delayMs(10);
    bootloader::State state;
    readBuffer((uint8_t *) & state, sizeof(bootloader::State));
    return ChipInfo{state};
}

void Programmer::enterBootloader() {
    INFO("Entering bootloader..." << std::endl);
    // attempt to SW reset the device
    resetToBootloader();
    // wait for th I2C comms to start
    size_t i = 0;
    while (true) {
        cpu::delayMs(250);
        if (i2c::transmit(address_, nullptr, 0, nullptr, 0))
            break;
        if (i++ == 0) 
            VERBOSE(std::endl << "  device not detected, waiting" << std::flush);
        VERBOSE("." << std::flush);
        if (i > 40) 
            ERROR("Unable to connect to device when entering bootloader mode");
    }
    // set the pin to input pullup so that AVR can pull low when busy
    gpio::inputPullup(avrIrq_);
}

void Programmer::erase() {
    enterBootloader();
    ChipInfo info = getChipInfo();
    size_t memsize = info.memsize - info.progstart;
    VERBOSE("  erasing " << memsize << " bytes" << std::endl);
    std::unique_ptr<uint8_t> mem{new uint8_t[memsize]};
    memset(mem.get(), 0xff, memsize);
    writeProgram(info, mem.get(), info.progstart, info.memsize);
    verifyProgram(info, mem.get(), info.progstart, info.memsize);
}

void Programmer::writeProgram(hex::Program pgm) {
    VERBOSE("Writing program:");
    VERBOSE(pgm << std::endl);
    enterBootloader();
    ChipInfo info = getChipInfo();
    if (info.progstart != pgm.start())
        ERROR("Incompatible program start detected (expected 0x" << std::hex << info.progstart << ", found 0x" << pgm.start() << ")");
    if (info.memsize <= pgm.end())
       ERROR("Program too large (available: " << std::dec << info.memsize << ", required " << pgm.end() << ")");
    VERBOSE("Padding to page size (" << info.pagesize << ")..." << std::endl);
    pgm.padToPage(info.pagesize, 0xff);
    VERBOSE(pgm << std::endl);
    writeProgram(info, pgm.data(), pgm.start(), pgm.end());
    verifyProgram(info, pgm.data(), pgm.start(), pgm.end());
}

void Programmer::setAddress(uint16_t addr) {
    using namespace platform;
    VERBOSE("  setting address to " << std::hex << addr << std::endl);
    uint8_t cmd[3] = { CMD_SET_ADDRESS, (uint8_t)(addr & 0xff),  (uint8_t)((addr >> 8) & 0xff)};
    if (! i2c::transmit(address_, cmd, 3, nullptr, 0))
        throw ProgrammerError{STR("Cannot set address to " << addr)};
    waitForDevice();        
}

uint16_t Programmer::getAddress(uint8_t & err) {
    sendCommand(CMD_INFO);
    State state;
    readBuffer((uint8_t *) & state, sizeof(State));
    err = state.lastError;
    TRACE("Last address: " << std::hex << state.nvmAddress);
    return state.address;
}

void Programmer::writeProgram(ChipInfo const & info, uint8_t const * pgm, uint16_t start, uint16_t end) {
    INFO("Writing " << std::dec << (end - start) << " bytes in " << (end - start) / info.pagesize << " pages" << std::endl);
    INFO("    from " << std::hex << "0x" << start << " to " << std::hex << "0x" << end << std::endl);
    uint16_t address = start;
    INFO(std::setw(5) << std::hex << address << ": " << std::flush);
    setAddress(address + info.mappedProgOffset);
    for (; address < end; address += info.pagesize) {
        setAddress(address + info.mappedProgOffset);
        writeBuffer(pgm + (address - start), info.pagesize);
        INFO("." << std::flush);
        if (!dry_)
            sendCommand(CMD_WRITE_PAGE);
    }
    INFO(std::endl);
}

void Programmer::verifyProgram(ChipInfo & info, uint8_t const * pgm, uint16_t start, uint16_t end) {
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






void Programmer::waitForDevice() {
    if (timeout_ != 0)
        cpu::delayMs(timeout_);
    size_t i = 0;
    while (gpio::read(avrIrq_) == false) {
        if (++i > 1000) {
            cpu::delayMs(1);
            if (i > 2000)
                ERROR("Waiting for command timed out");
        }
    };
}

void Programmer::sendCommand(uint8_t cmd, bool wait) {
    VERBOSE("  cmd: " << (int)cmd << std::endl);
    if (! i2c::transmit(address_, & cmd, 1, nullptr, 0))
        throw ProgrammerError{STR("Cannot send command" << (int)cmd)};
    if (wait)
        waitForDevice();        
}

void Programmer::readBuffer(uint8_t * buffer, size_t numBytes) {
    if (!i2c::transmit(address_, nullptr, 0, buffer, numBytes))
        throw ProgrammerError{STR("Cannot read buffer of length " << numBytes)};
    TRACE("  read: " << printBuffer(buffer, numBytes) << std::endl);
}

void Programmer::writeBuffer(uint8_t const * buffer, size_t numBytes) {
    std::unique_ptr<uint8_t> data{new uint8_t[numBytes + 1]};
    data.get()[0] = CMD_WRITE_BUFFER;
    memcpy(data.get() + 1, buffer, numBytes);
    if (!i2c::transmit(address_, data.get(), numBytes + 1, nullptr, 0))
        throw ProgrammerError{STR("Cannot write buffer of length " << numBytes)};
    TRACE("  write: " << printBuffer(buffer, numBytes) << std::endl);
}