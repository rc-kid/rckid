#pragma once

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/graphics/animation.h"
#include "rckid/graphics/png.h"
#include "rckid/graphics/bitmap.h"
#include "rckid/ui/menu.h"

namespace rckid {

    class Carousel : public App<FrameBuffer<ColorRGB>, size_t> {
    public:

        using EventOnSelect = std::function<void(size_t)>;

        Carousel(Menu * menu): menu_{menu} {}

        bool empty() const { return menu_ == nullptr ||  menu_->size() == 0; }

        bool idle() const { return dir_ == Btn::Home; }

    protected:

        void onFocus() override {
            App::onFocus();
            icon_.allocate(MemArea::VRAMorHeap);
            otherIcon_.allocate(MemArea::VRAMorHeap);
            loadItem(i_, text_, textWidth_, icon_);
        }

        void onBlur() override {
            TRACE("on blur begin");
            icon_.deallocate();
            otherIcon_.deallocate();
            text_ = nullptr;
            otherText_ = nullptr;
            App::onBlur();
        }

        void update() override {
            if (idle() && !empty()) {
                if (pressed(Btn::Left)) {
                    TRACE("  left");
                    dir_ = Btn::Left;
                    i_ = (i_ == 0) ? (menu_->size() - 1) : (i_ - 1);
                    loadItem(i_, otherText_, otherTextWidth_, otherIcon_);
                    a_.start();
                } else if (pressed(Btn::Right)) {
                    TRACE("  right");
                    dir_ = Btn::Right;
                    i_ = (i_ == menu_->size() - 1) ? 0 : (i_ + 1);
                    loadItem(i_, otherText_, otherTextWidth_, otherIcon_);
                    a_.start();
                } else if (pressed(Btn::A)) { // select
                    TRACE("  select" << i_);
                    exit(i_);
                } else if (pressed(Btn::B)) { // back
                    TRACE("  back");
                    exit();
                }
            }
        }

        void draw() override {
            driver_.fill();
            a_.update();
            if (dir_ == Btn::Home) {
                drawItem(icon_, text_, textWidth_);
            } else {
                int w = driver_.width();
                int offset = a_.interpolate(0, w);
                if (dir_ == Btn::Left) {
                    drawItem(icon_, text_, textWidth_, offset, 1, 2);
                    drawItem(otherIcon_, otherText_, otherTextWidth_, - (w - offset), 2, 1);
                } else {
                    drawItem(icon_, text_, textWidth_, -offset, 2, 1);
                    drawItem(otherIcon_, otherText_, otherTextWidth_, + (w - offset), 1, 2);
                }
                if (!a_.running()) {
                    dir_ = Btn::Home;
                    std::swap(text_, otherText_);
                    std::swap(textWidth_, otherTextWidth_);
                    std::swap(icon_, otherIcon_);
                }
            }
        }

        void loadItem(size_t index, char const * & text, int textWidth, Bitmap<Color> & icon) {
            if (empty())
                return;
            MenuItem const & i = (*menu_)[index];
            text = i.text;
            textWidth = driver_.textWidth(text);
            icon.loadImage(i.icon, i.iconBytes);
        }

        void drawItem( Bitmap<Color> const & icon, char const * text, int textWidth, int offset = 0, int iconSpeed = 1, int textSpeed = 1) {
            int h = driver_.height();
            Point iconStart{(driver_.width() - textWidth) / 2 - 72 + offset * iconSpeed, (h - icon_.height()) / 2};
            Point textStart{(driver_.width() - textWidth) / 2 + (offset * textSpeed) , (h - driver_.font().height()) / 2};
            driver_.draw(iconStart, icon);
            driver_.text(textStart, text);
        }
    

        Menu * menu_ = nullptr;
        Btn dir_ = Btn::Home;
        Animation a_{500};
        size_t i_ = 0;
        char const * text_ = nullptr;
        char const * otherText_ = nullptr;
        int textWidth_ = 0;
        int otherTextWidth_ = 0;
        Bitmap<Color> icon_{64, 64, MemArea::None};
        Bitmap<Color> otherIcon_{64, 64, MemArea::None};
    };


} // namespace rckid