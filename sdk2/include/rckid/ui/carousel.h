#pragma once

#include <rckid/ui/widget.h>
#include <rckid/ui/image.h>
#include <rckid/ui/label.h>
#include <rckid/ui/animation.h>

#include <assets/OpenDyslexic64.h>

namespace rckid::ui {

    /** Carousel menu
     
        This is the main menu widget used for the main & home menus as well as most other selections in apps. Each carousel item consists of an Image and Widget (usually a label) displayed to the right of the image. 
     */
    class Carousel : public Widget {
    public:

        static constexpr Coord ICON_SEPARATOR_WIDTH = 5;

        Carousel():
            aImg_{addChild(new Image())},
            aText_{addChild(new Label())},
            bImg_{addChild(new Image())},
            bText_{addChild(new Label())} 
        {
            with(aImg_)
                << SetVAlign(VAlign::Center)
                << SetHAlign(HAlign::Center);
            with(bImg_)
                << SetVAlign(VAlign::Center)
                << SetHAlign(HAlign::Center);
            with(aText_)
                << SetVAlign(VAlign::Center)
                << SetHAlign(HAlign::Left)
                << SetFont(assets::OpenDyslexic64);
            with(bText_)
                << SetVAlign(VAlign::Center)
                << SetHAlign(HAlign::Left)
                << SetFont(assets::OpenDyslexic64);
        }

        Font font() const { return aText_->font(); }

        void setFont(Font font) {
            aText_->setFont(font);
            bText_->setFont(font);
        }

        void set(String text, ImageSource icon) {
            if (! idle())
                return;
            setCurrent(std::move(text), std::move(icon));
            with(bImg_)
                << SetVisibility(false);
            with(bText_)
                << SetVisibility(false);
        }

        void set(String text, ImageSource icon, Direction dir) {
            if (! idle())
                return;
            setCurrent(std::move(text), std::move(icon));
            // start the appropriate animation
            switch (dir) {
                case Direction::Up:
                    // old goes down, new comes from the sides
                    animate()
                        << MoveHorizontally(aImg_, aImg_->x() - width(), aImg_->x(), 300)
                        << MoveHorizontally(aText_, aText_->x() + width(), aText_->x(), 300)
                        << MoveVertically(bImg_, bImg_->y() + height(), bImg_->y(), 300)
                        << MoveVertically(bText_, bText_->y() + height(), bText_->y(), 300);
                    break;
                case Direction::Down:
                    // old goes to the sides, new comes from the bottom
                    animate()
                        << MoveVertically(aImg_, aImg_->y() - height(), aImg_->y(), 300)
                        << MoveVertically(aText_, aText_->y() - height(), aText_->y(), 300)
                        << MoveHorizontally(bImg_, bImg_->x(), bImg_->x() - width(), 300)
                        << MoveHorizontally(bText_, bText_->x(), bText_->x() + width(), 300);
                    break;
                case Direction::Left:
                    // old goes to the right, new comes from the left
                    animate()
                        << MoveHorizontally(aImg_, aImg_->x() - width() * 2, aImg_->x(), 300)
                        << MoveHorizontally(aText_, aText_->x() - width(), aText_->x(), 300)
                        << MoveHorizontally(bImg_, bImg_->x(), bImg_->x() + width(), 300)
                        << MoveHorizontally(bText_, bText_->x(), bText_->x() + width() * 2, 300);
                    break;
                case Direction::Right:
                    // old goes to the left, new comes from right
                    animate()
                        << MoveHorizontally(aImg_, aImg_->x() + width(), aImg_->x(), 300)
                        << MoveHorizontally(aText_, aText_->x() + width() * 2, aText_->x(), 300)
                        << MoveHorizontally(bImg_, bImg_->x(), bImg_->x() - width() * 2, 300)
                        << MoveHorizontally(bText_, bText_->x(), bText_->x() - width(), 300);
                    break;
                default:
                    UNREACHABLE;
            }
        }

        /** Returns the image and label widgets for the currently selected element. 
         
            When idle, those are the visible elements. During animation those are the elements that are going away.
         */
        //@{
        Image * currentImage() const { return aImg_; }
        Label * currentLabel() const { return aText_; }
        //@}

    protected:

        void onIdle() override {
            // when animations are done, hide the previous elements
            with(bImg_)
                << SetVisibility(false);
            with(bText_)
                << SetVisibility(false);
        }

        void setCurrent(String text, ImageSource icon) {
            std::swap(aImg_, bImg_);
            std::swap(aText_, bText_);
            // first set up the widgets so that we can calculate their size
            Bitmap bmp{std::move(icon)};
            with(aText_)
                << SetText(std::move(text));
            Coord iconWidth = bmp.width();
            Coord textWidth = aText_->textWidth();
            // determine the final positions
            Coord iconLeft = (width() - (iconWidth + textWidth + ICON_SEPARATOR_WIDTH)) / 2;
            Coord textLeft = iconLeft + iconWidth + ICON_SEPARATOR_WIDTH;
            // and place & adjust the the widgets
            with(aImg_) 
                << SetBitmap(std::move(bmp))
                << SetRect(Rect::XYWH(iconLeft, 0, iconWidth, height()))
                << SetVisibility(true);
            with(aText_)
                << SetRect(Rect::XYWH(textLeft, 0, width() - textLeft, height())) 
                << SetVisibility(true);
        }

        /** Returns the image and label widgets for the next element. 
         
            Those elements are not visible when idle. When animated, those elements are getting into the view. When the animation is finished, those widgets will replace the current mage and label. 
         */

         //@{
        Image * prevImage() const { return bImg_; }
        Label * prevLabel() const { return bText_; }
        //@}

    private:
        Image * aImg_ = nullptr;
        Label * aText_ = nullptr;
        Image * bImg_ = nullptr;
        Label * bText_ = nullptr;

    }; // rckid::ui::Carousel

} // namespace rckid::ui