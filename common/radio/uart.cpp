#include "radio.h"
#include "controller.h"

namespace rckid::radio {

    void initialize(DeviceId deviceId) {
        // nothing to do IMO
    }

    void enable(bool silent) {

    }

    void disable() {

    }

    void transmit(DeviceId target, uint8_t const * msg, size_t length) {

    }

    void loop() {
        /*
        while (uart_is_readable(uart0)) {
            char c = uart_getc(uart0);
            LOG((uint32_t)c << ": " << c);
        }
        */
    }

} // rckid::radio