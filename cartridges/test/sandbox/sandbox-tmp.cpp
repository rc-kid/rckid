#define HAHA_
#ifdef HAHA

#include "rckid/app.h"
#include "rckid/stats.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/graphics/font.h"
#include "assets/fonts/Iosevka_16.h"


#include "assets/all.h"

#include "rckid/graphics/simple_tiles.h"

using namespace rckid;




int main() {
    rckid::initialize();
    ST7789::configure(DisplayMode::Native_RGB565);
    ST7789::enterContinuousUpdate(Rect::WH(320, 240));
    uint16_t palette[16];
    for (int i = 0; i < 16; ++i)
        palette[i] = ColorRGB::RGB(i * 16, i * 16, i * 16).rawValue16();
    SimpleTiles st{};
    for (int i = 0; i < 128; ++i)
        st.set(i % 20, i / 20, i);        
    st.setPalette(palette);
    st.render();
    while (true) {}
}

#endif


