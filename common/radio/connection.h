#pragma once

#include <platform/utils/ring_buffer.h>

#include "radio.h"

namespace rckid::radio {

    /** Defines a bi-directional connection between two devices. 

        Once opened, the connection can be written to and read from to move data to/from the other device. When the connection is created, the HW layer contacts the targer device and asks for a new connection to be created with itself. 
     */
    class Connection {
    public:

        using Event = std::function<void(Connection*)>;
    
        /** State of the connection. 
         */
        enum class State {
            Request, 
            Open, 
            Closing, 
            Closed,
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

    private:

        friend class Controller;

        /** Creates new connection request. 
         */
        Connection(uint8_t id, Event onAccept, Event onReject):
            state_{State::Request}, 
            ownId_{id},
            onAccept_{onAccept}, 
            onReject_{onReject} {
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
            ASSERT(state_ == State::Request);
            otherId_  = otherId_;
            state_ = State::Open;
            if (onAccept_)
                onAccept_(this);
        }

        /** Called when connection is rejected, marks the connection as closed. 
         */
        void rejected() {
            ASSERT(state_ == State::Request);
            state_ = State::Closed;
            if (onReject_)
                onReject_(this);
        }

        void closed(uint16_t reason, uint8_t const * extra) {
            // TODO 
        }

        bool tryReceive(uint8_t const * buffer, uint8_t length) {
            if (bufferRx_.canWrite() < length) {
                sendMessage(other_, msg::ConnectionReceived{otherId_, 0, static_cast<uint16_t>(bufferRx_.canWrite())});
                return false;
            } else {
                bufferRx_.write(buffer, length);
                sendMessage(other_, msg::ConnectionReceived{otherId_, length, static_cast<uint16_t>(bufferRx_.canWrite())});
                return true;
            }
        }

        bool transmit(unsigned available) {
            unsigned n = std::min(available, bufferTx_.canRead());
            if (n == 0)
                return false;
            if (n >= 30) {
                msg::ConnectionSend30 m{otherId_};
                bufferTx_.peek(m.data, 30);
                sendMessage(other_, m);
            } else {
                msg::ConnectionSend m{otherId_, static_cast<uint8_t>(n)};
                bufferTx_.peek(m.data, n);
                sendMessage(other_, m);
            }
            return true;
        }

        void transmitAck(uint8_t length) {
            if (length != 0) 
                bufferTx_.flush(length);
        }
       
        State state_;
        uint8_t ownId_;
        uint8_t otherId_ = 0;
        DeviceId other_ = 0;
        RingBuffer<512> bufferRx_;
        RingBuffer<512> bufferTx_;
        Event onAccept_;
        Event onReject_;
        Event onClose_;
        Event onDataReady_;
    };

} // namespace rckid::radio