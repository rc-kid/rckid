#pragma once

#include <rckid/rckid.h>
#include <rckid/task.h>
#include <rckid/string.h>
#include <rckid/ui/header.h>

namespace rckid {

    /** Jacdac communication capability.

        Jacdac protocol description here: https://jacdac.github.io/jacdac-docs/reference/protocol/

        ## Transmission

        Data transmission is *always* at 1MHz. First send what is an UART BREAK condition, which is a 11-15uS low pulse. After this we haveto wait at least 50uS before sending first byte. This is to ensure the devices can detect the BREAK condition and react to it properly by setting themselves to receive mode listeners. If no data is sent within 200uS, it is bus error. When data is sent another BREAK condition must be sent to indicate end of packet. This must happen within 80uS of sending the last byte. 

        A transmission can only start at least 100uS and some random backoff value after the stop break condition. 




        Very primitive implementation of JACDAC SWS protocol to send and receive packets. On the network layer all JACDAC has to do is send and receive packets. We might get fancier later on, but for now packet transmit is blocking and packet receive is handled by the driver where the packet is stored into an internal queue. The application is expected to check the queue periodically and process the messages accordingly.

        TODO or just do a rx callback for now and be done with it? simpler for demoing 

        NOTE this is work in progress and early demnonstration at best.
     */
    class Jacdac : public Task {
    public:

        static Jacdac * instance();

        std::optional<std::pair<TileIcon, uint8_t>> headerIcon() const override {
            // TODO
            return std::nullopt;
        }

        /** Sends JACDAC packet.
         */
        void sendPacket(uint8_t const * data, uint32_t numBytes);

    protected:

        /** Jacdac frame that is sent over the SWS wire.
         */
        struct Frame {
            uint16_t crc;
            uint8_t size;
            uint8_t flags;
            uint64_t device_identifier;
            uint8_t data[240]; // maximum
        } __attribute__((packed, aligned(4))); // Jacdac::Frame

        void onTick() override;

        void releaseResources() override {
            // don't really
            //delete this;
        }

        void doEnable();

        void doDisable();

        /** Transmits the raw data using SWS.
         
            The method assumes the data already form correct JACDAC frame.
         */
        bool doSend(uint8_t const * data, uint32_t numBytes);

        void doReceive(uint8_t * data, uint32_t maxBytes, void (*callback)(uint8_t const * data, uint32_t numBytes));

    }; // rckid::Jacdac

} // namespace rckid