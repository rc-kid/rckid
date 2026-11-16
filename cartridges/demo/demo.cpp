
#include <kiddo/apps/launcher.h>
#include <gbcemu/gbcemu.h>

using namespace kiddo;

int main() {
    kiddo::initialize();
    App::run<Launcher>(mainMenuGenerator({ .gamesExtender = gbcemu::GBCEmu::gamesMenuExtender }));
}