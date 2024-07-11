/** Radio controller. 
 
    The controller uses the low level radio message API to manage long-term connections and manage broadcasts. It should be used by inheriting from the class and overriding the event methods. 



    TODO 

    Name clashes - when a ping is received with same id, it means there is device with the same id, ouch ouch. We can renumber ourselves (unless there are connections pending, etc. Or we can cancel the connections and renumber)

    Actually have a ring buffer of messages to send, or even a queue? That way we can be flexible with retransmits, etc. And the memory cost is small - 32 bytes per message     

    */
class Controller {
public:
    size_t numConnections() const { return connections_.size(); }

protected:

    Controller() {
        ASSERT(instance_ == nullptr);
        instance_ = this;   
    }

    virtual ~Controller() {
        ASSERT(instance_ == this);
        instance_ = nullptr;
        // delete all connections - ignore closing them - will timeout at the other side
        // TODO is this right? 
        for (auto x : connections_)
            delete x;
    }

    /** Creates new connection to given device. 
     
        Returns the newly created connection and may set the onAccept and reject events. 
        */
    Connection * openConnection(DeviceId other) {
#if (defined RADIO_MAX_CONNECTIONS)
        if (connections_.size() == RADIO_MAX_CONNECTIONS)
            return nullptr;
#endif
        Connection * result = new Connection{getNextConnectionId()};
        sendMessage(other, msg::ConnectionOpen{id(), result->ownId()});
        lastConnection_ = result;
        connections_.push_back(result);
        return result;
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

    /** Rejects the given connection request. 
     
        Sends the connection reject message to the sender. 
        */
    void rejectConnection(msg::ConnectionOpen const & request) {
        sendMessage(request.sender, msg::ConnectionReject{request, Error::ConnectionUnsupported});
    }

    void closeConnection(Connection & conn, char const * reason = nullptr) {
        if (conn.open())
            sendMessage(conn.other_, msg::ConnectionClose{conn.otherId_, reason});
        conn.closedLocally();
    }

    void terminateConnection(Connection & conn) {
        if (conn.open())
            sendMessage(conn.other_, msg::ConnectionClose{conn.otherId_, nullptr});
        conn.terminated();
        // TODO delete the connection from our list
    }


    /** Incoming connection request handler. 
     
        Override this method to determine whether the connection request should be accepted, or rejected. Default implementation is to reject all connections.
        */
    virtual void onConnectionRequest(msg::ConnectionOpen const & request) {
        rejectConnection(request);
    };

    /** Called when the locally opened connection has been accepted by the target. 
        
        Argument is the accepted connection that is now ready to be written to. Does nothing by default. 
        */
    virtual void onConnectionAccepted(Connection & conn) { 
        LOG("Connection " << static_cast<uint32_t>(conn.ownId()) << " accepted, other id " << static_cast<uint32_t>(conn.otherId()));
    }

    /** Called when the locally opened connection has been explicitly rejected by the target. 

        This only happens if the target has explicitly rejected the connection. For timeouts and other connection errors, see the onConnectionClosed method below.

        After the connection has been rejected, it will be closed locally and the onConnectionCLosed event will be called.           
        */
    virtual void onConnectionRejected(Connection & conn, char const * extra) {
        LOG("Connection " << static_cast<uint32_t>(conn.ownId()) << " rejected");
    }

    /** Called when a connection has been closed. 
     
        The connection can either be closed locally, remotely, or can timeout. The connection can still be used even after this call returns - to dispose of a connection, call he terminateConnection() method. 
    */
    virtual void onConnectionClosed(Connection & conn, char const * extra) {} 

    /** Called when new data is ready to be read from the connection. 
     */
    virtual void onConnectionDataReady(Connection & conn) {}

    // TODO broadcasts






    virtual void onTransmitFail() {
        //LOG("MSG transmit fail");
        // TODO
    }

    virtual void onTransmitSuccess() {
        //LOG("MSG transmit success");
        // TODO
    }




private:

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

    /** Called when new data has been received for the given connection. 
     */
    void connectionDataReceived(uint8_t connId, uint8_t const * buffer, uint8_t length) {
        Connection * conn = getConnectionByOwnId(connId);
        if (conn == nullptr) {
            LOG("Received data for unknown connection " << (uint32_t)connId << " received, ignoring.");
            return;
        }

        conn->tryReceive(buffer, length);
    }

    /** Message received event. 
     
        This method is called automatically by the radio layer and there should be no need to override it as all the required functionality is either handled automatically, or delegated to the virtual event methods. 
        */
    void onMessageReceived(uint8_t const * msg) {
        switch (msg::getIdFrom(msg)) {
            case msg::Ping::ID: {
                LOG("We have ping!");
                // TODO deal with the ping in buddy list, etc. 
                break;
            }
            // on connection open request call the event which will either handle rejection, or will create the connection endpoint locally and send the accept
            case msg::ConnectionOpen::ID: {
                LOG("connection request");
                auto & m = msg::ConnectionOpen::fromBuffer(msg);
                onConnectionRequest(m);
                break;
            }
            // our connection has been accepted - if known, update the connection information, mark it as open and call the onConnectionAccepted event
            case msg::ConnectionAccept::ID: {
                auto & m = msg::ConnectionAccept::fromBuffer(msg);
                Connection * conn = getConnectionByOwnId(m.requestId);
                if (conn == nullptr) {
                    LOG("Accept for unknown connection " << (uint32_t)m.requestId << " received, ignoring.");
                } else {
                    conn->accepted(m.responseId);
                    onConnectionAccepted(*conn);
                }
                break;
            }
            // connection has been rejected, update connection information and call the appropriate event, but do *not* dispose of the connection just yet.
            case msg::ConnectionReject::ID: {
                auto & m = msg::ConnectionReject::fromBuffer(msg);
                Connection * conn = getConnectionByOwnId(m.requestId);
                if (conn == nullptr) {
                    LOG("Reject for unknown connection " << (uint32_t)m.requestId << " received, ignoring.");
                } else {
                    conn->rejected();
                    onConnectionRejected(*conn, m.extra);
                    onConnectionClosed(*conn, nullptr);
                }
                break;
            }
            case msg::ConnectionData::ID: {
                /*
                auto & m = msg::ConnectionSend::fromBuffer(msg);
                connectionDataReceived(m.connectionId, m.data, m.size);
                */
                break;
            }
            case msg::ConnectionReceived::ID: {
                /*
                auto & m = msg::ConnectionReceived::fromBuffer(msg);
                Connection * conn = getConnectionByOwnId(m.connectionId);
                if (conn != nullptr) {
                    conn->transmitAck(m.length);
                    conn->transmit(m.available);
                } else {
                    LOG("Transmit ACK for unknown connection " << (uint32_t)m.connectionId << " received, ignoring.");
                }
                break;
                */
            }
            case msg::ConnectionClose::ID: {
                /*
                auto m = msg::ConnectionClose::fromBuffer(msg);
                Connection * conn = getConnectionByOwnId(m.connectionId);
                if (conn)
                    conn->closed(m.reason, m.extra);
                break;
                */
            }
            // TODO broadcasts - do we want sth? maybe deduplicate & things
        }
    }

    // TODO do we still need the IRQ handler?
    friend void irqHandler();
    friend void rckid::radio::initialize(DeviceId);
    friend void rckid::radio::loop();
    friend void rckid::radio::transmit(DeviceId, uint8_t const *, size_t);

    std::vector<Connection *> connections_;
    Connection * lastConnection_ = nullptr;
    uint8_t nextConnectionId_ = 0;

    static inline Controller * instance_ = nullptr;


}; // Controller