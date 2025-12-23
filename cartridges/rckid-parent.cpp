#include "rckid-default.h"

using namespace rckid;

int main() {
    setParentMode(true);
    initialize();
    ui::runSystemMenu(mainMenu);
}

