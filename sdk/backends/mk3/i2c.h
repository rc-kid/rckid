#pragma once

#include <rckid/rckid.h>
#include "backend_internals.h"

/** I2C Queue
 
    Unlike v2 where I2C was handled either blocking, or async in the tick interrupts, which created cumbersome problems when user code was interfering with the tick routines, I2C in mkIII is handled via a queue. Each packet specifies a full transaction with bytes to send and bytes to receive. When the packet is processed an associated callback is executed from within the ISR.
    */
namespace rckid::i2c {
    class Packet {
    public:
        uint8_t address;
        uint8_t writeLen;
        uint8_t readLen;
        Packet * next = nullptr;
        void (* callback)(uint8_t) = nullptr;

        Packet(uint8_t addr, uint8_t wlen, uint8_t const * wdata, uint8_t rlen, void (* cb)(uint8_t)):
            address{addr}, 
            writeLen{wlen}, 
            readLen{rlen}, 
            callback{cb} {
            if (writeLen <= 4) {
                uint8_t * x = writeData();
                for (size_t i = 0; i < writeLen; ++i) {
                    x[i] = wdata[i];
                }
            } else {
                writeData_ = new uint8_t[writeLen];
                memcpy(writeData_, wdata, writeLen);
            }
        }
        Packet(uint8_t addr, uint8_t wlen, uint8_t const * wdata): Packet(addr, wlen, wdata, 0, nullptr) { }

        ~Packet() {
            if (writeLen > 4)
                delete [] writeData_;
        }

        /** Returns true if the packet is a control request callback, a special packet that does not get send, but instead calls the callback, thus actually giving the callback function full control over the I2C bus. 
         */
        bool isControlCallback() const {
            return address == 0 && writeLen == 0 && readLen == 0;
        }

        void transmit() {
            // if this is control callback, call the callback instead of transmitting
            if (isControlCallback()) {
                if (callback)
                    callback(0);
                return;
            }
            i2c0->hw->enable = 0;
            i2c0->hw->tar = address;
            i2c0->hw->enable = 1;
            // if there are data to send, write them first. Do not forget to set the stop bit if no data to red
            if (writeLen > 0) {
                uint8_t * x = writeData();
                for (uint32_t i = 0, e = writeLen - 1; i <= e; ++i)
                    i2c0->hw->data_cmd = x[i] | ((i == e && readLen == 0) ? I2C_IC_DATA_CMD_STOP_BITS : 0);
            }
            // if there are data to read, write the recv bits
            if (readLen > 0) {
                for (uint32_t i = 0, e = readLen - 1; i <= e; ++i) {
                    uint32_t cmd = I2C_IC_DATA_CMD_CMD_BITS;
                    if (i == 0 && writeLen != 0)
                        cmd |= I2C_IC_DATA_CMD_RESTART_BITS;
                    if (i == e)
                        cmd |= I2C_IC_DATA_CMD_STOP_BITS;
                    i2c0->hw->data_cmd = cmd;
                }
            }
            // set the rx threshold accordingly so that we get interrupt properly
            // TODO what if we are not reading anything only writing? 
            i2c0->hw->rx_tl = (readLen == 0) ? 0 : readLen - 1;
        }

        uint8_t * writeData() {
            if (writeLen <= 4) {
                return reinterpret_cast<uint8_t *>(&writeData_);
            } else {
                return writeData_;
            }
        }

        static Packet * current() { return current_; }

    private:
        uint8_t * writeData_;

        friend void enqueue(Packet * p);
        friend Packet * transmitNextPacket();

        // currently transmitting packet
        static inline Packet * current_ = nullptr;
        // last packet in the queue, this is where next packets to transmit are appended
        static inline Packet * last_ = nullptr;
    }; // i2c::Packet

    inline void enqueue(Packet * p) {
        if (Packet::last_ == nullptr) {
            // TODO no ISR
            Packet::current_ = p;
            Packet::last_ = p;
            Packet::current_->transmit();
        } else {
            // TODO no ISR
            Packet::last_->next = p;
            Packet::last_ = p;
        }
    }

    inline Packet * transmitNextPacket() {
        Packet * c = Packet::current_;
        ASSERT(c != nullptr);
        if (c->next != nullptr) {
            Packet::current_ = c->next;
            Packet::current_->transmit();
        } else {
            Packet::current_ = nullptr;
            Packet::last_ = nullptr;
        }
        return c;
    }

    template<typename T>
    void sendCommand(T const & cmd) {
        Packet * p = new Packet(
            RCKID_AVR_I2C_ADDRESS, 
            sizeof(T), 
            reinterpret_cast<uint8_t const *>(& cmd)
        );
        enqueue(p);
    }

    inline void readResponse(uint8_t * into, uint8_t numBytes) {
        while (numBytes-- > 0)
            *(into++) = i2c0->hw->data_cmd;
    }

} // namespace rckid::i2c

