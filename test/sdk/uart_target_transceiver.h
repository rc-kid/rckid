#pragma once

#include <rckid/comms/uart_transceiver.h>

namespace rckid {

    class UARTTarget {};     

    template<>
    inline void Transceiver<UARTTarget>::initialize() {
        // don't do anything, reset by the device
    }

    template<>
    inline bool Transceiver<UARTTarget>::enableHardware() {
        // nothing to do for FantasyUART & always succeeds
        return true;
    }

    template<>
    inline void Transceiver<UARTTarget>::disableHardware() {
        // nothing to do for FantasyUART
    }

    template<>
    inline void Transceiver<UARTTarget>::loop() {
        Packet p;
        while (FantasyUART::targetRx(p)) {
            // TODO if the message received requires ACK, send one immediately
            onMessageReceived(p);
        }
    }

    template<>
    inline bool Transceiver<UARTTarget>::transmit([[maybe_unused]] DeviceId target, Packet const & packet) {
        FantasyUART::targetTx(packet);
        return true;
    }

}