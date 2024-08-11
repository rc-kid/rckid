#include <rckid/rckid.h>

int main() {
    rckid::initialize();
    LOG("Initialized, running the app!");

    while (true) {
        rckid::tick();
    };
}