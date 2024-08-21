#pragma once

#include "../rckid.h"

/** \defgroup Communication
 
    RCKid provides a simple interface for sending packets of data, upon which more complex communication protocols are built in the SDK. As the main purpose of the communication libraries is to be used with the radio enabled cartridges (ESP8266 via UART, LoRa and NRF24L01p), the basic communications interface closely follows the lowest common denominator. Packets of up to 32 bytes can be sent to one of 255 devices, or to everyone listening. A packet that is not a broadcast can request acknowledgement so sender can be sure the package has been received by the sender. The API is simple on purpose sacrificing high bandwidth fo ease of use and hardware independence. 

    Step above the packet communication interface are the _connections_, which allow bi-directional communication between two devices that can be kept open and written to/read from throughout its existence. 
 */

namespace rckid {

    using Packet = uint8_t[32];

    /** Interface for a transceiver capable of sending and receiving packets. 
     
        Provides basic API for sending and receiving packets of up to 32 bytes of data to various targets (identified by one byte address) as well as optional receive receipt confirmations. This rather limited functionality is 
     */
    class PacketTransceiver {
    public:

        

    }; // rckid::PacketTransceiver







} // namespace rckid