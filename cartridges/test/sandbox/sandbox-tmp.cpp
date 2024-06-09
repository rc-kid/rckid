#define HAHA
#ifdef HAHA

#include "rckid/app.h"
#include "rckid/stats.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/graphics/font.h"
#include "assets/fonts/Iosevka_16.h"


#include "assets/all.h"

#include "rckid/graphics/simple_tiles.h"

using namespace rckid;

constexpr Tile<8,8,8> tiles_[] = {
    Tile<8,8,8>{{
        1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1
    }}
}; 



int main() {
    rckid::initialize();
    ST7789::configure(DisplayMode::Native_RGB565);
    ST7789::enterContinuousUpdate(Rect::WH(320, 240));
    uint16_t palette[16];
    for (int i = 0; i < 16; ++i)
        palette[i] = ColorRGB::RGB(i * 16, i * 16, i * 16).rawValue16();
    SimpleTiles st{};
    st.text(1,1) << " Q W E R T Y U I O P ";
    st.text(1,2) << "  A S D F G H J K L";
    st.text(1,3) << "   Z X C V B N M";
//    for (int i = 0; i < 128; ++i)
//        st.set(i % 20, i / 20, i);        
    st.setPalette(palette);
    st.render();
    while (true) {}
}

#endif


