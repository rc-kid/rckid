#ifndef MESSAGE
#error "MESSAGE macro must be defined before including"
#endif

    /** Simple ping indicating the device exists and is in range. 
     
        The devices can periodically send the ping signal to detect other devices in range. The base stations never ping. Contains the device ID and some other attributes as payload. ALl pings are numbered so that observing multiple pings can be used to determine signal strength of the connection by calculating the number of dropped messages. 

     */
    MESSAGE(Ping, false,
        DeviceId sender;
        uint8_t index;
        uint64_t userId;
        char userName[];
    )

    /** Request to open connection. 
     
        Contains the target device address and the connection type we wish to open as well as the id for the connection under which it will be recognized on the sender. 
     */
    MESSAGE(ConnectionOpen, true,
        DeviceId sender;
        uint8_t requestId;
        ConnectionKind kind;

        ConnectionOpen(DeviceId sender, uint8_t requestId, ConnectionKind kind):
            sender{sender}, requestId{requestId}, kind{kind} {}
    )

    /** Connection accepted by the target device. 
     
        Contains the sender issued connection id (requestId) and the target connection id (responseId). 
     */
    MESSAGE(ConnectionAccept, true,
        uint8_t requestId;
        uint8_t responseId;

        ConnectionAccept(uint8_t requestId, uint8_t responseId): requestId{requestId}, responseId{responseId} {}
    )

    /** Connection rejected by the target device. 
     
        Contains the temporary connection id and reason for rejection. 
     */
    MESSAGE(ConnectionReject, true,
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
    )

    /** Sends data as part of the opened connection.
     
        Contains the connection id and length of data to be sent. Can go from either point of the connection once the connection is opened. 
     */
    MESSAGE(ConnectionSend, true, 
        uint8_t connectionId;
        uint8_t size;
        uint8_t data[29];

        ConnectionSend(uint8_t id, uint8_t size): connectionId{id}, size{size} {}
    )

    /** Sends full 30 bytes of data as part of the opened connection. 
     
        Similar to ConnectionSend, but always transmits full 30 bytes of data so does not need the byte for length. 
     */
    MESSAGE(ConnectionSend30, true, 
        uint8_t connectionId; 
        uint8_t data[30];

        ConnectionSend30(uint8_t id): connectionId{id} {}
    )

    /** Connection data send receipt.
        
        Since the connection buffers are not infinite, the ConnectionReceived message must be issued after each connection send message has been accepted and the data has been written to the buffer at the target side. The returned length must be the same as the length of data sent by the last send message, or 0, indicating the data has not been written and the send message must be repeated. 
     */
    MESSAGE(ConnectionReceived, true,
        uint8_t connectionId;
        uint8_t length;
        uint16_t available;

        ConnectionReceived(uint8_t id, uint8_t length, uint16_t available) : connectionId{id}, length{length}, available{available} {}
    )

    /** Closes the connection. 
     
        Contains the connection id and a reason to close. Can be issued by either connection endpoint. 
     */
    MESSAGE(ConnectionClose, true, 
        uint8_t connectionId;
        uint16_t reason;
        uint8_t extra[28];
    )

    /** Start of a broadcast message
     
        Unlike connections, broadcasts are one way only messages that may have multiple recipients (anyone in range). Broadcasts do not have an ID as only one broadcast is allowed from a device at a time. A broadcast type is also identified in the opening message. 

        All broadcast messages have broadcast index, which increases with every unique message sent as part of the broadcast that can be used for deduplication of message re-sends. Additional field is the number of resends planned per message which can be used by the receiving devices to indetify the signal quality. 

        Also contains a broadcast kind information.

     */
    MESSAGE(BroadcastStart, false, 
        DeviceId sender;
        BroadcastKind kind;
        uint8_t repeatCount;
    )

    /** Broadcast data. 
     
        Contains the broadcast message index and raw data.
        The broadcast index must be part of the broadcast message. 
        
     */
    MESSAGE(BroadcastData, false, 
        uint8_t index;
        uint8_t size;
        uint8_t data[29];
        // TODO do we want sender id here as well??
    )

    MESSAGE(BroadcastData30, false,
        uint8_t index;
        uint8_t data[30];
    )

    /** End of a broadcast. 
     
     */
    MESSAGE(BroadcastEnd, false,
        DeviceId sender;
    )

    /** Simple single messagepayload.  
     
        Has the sender and payload length information in it. 
     */
    MESSAGE(Payload, true, 
        DeviceId sender;
        uint8_t size;
        uint8_t data[29];
    )

    /** Full message payload. 
        
        Has sender information, length is fixed to 30 bytes. 
     */
    MESSAGE(Payload30, true, 
        DeviceId sender;
        uint8_t data[30];
    )

#undef MESSAGE