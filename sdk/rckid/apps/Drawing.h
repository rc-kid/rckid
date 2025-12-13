#pragma once

#include "../app.h"
#include "../graphics/canvas.h"
#include "../graphics/png.h"
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
            image_ = g_.addChild(new ui::Image{Rect::XYWH(22, 24, 64, 64),Bitmap{16, 16, 16}});
            editor_ = g_.addChild(new ZoomedEditor{Point{116, 32}, image_});
            palette_ = g_.addChild(new Palette{Point{8, 128}});
            fg_ = g_.addChild(new ui::Panel{Rect::XYWH(16,96,32,24)});
            bg_ = g_.addChild(new ui::Panel{Rect::XYWH(48 + 16,96,32,24)});
            fg_->setBg(ui::Style::accentBg());
            bg_->setBg(ui::Style::accentBg());

            blink_.startContinuous();
            editor_->focus();
            image_->bitmap()->fill(palette_->bg().toRaw());
        }

    protected:

        void focus() override {
            ui::Form<void>::focus();
            //btnEnableRapidFire(true);
        }

        void blur() override {
            ui::Form<void>::blur();
            //btnEnableRapidFire(false);
        }

        void update() override {
            ui::Form<void>::update();
            if (btnPressed(Btn::Start)) {
                if (editor_->focused()) {
                    palette_->focus();
                } else {
                    editor_->focus();
                }
            }
            if (btnPressed(Btn::Select)) {
                ui::ActionMenu popup{
                    ui::ActionMenu::Item("Clear", [&](){
                        image_->bitmap()->fill(palette_->bg().toRaw());
                    }),
                    ui::ActionMenu::Item("New small (8x8)", [&](){
                        Bitmap b{8, 8, 16};
                        b.fill(palette_->bg().toRaw());
                        setBitmap(std::move(b));
                    }),
                    ui::ActionMenu::Item("New medium (16x16)", [&](){
                        Bitmap b{16, 16, 16};
                        b.fill(palette_->bg().toRaw());
                        setBitmap(std::move(b));
                    }),
                    ui::ActionMenu::Item("New large (32x32)", [&](){
                        Bitmap b{32, 32, 16};
                        b.fill(palette_->bg().toRaw());
                        setBitmap(std::move(b));
                    }),
                    ui::ActionMenu::Item("New huge (64x64)", [&](){
                        Bitmap b{64, 64, 16};
                        b.fill(palette_->bg().toRaw());
                        setBitmap(std::move(b));
                    }),
                    ui::ActionMenu::Item("Load", [&](){
                        auto icon = App::run<FileDialog>("Select friend icon", "/files/icons");
                        if (icon.has_value()) {
                            PNG decoder = PNG::fromStream(fs::fileRead(icon.value()));
                            Bitmap bmp{decoder.width(), decoder.height(), 16};
                            bmp.loadImage(std::move(decoder));
                            setBitmap(std::move(bmp));
                        }
                    }),
                };
                auto x = App::run<PopupMenu<ui::Action>>(& popup);
                if (x.has_value()) {
                    x.value()();
                }
            }
        }

        void draw() override {
            // blink the cursor
            blink_.update();
            ColorRGB cursorColor = ui::Style::accentFg().withAlpha(interpolation::cosineLoop(blink_, 0, 255).round());
            editor_->setCursorColor(cursorColor);
            palette_->setCursorColor(cursorColor);
            fg_->setBg(palette_->fg());
            bg_->setBg(palette_->bg());
            editor_->setColorFg(palette_->fg());
            editor_->setColorBg(palette_->bg());
            ui::Form<void>::draw();
        }

        void setBitmap(Bitmap && bmp) {
            image_->setBitmap(std::move(bmp));
            editor_->setCursor(Point{0,0});
            switch (image_->bitmap()->width()) {
                case 8:
                    editor_->setZoom(24);
                    break;
                case 16:
                    editor_->setZoom(12);
                    break;
                case 32:
                    editor_->setZoom(6);;
                    break;
                case 64:
                    editor_->setZoom(3);
                    break;
                default:
                    UNREACHABLE; // unsupported size
            }
        }

        /** Canvas that allows editing & zooming and stuff.  */
        class ZoomedEditor : public ui::Widget {
        public:

            ZoomedEditor(Point pos, ui::Image * source):
                ui::Widget{Rect::XYWH(pos.x, pos.y, 196, 196)},
                source_{source}
            {
                switch (source->bitmap()->width()) {
                    case 8:
                        zoom_ = 24;
                        break;
                    case 16:
                        zoom_ = 12;
                        break;
                    case 32:
                        zoom_ = 6; ;
                        break;
                    case 64:
                        zoom_ = 3;
                        break;
                    default:
                        UNREACHABLE; // unsupported size
                }
                cursor_ = addChild(new ui::Rectangle{Rect::WH(zoom_ + 2, zoom_ + 2)});
                setCursor(Point{0,0});
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
                cursor_->setRect(Rect::XYWH(p.x * zoom_ + 1, p.y * zoom_ + 1, zoom_ + 2, zoom_ + 2));
            }

            ColorRGB colorFg() const { return colorFg_; }
            ColorRGB colorBg() const { return colorBg_; }

            void setColorFg(ColorRGB color) { colorFg_ = color; }
            void setColorBg(ColorRGB color) { colorBg_ = color; }

            ColorRGB cursorColor() const { return cursor_->color(); }

            void setCursorColor(ColorRGB color) { cursor_->setColor(color); }

            Coord zoom() const { return zoom_; }
            void setZoom(Coord value) { 
                zoom_ = value; 
                setCursor(cursorPos_);
                //cursor_->setRect(Rect::XYWH(cursorPos_.x * zoom_ + 1, cursorPos_.y * zoom_ + 1, zoom_ + 2, zoom_ + 2));
            }

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
                    source_->bitmap()->setPixelAt(cursorPos_.x, cursorPos_.y, colorFg_.toRaw());
                }
                if (btnDown(Btn::B)) {
                    source_->bitmap()->setPixelAt(cursorPos_.x, cursorPos_.y, colorBg_.toRaw());
                }
            }

            void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
                for (Coord y = 0; y < numPixels; ++y) {
                    buffer[y] = colorAt(column, starty + y).toRaw();
                }
                if (focused())
                    Widget::renderColumn(column, buffer, starty, numPixels);
            }

            ColorRGB colorAt(Coord x, Coord y) const {
                if (isBorder(x, y))
                    return focused() ? ui::Style::accentBg() : ColorRGB::Black();
                x -= 2;
                y -= 2;
                Coord xc = x / zoom_;
                Coord yc = y / zoom_;
                if (xc >= source_->bitmap()->width() || yc >= source_->bitmap()->height())
                    return ColorRGB::Black();
                return source_->bitmap()->colorAt(xc, yc);
            }

            bool isBorder(Coord x, Coord y) const {
                return x < 1 || y < 1 || x >= width() - 1 || y >= height() - 1;
            }

        private:
            ui::Image * source_;
            Point cursorPos_;
            ui::Rectangle * cursor_;
            uint32_t zoom_ = 3;
            ColorRGB colorFg_ = ColorRGB::White();
            ColorRGB colorBg_ = ColorRGB::Black();
        }; 

        /** Palette visualization. 
         
            Depending on the BPP, either displays the colors in the bitmap, of list of recently used colors. 

            Palette size is 96x 140
         */
        class Palette : public ui::Widget {
        public:
            Palette(Point pos):
                ui::Widget{Rect::XYWH(pos.x, pos.y, 100, 100)} {
                uint32_t idx = 0;
                uint8_t levels[4] = {0, 85, 170, 255}; // evenly spaced values
                for (uint8_t r : levels)
                    for (uint8_t g : levels)
                        for (uint8_t b : levels)
                            colors_[idx++] = ColorRGB{r, g, b};
                cursor_ = addChild(new ui::Rectangle{Rect::WH(13, 13)});
                setCursorIndex(0);
            }

            ColorRGB cursorColor() const { return cursor_->color(); }

            void setCursorColor(ColorRGB color) { cursor_->setColor(color); }

            ColorRGB fg() const { return colors_[fgIndex_]; }
            ColorRGB bg() const { return colors_[bgIndex_]; }

        protected:

            void processEvents() override {
                ui::Widget::processEvents();
                if (btnPressed(Btn::Left))
                    setCursorIndex(cursorIndex_ - 1);
                if (btnPressed(Btn::Right))
                    setCursorIndex(cursorIndex_ + 1);
                if (btnPressed(Btn::Up))
                    setCursorIndex(cursorIndex_ - 8);
                if (btnPressed(Btn::Down))
                    setCursorIndex(cursorIndex_ + 8);
                if (btnDown(Btn::A)) {
                    fgIndex_ = cursorIndex_;
                }
                if (btnDown(Btn::B)) {
                    bgIndex_ = cursorIndex_;
                }
            }

            void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
                for (Coord y = 0; y < numPixels; ++y) {
                    buffer[y] = colorAt(column, starty + y).toRaw();
                }
                if (focused())
                    Widget::renderColumn(column, buffer, starty, numPixels);
            }

            ColorRGB colorAt(Coord x, Coord y) const {
                if (isBorder(x, y))
                    return focused() ? ui::Style::accentBg() : ColorRGB::Black();
                if (x < 2 || y < 2)
                    return ColorRGB::Black();
                x -= 2;
                y -= 2;
                Coord xc = x / 12;
                Coord yc = y / 12;
                if (xc >= 8 || yc >= 8)
                    return ColorRGB::Black();
                return colors_[yc * 8 + xc];
            }

            bool isBorder(Coord x, Coord y) const {
                return x < 1 || y < 1 || x >= width() - 1 || y >= height() - 1;
            }

            void setCursorIndex(int32_t index) {
                if (index < 0)
                    index = 64 - index;
                if (index >= 64)
                    index -= 64;
                cursorIndex_ = index;
                Coord x = (index % 8) * 12 + 1;
                Coord y = (index / 8) * 12 + 1;
                cursor_->setPos(x, y);
            }

        private:

            ColorRGB colors_[64];

            ui::Rectangle * cursor_;
            int32_t cursorIndex_;
            int32_t fgIndex_ = 63;
            int32_t bgIndex_ = 0;
        }; 

    private:

        Timer blink_{1000};

        ui::Image * image_;
        ZoomedEditor * editor_;
        Palette * palette_;
        ui::Panel * fg_;
        ui::Panel * bg_;
    

    }; // rckid::Drawing

} // namespace rckid 