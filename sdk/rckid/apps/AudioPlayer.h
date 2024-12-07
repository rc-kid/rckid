#pragma once

#include <platform/buffer.h>
#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/audio/mp3.h>
#include <rckid/filesystem.h>
#include <rckid/ui/header.h>
#include <rckid/ui/carousel.h>
#include <rckid/assets/icons64.h>
#include <rckid/assets/icons24.h>

namespace rckid {

    /** MP3 Audio Player
     
        Because there is not enough memory for a full screen framebuffer, we cheat and split the screen in three thirds: the header, the current playback info and the carousel for music selection in the bottom part of the screen. We this switch and each frame only draw the part of the screen that is necessary. 
     */
    class AudioPlayer : public GraphicsApp<Canvas<ColorRGB>> {
    public:

        static void run() {
            AudioPlayer app{};
            app.loop();
            filesystem::unmount();
        }

    protected:

        class Item : public MenuItem {
        public:

            Item(filesystem::Entry const & e):
                MenuItem(e.isFolder() ? PAYLOAD_FOLDER : PAYLOAD_FILE) {
                text_ = e.name();
            }

            bool isFile() const { return payload() == PAYLOAD_FILE; }


            std::string const & filename() const { return text_; }    

            void text(std::string & text) const override {
                text = filesystem::stem(text_); 
            }

            bool icon(Bitmap<ColorRGB> &bmp) const override {
                if (isFile())
                    bmp.loadImage(PNG::fromBuffer(assets::icons64::music_note));
                else
                    bmp.loadImage(PNG::fromBuffer(assets::icons64::folder));
                // TODO actually return icon for mp3 or folder
                return true;
            }

        private:
            static constexpr uint32_t PAYLOAD_FILE = 0;
            static constexpr uint32_t PAYLOAD_FOLDER = 1;

            std::string text_;
        };

        AudioPlayer(): 
            GraphicsApp{ARENA(Canvas<ColorRGB>{320, 80})}, 
            out_{ARENA(DoubleBuffer<int16_t>{1152 * 4})} {
            filesystem::mount();
            play_.loadImage(PNG::fromBuffer(assets::icons64::play));
            pause_.loadImage(PNG::fromBuffer(assets::icons64::pause));
            repeatIcon_.loadImage(PNG::fromBuffer(assets::icons24::multimedia));

            loadFolder("audio");
        }

        ~AudioPlayer() {
            audioOff();
            if (mp3_ != nullptr)
               delete mp3_;
        }

        void update() override {
            // stop playing if we are finished
            if (mp3_ != nullptr && mp3_->eof()) {
                if (repeat_) {
                    stop();
                } else {
                    do {
                        moveRight();
                    } while (! currentItem()->isFile());
                }
                playAfterAnimation_ = true;
            }

            if (btnPressed(Btn::Home)) {
                if (screenOff_) {
                    screenOff_ = false;
                    displaySetBrightness(128);
                    rckid::ledSetEffects(
                        RGBEffect::Rainbow(0, 1, 1, 32), 
                        RGBEffect::Rainbow(51, 1, 1, 32), 
                        RGBEffect::Rainbow(102, 1, 1, 32), 
                        RGBEffect::Rainbow(153, 1, 1, 32), 
                        RGBEffect::Rainbow(204, 1, 1, 32)
                    );

                } else if (mp3_ != nullptr) {
                    screenOff_ = true;
                    displaySetBrightness(0);
                    rckid::ledsOff();
                }
            }

            // other keys don't work when screen is off
            if (screenOff_ == false) {
                if (btnPressed(Btn::Left)) {
                    moveLeft();
                    rumbleNudge();
                    playAfterAnimation_ = false;
                }
                if (btnPressed(Btn::Right)) {
                    moveRight();
                    rumbleNudge();
                    playAfterAnimation_ = false;
                }
                // A button either toggles pause, if playing, or starts playback if current item is file, or enters directory if current item is dir
                if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                    if (mp3_ != nullptr) {
                        audioPause();
                        frame_ = 0; // so that we change the icon quickly
                    } else {
                        rumbleNudge();
                        Item * i = currentItem();
                        if (i->isFile())
                            play(filesystem::join(path_, i->filename()));
                        else
                            loadFolder(filesystem::join(path_, i->filename()));
                    }
                    playAfterAnimation_ = false;
                }
                // if we pressed the back button, then if playing, stop playing, or see if we can go back
                if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                    if (mp3_ != nullptr) {
                        stop();
                    } else {
                        path_ = filesystem::parent(path_);
                        if (path_ != "/")
                            loadFolder(path_, /* down */true);
                        else
                            exit();
                        rumbleNudge();
                    }
                    btnPressedClear(Btn::B);
                    playAfterAnimation_ = false;
                }
                if (btnPressed(Btn::Start)) {
                    repeat_ = ! repeat_;
                    frame_ = 0;
                }
                GraphicsApp::update();
            }
            if (playAfterAnimation_ && carousel_.idle()) {
                playAfterAnimation_ = false;
                play(filesystem::join(path_, currentItem()->filename()));
            }
        }

        void stop() {
            if (mp3_ != nullptr) {
                audioStop();
                if (mp3_ != nullptr)
                    delete mp3_;
                mp3_ = nullptr;
                f_.close();
                memoryFree();
                clearPlayback_ = true;
            }
        }

        void play(std::string const & file) {
            stop();
            f_ = filesystem::fileRead(file);
            mp3_ = new MP3{ &f_};
            memoryFree();
            audioOn();
            mp3_->play(out_);
            memoryFree();
            frame_ = 0;
        }

        void moveLeft() {
            stop();
            i_ = (i_ == 0) ? files_.size() - 1 : i_ - 1;
            carousel_.moveLeft(files_[i_]);    
        }

        void moveRight() {
            stop();
            ++i_;
            if (i_ >= files_.size())
                i_ = 0;
            carousel_.moveRight(files_[i_]);    
        }

        void draw() override {
            NewArenaScope _{};
            if (clearPlayback_) {
                g_.fill();
                displaySetUpdateRegion(Rect::XYWH(0, 80, 320, 80));
                frame_ = 0;
                clearPlayback_ = false;
                return;
            }
            if (mp3_ != nullptr) {
                if (screenOff_)
                    return;
                switch (frame_++ % 30) {
                    case 0:
                        g_.fill();
                        displaySetUpdateRegion(Rect::XYWH(0, 0, 320, 80));
                        Header::drawOn(g_);
                        break;
                    case 1:
                        g_.fill();
                        displaySetUpdateRegion(Rect::XYWH(0, 80, 320, 80));
                        g_.blit(Point{128, 0}, audioPaused() ? pause_ : play_);
                        if (repeat_)
                            g_.blit(Point{195, 20}, repeatIcon_);
                        // draw here that we are playing and how much and repeat
                        break;
                    default:
                        // don't draw anything - nothing changes   
                        break;                 
                }
                /*
                g_.text(10, 30) << "Last errror:  " << mp3_->lastError() << "\n"
                                << "Channels:     " << mp3_->channels() << "\n"
                                << "Sample rate:  " << mp3_->sampleRate() << "\n"
                                << "Bitrate:      " << mp3_->bitrate() << "\n"
                                << "Frames:       " << mp3_->frames() << "\n"
                                << "Frame errors: " << mp3_->frameErrors(); 
                */
            } else {
                g_.fill();
                switch (frame_++ % 60) {
                    case 0:
                        displaySetUpdateRegion(Rect::XYWH(0, 0, 320, 80));
                        Header::drawOn(g_);
                        break;
                    case 1:
                        displaySetUpdateRegion(Rect::XYWH(0, 160, 320, 80));
                        [[fallthrough]];
                    default:
                        carousel_.drawOn(g_, Rect::XYWH(0, 0, 320, 80));
                        break;
                }
            }
        }

        Item * currentItem() { return (Item*) & files_[i_]; }

        std::string path_;
        Menu files_;
        std::vector<uint32_t> indices_;
        uint32_t i_;
        Carousel carousel_{assets::font::OpenDyslexic48::font};

        DoubleBuffer<int16_t> out_; 
        MP3 * mp3_ = nullptr;
        filesystem::FileRead f_;

        bool repeat_ = false;
        bool screenOff_ = false;
        bool playAfterAnimation_ = false;
        uint32_t frame_ = 0;
        Bitmap<ColorRGB> play_{64, 64};
        Bitmap<ColorRGB> pause_{64, 64};
        Bitmap<ColorRGB> repeatIcon_{24, 24};
        bool clearPlayback_ = false;

        bool loadFolder(std::string const & dir, bool down = false) {
            filesystem::Folder f = filesystem::folderRead(dir);
            if (!f.good())
                return false;
            files_.clear();
            for (auto const & i : f)
                files_.add(new Item{i});
            path_ = std::move(dir);
            if (down) {
                i_ = indices_.back();
                indices_.pop_back();
                carousel_.moveDown(files_[i_]);
            } else {
                indices_.push_back(i_);
                i_ = 0;
                carousel_.moveUp(files_[i_]);
            }
            return true;
        }
        

    }; // rckid::AudioPlayer

} // namespace rckid