#pragma once

#include <cstdint>
#include <functional>

/** A very simple ring buffer implementation with static size. 
 
    TODO The ring buffer should implement the Stream input and output interfaces
    */
template<unsigned SIZE>
class RingBuffer {
public:

    bool empty() const { return canRead() == 0; }

    bool full() const { return canWrite() == 0; }

    void flush() { r_ = 0; w_ = 0; }

    /** Returns the number of bytes that can be read from the buffer without blocking. 
     */
    unsigned canRead() const {
        // TODO needs to be sycnhronized
        if (w_ >= r_)
            return w_ - r_;
        else
            return w_ + SIZE - r_;        
    }

    /** Returns the number of bytes that can be read from the ring buffer and that are placed in continous memory (i.e. breks at read index, or ring end)
     */
    unsigned canReadContinuous() const {
        if (w_ >= r_)
            return w_ - r_;
        else
            return SIZE - r_;
    }

    /** Returns the read buffer pointer from which up to canReadContinuous() bytes can be copied. 
     */
    uint8_t const * readBuffer() const { return buffer_ + r_; }

    /** Returns the number of bytes that can be written to the buffer without blocking. 
     */
    unsigned canWrite() const {
        // TODO needs to be synchronized
        if (r_ > w_)
            return r_ - w_;
        else
            return SIZE - w_ + r_ - 1;
    }

    /** Writes data to the buffer. 
     
        Returns the number of characters actually written. 
    */
    unsigned write(uint8_t const * buffer, unsigned numBytes) {
        unsigned num = std::min(canWrite(), numBytes);
        for (unsigned i = 0; i < num; ++i)
            write(buffer[i]);
        return num;
    }

    void write(uint8_t value) {
        buffer_[w_++] = value;
        w_ = w_ % SIZE;
    }

    /** Reads the given data from the buffer advancing the read pointer. 
     
        Returns the actual number of bytes transferred to the bufer, which must be smaller or equal to numBytes. 
    */
    unsigned read(uint8_t * buffer, unsigned numBytes) {
        unsigned num = std::min(canRead(), numBytes);
        for (unsigned i = 0; i < num; ++i)
            buffer[i] = read();
        return num;
    }

    uint8_t read() {
        uint8_t result = buffer_[r_++];
        r_ = r_ % SIZE;
        return result;
    }

    /** Reads up to numBytes from the buffer without advancing the read pointer (i.e. the same data can be read multiple times). 
     
        Returns the actual number of bytes read.
    */
    unsigned peek(uint8_t * buffer, unsigned numBytes) {
        unsigned num = std::min(canRead(), numBytes);
        unsigned r = r_;
        for (unsigned i = 0; i < num; ++i) {
            buffer[i] = buffer_[r++];
            r = r % SIZE;
        }
        return num;
    }

    uint8_t peek(unsigned offset = 0) const {
        return buffer_[(r_ + offset) % SIZE];
    }

    /** Moves the read pointer by numBytes. 
     
        Useful for advancing the read pointer after previous peeks. Returns the number of bytes actually advanced (if lower than numBytes). 
    */
    unsigned flush(unsigned numBytes) {
        unsigned num = std::min(canRead(), numBytes);
        r_ = (r_ + num) % SIZE;
        return num;
    }

private:
    uint8_t buffer_[SIZE];
    unsigned r_ = 0;
    unsigned w_ = 0;
}; // RingBuffer

/** A simple double buffer.  

 */
class DoubleBuffer {
public:

    using SwapCallback = std::function<void(DoubleBuffer &)>;

    /** Creates the buffer of particular size.
     */
    DoubleBuffer(uint32_t size):
        size_{size}, 
        buffer_{new uint8_t[size * 2]} {
    }

    DoubleBuffer(uint32_t size, SwapCallback cb):
        size_{size}, 
        buffer_{new uint8_t[size * 2]},
        cb_{cb} {
    }

    ~DoubleBuffer() {
        delete [] buffer_;
    }

    uint32_t size() { return size_; }

    /** Returns the current back buffer, i.e. the empty buffer that needs to be filled. 
     
        This is typically the buffer to which the app renders, or the buffer to which audio playback should store the waveform, or where audio recording should store the recorded data. 
        */
    uint8_t * getBackBuffer() {
        return state_ ? buffer_ : (buffer_ + size_);
    }

    /** Returns the current front buffer, i.e. the buffer containing valid data that is to be processed. 
     
        This typically involves a DMA transfer to peripherals in case of graphics or audio playback, or actual on-chip processing in case of audio recording. 
        */
    uint8_t * getFrontBuffer() {
        return state_ ? (buffer_ + size_) : buffer_;
    }

    /** Swaps the front and back buffers. 

        Executes the appropriate callbacks if provided.  
        */
    void swap() {
        state_ = ! state_;
        if (cb_)
            cb_(*this);
    }

private:
    uint32_t size_;
    uint8_t * buffer_;
    bool state_ = false;
    SwapCallback cb_;
}; // DoubleBuffer
