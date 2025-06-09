#include "../../rckid.h"
#include "../../app.h"
#include "../../graphics/bitmap.h"
#include "../../assets/fonts/Iosevka16.h"

namespace rckid {

    /** A simple tetris game. 
     */
    class Tetris : public BitmapApp<ColorRGB> {
    public:

        Tetris(): BitmapApp<ColorRGB>{Rect::WH(320, 240)} {
            
        }
       

    protected:



    }; // rckid::Tetris

}