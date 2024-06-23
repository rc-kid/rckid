#pragma once

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
     */
    unsigned write(uint8_t const * buffer, unsigned numBytes) {
        unsigned num = std::min(canWrite(), numBytes);
        for (unsigned i = 0; i < num; ++i) {
            buffer_[w_++]  = buffer[i];
            w_ = w_ % SIZE;
        }
        return num;
    }

    unsigned read(uint8_t * buffer, unsigned numBytes) {
        unsigned num = std::min(canRead(), numBytes);
        for (unsigned i = 0; i < num; ++i) {
            buffer[i] = buffer_[r_++];
            r_ = r_ % SIZE;
        }
        return num;
    }

private:
    uint8_t buffer_[SIZE];
    unsigned r_ = 0;
    unsigned w_ = 0;
}; // RingBuffer
