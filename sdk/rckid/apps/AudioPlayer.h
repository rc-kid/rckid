#pragma once

#include <platform/buffer.h>
#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/audio/mp3.h>
#include <rckid/filesystem.h>
#include <rckid/ui/header.h>

namespace rckid {

    class AudioPlayer : public GraphicsApp<Canvas<ColorRGB>> {
    public:

        static void run() {
            AudioPlayer app{};
            app.loop();
        }

    protected:

        AudioPlayer(): 
            GraphicsApp{ARENA(Canvas<ColorRGB>{320, 240})}, 
            out_{ARENA(DoubleBuffer<int16_t>{1152 * 2})} {
            filesystem::mount();
            //LOG("128kbps.mp3: " << filesystem::hash("128kbps.mp3"));
            //yield();
            //LOG("160kbps.mp3: " << filesystem::hash("160kbps.mp3"));
            //yield();
            //LOG("192kbps.mp3: " << filesystem::hash("192kbps.mp3"));
            //yield();
            //LOG("320kbps.mp3: " << filesystem::hash("320kbps.mp3"));
            //yield();
        }

        ~AudioPlayer() {
            audioStop();
            if (mp3_ != nullptr)
               delete mp3_;
        }

        void update() override {
            GraphicsApp::update();
            if (btnPressed(Btn::A)) {
                audioStop();
                if (mp3_ != nullptr)
                    delete mp3_;
                f_ = filesystem::fileRead("128kbps.mp3");
                mp3_ = new MP3{ &f_};
                memoryFree();
                mp3_->play(out_);
            }
        }

        void draw() override {
            NewArenaScope _{};
            g_.fill();
            Header::drawOn(g_);
            if (mp3_ != nullptr) {
                g_.text(10, 30) << "Last errror:  " << mp3_->lastError() << "\n"
                                << "Channels:     " << mp3_->channels() << "\n"
                                << "Sample rate:  " << mp3_->sampleRate() << "\n"
                                << "Bitrate:      " << mp3_->bitrate() << "\n"
                                << "Frames:       " << mp3_->frames() << "\n"
                                << "Frame errors: " << mp3_->frameErrors(); 
            }
        }

        DoubleBuffer<int16_t> out_; 
        MP3 * mp3_ = nullptr;
        filesystem::FileRead f_;

    }; // rckid::AudioPlayer

} // namespace rckid