#include <platform/tests.h>

#include <kiddo/memory.h>

namespace kiddo::hal::device {
    void initializeNoWindow();
}

TEST(tests, AtLeastOneTestWorks) {
    EXPECT(true);
}

int main(int argc, char * argv[]) {
    kiddo::hal::device::initializeNoWindow();
    return Test::RunAll(argc, argv);
}    
