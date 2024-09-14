#pragma once

#include <functional>
#include <platform/writer.h>
#include <platform/reader.h>
#include <platform/buffer.h>

#include "../rckid.h"
#include "messages.h"

namespace rckid {

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
            Closed, // closed, will be terminated soon
        };

        /** Returns the connection parameter that is created when the connection is opened. 
         */
        uint8_t param() const { return param_; }

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
        unsigned canReadContinuous() const { return bufferRx_.canReadContinuous(); }
        unsigned read(uint8_t * buffer, unsigned numBytes) { return bufferRx_.read(buffer, numBytes); }
        uint8_t const * readBuffer() { return bufferRx_.readBuffer(); }

        /** Determines if there is enough stored in the buffer to read the next value of given type. If the method returns true, the reader() can be used to read the value. 
         */
        template<typename T>
        bool canRead() const;

        Reader reader() { return Reader{[this](){ return bufferRx_.read(); }}; }

        Reader peek() const { unsigned offset = 0; return Reader{[& offset, this]() mutable { return bufferRx_.peek(offset++); }}; }

        unsigned canWrite() const { return open() ? bufferTx_.canWrite() : 0; }
        unsigned write(uint8_t const * buffer, unsigned numBytes) { 
            ASSERT(open());
            return bufferTx_.write(buffer, numBytes); 
        }

        /** Returns writer that can be used to write or serialize data to the connection. 
         */
        Writer writer() { return Writer{[this](char c){ bufferTx_.write(c); }}; }


        /** A connection can be assigned metadata.
         */
        template<typename T> 
        T const * metadata() const { return static_cast<T const *>(metadata_); }

        template<typename T>
        T * metadata() { return static_cast<T const *>(metadata_); }

        bool hasMetadata() { return metadata_ != nullptr; }


        template<typename T>
        void setMetadata(T * value) { metadata_ = static_cast<void*>(value); }



    private:

        friend class Controller;

        /** Creates new connection request. 
         */
        Connection(uint8_t id, uint8_t param):
            state_{State::Requested}, 
            ownId_{id},
            param_{param} {
        }

        /** Creates new accepted connection. 
         */
        Connection(uint8_t ownId, uint8_t otherId, DeviceId other, uint8_t param):
            state_{State::Open}, 
            ownId_{ownId}, 
            otherId_{otherId},
            other_{other},
            param_{param} {
        }

        /** Marks the connection as open. 
         */
        void accepted(uint8_t otherId) {
            ASSERT(state_ == State::Requested);
            otherId_  = otherId;
            state_ = State::Open;
        }

        /** Called when connection is rejected, marks the connection as closed. 
         */
        void rejected() {
            ASSERT(state_ == State::Requested);
            state_ = State::Rejected;
        }

        void closed() {
            ASSERT(state_ == State::Open);
            state_ = State::Closed;
        }

        void transmit(unsigned available) {
            if (commState_ & TXSTATE_PENDING)
                return;
            unsigned n = std::min(available, bufferTx_.canRead());
            if (n == 0)
                return; // nothing to transmit
            if (n > 30)
                n = 30;
            LOG("Transmitting connection " << (uint32_t)ownId_ << ", other id " << (uint32_t)otherId_);
            msg::ConnectionData m{otherId_, static_cast<bool>(commState_ & TXSTATE_ODD), static_cast<uint8_t>(n)};
            bufferTx_.peek(m.payload, n);
            commState_ |= TXSTATE_PENDING;
            sendMessage(other_, m);
        }

        void transmitAck(uint8_t numBytes, unsigned available) {
            commState_ &= ~ TXSTATE_PENDING;
            if (numBytes > 0) {
                commState_ ^= TXSTATE_ODD;
                bufferTx_.flush(numBytes);
            }
            transmit(available);
        }

        bool receive(uint8_t const * buffer, uint8_t numBytes, bool odd) {
            uint16_t available = bufferRx_.canWrite();
            if (available >= numBytes) {
                if ((commState_ & RXSTATE_ODD) != odd) {
                    bufferRx_.write(buffer, numBytes);
                    commState_ ^= RXSTATE_ODD;
                }
                sendMessage(other_, msg::ConnectionReceived{otherId_, numBytes, static_cast<uint32_t>(available - numBytes)});
                return true;
            } else {
                sendMessage(other_, msg::ConnectionReceived{otherId_, 0, available});
                return false;
            }
        }

        void * metadata_ = nullptr;

        State state_;
        uint8_t ownId_;
        uint8_t otherId_ = 0;
        DeviceId other_ = 0;
        uint8_t param_; 
        RingBuffer<512> bufferRx_;
        RingBuffer<512> bufferTx_;


        static constexpr uint8_t TXSTATE_ODD = 0x01;
        static constexpr uint8_t TXSTATE_PENDING = 0x02; 
        static constexpr uint8_t RXSTATE_ODD = 0x04;
        uint8_t commState_ = RXSTATE_ODD;
        // TODO timeout for acks

    }; // Connection


    /** A string can be read from connection if there is enough to be read to get the length of the string (2 bytes) and the data itself. 
     */
    template<>
    inline bool Connection::canRead<std::string>() const {
        unsigned x = canRead();
        if (x < 2) 
            return false;
        unsigned len = peek().deserialize<uint16_t>();
        return 2 + len <= x;
    }



} // namespace rckid