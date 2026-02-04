#pragma once

#include <rckid/ui/widget.h>
#include <rckid/ui/image.h>
#include <rckid/ui/label.h>
#include <rckid/ui/animation.h>

namespace rckid::ui {

    /** Carousel menu
     
        This is the main menu widget used for the main & home menus as well as most other selections in apps. Each carousel item consists of an Image and Widget (usually a label) displayed to the right of the image. 
     */
    class Carousel : public Widget {
    public:
        Carousel():
            aImg_{addChild(new Image())},
            aText_{addChild(new Label())},
            bImg_{addChild(new Image())},
            bText_{addChild(new Label())} 
        {
            a_.setMode(Animation::Mode::Single);
            with(aImg_)
                << SetHeight(height())
                << SetVAlign(VAlign::Center)
                << SetHAlign(HAlign::Center);
            with(aText_)
                << SetHeight(height())
                << SetVAlign(VAlign::Center)
                << SetHAlign(HAlign::Left);
        }

        bool idle() const { return ! a_.active(); }

    protected:

        void set(String text, ImageSource icon, Direction dir) {
            if (! idle())
                return;
            // first set up the widgets and resize them
            Bitmap bmp{std::move(icon)};

        }

        /** Returns the image and label widgets for the currently selected element. 
         
            When idle, those are the visible elements. During animation those are the elements that are going away.
         */
        //@{
        Image * currentImage() const { return aImg_; }
        Label * currentLabel() const { return aText_; }
        //@}

        /** Returns the image and label widgets for the next element. 
         
            Those elements are not visible when idle. When animated, those elements are getting into the view. When the animation is finished, those widgets will replace the current mage and label. 
         */

         //@{
        Image * nextImage() const { return bImg_; }
        Label * nextLabel() const { return bText_; }
        //@}
    
        
        

    private:
        Image * aImg_ = nullptr;
        Label * aText_ = nullptr;
        Image * bImg_ = nullptr;
        Label * bText_ = nullptr;
        // animation for the widget transitions
        Animation a_;
        // Direction of the widget transitions
        Direction dir_;

    }; // rckid::ui::Carousel

} // namespace rckid::ui