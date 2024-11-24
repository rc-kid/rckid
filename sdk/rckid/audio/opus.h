#pragma once

#include <libopus/include/opus.h>

#include "audio.h"


namespace rckid::opus {

    enum class FrameSize {
        ms2_5 = 1, 
        ms5 = 2, 
        ms10 = 4, 
        ms20 = 8, 
        ms40 = 16,
        ms60 = 24,
    };

    /** Returns the size of opus frame based on given sample rate, number of audio channels and desired frame size in milliseconds. This number is the number of int16_t samples in the frame. 
     */
    inline uint32_t frameSize(audio::SampleRate sr, audio::Channels ch, FrameSize fs) {
        return static_cast<uint32_t>(ch) * static_cast<uint32_t>(sr) / 400 * static_cast<uint32_t>(fs);
    }

    /** Decoder of raw opus packets. 
     
        The decoder is initialized with sample rate and number of channels expected. Then its decodePacket method can be called repeatedly only opus packets, which will be decoded into the provides output stream. 
    
     */
    class Decoder {
    public:
        Decoder(audio::SampleRate sr, audio::Channels channels) {
            int err;
            decoder_ = opus_decoder_create(static_cast<int32_t>(sr), static_cast<int32_t>(channels), &err);
            if (err != OPUS_OK) {
                LOG("Unable to create opus decoder, code: " << err);
                decoder_ = nullptr;
            }
        }

        ~Decoder() {
            opus_decoder_destroy(decoder_);
        }

        /** Takes raw packet and decodes it into provided buffer.
         
            This is just a super thin wrapper around the opus decoder. It leaves up to the user to ensure that the output buffer is large enough to hold the opus frame being decoded.
          */
        uint32_t decodePacket(uint8_t const * rawPacket, uint32_t inBytes, int16_t * out, uint32_t outBytes) {
            int result = opus_decode(decoder_, rawPacket, inBytes, out, outBytes, false); 
            if (result < 0)
                return 0;
            return static_cast<uint32_t>(result);
        }

    private:

        OpusDecoder * decoder_;
    }; 

    class Encoder {
    public:
        /** Creates new encoder with given sample rate, number of channels and desired bitrate. 
         */
        Encoder(audio::SampleRate sr, audio::Channels channels, uint32_t bitrate) {
            int err;
            encoder_ = opus_encoder_create(static_cast<int>(sr), static_cast<int>(channels), OPUS_APPLICATION_VOIP, &err);
            if (err != OPUS_OK) {
                LOG("Unable to create opus encoder, code: " << err);
                encoder_ = nullptr;
                return;
            }
            opus_encoder_ctl(encoder_, OPUS_SET_BITRATE(bitrate));                
        }

        ~Encoder() {
            opus_encoder_destroy(encoder_);
        }

        /** Encodes provided packet. 
         */
        uint32_t encodePacket(int16_t const * rawAudio, uint32_t samples, uint8_t * out, uint32_t outBytes) {
            int result = opus_encode(encoder_, rawAudio, samples, out, outBytes);
            if (result < 0)
                return 0;
            return static_cast<uint32_t>(result);
        }

    private:

        OpusEncoder * encoder_;

    }; // rckid::opus::Encoder




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


}