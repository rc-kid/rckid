#pragma once

#include "carousel.h"

namespace rckid::ui {

    /** File browser. 
     
        Ideally customizable with some extra events that allow specifying the icon, etc. 
     */
    class FileBrowser : public Carousel {
    public:

        void moveLeft() {

        }

        void moveRight() {

        }

    private:
        String path_;

    }; 

}