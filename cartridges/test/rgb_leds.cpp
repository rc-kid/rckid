#include <platform.h>

#include <rckid/rckid.h>

using namespace rckid;

int main() {
    initialize();
    while (true) {
        rumbleOk();
        ledSetEffect(Btn::Left, RGBEffect::Solid(32, 0, 0, 255));
        cpu::delayMs(500);
        ledSetEffect(Btn::Left, RGBEffect::Solid(0, 0, 0, 1));
        ledSetEffect(Btn::A, RGBEffect::Solid(0, 32, 0, 255));
        cpu::delayMs(500);
        ledSetEffect(Btn::A, RGBEffect::Solid(0, 0, 0, 1));
        ledSetEffect(Btn::B, RGBEffect::Solid(0, 0, 32, 255));
        cpu::delayMs(500);
        ledSetEffect(Btn::B, RGBEffect::Solid(0, 0, 0, 1));
        ledSetEffect(Btn::Select, RGBEffect::Solid(32, 32, 0, 255));
        cpu::delayMs(500);
        ledSetEffect(Btn::Select, RGBEffect::Solid(0, 0, 0, 1));
        ledSetEffect(Btn::Start, RGBEffect::Solid(0, 32, 32, 255));
        cpu::delayMs(500);
        ledSetEffect(Btn::Start, RGBEffect::Solid(0, 0, 0, 1));
    }
}