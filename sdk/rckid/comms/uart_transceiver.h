#pragma once

#if defined (ARCH_FANTASY)
#include <deque>
#include <mutex>
#endif

#include "transceiver.h"

namespace rckid {

    /** Marker class for the UART hardware on RP2040 */
    class UART {};

#if defined (ARCH_FANTASY)
    /** UART mockup for the fantasy console. 
     
        This is a simple UART backend that allows sending and receiving messages based on the 
     */
    class FantasyUART {
    public:

        static void reset() {
            deviceTx_.clear();
            deviceRx_.clear();
        }

        static void deviceTx(Packet const & packet) {
            deviceTx_.push_back(Pkt{packet});
        }

        static bool deviceRx(Packet & buffer) {
            if (deviceRx_.empty())
                return false;
            memcpy(& buffer, deviceRx_.front().bytes, sizeof(Packet));
            deviceRx_.pop_front();
            return true;
        }

        static void targetTx(Packet const & packet) {
            deviceRx_.push_back(Pkt{packet});
        }

        static bool targetRx(Packet & buffer) {
            if (deviceTx_.empty())
                return false;
            memcpy(& buffer, deviceTx_.front().bytes, sizeof(Packet));
            deviceTx_.pop_front();
            return true;
        }

        // TODO add mutex

    private:

        struct Pkt {
            uint8_t bytes[sizeof(Packet)];
            Pkt(Packet const & p) {
                memcpy(bytes, &p, sizeof(Packet));
            }
        };

        static inline std::deque<Pkt> deviceTx_; 
        static inline std::deque<Pkt> deviceRx_;
    };

#endif

    template<>
    inline void Transceiver<UART>::initialize() {
#if defined (ARCH_FANTASY)
        FantasyUART::reset();
#else
        // TODO
        UNIMPLEMENTED;
#endif
    }

    template<>
    inline bool Transceiver<UART>::enableHardware() {
#if defined (ARCH_FANTASY)
        return true; // in fantasy world, this always succeeds
#else
        // TODO
        UNIMPLEMENTED;
#endif
    }

    template<>
    inline void Transceiver<UART>::disableHardware() {
#if defined (ARCH_FANTASY)
        // nothing to do for FantasyUART
#else
        // TODO
        UNIMPLEMENTED;
#endif
    }

    template<>
    inline void Transceiver<UART>::loop() {
#if defined (ARCH_FANTASY)
        Packet p;
        while (FantasyUART::deviceRx(p)) {
            // TODO if the message received requires ACK, send one immediately
            onMessageReceived(p);
        }
#else
        // TODO
        UNIMPLEMENTED;
#endif
    }

    template<>
    inline bool Transceiver<UART>::transmit([[maybe_unused]] DeviceId target, Packet const & packet) {
#if defined (ARCH_FANTASY)
        FantasyUART::deviceTx(packet);
        return true;
#else
        // TODO
        UNIMPLEMENTED;
#endif
    }

} // namespace rckid