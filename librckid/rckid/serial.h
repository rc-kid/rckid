#pragma once

#include <stdio.h>
#include <hardware/uart.h>
#include <pico/stdlib.h>

namespace rckid {

    /** Serial port interface for RCKid allowing for printf statements and somewhat easier debugging. 
     
        To use the serial port on Raspberry Pi, start minicom with the following arguments:

        minicom -b 115200 -o -D /dev/ttyAMA0
     */
    class Serial {
    public:
        static void initialize() {
            stdio_uart_init_full(uart1, 115200, 20, 21);
        }

    }; // rckid::Serial



} // namespace rckid