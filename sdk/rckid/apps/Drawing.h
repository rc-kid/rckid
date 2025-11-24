#pragma once

#include "../app.h"
#include "../graphics/canvas.h"
#include "../ui/form.h"
#include "../ui/image.h"
#include "../ui/geometry.h"
#include "../assets/icons_64.h"

namespace rckid {

    /** Simple drawing app. 
     
        The point of the drawing app is to create icons and pixel art that can be stored on the device.

        Start switches focus between the image and the palette. Volume up & down change the zoom levels. 

     */
    class Drawing : public ui::Form<void> {
    public:

        String name() const override { return "Drawing"; }

        Drawing() : ui::Form<void>{} {
            // TODO implement
            g_.enableBgImage(false);
            g_.setBg(ColorRGB::Black());
            g_.addChild(image_);
            g_.addChild(zoomedImage_);

            blink_.startContinuous();
            zoomedImage_.focus();
        }

    protected:

        void focus() override {
            ui::Form<void>::focus();
            btnEnableRapidFire(true);
        }

        void blur() override {
            ui::Form<void>::blur();
            btnEnableRapidFire(false);
        }

        void update() override {
            ui::Form<void>::update();
        }

        void draw() override {
            // blink the cursor
            blink_.update();
            zoomedImage_.setCursorColor(ui::Style::accentFg().withAlpha(interpolation::cosineLoop(blink_, 0, 255).round()));

            ui::Form<void>::draw();
        }


        /** Canvas that allows editing & zooming and stuff.  */
        class ZoomedImage : public ui::Widget {
        public:

            ZoomedImage(Rect rect, ui::Image * source):
                ui::Widget{rect},
                source_{source},
                cursorPos_{0,0},
                cursor_{Rect::WH(5, 5)}
            {
                addChild(cursor_);

            }

            Point cursor() const { return cursorPos_; }

            void setCursor(Point p) {
                if (p.x < 0)
                    p.x = source_->bitmap()->width() - 1;
                else if (p.x >= source_->bitmap()->width())
                    p.x = 0;
                if (p.y < 0)
                    p.y = source_->bitmap()->height() - 1;
                else if (p.y >= source_->bitmap()->height())
                    p.y = 0;
                cursorPos_ = p;
                cursor_.setRect(Rect::XYWH(p.x * zoom_, p.y * zoom_, zoom_ + 2, zoom_ + 2));
            }

            ColorRGB cursorColor() const { return cursor_.color(); }

            void setCursorColor(ColorRGB color) { cursor_.setColor(color); }

        protected:

            void processEvents() override {
                ui::Widget::processEvents();
                if (btnPressed(Btn::Left))
                    setCursor(cursorPos_ - Point{1, 0});
                if (btnPressed(Btn::Right))
                    setCursor(cursorPos_ + Point{1, 0});
                if (btnPressed(Btn::Up))
                    setCursor(cursorPos_ - Point{0, 1});
                if (btnPressed(Btn::Down))
                    setCursor(cursorPos_ + Point{0, 1});
                if (btnDown(Btn::A)) {
                    source_->bitmap()->setPixelAt(cursorPos_.x, cursorPos_.y, colorFg_);
                }
                if (btnDown(Btn::B)) {
                    source_->bitmap()->setPixelAt(cursorPos_.x, cursorPos_.y, colorBg_);
                }
            }

            void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
                for (Coord y = 0; y < numPixels; ++y) {
                    buffer[y] = colorAt(column, starty + y).toRaw();
                }
                Widget::renderColumn(column, buffer, starty, numPixels);
            }

            ColorRGB colorAt(Coord x, Coord y) const {
                if (x < 1 || y < 1 || x > 192 || y > 192)
                    return focused() ? ui::Style::bg() : ui::Style::accentBg();
                x -= 1;
                y -= 1;
                Coord xc = x / zoom_;
                Coord yc = y / zoom_;
                return source_->bitmap()->colorAt(xc, yc);
            }

        private:
            ui::Image * source_;
            Point cursorPos_;
            ui::Rectangle cursor_;
            uint32_t zoom_ = 3;
            uint16_t colorFg_ = ColorRGB::White().toRaw();
            uint16_t colorBg_ = ColorRGB::Black().toRaw();
        }; 

        /** Palette visualization. 
         
            Depending on the BPP, either displays the colors in the bitmap, of list of recently used colors. 
         */
        class Palette : public ui::Widget {

        }; 

    private:

        Timer blink_{1000};

        ui::Image image_{Rect::XYWH(0, 32, 96, 96), Icon{assets::icons_64::paint_palette}};
        ZoomedImage zoomedImage_{Rect::XYWH(108,32,194,194), &image_};
    

    }; // rckid::Drawing

} // namespace rckid 