#include <rckid/rckid.h>

using namespace rckid;


static constexpr size_t FB_SIZE = 320 * 240;

void fill(ColorRGB * fb, size_t size, ColorRGB value)  {
    while (size-- != 0)
        *(fb++) = value;
}

int main() {
    initialize();
    displaySetMode(DisplayMode::Native);
    displaySetUpdateRegion(Rect::WH(320, 240));
    ColorRGB * fb = new ColorRGB[FB_SIZE];
    while (true) {
        fill(fb, FB_SIZE, ColorRGB{255, 0, 0});
        displayUpdate(fb, FB_SIZE);
        displayWaitUpdateDone();

        fill(fb, FB_SIZE, ColorRGB{0, 255, 0});
        displayUpdate(fb, FB_SIZE);
        displayWaitUpdateDone();

        fill(fb, FB_SIZE, ColorRGB{0, 0, 255});
        displayUpdate(fb, FB_SIZE);
        displayWaitUpdateDone();

        fill(fb, FB_SIZE, ColorRGB{0, 0, 0});
        displayUpdate(fb, FB_SIZE);
        displayWaitUpdateDone();
    }
}