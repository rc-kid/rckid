/** Defines a bi-directional connection between two devices. 

    Once opened, the connection can be written to and read from to move data to/from the other device. When the connection is created, the HW layer contacts the targer device and asks for a new connection to be created with itself. 
    */
class Connection {
public:

    using Event = std::function<void(Connection*)>;

    /** State of the connection. 
     
        The connection can be in four general states:
        
        - requested, in which case there is nothing that can be done with the connection, but to wait for the target to accept or reject
        - open, when the connection can be read from (if data available), written to (if not full), or closed. 
        - closed (for various reasons), in which case the connection will not receive further data, but any data already in there can be read, and no data can be written to the connection. 
        - terminated, a terminated connection is like closed, but no data can be read from it and it may be deleted by the controller at any time. 
        */
    enum class State {
        Requested, // locally requested, not heard back
        Open, // valid connection that can be written to & read from
        Rejected, // connection has been explicitly rejected by the traget, closed
        Timeout, // communication failure, closed
        ClosedLocally, // explicitly closed by this end, closed
        ClosedRemotely, // explicitly closed by the other end, closed
        Terminated, // no longer valid to use
    };

    /** Returns the state of the connection. 
     */
    State state() const { return state_; }

    /**Shortand for checking of the connection is open. 
     */
    bool open() const { return state_ == State::Open; }

    /** Returns the id of this connection used by this device.          
     */
    uint8_t ownId() const { return ownId_; }

    /** Returns the id of the connection used by the other device. Only valid if the connection has been accepted.
     */
    uint8_t otherId() const { return otherId_; }

    /** Returns the  device to which the connection is established. 
     */
    DeviceId other() const { return other_; }

    unsigned canRead() const { return bufferRx_.canRead(); }
    unsigned read(uint8_t * buffer, unsigned numBytes) { return bufferRx_.read(buffer, numBytes); }

    unsigned canWrite() const { return open() ? bufferTx_.canWrite() : 0; }
    unsigned write(uint8_t const * buffer, unsigned numBytes) { 
        ASSERT(open());
        return bufferTx_.write(buffer, numBytes); 
    }

    /** A connection can be assigned metadata.
     */
    template<typename T> 
    T const * getMetadata() const { return static_cast<T const *>(metadata_); }

    template<typename T>
    T * getMetadata() { return static_cast<T const *>(metadata_); }

    template<typename T>
    void setMetadata(T * value) { metadata_ = static_cast<void*>(value); }

private:

    friend class Controller;

    /** Creates new connection request. 
     */
    Connection(uint8_t id):
        state_{State::Requested}, 
        ownId_{id} {
    }

    /** Creates new accepted connection. 
     */
    Connection(uint8_t ownId, uint8_t otherId, DeviceId other):
        state_{State::Open}, 
        ownId_{ownId}, 
        otherId_{otherId},
        other_{other} {
    }

    /** Marks the connection as open. 
     */
    void accepted(uint8_t otherId) {
        ASSERT(state_ == State::Requested);
        otherId_  = otherId_;
        state_ = State::Open;
    }

    /** Called when connection is rejected, marks the connection as closed. 
     */
    void rejected() {
        ASSERT(state_ == State::Requested);
        state_ = State::Rejected;
    }

    void closedLocally() {
        ASSERT(state_ == State::Open);
        state_ = State::ClosedLocally;
    }

    void terminated() {
        state_ = State::Terminated;
    }

    void * metadata_ = nullptr;

    State state_;
    uint8_t ownId_;
    uint8_t otherId_ = 0;
    DeviceId other_ = 0;
    RingBuffer<512> bufferRx_;
    RingBuffer<512> bufferTx_;
    // TODO we also need to remmeber the tx timeout and resend the messages when failed 
    bool txOdd_ = false;
}; // Connection
