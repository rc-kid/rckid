#ifndef MESSAGE
#error "MESSAGE macro must be defined before including"
#endif

    // 1 byte
    // 128 64 32 x x x x x
    //  0  0   0           -- send odd
    //  0  0   1           -- send even
    //  0  1   0 x x x x x -- broadcast data (or end if size is 0)
    //  0  1   1           -- broadcast start
    //  1  0   x x x x x x -- custom messages
    //                     -- request, accept, reject, 
    //  1  1   x x x x x x -- ping 
    //  0  1   1           -- connection request
    //  1  0   0           -- connection accept
    //  1  0   1           -- connection reject
    //  1  1   0
    //  1  1   1           -- ping 


    /*
                  request -----> 
                          <----- ACK

                          <----- Accept
                          -----> ACK (if no ACK, resend Accept), if failed forget session (!)    

        --> need to know what is the message that failed (!!)


    
    */

   MESSAGE(= 0x00, ConnectionData, true, 

        bool odd() const { return id_ & 0b00100000; }
        uint8_t length() const { return id_ & 0x1f; }

        uint8_t connectionId;
        uint8_t payload[30];

        ConnectionData(uint8_t otherId, bool odd, uint8_t length):
            connectionId{otherId} {
            id_ |= (length & 0x1f);
            if (odd)
                id_ |= 0b00100000;
        }
   )

   MESSAGE(= 0x40, BroadcastData, false, 
        uint8_t length() const { return id_ & 0x1f; }

        uint8_t payloadIndex;
        uint8_t payload[30];
   )

    /** Simple ping indicating the device exists and is in range. 
     
        The devices can periodically send the ping signal to detect other devices in range. The base stations never ping. Contains the device ID and some other attributes as payload. ALl pings are numbered so that observing multiple pings can be used to determine signal strength of the connection by calculating the number of dropped messages. 

     */
   
    MESSAGE(= 0x80, Ping, true,
        DeviceId sender;
        uint8_t index;
        uint64_t userId;
        char userName[21];
        Ping(DeviceId sender, uint8_t index, uint64_t userId):
            sender{sender}, index{index}, userId{userId} {
            userName[0] = 0;
        }
    )

    /** Request to open connection. 
     
        Contains the target device address and the connection type we wish to open as well as the id for the connection under which it will be recognized on the sender. 
     */
    MESSAGE(, ConnectionOpen, true,
        DeviceId sender;
        uint8_t requestId;
        uint8_t param;

        ConnectionOpen(DeviceId sender, uint8_t requestId, uint8_t param = 0):
            sender{sender}, requestId{requestId}, param{param} {}
    )

    /** Connection accepted by the target device. 
     
        Contains the sender issued connection id (requestId) and the target connection id (responseId). 
     */
    MESSAGE(, ConnectionAccept, true,
        uint8_t requestId;
        uint8_t responseId;

        ConnectionAccept(uint8_t requestId, uint8_t responseId): requestId{requestId}, responseId{responseId} {}
    )

    /** Connection rejected by the target device. 
     
        Contains the temporary connection id and reason for rejection. 
     */
    MESSAGE(, ConnectionReject, true,
        uint8_t requestId;
        uint16_t reason;
        char extra[28];

        ConnectionReject(uint8_t requestId, uint16_t reason):
            requestId{requestId}, 
            reason{reason} {
            extra[0] = 0;
        }

        ConnectionReject(uint8_t requestId, Error reason): ConnectionReject(requestId, static_cast<uint16_t>(reason)) {}

        ConnectionReject(ConnectionOpen const & request, uint16_t reason): ConnectionReject(request.requestId, reason) {}

        ConnectionReject(ConnectionOpen const & request, Error reason): ConnectionReject(request.requestId, static_cast<uint16_t>(reason)) {}
    )

    /** Connection data send receipt.
        
        Since the connection buffers are not infinite, the ConnectionReceived message must be issued after each connection send message has been accepted and the data has been written to the buffer at the target side. The returned length must be the same as the length of data sent by the last send message, or 0, indicating the data has not been written and the send message must be repeated. 
     */
    MESSAGE(, ConnectionReceived, true,
        uint8_t connectionId;
        uint8_t length;
        uint32_t available;

        ConnectionReceived(uint8_t id, uint8_t length, uint32_t available) : connectionId{id}, length{length}, available{available} {}
    )

    /** Closes the connection. 
     
        Contains the connection id and a reason to close. Can be issued by either connection endpoint. 
     */
    MESSAGE(, ConnectionClose, true, 
        uint8_t connectionId;
        char extra[29];

        ConnectionClose(uint8_t id, char const * extra):
            connectionId{id} {
            if (extra == nullptr) {
                this->extra[0] = 0;
            } else {
                strncpy(this->extra, extra, sizeof(this->extra));
                this->extra[sizeof(extra)] = 0;
            }
        }
    )

    /** Start of a broadcast message
     
        Unlike connections, broadcasts are one way only messages that may have multiple recipients (anyone in range). Broadcasts do not have an ID as only one broadcast is allowed from a device at a time. A broadcast type is also identified in the opening message. 

        All broadcast messages have broadcast index, which increases with every unique message sent as part of the broadcast that can be used for deduplication of message re-sends. Additional field is the number of resends planned per message which can be used by the receiving devices to indetify the signal quality. 

        Also contains a broadcast kind information.

     */
    MESSAGE(, BroadcastStart, false, 
        DeviceId sender;
        uint8_t broadcastKind;
        uint8_t repeatCount;
    )

    /** End of a broadcast. 
     
     */
    MESSAGE(, BroadcastEnd, false,
        DeviceId sender;
    )

    /** Debug print. When received by a controller, prints payload, which is a null terminated string. 
     */
    MESSAGE(= 0xff, DebugPrint, false, 
        char payload[31];

        DebugPrint() {
            payload[30] = 0;
        };

        DebugPrint(char const * payload) {
            strncpy(this->payload, payload, sizeof(this->payload));
            this->payload[sizeof(this->payload)] = 0;
        }

        // TODO add writer interface
   )

#undef MESSAGE