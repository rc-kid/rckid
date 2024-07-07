#pragma once

#include "radio.h"
#include "connection.h"

namespace rckid::radio {

    /** Radio controller. 



        TODO 

        Name clashes - when a ping is received with same id, it means there is device with the same id, ouch ouch. We can renumber ourselves (unless there are connections pending, etc. Or we can cancel the connections and renumber)

        Actually have a ring buffer of messages to send, or even a queue? That way we can be flexible with retransmits, etc. And the memory cost is small - 32 bytes per message     

     */
    class Controller {
    public:

        /** Creates new connection to given device. 
         
            Returns the newly created connection and may set the onAccept and reject events. 
         */
        Connection * openConnection(DeviceId other, ConnectionKind kind, Connection::Event onAccept = nullptr, Connection::Event onReject = nullptr) {
#if (defined RADIO_MAX_CONNECTIONS)
            if (connections_.size() == RADIO_MAX_CONNECTIONS)
                return nullptr;
#endif
            Connection * result = new Connection{getNextConnectionId(), onAccept, onReject};
            sendMessage(other, msg::ConnectionOpen{id(), result->ownId(), kind});
            lastConnection_ = result;
            connections_.push_back(result);
            return result;
        }

        Controller() {
            ASSERT(instance_ == nullptr);
            instance_ = this;
        }

        virtual ~Controller() {
            ASSERT(instance_ == this);
            instance_ = nullptr;
        }


    protected:
        


        /** Incoming connection request handler. 
         
            Override this method to determine whether the connection request should be accepted, or rejected. Default implementation is to reject all connections. To accept the connection, call the acceptConnection() method from within. 
         */
        virtual void onConnectionRequest(msg::ConnectionOpen const & request) {
            rejectConnection(request);
        };

        /** Broadcast begin handler. 
         */
        virtual void onBroadcastStart(msg::BroadcastStart const & request) {}

        virtual void onTransmitFail() {
            //LOG("MSG transmit fail");
            // TODO
        }

        virtual void onTransmitSuccess() {
            //LOG("MSG transmit success");
            // TODO
        }

        /** Rejects the given connection. 
         
            Sends the connection reject message to the sender. 
         */
        void rejectConnection(msg::ConnectionOpen const & request) {
            sendMessage(request.sender, msg::ConnectionReject{request, Error::ConnectionUnsupported});
        }

        /** Accepts the given connection reuquest and returns the connection itself. 
         
            May return null if creating the new connection failed. 
         */
        Connection * acceptConnection(msg::ConnectionOpen const & request) {
#if (defined RADIO_MAX_CONNECTIONS)
            if (connections_.size() == RADIO_MAX_CONNECTIONS) {
                sendMessage(request.sender, msg::ConnectionReject{request, Error::TooManyConnections});
                return nullptr;
            }
#endif
            Connection * result = new Connection{getNextConnectionId(), request.requestId, request.sender};
            sendMessage(request.sender, msg::ConnectionAccept{result->otherId(), result->ownId()});
            lastConnection_ = result;
            connections_.push_back(result);
            return result;
        }

        /** Returns next valid connection id.
         
            Keeps instrumenting the connection id for as long as there are conflicts with any currently active connections. 
         */
        uint8_t getNextConnectionId() {
            while (true) {
                uint8_t result = nextConnectionId_++;
                bool ok = true;
                for (Connection * conn : connections_)
                    if (conn->ownId() == result) {
                        ok = false;
                        break;
                    }
                if (ok)
                    return result;
            }
        }

        /** Returns own connection corresponding to the provided id. 
         
            If no such active connection exists, returns null. 
         */
        Connection * getConnectionByOwnId(uint8_t id) {
            if (lastConnection_ && lastConnection_->ownId() == id)
                return lastConnection_;
            for (Connection * conn : connections_) {
                if (conn->ownId() == id)
                    return conn;
            }
            return nullptr;
        }

        void connectionDataReceived(uint8_t connId, uint8_t const * buffer, uint8_t length) {
            Connection * conn = getConnectionByOwnId(connId);
            if (conn == nullptr) {
                LOG("Received data for unknown connection " << (uint32_t)connId << " received, ignoring.");
                return;
            }
            conn->tryReceive(buffer, length);
        }

        /** Message received event. 
         */
        void onMessageReceived(uint8_t const * msg) {
            switch (static_cast<msg::Id>(msg[0])) {
                case msg::ConnectionOpen::ID: {
                    auto & m = msg::ConnectionOpen::fromBuffer(msg);
                    LOG("connection request");
                    onConnectionRequest(m);
                    break;
                }
                case msg::ConnectionAccept::ID: {
                    auto & m = msg::ConnectionAccept::fromBuffer(msg);
                    Connection * conn = getConnectionByOwnId(m.requestId);
                    if (conn == nullptr)
                        LOG("Accept for unknown connection " << (uint32_t)m.requestId << " received, ignoring.");
                    else
                        conn->accepted(m.responseId);
                    break;
                }
                case msg::ConnectionReject::ID: {
                    auto & m = msg::ConnectionReject::fromBuffer(msg);
                    Connection * conn = getConnectionByOwnId(m.requestId);
                    if (conn == nullptr)
                        LOG("Reject for unknown connection " << (uint32_t)m.requestId << " received, ignoring.");
                    else
                        conn->rejected(); // TODO reason & stuff
                    break;
                }
                case msg::ConnectionSend::ID: {
                    auto & m = msg::ConnectionSend::fromBuffer(msg);
                    connectionDataReceived(m.connectionId, m.data, m.size);
                    break;
                }
                case msg::ConnectionSend30::ID: {
                    auto & m = msg::ConnectionSend30::fromBuffer(msg);
                    connectionDataReceived(m.connectionId, m.data, 30);
                    break;
                }
                case msg::ConnectionReceived::ID: {
                    auto & m = msg::ConnectionReceived::fromBuffer(msg);
                    Connection * conn = getConnectionByOwnId(m.connectionId);
                    if (conn != nullptr) {
                        conn->transmitAck(m.length);
                        conn->transmit(m.available);
                    } else {
                        LOG("Transmit ACK for unknown connection " << (uint32_t)m.connectionId << " received, ignoring.");
                    }
                    break;
                }
                case msg::ConnectionClose::ID: {
                    auto m = msg::ConnectionClose::fromBuffer(msg);
                    Connection * conn = getConnectionByOwnId(m.connectionId);
                    if (conn)
                        conn->closed(m.reason, m.extra);
                    break;
                }
                // TODO broadcasts - do we want sth? maybe deduplicate & things
            }
        }

    private:

        friend void irqHandler();

        std::vector<Connection *> connections_;
        Connection * lastConnection_ = nullptr;
        uint8_t nextConnectionId_ = 0;

        static inline Controller * instance_ = nullptr;


    }; // radio::Controller


} // rckid::radio