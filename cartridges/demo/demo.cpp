
#include <rckid/apps/launcher.h>
#include <gbcemu/gbcemu.h>

using namespace rckid;

int main() {
    rckid::initialize();
    App::run<Launcher>(mainMenuGenerator({ .gamesExtender = gbcemu::GBCEmu::gamesMenuExtender }));
}