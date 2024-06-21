#define HAHA_
#ifdef HAHA

#include "rckid/app.h"
#include "rckid/stats.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/graphics/font.h"
#include "rckid/graphics/animation.h"
#include "assets/fonts/Iosevka_16.h"
#include "rckid/ui/ui.h"
#include "rckid/ui/text_input.h"

#include "assets/all.h"

//#include "rckid/graphics/simple_tiles.h"


#include "rckid/graphics/tile.h"
#include "rckid/graphics/tile_engine.h"

using namespace rckid;

int main() {
    rckid::initialize();
    TextInput{}.run();
    /*
    ST7789::configure(DisplayMode::Native_RGB565);
    ST7789::enterContinuousUpdate(Rect::WH(320, 240));

    ColorRGB palette[16];
    for (int i = 0; i < 16; ++i)
        palette[i] = ColorRGB::RGB(i * 16, i * 16, i * 16);
    //SimpleEngine<Tile<8,8,PixelFormat::Color16>> st{40, 30, tiles_, palette};
    SimpleEngine<Tile<12,24,PixelFormat::Color16>> st{26, 10, Iosevka_Tiles_12x24::tileset, palette};
    st.fill('A');
    /*
    SimpleTiles st{};
    st.text(1,1) << " Q W E R T Y U I O P ";
    st.text(1,2) << "  A S D F G H J K L";
    st.text(1,3) << "   Z X C V B N M";
    */
//    for (int i = 0; i < 128; ++i)
//        st.set(i % 20, i / 20, i);        
    //st.setPalette(palette);
    /*
    st.render();
    while (true) {}
    */
}

#endif


