#pragma once

#include "definitions.h"
#include "messages.h"
#include "connection.h"

namespace rckid::radio {

    /** Radio controller. 



        TODO 

        Name clashes - when a ping is received with same id, it means there is device with the same id, ouch ouch. We can renumber ourselves (unless there are connections pending, etc. Or we can cancel the connections and renumber)
     
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

    protected:

        /** Sends given message. 
         */
        template<typename T>
        void send(T const & msg) {
            send(reinterpret_cast<uint8_t const *>(&msg), sizeof(T));
        }

        void send(uint8_t const * msg, size_t length);

        virtual void onConnectionOpen(msg::ConnectionOpen const & request) {
            // TODO default behavior is to send the connection reject message
            send(msg::ConnectionReject{request, Error::ConnectionUnsupported});
        };

        virtual void onBroadcastStart(msg::BroadcastStart const & request) {}

    }; 


} // namespace rckid::radio