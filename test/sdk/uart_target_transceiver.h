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
            if (p[0] == static_cast<uint8_t>(msg::Ack::ID)) {
                onAckReceived(true);    
            } else {
                if (msg::requiresAck(static_cast<msg::Id>(p[0])))
                    send(BroadcastId, msg::Ack{});
                onMessageReceived(p);
            }
        }
    }

    template<>
    inline bool Transceiver<UARTTarget>::transmit([[maybe_unused]] DeviceId target, Packet const & packet) {
        FantasyUART::targetTx(packet);
        return true;
    }

    class UARTTargetTransceiver : public Transceiver<UARTTarget> {
    public:
        UARTTargetTransceiver(DeviceId ownId): Transceiver<UARTTarget>{ownId} {}
    protected:
        void onMessageReceived([[maybe_unused]] Packet const & msg) override {}
    };
}