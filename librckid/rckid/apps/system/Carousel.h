#pragma once

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/graphics/animation.h"
#include "rckid/graphics/png.h"
#include "rckid/graphics/bitmap.h"

namespace rckid {

    class Carousel : public App<FrameBuffer> {
    public:
        class Item {
        public:

            Point imageOffset() const { 
                return Point{
                    160 - img_.width() / 2, 
                    120 - img_.height() / 2
                };
            }

            Point textOffset() const {
                return Point{
                    160 - textWidth_ / 2,
                    200
                };
            }

            std::string const & text() const { return text_; }

            Bitmap<ColorRGB> const & img() const { return img_; }

            void set(PNG & img, char const * text) {
                text_ = text;
                //img_.loadImage(img);
            }

            void set(PNG && img, char const * text) {
                text_ = text;
                //img_.loadImage(img);
            }

        private:

            friend class Carousel;

            void swapWith(Item & other) {
                std::swap(text_, other.text_);
                std::swap(textWidth_, other.textWidth_);
                //img_.swapWith(other.img_);
            }

            std::string text_;
            Bitmap<ColorRGB> img_{0,0};
            int textWidth_{0}; 
        }; 

        Carousel():
            numItems_{0} {
        }

    protected:

        virtual void getItem(size_t index, Item & item) = 0;

        void onFocus(BaseApp * previous) override {
            App::onFocus(previous);
            getItem(i_, current_);
            current_.textWidth_ = renderer().textWidth(current_.text());
            renderer().setFg(ColorRGB::White());
        }

        void onBlur(BaseApp * next) override {
            App::onBlur(next);
        }

        void update() override {
            if (dir_ == Btn::Home && numItems_ > 0) {
                if (pressed(Btn::Left)) {
                    dir_ = Btn::Left;
                    i_ = (i_ == 0) ? (numItems_ - 1) : (i_ - 1);
                    LOG("Moving left");
                } else if (pressed(Btn::Right)) {
                    dir_ = Btn::Right;
                    ++i_;
                    if (i_ == numItems_)
                        i_ = 0;
                    LOG("Moving right");
                } else {
                    // TODO back
                    return;
                }
                getItem(i_, other_);
                other_.textWidth_ = renderer().textWidth(other_.text());
                a_.start();
                /*
                if (current_.img_.rawPixels() == nullptr)
                    FATAL_ERROR(10);
                if (other_.img_.rawPixels() == nullptr)
                    FATAL_ERROR(12);
                */
            }
        }

        void draw() override {
            Renderer & r = renderer();
            r.fill();
            a_.update();
            if (dir_ == Btn::Home) {
                // TODO draw the current item only
                //r.draw(current_.img(), current_.imageOffset());
                r.text(current_.textOffset()) << current_.text();
            } else {
                //int xImg = a_.interpolate(0, 320);
                int xText = a_.interpolate(0, 640);
                switch (dir_) {
                    case Btn::Left: {
                        //r.draw(current_.img(), current_.imageOffset() + Point{xImg, 0});
                        r.text(current_.textOffset() + Point{xText, 0}) << current_.text();
                        //r.draw(other_.img(), other_.imageOffset() - Point{320 - xImg, 0});
                        r.text(other_.textOffset() - Point{640 - xText, 0}) << other_.text();
                        break;
                    }
                    case Btn::Right:
                        //r.draw(current_.img(), current_.imageOffset() - Point{xImg, 0});
                        r.text(current_.textOffset() - Point{xText, 0}) << current_.text();
                        //r.draw(other_.img(), other_.imageOffset() + Point{320 - xImg, 0});
                        r.text(other_.textOffset() + Point{640 - xText, 0}) << other_.text();
                        break;
                    default:
                        break;
                }
                if (! a_.running()) {
                    // TODO swap other and current 
                    dir_ = Btn::Home;
                    current_.swapWith(other_);
                }
            }
        }

        void setNumItems(size_t n) {
            numItems_ = n;
            i_ = 0;
            getItem(i_, current_);
        }


    private:

        size_t numItems_ = 0;
        size_t i_ = 0;

        Btn dir_ = Btn::Home;

        Item current_;
        Item other_;

        Animation a_{500};

    }; // rckid::Carousel

    class Menu : public Carousel {
    public:

        Menu() {
            setNumItems(10);
        }

    protected:

        void getItem(size_t index, Item & item) {
            static bool x;
            item.set(BaseApp::imgX(), x ? "First Application" : "Second Is The Charm");
            x = !x;
        }

    }; // rckid::Menu

} // namespace rckid