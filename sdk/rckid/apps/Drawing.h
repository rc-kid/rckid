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
            if (btnPressed(Btn::Left)) {
                Point p = zoomedImage_.cursor();
                zoomedImage_.setCursor(Point{p.x - 1, p.y});
            }
            if (btnPressed(Btn::Right)) {
                Point p = zoomedImage_.cursor();
                zoomedImage_.setCursor(Point{p.x + 1, p.y});
            }
            if (btnPressed(Btn::Up)) {
                Point p = zoomedImage_.cursor();
                zoomedImage_.setCursor(Point{p.x, p.y - 1});
            }
            if (btnPressed(Btn::Down)) {
                Point p = zoomedImage_.cursor();
                zoomedImage_.setCursor(Point{p.x, p.y + 1});
            }
        }

        void draw() override {
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
                    p.x = source_->width() - 1;
                else if (p.x >= source_->width())
                    p.x = 0;
                if (p.y < 0)
                    p.y = source_->height() - 1;
                else if (p.y >= source_->height())
                    p.y = 0;
                cursorPos_ = p;
                cursor_.setRect(Rect::XYWH(p.x * zoom_, p.y * zoom_, zoom_ + 2, zoom_ + 2));
            }

        protected:
            void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {

                for (Coord y = 0; y < numPixels; ++y) {
                    buffer[y] = colorAt(column, starty + y).toRaw();
                }
                Widget::renderColumn(column, buffer, starty, numPixels);
            }

            ColorRGB colorAt(Coord x, Coord y) const {
                if (x < 1 || y < 1 || x > 192 || y > 192)
                    return ui::Style::accentBg();
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

        }; 

    private:
        ui::Image image_{Rect::XYWH(0, 32, 64, 64), Icon{assets::icons_64::paint_palette}};
        ZoomedImage zoomedImage_{Rect::XYWH(108,32,194,194), &image_};
    

    }; // rckid::Drawing

} // namespace rckid 