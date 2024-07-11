#include "radio.h"

namespace rckid::radio {

    DeviceId id_;

    DeviceId id() { return id_; }

    void initialize(DeviceId deviceId) {
        // nothing to do IMO
        id_ = deviceId;
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
