#pragma once

#include "rckid/rckid.h"
#include "menu.h"
#include "rckid/graphics/canvas.h"
#include "rckid/graphics/animation.h"

namespace rckid {

    /** App add-on that displays carousel menu. */
    template<typename COLOR>
    class Carousel {
    public:

        Carousel(Menu * menu = nullptr): menu_{menu} {
            if (!empty())
                loadItem(i_, text_, textWidth_, icon_);
        }

        void setMenu(Menu * menu, size_t index) {
            a_.stop();
            menu_ = menu;
            i_ = index;
            if (!empty())
                loadItem(i_, text_, textWidth_, icon_);
        }

        /** Returns true of the carousel is not currently rendering any animation.
         */
        bool idle() const { return dir_ == Btn::Home; }

        bool empty() const { return menu_ == nullptr ||  menu_->size() == 0; }

        size_t current() const { return i_; }

        void prev() {
            if (!idle() || empty())
                return;
            dir_ = Btn::Left;
            i_ = (i_ == 0) ? (menu_->size() - 1) : (i_ - 1);
            loadItem(i_, otherText_, otherTextWidth_, otherIcon_);
            a_.start();
        }

        void next() {
            if (!idle() || empty())
                return;
            dir_ = Btn::Right;
            i_ = (i_ == menu_->size() - 1) ? 0 : (i_ + 1);
            loadItem(i_, otherText_, otherTextWidth_, otherIcon_);
            a_.start();
        }

        /** Draws the carousel on given canvas. Uses the canvas' font and foreground color to control the rendering. 
         */
        void drawOn(Canvas<COLOR> & canvas, Rect where) {
            a_.update();
            if (dir_ == Btn::Home) {
                drawItem(canvas, where, icon_, text_, textWidth_);
            } else {
                int w = where.width();
                int offset = a_.interpolate(0, w);
                if (dir_ == Btn::Left) {
                    drawItem(canvas, where, icon_, text_, textWidth_, offset, 1, 2);
                    drawItem(canvas, where, otherIcon_, otherText_, otherTextWidth_, - (w - offset), 2, 1);
                } else {
                    drawItem(canvas, where, icon_, text_, textWidth_, -offset, 2, 1);
                    drawItem(canvas, where, otherIcon_, otherText_, otherTextWidth_, + (w - offset), 1, 2);
                }
                if (!a_.running()) {
                    dir_ = Btn::Home;
                    std::swap(text_, otherText_);
                    std::swap(textWidth_, otherTextWidth_);
                    std::swap(icon_, otherIcon_);
                }
            }
        }   

        void drawOn(Canvas<COLOR> & canvas) {
            drawOn(canvas, Rect::WH(canvas.width(), canvas.height()));
        }

    private:

        void loadItem(size_t index, char const * & text, int & textWidth, Bitmap<COLOR> & icon) {
            MenuItem const & i = (*menu_)[index];
            text = i.text;
            textWidth = -1;
            icon.loadImage(i.icon, i.iconBytes);
        }

        void drawItem( Canvas<COLOR> & canvas, Rect where, Bitmap<COLOR> const & icon, char const * text, int & textWidth, int offset = 0, int iconSpeed = 1, int textSpeed = 1) {
            // if we haven't measured the text yet, do so
            if (textWidth == -1)
                textWidth = canvas.font().textWidth(text);
            // now determine the beginning of the icon and the beginning of the text in the 
            int h = where.height();
            int totalWidth = 64 + 8 + textWidth;
            Point iconStart{where.left() + (where.width() - totalWidth) / 2 + offset * iconSpeed, where.top() + (h - icon_.height()) / 2};
            Point textStart{where.left() + (where.width() - totalWidth) / 2 + 72 + (offset * textSpeed) , where.top() + (h - canvas.font().size) / 2};
            canvas.blit(iconStart, icon);
            canvas.text(textStart) << text;
        }

        Btn dir_ = Btn::Home;
        Animation a_{500};

        Menu * menu_ = nullptr;
        size_t i_ = 0;
        char const * text_ = nullptr;
        char const * otherText_ = nullptr;
        int textWidth_ = -1;
        int otherTextWidth_ = -1;
        Bitmap<COLOR> icon_{64, 64};
        Bitmap<COLOR> otherIcon_{64, 64}; 
    }; // rckid::Carousel

} // namespace rckid