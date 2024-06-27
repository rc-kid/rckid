#pragma once

#include "definitions.h"
#include "messages.h"
#include "connection.h"

namespace rckid::radio {

    /** Radio controller. 



        TODO 

        Name clashes - when a ping is received with same id, it means there is device with the same id, ouch ouch. We can renumber ourselves (unless there are connections pending, etc. Or we can cancel the connections and renumber)

        Actually have a ring buffer of messages to send, or even a queue? That way we can be flexible with retransmits, etc. And the memory cost is small - 32 bytes per message     

     */
    class Controller {
    public:

        /** Initializes the radio HW. 
         */
        static void initialize(DeviceId deviceId);

        /** Enables the radio in receiver mode.
         
            When enabled, the radio will periodically transmit ping messages, unless the silent option is enabled. 
         */
        static void enable(bool silent = false);

        /** Disables the radio. 
         
            Puts the associated hardware to sleep. 
         */
        static void disable();

        /** Sends given message. 
         */
        template<typename T>
        static void send(T const & msg) { sendMessageRaw(reinterpret_cast<uint8_t const *>(&msg), sizeof(T)); }

    protected:

        virtual void onConnectionOpen(msg::ConnectionOpen const & request) {
            // TODO default behavior is to send the connection reject message
            send(msg::ConnectionReject{request, Error::ConnectionUnsupported});
        };

        virtual void onBroadcastStart(msg::BroadcastStart const & request) {}

        /** Message received event. 
         */
        void messageReceived(uint8_t const * msg, DeviceId sender) {
            switch (static_cast<msg::Id>(msg[0])) {
                case msg::ConnectionOpen::ID:
                    return onConnectionOpen(msg::ConnectionOpen::fromBuffer(msg));
                case msg::ConnectionAccept::ID:
                case msg::ConnectionReject::ID:
                case msg::ConnectionClose::ID:
                case msg::ConnectionSend::ID:
                case msg::ConnectionSend30::ID:
                   // TODO do something
                   break;
                // TODO broadcasts - do we want sth? maybe deduplicate & things

            }
        }

    private:

        /** Sends raw data. */
        static void sendMessageRaw(uint8_t const * msg, size_t length);

    }; 


} // namespace rckid::radio