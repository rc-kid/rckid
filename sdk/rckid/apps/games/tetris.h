#include "../../rckid.h"
#include "../../app.h"
#include "../../graphics/bitmap.h"
#include "../../assets/fonts/Iosevka16.h"

namespace rckid {

    /** A simple tetris game. 
     */
    class Tetris : public BitmapApp<16> {
    public:
        Tetris(): BitmapApp<16>{RenderableBitmap<16>{}} {

        }

    protected:



    }; // rckid::Tetris

}