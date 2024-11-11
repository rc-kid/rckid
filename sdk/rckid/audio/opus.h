#pragma once

#include <libopus/include/opus.h>

namespace rckid::opus {

    // TODO this is code from V1 and needs to be ported and updated to the latest SDK
    // kept here as a starting point

    /** Raw Opus Encoder
     
        The raw encoder is a specialzed direct opus codec encoder tuned for transmitting voice over the NRF packets. It wraps the opus packets of max 30 bytes with 2 bytes of extra information - the length of the opus packet and a packet index that can be used to detect multiple sends of the same packet as well as packet loss.
     */

    class RawEncoder {
    public:
        /** Creates new encoder. 
         
         */
        RawEncoder() {
            int err;
            encoder_ = opus_encoder_create(8000, 1, OPUS_APPLICATION_VOIP, &err);
            if (err != OPUS_OK) {
                LOG("Unable to create opus encoder, code: " << err);
                encoder_ = nullptr;
                return;
            }
            opus_encoder_ctl(encoder_, OPUS_SET_BITRATE(6000));                
            frame_[0] = 0;
            frame_[1] = 0;
        }

        ~RawEncoder() {
            LOG("Destroying opus encoder");
            opus_encoder_destroy(encoder_);
        }

        void reset() {
            opus_encoder_destroy(encoder_);
            int err;
            encoder_ = opus_encoder_create(8000, 1, OPUS_APPLICATION_VOIP, &err);
            if (err != OPUS_OK) {
                LOG("Unable to create opus encoder, code: " << err);
                encoder_ = nullptr;
                return;
            }
            opus_encoder_ctl(encoder_, OPUS_SET_BITRATE(6000));                
            frame_[0] = 0;
            frame_[1] = 0;
            buffer_.clear();
        }

        /** Encodes the provided buffer. 
         
            Returns true if there has been a new encoded frame created during the recording, in which case the call should be followed by sending the frame packet. 
         */
        bool encode(uint8_t const * data, size_t len) {
            bool newFrame = false;
            while (len-- > 0) {
                buffer_.push_back((static_cast<opus_int16>(*(data++)) - 128) * 256);
                if (buffer_.size() == RAW_FRAME_LENGTH) {
                    ++frame_[1];
                    int result = opus_encode(encoder_, buffer_.data(), buffer_.size(), frame_ + 2, sizeof(frame_) - 2);
                    if (result > 0) {
                        frame_[0] = result & 0xff;
                        newFrame = true;
                    } else {
                        LOG("Unable to encode frame, code:  " << result);
                    }
                    buffer_.clear();
                }
            }
            return newFrame;
        }

        /** Returns true if the current frame is valid, i.e. it has length of greater than 0. This will be always true after enough data for at least a single frame has been accumulated. 
         */
        bool currentFrameValid() const { return frame_[0] != 0; }

        /** Returns the encoded frame buffer.
         */
        unsigned char const * currentFrame() const {
            return frame_;
        }

        /** Returns the actual size of the opus frame encoded within the buffer, or 0 if the buffer is not valid at this time. 
        */
        size_t currentFrameSize() const {
            return frame_[0];
        }

        /** Returns the frame index. This is a monotonically increasing number wrapper around single byte so that we can send same packet multiple times for greater reach.
         */
        uint8_t currentFrameIndex() const {
            return frame_[1];
        }

    private:
        /** Number of samples in the unencoded frame. Corresponds to 8000Hz sample rate and 40ms window time, which at 6kbps gives us 30 bytes length encoded frame. 
         */
        static constexpr size_t RAW_FRAME_LENGTH = 320;
        OpusEncoder * encoder_ = nullptr;

        std::vector<opus_int16> buffer_;
        // 32 byte packets actually fit in single NRF message
        unsigned char frame_[32];
    }; // opus::RawEncoder

    /** Decoder ofthe raw packets encoded via the RawEncoder. 
     
        Together with the decoding also keeps track of packet loss and uses them to calculate approximate signal loss. 
     */
    class RawDecoder {
    public:
        RawDecoder() {
            int err;
            decoder_ = opus_decoder_create(8000, 1, &err);
            if (err != OPUS_OK) {
                LOG("Unable to create opus decoder, code: " << err);
                decoder_ = nullptr;
            }
        }

        ~RawDecoder() {
            opus_decoder_destroy(decoder_);
        }

        void reset() {
            opus_decoder_destroy(decoder_);
            int err;
            decoder_ = opus_decoder_create(8000, 1, &err);
            if (err != OPUS_OK) {
                LOG("Unable to create opus decoder, code: " << err);
                decoder_ = nullptr;
            }
            lastIndex_ = 0xff;
            missingPackets_ = 0;
            packets_ = 0;
        }

        size_t decodePacket(unsigned char const * rawPacket) {
            // if the packet index is the same as last one, it's a packet retransmit and there is nothing we need to do
            if (rawPacket[1] == lastIndex_)
                return 0;
            // if we have missed any packet, report it as lost
            while (++lastIndex_ != rawPacket[1])
                reportPacketLoss();
            // decode the packet and return the decoded size
            int result = opus_decode(decoder_, rawPacket + 2, rawPacket[0], buffer_, 320, false);
            if (result < 0) {
                LOG("Unable to decode packet, code: " << result);
                // TODO
            }
            ++packets_;
            return result;
        }

        opus_int16 const * buffer() const { return buffer_; }

        size_t missingPackets() const { return missingPackets_; }
        size_t packets() const { return packets_; }

    private:

        void reportPacketLoss() {
            ++missingPackets_;
            int result = opus_decode(decoder_, nullptr, 0, buffer_, 320, false);
            if (result < 0)
                LOG("Unable to decode missing packet, code: " << result);
        }

        OpusDecoder * decoder_;
        size_t missingPackets_{0};
        size_t packets_{0};
        opus_int16 buffer_[320];
        uint8_t lastIndex_{0xff};
    }; 

}