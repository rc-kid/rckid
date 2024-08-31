#pragma once

#include <optional>

#include "../rckid.h"

namespace rckid {

    /** A simple double buffer.  

        For playback:
            - get the current chunk to be sent
            - swap
            - callback on refilling the buffer
            - initialize

        For recording:
            - callback on buffer filled, 
            - get the current chunk to be written to
            - swap


     */
    class DoubleBuffer {
    public:

        using SwapCallback = std::function<void(DoubleBuffer &)>;

        /** Creates the buffer of particular size.
         */
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
        uint8_t * getFontBuffer() {
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
        uint8_t * buffer_;
        uint32_t size_;
        bool state_;
        SwapCallback cb_;
    }; // rckid::DoubleBuffer

} // namespace rckid