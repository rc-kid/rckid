#include "rckid/rckid.h"
#include "rckid/apps/test/RawAudioTest.h"

using namespace rckid;

int main() {
    initialize();
    RawAudioTest test;
    test.run();
}

