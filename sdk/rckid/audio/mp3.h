#pragma once

#include <libhelix-mp3/mp3dec.h>

#include "../utils/stream.h"

namespace rckid {

    /** MP3 Decoder. 
     
        Takes stream 
     */
    class MP3 {
    public:
        MP3(ReadStream * input):
            in_{input}, 
            buffer_{new uint8_t[BUFFER_SIZE]},
            dec_{MP3InitDecoder()} {
        }

        ~MP3() {
            delete buffer_;
            MP3FreeDecoder(dec_);
        }

        void play(DoubleBuffer<int16_t> & sampleBuffer) {
            audioPlay(sampleBuffer, 44100, [this](int16_t * buffer, [[maybe_unused]] uint32_t samples) {
                uint32_t res = 0;
                while (res == 0) {
                    res = decodeNextFrame(buffer);
                    if (res == 0 && eof()) {
                        audioStop();
                        return res / 2;
                    }
                }
                yield();
                return res / 2;
            });
        }

        /** Decodes next frame in the mp3 stream and returns the number of samples read. Returning 0 signifies eof, or an error.
         */
        uint32_t decodeNextFrame(int16_t * out) {
            // we always start at the beginning of the buffer 
            int sw = -1;
            while (true) {
                sw = MP3FindSyncWord(buffer_, bufferSize_);
                // if we have found the sync word, progress to decoding
                if (sw != -1)
                    break;
                // otherwise read next chunk - we can completely throw what we have in the buffer because if there is no sync word, no need to worry about it. This should definitely be enough to hold an mp3 frame
                bufferSize_ = in_->read(buffer_, 512 * 3);
                //TRACE_MP3("buffer reset: " << bufferSize_);
                // if nothing was read, we are done decoding
                if (bufferSize_ == 0) {
                    eof_ = true;
                    return 0;
                }
            }
            //TRACE_MP3("Sync word at " << sw << ", bufferSize " << bufferSize_);
            uint8_t * buf;
            int remaining;
            // we have found sync word, try decoding starting from the sync word
            while (true) {
                buf = buffer_ + sw;
                remaining = bufferSize_ - sw;
                TRACE_MP3("Before: " << platform::hash(buffer_, bufferSize_));
                err_ = MP3Decode(dec_, & buf,  & remaining, out, 0);
                TRACE_MP3("      : " << platform::hash(buffer_, bufferSize_));
                TRACE_MP3("Decoding: " << err_ << " buffer hash " << platform::hash(buffer_ + sw, bufferSize_ - sw - remaining) << ", output hash " << platform::hash((uint8_t *)out, 1152 * 4) << " remaining: " << remaining << ", sw: " << sw << ", bs: " << bufferSize_);
                // no error, we are done
                if (err_ == ERR_MP3_NONE)
                    break;
                // input data underflow, read some more and try again - if refill buffer returns 0, we can't really refill thebufer and should terminate as there is no more data
                if (err_ == ERR_MP3_INDATA_UNDERFLOW) {
                    if (refillBuffer() > 0)
                        continue;
                }
                ++frameErrors_;
                TRACE_MP3("frame error: " << err_ << ", remaining " << remaining);
                //return 0;
                if (remaining == static_cast<int>(bufferSize_ - sw))
                    remaining -= 3; // MP3 sync word length 
                if (remaining < 0)
                    remaining = 0;
                // we have decoding error on our hands, hopefully this will be reflected in the samples
                break;
            }
            ++frames_;
            // if there are data remaining, we must copy them to the beginning of the buffer 
            if (remaining > 0)
                memmove(buffer_, buf, remaining);
            bufferSize_ = remaining;
            // and return the number of samples created
            MP3GetLastFrameInfo(dec_, &fInfo_);
            return fInfo_.outputSamps;
        }

        uint32_t refillBuffer() {
            uint32_t max = BUFFER_SIZE - bufferSize_;
            if (max > 512)
                max = max - (max % 512);
            uint32_t rd = in_->read(buffer_ + bufferSize_, max);
            TRACE_MP3("buffer refill " << rd << " bytes (max " << max << "), from index " << bufferSize_ << ", hash " << platform::hash(buffer_ + bufferSize_, rd));
            if (rd == 0)
                eof_ = true;
            bufferSize_ += rd;
            return rd;
        }

        int lastError() const { return err_; }

        bool eof() const { return eof_ && bufferSize_ == 0; }

        uint32_t frames() const { return frames_; }

        uint32_t frameErrors() const { return frameErrors_; }

        uint32_t channels() const { return fInfo_.nChans; }

        uint32_t sampleRate() const { return fInfo_.samprate; }

        uint32_t bitrate() const { return fInfo_.bitrate; }

    private:

        static constexpr uint32_t BUFFER_SIZE = 2048;
        ReadStream * in_;
        uint8_t * buffer_;
        uint32_t bufferSize_ = 0;
        HMP3Decoder dec_;
        MP3FrameInfo fInfo_;       
        int err_; 
        uint32_t frameErrors_ = 0;
        uint32_t frames_ = 0;
        bool eof_ = false;
    }; // rckid::MP3

} // namespace rckid

