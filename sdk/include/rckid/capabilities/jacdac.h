#pragma once

#include <rckid/rckid.h>
#include <rckid/task.h>
#include <rckid/string.h>
#include <rckid/ui/header.h>

namespace rckid {

    // forward declaration of the internal implementation class for friendship purposes
    class JacdacImpl;
    namespace jacdac {

        /** Jacdac packet. 
         
            As packets are always part of a frame, the do not own their payload, but the payload comes immediately after the packet header in the frame.
         */
        struct Packet {
        public:
           uint8_t size;
           uint8_t index;
           uint16_t command;
           uint8_t payload[];
        } __attribute__((packed, aligned(4))); // Jacdac::Packet

        /** Jacdac frame that is sent over the SWS wire.
         */
        struct Frame {
        public:
            uint16_t crc;
            uint8_t size;
            uint8_t flags;
            uint64_t device_identifier;
            uint8_t data[240]; // maximum

            /** Calculates CRC of thhe frame (crc16_ccitt.
             */
            uint16_t calculateCrc() const { 
                uint16_t crc = 0xffff;
                uint32_t len = size + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint64_t);
                uint8_t const * data = reinterpret_cast<uint8_t const *>(this) + 2; // skip the crc field itself

                for (uint32_t i = 0; i < len; i++) {
                    crc ^= (uint16_t)data[i] << 8;
                    for (int bit = 0; bit < 8; bit++) {
                        if (crc & 0x8000)
                            crc = (crc << 1) ^ 0x1021;
                        else
                            crc <<= 1;
                        crc &= 0xffff;
                    }
                }

                return crc;
            }

            void updateCrc() { crc = calculateCrc(); }

            bool checkCrc() const { return crc == calculateCrc(); }

            bool isCommand() const { return (flags & FLAG_COMMAND) != 0; }
            bool requiresAck() const { return (flags & FLAG_ACK) != 0; }

            static constexpr uint8_t FLAG_COMMAND = 1 << 0;
            static constexpr uint8_t FLAG_ACK = 1 << 1;

        } __attribute__((packed, aligned(4))); // Jacdac::Frame

    } // namespace rckid::jacdac

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

        void enable() { doEnable(); }
        void disable() { doDisable(); }

    protected:

        friend class JacdacImpl;

        void onTick() override;

        void releaseResources() override {
            // TODO Maybe do delete this eventually
            //delete this;
        }

        void doEnable();

        void doDisable();

        /** Transmits the raw data using SWS.
         
            The method assumes the data already form correct JACDAC frame.
         */
        bool doSend(uint8_t const * data, uint32_t numBytes);

        void doReceive();

    public:
        // TODO reporting counters, to be deleted when more mature
        static inline volatile uint32_t receivedPackets = 0;
        static inline volatile uint32_t receivedBytes = 0;

    }; // rckid::Jacdac

} // namespace rckid