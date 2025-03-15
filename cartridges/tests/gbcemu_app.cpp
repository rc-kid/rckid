#include <rckid/rckid.h>
#include <gbcemu/gbcemu.h>
#include <gbcemu/../tests/bootloader.h>
#include <gbcemu/gamepak.h>

using namespace rckid;

int main() {
    initialize();
    //while (true) {
        auto app = gbcemu::GBCEmu{};
        app.loadCartridge(new gbcemu::FlashGamePak{gbcemu::DMGBootloader});
        app.run();
        
    //}
}