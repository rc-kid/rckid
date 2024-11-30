#pragma once

#include "rckid/filesystem.h"
#include "rckid/audio/mp3.h"
#include "benchmark.h"

namespace rckid {

    /** A very simple barebones MP3 decoder benchmark. 
     
        Results on RCKid running at normal speed are:

        320kbps : 94.5%


        320 kbps:

        Total:     76_611_429
        SD read:    4_012_615
        Decode:    72_481_554
        Frames:         3_092
        Bytes read: 3_230_824

     */

    class MP3DecoderBenchmark {
    public:
        void run(Canvas<ColorRGB> & g_) {
            measure("128kbps.mp3");
            g_.text(0, 30) << 
                status_ << "\n\n" << 
                "MP3 Decodring [us]\n\n" <<
                "Total:      " << total_ << "\n" <<
                "SD read:    " << read_ << "\n" <<
                "Decode:     " << decode_ << "\n" <<
                "Frames:     " << frames_ << "\n" <<
                "Bytes read: " << bytesRead_ << "\n" << 
                "Error:      " << error_;
        }

        void measure(char const * filepath) {
            frames_ = 0;
            read_ = 0;
            decode_ = 0;
            bytesRead_ = 0;
            error_ = 0;
            status_ = "Working...";
            MEASURE_TIME(total_, {
                status_ = decodeFile(filepath);
            });
        }

        char const *  decodeFile(char const * filepath) {
            namespace fs = rckid::filesystem;
            if (!fs::mount())
                return "Cannot mount SD card";
            fs::FileRead f = fs::fileRead(filepath);
            if (!f.good())
                return "Cannot open file";
            NewArenaScope _{};
            uint8_t * buffer = new uint8_t[2048];
            uint32_t bufSize = 0;
            DoubleBuffer<int16_t> out{1152 * 2};
            bool eof = false;
            HMP3Decoder dec = MP3InitDecoder();
            while (!eof/* || bufSize > 0*/) {
                uint32_t t;
                MEASURE_TIME(t, {
                    if (!eof && bufSize < 1536) {
                        uint32_t x = f.read(buffer + bufSize, 1024);
                        bufSize += x;
                        bytesRead_ += x;
                        if (x == 0)
                            eof = true;
                    }
                });
                read_ += t;
                int32_t syncWord = MP3FindSyncWord(buffer, bufSize);
                if (syncWord == -1 && !eof)
                    continue;
                uint8_t * x = buffer + syncWord;
                int remaining = bufSize - syncWord;
                MEASURE_TIME(t, {
                    error_ = MP3Decode(dec, & x, & remaining, out.front(), 0);
                });
                decode_ += t;
                // not enough data in the buffer, read some more
                if (error_ == -1 && !eof)
                    continue; 
                if (error_)
                    return "Error decoding MP3 file";
                // move the remaining data in the buffer
                memcpy(buffer, x, remaining);
                bufSize = remaining;
                frames_ += 1;
            }
            return "All OK";
        }

        char const * status_;
        uint32_t total_;
        uint32_t frames_;
        uint32_t read_;
        uint32_t decode_;
        uint32_t bytesRead_;
        int error_;
    }; 


} // namespace rckid 