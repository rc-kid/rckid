#pragma once

#include "rckid/rckid.h"
#include "audio.h"

#include "mp3dec.h"

namespace rckid {

    /** MP3 audio stream. 

        Uses libhelix for mp3 decoding. A simple test reveals that decoding a single mp3 frame at 320kbps (which is the maximum) takes about 14.5ms, while we need new frame every 26ms, which should be doable (at 44.1 kHz).  


        https://github.com/pschatzmann/arduino-libhelix/tree/main/src


     */
    class MP3 : public audio::OutStream  {
    public:

        MP3():
            mp3_{MP3InitDecoder()},
            buffer_{new uint16_t [2048]} {
        }

        ~MP3() {
            MP3FreeDecoder(mp3_);
        }

        /** Fills the buffer from the decoded MP3 file. 
         */
        void fillBuffer(uint16_t * buffer, size_t bufferSize) override {

        }


    protected:
        void decodeNext() {
            //MP3Decode(mp3_, )
        }

        HMP3Decoder mp3_;
        uint16_t * buffer_;

    }; // rckid::MP3



} // namespace rckid 