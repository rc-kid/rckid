#pragma once

class cpu {
public:
    static void delayUs(unsigned value) {
        sleep_us(value);  
    }

    static void delayMs(unsigned value) {
        sleep_ms(value);  
    }

    static void sleep() {
        // TODO TODO TODO 
    }

}; // cpu

