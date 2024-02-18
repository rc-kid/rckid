#include "rckid/rckid.h"
#include "rckid/apps/test/SensorsTest.h"

using namespace rckid;

int main() {
    initialize();
    start(SensorsTest{});
}
