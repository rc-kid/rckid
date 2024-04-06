#pragma once

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/graphics/animation.h"
#include "rckid/graphics/png.h"
#include "rckid/graphics/bitmap.h"

namespace rckid {

    /** Carousel for selecting menu items from a predefined list. 
     
        To conserve memory, the carousel uses only 80 rows, which at full color gives us 51.2k memory usage. 
     */
    class Carousel : public App<FrameBuffer<ColorRGB>> {
    public:
        class Item {
        public:

            Point imageOffset() const { 
                return Point{
                    160 - img_.width() / 2, 
                    40 - img_.height() / 2
                };
            }

            Point textOffset() const {
                return Point{
                    160 - textWidth_ / 2,
                    40
                };
            }

            std::string const & text() const { return text_; }

            Bitmap<Color> const & img() const { return img_; }

            void set(PNG & img, char const * text) {
                text_ = text;
                img_.loadImage(std::move(img));
            }

            void set(PNG && img, char const * text) {
                text_ = text;
                img_.loadImage(std::move(img));
            }

        private:

            friend class Carousel;

            /*
            void swapWith(Item & other) {
                std::swap(text_, other.text_);
                std::swap(textWidth_, other.textWidth_);
                std::swap(img_, other.img_);
                //img_.swapWith(other.img_);
            }
            */

            std::string text_;
            Bitmap<Color> img_{64, 64, MemArea::None};
            int textWidth_{0}; 
        }; 

        Carousel(): 
            App{320, 80},
            numItems_{0} {
        }

    protected:

       virtual void getItem(size_t index, Item & item) = 0;

        void onFocus() override {
            App::onFocus();;
            current_.img_.allocate(MemArea::VRAM);
            other_.img_.allocate(MemArea::VRAM);
            setNumItems(10);
            getItem(i_, current_);
            current_.textWidth_ = driver_.textWidth(current_.text());
            driver_.setFg(Color::White());
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
                other_.textWidth_ = driver_.textWidth(other_.text());
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
            driver_.fill();
            a_.update();
            if (dir_ == Btn::Home) {
                // TODO draw the current item only
                driver_.draw(current_.img(), current_.imageOffset());
                driver_.text(current_.textOffset()) << current_.text();
            } else {
                int xImg = a_.interpolate(0, 320);
                int xText = a_.interpolate(0, 640);
                switch (dir_) {
                    case Btn::Left: {
                        driver_.draw(current_.img(), current_.imageOffset() + Point{xImg, 0});
                        driver_.text(current_.textOffset() + Point{xText, 0}) << current_.text();
                        driver_.draw(other_.img(), other_.imageOffset() - Point{320 - xImg, 0});
                        driver_.text(other_.textOffset() - Point{640 - xText, 0}) << other_.text();
                        break;
                    }
                    case Btn::Right:
                        driver_.draw(current_.img(), current_.imageOffset() - Point{xImg, 0});
                        driver_.text(current_.textOffset() - Point{xText, 0}) << current_.text();
                        driver_.draw(other_.img(), other_.imageOffset() + Point{320 - xImg, 0});
                        driver_.text(other_.textOffset() + Point{640 - xText, 0}) << other_.text();
                        break;
                    default:
                        break;
                }
                if (! a_.running()) {
                    // TODO swap other and current 
                    dir_ = Btn::Home;
                    //std::swap(current_, other_); //current_.swapWith(other_);
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
        }

    protected:

        void getItem(size_t index, Item & item) {
            static bool x;
            item.set(BaseApp::appIcon(), x ? "First Application" : "Second Is The Charm");
            x = !x;
        }

    }; // rckid::Menu

} // namespace rckid