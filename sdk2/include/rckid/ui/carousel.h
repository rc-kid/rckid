#pragma once

#include <rckid/ui/widget.h>
#include <rckid/ui/image.h>
#include <rckid/ui/label.h>

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
            bText_{addChild(new Label())} {
        }

    protected:

    
        
        

    private:
        Image * aImg_ = nullptr;
        Label * aText_ = nullptr;
        Image * bImg_ = nullptr;
        Label * bText_ = nullptr;

    }; // rckid::ui::Carousel

} // namespace rckid::ui