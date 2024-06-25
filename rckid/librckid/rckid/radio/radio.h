#pragma once

#include "platform.h"

/** Radio protocol

    NOTE this is only enabled, and should only be included in cartridges that contain necessary radio hardware. 

    The radio protocol works on two layers - message and connection based. 

    The lower level message based layer concerns itself only with sending single messages between two devices. Messages can be acked, i.e. the sender will know if a message has been received corrently, but other than the ACK, messages are always uni-directional from sender to receiver. 

    Higher level connection based layer work on connections, which comprise of multiple messages and allow bi-directional data transfers between two devices. 

 */
namespace rckid::radio {

    using DeviceId = uint8_t;

    enum class ConnectionKind : uint8_t {

    }; // radio::ConnectionKind

    enum class BroadcastKind : uint8_t {

    }; // radio::BroadcastKind

    namespace msg {

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

            ConnectionReject(ConnectionOpen const & request, uint16_t reason): ConnectionReject(request.requestId, reason) {}
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
    } // namespace rckid::remote::msg

    /** Defines a connection between two devices, or a device and base station. 
     
        Once opened, the connection can be written to and read from to move data to/from the other device. When the connection is created, the HW layer contacts the targer device and asks for a new connection to be created with itself. 
     */
    class Connection {

    };


    /** Radio controller. 



        TODO 

        Name clashes - when a ping is received with same id, it means there is device with the same id, ouch ouch. We can renumber ourselves (unless there are connections pending, etc. Or we can cancel the connections and renumber)
     
     */
    class RadioController {
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
            send(reinterpret_cast<uint8_t const *>(&mgs), sizeof(T));
        }

        void send(uint8_t const * msg, size_t length);

        virtual void onConnectionOpen(msg::ConnectionOpen const & request) {
            // TODO default behavior is to send the connection reject message
            send(msg::ConnectionReject{request, Error::ConnectionUnsupported});
        };

        virtual void onBroadcastStart(msg::BroadcastStart const & request) {}

    }; 

} // namespace rckid