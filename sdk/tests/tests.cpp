#include "platform/tests.h"

namespace rckid {
    void initializeNoWindow(int argc, char * argv[]);
}


TEST(tests, AtLeastOneTestWorks) {
    EXPECT(true);
}

int main(int argc, char * argv[]) {
    rckid::initializeNoWindow(argc, argv);
    return Test::RunAll(argc, argv);
}    
