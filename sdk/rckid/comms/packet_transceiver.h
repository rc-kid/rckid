#pragma once

#include "../rckid.h"

namespace rckid {

    using Packet = uint8_t[32];

    /** Interface for a transceiver capable of sending and receiving packets. 
     
        Provides basic API for sending and receiving packets of up to 32 bytes of data to various targets (identified by one byte address) as well as optional receive receipt confirmations. This rather limited functionality is 
     */
    class PacketTransceiver {

    }; // rckid::PacketTransceiver







} // namespace rckid