#pragma once

#include "definitions.h"

namespace rckid::radio::msg {

#define MESSAGE(MSG_ID, NAME, ...)                               \
    class NAME {                                                 \
    public:                                                      \
        static uint8_t constexpr ID = MSG_ID;                    \
        uint8_t const id = MSG_ID;                               \
        static NAME const & fromBuffer(uint8_t const * buffer) { \
            return * reinterpret_cast<NAME const *>(buffer);     \
        }                                                        \
        __VA_ARGS__                                              \
    } __attribute__((packed));                                   \
    static_assert(sizeof(NAME) <= 32)              

    /** Simple ping indicating the device exists and is in range. 
     
        The devices can periodically send the ping signal to detect other devices in range. The base stations never ping. Contains the device ID and some other attributes as payload. ALl pings are numbered so that observing multiple pings can be used to determine signal strength of the connection by calculating the number of dropped messages. 

     */
    MESSAGE(0, Ping, 
        DeviceId sender;
        uint8_t index;
        // TODO add extra ping attributes, 
    );

    /** Request to open connection. 
     
        Contains the target device address and the connection type we wish to open as well as a temporary connection ID issued by the sender. 
     */
    MESSAGE(1, ConnectionOpen,
        DeviceId sender;
        ConnectionKind kind;
        uint8_t requestId;
    );

    /** Connection accepted by the target device. 
     
        Contains the temporary connection id created by the connection innitiating device (recipient of this mesage) and the non-temporary connection ID that must be used for the connection in the future, the ID is issued by the connection target (i.e. the sender of this message). 
     */
    MESSAGE(2, ConnectionAccept,
        uint8_t requestId;
        uint8_t connectionId;
    );

    /** Connection rejected by the target device. 
     
        Contains the temporary connection id and reason for rejection. 
     */
    MESSAGE(3, ConnectionReject, 
        uint8_t requestId;
        uint16_t reason;
        uint8_t extra[28];

        ConnectionReject(uint8_t requestId, uint16_t reason):
            requestId{requestId}, 
            reason{reason} {
            extra[0] = 0;
        }

        ConnectionReject(uint8_t requestId, Error reason): ConnectionReject(requestId, static_cast<uint16_t>(reason)) {}

        ConnectionReject(ConnectionOpen const & request, uint16_t reason): ConnectionReject(request.requestId, reason) {}

        ConnectionReject(ConnectionOpen const & request, Error reason): ConnectionReject(request.requestId, static_cast<uint16_t>(reason)) {}
    );

    /** Sends data as part of the opened connection.
     
        Contains the connection id and length of data to be sent. Can go from either point of the connection once the connection is opened. 
     */
    MESSAGE(4, ConnectionSend, 
        uint8_t connectionId;
        uint8_t size;
        uint8_t data[29];
    );

    /** Sends full 30 bytes of data as part of the opened connection. 
     
        Similar to ConnectionSend, but always transmits full 30 bytes of data so does not need the byte for length. 
     */
    MESSAGE(5, ConnectionSend30, 
        uint8_t connectionId; 
        uint8_t data[30];
    );

    /** Closes the connection. 
     
        Contains the connection id and a reason to close. Can be issued by either connection endpoint. 
     */
    MESSAGE(6, ConnectionClose,
        uint8_t connectionId;
        uint16_t reason;
        uint8_t extra[28];
    );

    /** Start of a broadcast message
     
        Unlike connections, broadcasts are one way only messages that may have multiple recipients (anyone in range). Broadcasts do not have an ID as only one broadcast is allowed from a device at a time. A broadcast type is also identified in the opening message. 

        All broadcast messages have broadcast index, which increases with every unique message sent as part of the broadcast that can be used for deduplication of message re-sends. Additional field is the number of resends planned per message which can be used by the receiving devices to indetify the signal quality. 

        Also contains a broadcast kind information.

     */
    MESSAGE(7, BroadcastStart, 
        DeviceId sender;
        BroadcastKind kind;
        uint8_t repeatCount;
    );

    /** Broadcast data. 
     
        Contains the broadcast message index and raw data.
        The broadcast index must be part of the broadcast message. 
        
     */
    MESSAGE(8, BroadcastData, 
        uint8_t index;
        uint8_t size;
        uint8_t data[29];
        // TODO do we want sender id here as well??
    );

    MESSAGE(9, BroadcastData30,
        uint8_t index;
        uint8_t data[30];
    ); 

    /** End of a broadcast. 
     
     */
    MESSAGE(10, BroadcastEnd, 
        DeviceId sender;
    );

    /** Simple single messagepayload.  
     
        Has the sender and payload length information in it. 
     */
    MESSAGE(12, Payload, 
        DeviceId sender;
        uint8_t size;
        uint8_t data[29];
    );

    /** Full message payload. 
        
        Has sender information, length is fixed to 30 bytes. 
     */
    MESSAGE(13, Payload30, 
        DeviceId sender;
        uint8_t data[30];
    );

#undef MESSAGE

} // namespace rkid::radio::msg