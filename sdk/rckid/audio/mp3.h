#pragma once

#include <libhelix-mp3/mp3dec.h>

#include "audio.h"
#include "../utils/stream.h"

namespace rckid {

    class MP3Stream : public AudioStream {
    public:
        MP3Stream(ReadStream & input):
            AudioStream{1152 * 4},
            in_{input}, 
            buffer_{new uint8_t[MP3_BUFFER_SIZE]},
            dec_{MP3InitDecoder()} {
            // initially fill the buffer
            bufferSize_ = in_.read(buffer_, MP3_BUFFER_SIZE);
            // and skip the ID3 tag, if any - this is necessary for the decoder to work properly
            skipID3v2Tags();
            // get next frame infor for sample rate
            int32_t sw = ensureFrameInBuffer();
            err_ = MP3GetNextFrameInfo(dec_, &fInfo_, buffer_ + sw);
        }

        MP3Stream(MP3Stream const &) = delete;

        ~MP3Stream() override {
            delete [] buffer_;
            MP3FreeDecoder(dec_);
        }

        uint32_t refillSamples(int16_t * buffer, [[maybe_unused]] uint32_t numSamples) override {
            // we don't expect to be called with less free space than one full frame
            ASSERT(numSamples >= 1152);
            uint32_t res = 0;
            while (res == 0) {
                res = decodeNextFrame(buffer);
                if (res == 0 && eof()) {
                    audioStop();
                    return res / 2;
                }
            }
            int32_t crc = 0;
            for (uint32_t i = 0; i < res; ++i)
                crc += buffer[i];
            return res / 2;
        };

        uint32_t sampleRate() const override { 
            return fInfo_.samprate; 
        }

        int lastError() const { return err_; }

        bool eof() const { return eof_ && bufferSize_ == 0; }

        uint32_t frames() const { return frames_; }

        uint32_t frameErrors() const { return frameErrors_; }

        uint32_t channels() const { return fInfo_.nChans; }

        uint32_t bitrate() const { return fInfo_.bitrate; }

    protected:

        void skipID3v2Tags() {
            // check for ID3v2 tag at the beginning of the stream
            if (bufferSize_ >= 10 && buffer_[0] == 'I' && buffer_[1] == 'D' && buffer_[2] == '3') {
                // size is stored in bytes 6-9 as "synchsafe integers"
                uint32_t tagSize = (static_cast<uint32_t>(buffer_[6] & 0x7f) << 21) |
                                   (static_cast<uint32_t>(buffer_[7] & 0x7f) << 14) |
                                   (static_cast<uint32_t>(buffer_[8] & 0x7f) << 7)  |
                                   (static_cast<uint32_t>(buffer_[9] & 0x7f) << 0);
                tagSize += 10; // include header size
                LOG(LL_MP3, "ID3v2 tag detected, size " << tagSize);
                while (tagSize > 0) {
                    if (tagSize > bufferSize_) {
                        tagSize -= bufferSize_;
                        bufferSize_ = in_.read(buffer_, MP3_BUFFER_SIZE);
                    } else {
                        // tag is fully in the buffer
                        memmove(buffer_, buffer_ + tagSize, bufferSize_ - tagSize);
                        bufferSize_ -= tagSize;
                        tagSize = 0;
                    }
                }
            }
        }

        int32_t ensureFrameInBuffer() {
            // we always start at the beginning of the buffer 
            int32_t sw = -1;
            while (! eof_) {
                sw = MP3FindSyncWord(buffer_, bufferSize_);
                // if we have found the sync word, progress to decoding
                if (sw != -1)
                    break;
                // otherwise read next chunk - we can completely throw what we have in the buffer because if there is no sync word, no need to worry about it. This should definitely be enough to hold an mp3 frame
                bufferSize_ = in_.read(buffer_, 512 * 3);
                //TRACE_MP3("buffer reset: " << bufferSize_);
                // if nothing was read, we are done decoding
                if (bufferSize_ == 0) {
                    eof_ = true;
                    return -1;
                }
            }
            return sw;
        }

        /** Decodes next frame in the mp3 stream and returns the number of samples read. Returning 0 signifies eof, or an error.
         */
        uint32_t decodeNextFrame(int16_t * out) {
            int32_t sw = ensureFrameInBuffer();
            if (sw == -1)
                return 0;
            LOG(LL_MP3, "Sync word at " << (int32_t)sw << ", bufferSize " << bufferSize_);
            uint8_t * buf;
            int remaining;
            // we have found sync word, try decoding starting from the sync word
            while (true) {
                buf = buffer_ + sw;
                remaining = bufferSize_ - sw;
                LOG(LL_MP3, "Before: " << platform::hash(buffer_, bufferSize_));
                err_ = MP3Decode(dec_, & buf,  & remaining, out, 0);
                LOG(LL_MP3, "      : " << platform::hash(buffer_, bufferSize_));
                LOG(LL_MP3, "Decoding: " << (int32_t)err_ << " buffer hash " << platform::hash(buffer_ + sw, bufferSize_ - sw - remaining) << ", output hash " << platform::hash((uint8_t *)out, 1152 * 4) << " remaining: " << (int32_t)remaining << ", sw: " << (int32_t)sw << ", bs: " << bufferSize_);
                // input data underflow, read some more and try again - if refill buffer returns 0, we can't really refill thebufer and should terminate as there is no more data
                // the same goes for free bitrate sync errors, just load more data so that the sync word can be found
                switch (err_) {
                    case ERR_MP3_NONE:
                        ++frames_;
                        // if there are data remaining, we must copy them to the beginning of the buffer 
                        removeConsumedBytes(buf, remaining);
                        refillBuffer();
                        // and return the number of samples created
                        MP3GetLastFrameInfo(dec_, &fInfo_);
                        if (fInfo_.nChans == 1) {
                            audio::convertToStereo(out, fInfo_.outputSamps);
                            return fInfo_.outputSamps * 2;
                        } else {
                            return fInfo_.outputSamps;
                        }
                        break;
                    case ERR_MP3_INDATA_UNDERFLOW:
                        if (refillBuffer() > 0)
                            continue;
                        break;
                    case ERR_MP3_FREE_BITRATE_SYNC:
                        removeConsumedBytes(buf, remaining);
                        refillBuffer();
                        sw = MP3FindSyncWord(buffer_, bufferSize_);
                        if (sw != -1)
                            continue;
                        break;
                    default:
                        break;
                }
                ++frameErrors_;
                LOG(LL_MP3, "frame error: " << (int32_t)err_ << ", remaining " << (int32_t)remaining);
                //return 0;
                if (remaining == static_cast<int>(bufferSize_ - sw))
                    remaining -= 3; // MP3 sync word length 
                if (remaining < 0)
                    remaining = 0;
                // we have decoding error on our hands, hopefully this will be reflected in the samples
            }
            UNREACHABLE;
            return 0;
        }

        void removeConsumedBytes(uint8_t * buf, int remaining) {
            if (remaining > 0)
                memmove(buffer_, buf, remaining);
            bufferSize_ = remaining;
        }

        uint32_t refillBuffer() {
            uint32_t max = MP3_BUFFER_SIZE - bufferSize_;
            if (max > 512)
                max = max - (max % 512);
            uint32_t rd = in_.read(buffer_ + bufferSize_, max);
            LOG(LL_MP3, "buffer refill " << rd << " bytes (max " << max << "), from index " << bufferSize_ << ", hash " << platform::hash(buffer_ + bufferSize_, rd));
            if (rd == 0)
                eof_ = true;
            bufferSize_ += rd;
            return rd;
        }


    private:
    
        ReadStream & in_;

        static constexpr uint32_t MP3_BUFFER_SIZE = 2048;
        uint8_t * buffer_;
        uint32_t bufferSize_ = 0;
        HMP3Decoder dec_;
        MP3FrameInfo fInfo_;       

        int err_; 
        uint32_t frameErrors_ = 0;
        uint32_t frames_ = 0;
        bool eof_ = false;

    }; // rckid::MP3Player

} // namespac rckid