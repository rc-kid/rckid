#include <rckid/rckid.h>
#include <rckid/graphics/color.h>

using namespace rckid;


static constexpr size_t FB_SIZE = 320 * 240;

void fill(ColorRGB * fb, size_t size, ColorRGB value)  {
    while (size-- != 0)
        *(fb++) = value;
}

int main() {
    initialize();
    displaySetResolution(DisplayResolution::Normal);
    displaySetUpdateRegion(Rect::WH(320, 240));
    ColorRGB * fb = new ColorRGB[FB_SIZE];
    while (true) {
        fill(fb, FB_SIZE, ColorRGB{255, 0, 0});
        //displayWaitVSync();
        displayUpdate(fb, FB_SIZE);
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