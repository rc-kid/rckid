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

        bool idle() const { return ! aImg_->idle(); }

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
            // TODO start the animation
        }

        /** Returns the image and label widgets for the currently selected element. 
         
            When idle, those are the visible elements. During animation those are the elements that are going away.
         */
        //@{
        Image * currentImage() const { return aImg_; }
        Label * currentLabel() const { return aText_; }
        //@}

    protected:

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
        // Direction of the widget transitions
        Direction dir_;

    }; // rckid::ui::Carousel

} // namespace rckid::ui