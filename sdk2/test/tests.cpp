#include <platform/tests.h>

#include <rckid/memory.h>

namespace rckid::hal::device {
    void initializeNoWindow();
}

TEST(tests, AtLeastOneTestWorks) {
    EXPECT(true);
}

int main(int argc, char * argv[]) {
    rckid::hal::device::initializeNoWindow();
    return Test::RunAll(argc, argv);
}    
