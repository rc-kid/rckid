#if (defined LIBRCKID_MOCK)

#include <iostream>

#include "rckid.h"
#include "app.h"


namespace rckid {

    void initialize() {
        // TODO initialize the mock display & friends
    }

    void yield() {
        // TODO do we do anything here actually? 
    }

    void start(BaseApp && app) {
        int errorCode = setjmp(Device::fatalError_);
        if (errorCode != 0) 
            Device::BSOD(errorCode);
        app.run();
    }

    Writer writeToUSBSerial() {
        return Writer([](char x) {
            std::cout << x;
            if (x == '\n')
                std::cout << std::flush;
        });
    }

    void powerOff() {
        // TODO exit the app
        UNIMPLEMENTED;
    }

    void cpuOverclock(unsigned hz, bool overvolt) {
        // does nothing, overclocking is ignored in mock mode 
    }

    void Device::tick() {
        // TODO get state
        // UNIMPLEMENTED;
    }

    void Device::BSOD(int code) {
        // TODO show BSOD in window?
        std::cout << "BSOD:" << std::endl << std::endl;
        std::cout << "If you ran this on RCKid, you would have been treated to its blue screen of death." << std::endl;
        std::cout << "Error code: " << code << std::endl;
        std::cout << "File:       " << fatalErrorFile_ << ":" << fatalErrorLine_ << std::endl;
        exit(EXIT_FAILURE);
    }

    size_t Stats::freeHeap() {
        // TODO actually implement
        UNIMPLEMENTED;
        return 0;
    }

} // namespace rckid

#endif // RCKID_MOCK