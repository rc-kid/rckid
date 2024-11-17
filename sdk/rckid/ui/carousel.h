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

        Carousel(Font font): font_{font} { }

        bool idle() const { return ! a_.running(); }

        void setCurrent(MenuItem const & item) { setItem(item, current_, Btn::Home); }

        void moveLeft(MenuItem const & item) { setItem(item, other_, Btn::Left); }
        void moveRight(MenuItem const & item) { setItem(item, other_, Btn::Right); }
        void moveUp(MenuItem const & item) { setItem(item, other_, Btn::Up); }
        void moveDown(MenuItem const & item) { setItem(item, other_, Btn::Down); }

        /** Draws the carousel's current state onto the given surface. 
         */
        void drawOn(Bitmap<ColorRGB> & surface, Rect where) {
            a_.update();
            Point pos = getItemPosition(current_, where);
            switch (dir_) {
                // no animation - only display the current icon & text
                default:
                case Btn::Home: {
                    drawIcon(surface, pos.x, pos.y, current_);
                    doDrawText(surface, pos.x, pos.y, current_);
                    break;
                }
                // other comes from left
                case Btn::Left: {
                    Point opos = getItemPosition(other_, where);  
                    int offset = interpolation::easingCos(a_, 0, where.w);
                    drawIcon(surface, pos.x + offset, pos.y, current_);
                    doDrawText(surface, pos.x + offset * 2, pos.y, current_);
                    drawIcon(surface, opos.x - (where.w - offset) * 2, opos.y, other_);
                    doDrawText(surface, opos.x - (where.w - offset), opos.y, other_);
                    break;
                }
                // other comes from right
                case Btn::Right: {
                    Point opos = getItemPosition(other_, where);    
                    int offset = interpolation::easingCos(a_, 0, where.w);
                    drawIcon(surface, pos.x - offset * 2, pos.y, current_);
                    doDrawText(surface, pos.x - offset, pos.y, current_);
                    drawIcon(surface, opos.x + (where.w - offset), opos.y, other_);
                    doDrawText(surface, opos.x + (where.w - offset) * 2, opos.y, other_);
                    break;
                }
                // other comes from sides, current goes down (we are moving up in the menu hierarchy)
                case Btn::Up: {
                    Point opos = getItemPosition(other_, where);    
                    int offset = interpolation::easingCos(a_, where.w, 0);
                    drawIcon(surface, opos.x - offset, opos.y, other_);
                    doDrawText(surface, opos.x + offset, opos.y, other_);
                    offset = interpolation::easingCos(a_, 0, where.h);
                    drawIcon(surface, pos.x, pos.y + offset, current_);
                    doDrawText(surface, pos.x, pos.y + offset, current_);
                    break;
                }
                case Btn::Down: {
                    Point opos = getItemPosition(other_, where);    
                    int offset = interpolation::easingCos(a_, 0, where.w);
                    drawIcon(surface, pos.x - offset, pos.y, current_);
                    doDrawText(surface, pos.x + offset, pos.y, current_);
                    offset = interpolation::easingCos(a_, where.h, 0);
                    drawIcon(surface, opos.x, opos.y + offset, other_);
                    doDrawText(surface, opos.x, opos.y + offset, other_);
                    break;
                }
            }
            if (dir_ != Btn::Home && !a_.running()) {
                std::swap(current_, other_);
                dir_ = Btn::Home;
            }            
        }

        Font const & font() const { return font_; }

    protected:

        class Item {
        public:
            std::string text;
            int textWidth;
            Bitmap<ColorRGB> icon{64, 64};
            void * payloadPtr;
            uint32_t payload;

            Item() { icon.fill(color::Black); }
        }; 

    private:

        void setItem(MenuItem const & from, Item & item, Btn dir) {
            from.text(item.text);
            item.textWidth = font_.textWidth(item.text);
            if (! from.icon(item.icon))
                item.icon.fill(color::Black);
            item.payloadPtr = from.payloadPtr();
            item.payload = from.payload();
            dir_ = dir;
            if (dir_ != Btn::Home)
                a_.start();
        }

        Point getItemPosition(Item & item, Rect where) {
            int left = where.left() + (where.w - item.icon.width() - 5 - item.textWidth) / 2;
            int top = where.top() + (where.h - item.icon.height()) / 2;
            return Point{left, top};
        }

        void doDrawText(Bitmap<ColorRGB> & surface, int x, int y, Item & item) {
            y += (item.icon.height() - font_.size) / 2;
            x += item.icon.width() + 5;
            drawText(surface, x, y, item);
        }

        virtual void drawText(Bitmap<ColorRGB> & surface, int x, int y, Item & item) {
            if (item.text.empty())
                return;
            // TODO does nothing interesting - eventually might scroll the text if too large to fit, etc
            surface.text(x, y, font_, color::White) << item.text;
        }

        virtual void drawIcon(Bitmap<ColorRGB> & surface, int x, int y, Item & item) {
            surface.blit(Point{x, y}, item.icon);
        }

        Font font_;

        Item current_;
        Item other_;

        Btn dir_ = Btn::Home;
        Timer a_{RCKID_UI_EFFECT_DURATION_MS};
    }; 


} // namespace rckid