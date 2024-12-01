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
        }

        void update() override {
            GraphicsApp::update();
            if (btnPressed(Btn::A)) {
                if (mp3_ != nullptr)
                    delete mp3_;
                f_ = filesystem::fileRead("320kbps.mp3");
                mp3_ = new MP3{ &f_};
                mp3_->play(out_);
            }
        }

        void draw() override {
            g_.fill();
            Header::drawOn(g_);
            if (mp3_ != nullptr) {
                g_.text(10, 30) << "Last errror:  " << mp3_->lastError() << "\n"
                                << "Frames:       " << mp3_->frames() << "\n"
                                << "Frame errors: " << mp3_->frameErrors(); 
            }
        }

        DoubleBuffer<int16_t> out_; 
        MP3 * mp3_ = nullptr;
        filesystem::FileRead f_;

    }; // rckid::AudioPlayer

} // namespace rckid