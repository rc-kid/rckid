#pragma once

#include <platform.h>
#include "utils/writer.h"

inline Writer writeToSerial() {
    return Writer{[](char x) {
        Serial.write(&x, 1);
        //if (x == '\n')
        //    tud_cdc_write_flush();            
    }};
}

#define LOG(...) ::writeToSerial() << __VA_ARGS__ << "\n"