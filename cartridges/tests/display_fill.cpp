#include <rckid/rckid.h>
#include <rckid/graphics/color.h>

using namespace rckid;


static constexpr size_t FB_SIZE = 320 * 240;

void fill(uint16_t * fb, size_t size, uint16_t value)  {
    while (size-- != 0)
        *(fb++) = value;
}

int main() {
    initialize();
    displaySetRefreshDirection(DisplayRefreshDirection::ColumnFirst);
    displaySetUpdateRegion(Rect::WH(320, 240));
    uint16_t * fb = new uint16_t[FB_SIZE];
    uint32_t x = 0;
    while (true) {
        LOG(LL_INFO, "Tick " << (x++));
        fill(fb, FB_SIZE, ColorRGB{255, 0, 0});
        //displayWaitVSync();
        displayUpdate(fb, FB_SIZE );
        displayWaitUpdateDone();

        fill(fb, FB_SIZE, ColorRGB{0, 255, 0});
        //displayWaitVSync();
        displayUpdate(fb, FB_SIZE);
        displayWaitUpdateDone();

        fill(fb, FB_SIZE, ColorRGB{0, 0, 255});
        //displayWaitVSync();
        displayUpdate(fb, FB_SIZE);
        displayWaitUpdateDone();

        fill(fb, FB_SIZE, ColorRGB{0, 0, 0});
        //displayWaitVSync();
        displayUpdate(fb, FB_SIZE);
        displayWaitUpdateDone();

    }
}