#pragma once

class cpu {
public:
    static void delayUs(unsigned value) {
        std::this_thread::sleep_for(std::chrono::microseconds(value));        
    }

    static void delayMs(unsigned value) {
        std::this_thread::sleep_for(std::chrono::milliseconds(value));        
    }

    //static void sleep() {}

}; // cpu
