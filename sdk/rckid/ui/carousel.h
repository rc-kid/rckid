#pragma once

#include "../app.h"
#include "../graphics/canvas.h"
#include "../ui/timer.h"

#include "menu.h"

namespace rckid {

    /** Simple carousel animation controller. 

        The carousel does not control the menu it displays - it only provides the carousel visualization. 

     */
    class Carousel {
    public:

        Carousel(Font font): font_{font} {
            icon_.fill(color::Black);
            otherIcon_.fill(color::Black);
        }

        bool idle() const { return ! a_.running(); }

        void setCurrent(MenuItem const & item) {
            text_ = item.text();
            textWidth_ = font_.textWidth(text_);
            if (! item.icon(icon_))
                icon_.fill(color::Black);
            //auto x = item.icon();
            //if (x) {
            //    icon_ = std::move(*x);
            //}
            // TODO else
            dir_ = Btn::Home;
        }

        void moveLeft(MenuItem const & item) { setOther(item, Btn::Left); }
        void moveRight(MenuItem const & item) { setOther(item, Btn::Right); }
        void moveUp(MenuItem const & item) { setOther(item, Btn::Up); }
        void moveDown(MenuItem const & item) { setOther(item, Btn::Down); }

        /** Draws the carousel's current state onto the given surface. 
         */
        void drawOn(Bitmap<ColorRGB> & surface, Rect where) {
            a_.update();
            Point pos = getItemPosition(icon_, textWidth_, where);
            switch (dir_) {
                // no animation - only display the current icon & text
                default:
                case Btn::Home: {
                    drawIcon(surface, pos.x, pos.y, icon_);
                    drawText(surface, pos.x, pos.y, icon_, text_);
                    break;
                }
                // other comes from left
                case Btn::Left: {
                    Point opos = getItemPosition(otherIcon_, otherTextWidth_, where);  
                    int offset = interpolation::easingCos(a_, 0, where.w);
                    drawIcon(surface, pos.x + offset, pos.y, icon_);
                    drawText(surface, pos.x + offset * 2, pos.y, icon_, text_);
                    drawIcon(surface, opos.x - (where.w - offset) * 2, opos.y, otherIcon_);
                    drawText(surface, opos.x - (where.w - offset), opos.y, otherIcon_, otherText_);
                    break;
                }
                // other comes from right
                case Btn::Right: {
                    Point opos = getItemPosition(otherIcon_, otherTextWidth_, where);    
                    int offset = interpolation::easingCos(a_, 0, where.w);
                    drawIcon(surface, pos.x - offset * 2, pos.y, icon_);
                    drawText(surface, pos.x - offset, pos.y, icon_, text_);
                    drawIcon(surface, opos.x + (where.w - offset), opos.y, otherIcon_);
                    drawText(surface, opos.x + (where.w - offset) * 2, opos.y, otherIcon_, otherText_);
                    break;
                }
                // other comes from sides, current goes down (we are moving up in the menu hierarchy)
                case Btn::Up: {
                    Point opos = getItemPosition(otherIcon_, otherTextWidth_, where);    
                    int offset = interpolation::easingCos(a_, where.w, 0);
                    drawIcon(surface, opos.x - offset, opos.y, otherIcon_);
                    drawText(surface, opos.x + offset, opos.y, otherIcon_, otherText_);
                    offset = interpolation::easingCos(a_, 0, where.h);
                    drawIcon(surface, pos.x, pos.y + offset, icon_);
                    drawText(surface, pos.x, pos.y + offset, icon_, text_);
                    break;
                }
                case Btn::Down: {
                    Point opos = getItemPosition(otherIcon_, otherTextWidth_, where);    
                    int offset = interpolation::easingCos(a_, 0, where.w);
                    drawIcon(surface, pos.x - offset, pos.y, icon_);
                    drawText(surface, pos.x + offset, pos.y, icon_, text_);
                    offset = interpolation::easingCos(a_, where.h, 0);
                    drawIcon(surface, opos.x, opos.y + offset, otherIcon_);
                    drawText(surface, opos.x, opos.y + offset, otherIcon_, otherText_);
                    break;
                }
            }
            if (dir_ != Btn::Home && !a_.running()) {
                std::swap(text_, otherText_);
                std::swap(textWidth_, otherTextWidth_);
                std::swap(icon_, otherIcon_);
                dir_ = Btn::Home;
            }            
        }

    private:

        void setOther(MenuItem const & item, Btn dir) {
            otherText_ = item.text();
            otherTextWidth_ = font_.textWidth(otherText_);
            if (! item.icon(otherIcon_))
                otherIcon_.fill(color::Black);

            //auto x = item.icon();
            //if (x) {
            //    otherIcon_ = std::move(*x);
            //}
            // TODO else
            dir_ = dir;
            a_.start();
        }

        Point getItemPosition(Bitmap<ColorRGB> const & icon, int textWidth, Rect where) {
            int left = where.left() + (where.w - icon.width() - 5 - textWidth) / 2;
            int top = where.top() + (where.h - icon.height()) / 2;
            return Point{left, top};
        }

        void drawText(Bitmap<ColorRGB> & surface, int x, int y, Bitmap<ColorRGB> const & icon, char const * text) {
            if (text == nullptr)
                return;
            y += (icon.height() - font_.size) / 2;
            x += icon.width() + 5;
            // TODO does nothing interesting - eventually might scroll the text if too large to fit, etc
            surface.text(x, y, font_, color::White) << text;
        }

        void drawIcon(Bitmap<ColorRGB> & surface, int x, int y, Bitmap<ColorRGB> const & icon) {
            surface.blit(Point{x, y}, icon);
        }

        Font font_;

        char const * text_ = nullptr;
        char const * otherText_ = nullptr;
        int textWidth_ = 0;
        int otherTextWidth_ = 0;
        Bitmap<ColorRGB> icon_{64, 64};
        Bitmap<ColorRGB> otherIcon_{64, 64}; 

        Btn dir_ = Btn::Home;
        Timer a_{RCKID_UI_EFFECT_DURATION_MS};
    }; 


} // namespace rckid