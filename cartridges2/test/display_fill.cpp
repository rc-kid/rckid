#include <rckid/hal.h>
#include <rckid/graphics/color.h>
#include <rckid/util/log.h>

using namespace rckid;


static constexpr size_t FB_SIZE = 320 * 240;

int main() {
    initialize();
    hal::display::enable(Rect::WH(320, 240), hal::display::RefreshDirection::ColumnFirst);
    Color::RGB565 * fb = new Color::RGB565[FB_SIZE];
    auto updateCallback = [&](Color::RGB565 * & buffer, uint32_t & bufferSize) {
        if (buffer == nullptr) {
            buffer = fb;
            bufferSize = FB_SIZE;
        } else {
            buffer = nullptr;
            bufferSize = 0;
        }
    };
    uint32_t x = 0;
    while (true) {
        LOG(LL_INFO, "Tick " << (x++));
        memset16(reinterpret_cast<uint16_t*>(fb), Color::RGB(255, 0, 0).toRGB565(), FB_SIZE);
        //displayWaitVSync();
        hal::display::update(updateCallback);
        while (hal::display::updateActive())
            yield();

        memset16(reinterpret_cast<uint16_t*>(fb), Color::RGB(0, 255, 0).toRGB565(), FB_SIZE);
        //displayWaitVSync();
        hal::display::update(updateCallback);
        while (hal::display::updateActive())
            yield();

        memset16(reinterpret_cast<uint16_t*>(fb), Color::RGB(0, 0, 255).toRGB565(), FB_SIZE);
        //displayWaitVSync();
        hal::display::update(updateCallback);
        while (hal::display::updateActive())
            yield();

        memset16(reinterpret_cast<uint16_t*>(fb), Color::RGB(0, 0, 0).toRGB565(), FB_SIZE);
        //displayWaitVSync();
        hal::display::update(updateCallback);
        while (hal::display::updateActive())
            yield();

    }
}