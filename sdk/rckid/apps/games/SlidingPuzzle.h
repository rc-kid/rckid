#pragma once

#include "../../rckid.h"
#include "../../app.h"
#include "../../graphics/canvas.h"
#include "../../assets/fonts/Iosevka16.h"

namespace rckid {

    /** Sliding puzzle game. 
     
        Takes images from app location on the SD card
     */
    class SlidingPuzzle : public CanvasApp<ColorRGB> {
    public:

        SlidingPuzzle(): CanvasApp<ColorRGB>{} {
            
        }
       

    protected:



    }; // rckid::Tetris

}