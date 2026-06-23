#include <rckid/rckid.h>
#include <rckid/hal.h>
#include <rckid/graphics/color.h>

using namespace rckid;

void showStatus() {
//    LOG(LL_INFO, "Headphones: " << hal::audio::)
}

/** Very simple test that serial output is working. 
 
    Counts display TE periods (fps) and displays update each second (60fps). Also shows the state of various pins that are RP2350 only as there is no communication with AVR yet in this test.
 */
int main() {
    initialize();
    LOG(LL_INFO, "Init done");
    uint32_t i = 0;
    uint32_t fps = 0;
    bool te = false;
    uint32_t numTe = 0;
    uint64_t next = time::uptimeUs() + 16666;
    while (true) {
        if (te != hal::display::vSync()) {
            te = ! te;
            if (te)
                ++ numTe;
        }
        if (time::uptimeUs() > next) {
            ++fps;
            next += 16666;
            tick();
            if (btnPressed(Btn::Left))
                LOG(LL_INFO, "L");
            if (btnPressed(Btn::Right))
                LOG(LL_INFO, "R");
            if (btnPressed(Btn::Up))
                LOG(LL_INFO, "U");
            if (btnPressed(Btn::Down))
                LOG(LL_INFO, "D");
            if (btnPressed(Btn::A))
                LOG(LL_INFO, "A");
            if (btnPressed(Btn::B))
                LOG(LL_INFO, "B");
            if (btnPressed(Btn::Select))
                hal::device::setDebugMode(false);
        }
        if (fps == 60) {
            LOG(LL_INFO, ++i << " num te: " << numTe);
            Point3D acc = hal::io::accelerometerState();
            LOG(LL_INFO, "    x: " << acc.x);
            LOG(LL_INFO, "    y: " << acc.y);
            LOG(LL_INFO, "    z: " << acc.z);
            LOG(LL_INFO, "   hp: " << (audio::headphonesConnected() ? "hp" : "spkr"));
            numTe = 0;
            fps = 0;
        }
        yield();
    }
}