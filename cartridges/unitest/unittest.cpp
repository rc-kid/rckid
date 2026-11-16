#include "platform/tests.h"

#include "kiddo/memory.h"

namespace kiddo {
    void initializeNoWindow(int argc, char * argv[]);
}


TEST(tests, AtLeastOneTestWorks) {
    EXPECT(true);
}

int main(int argc, char * argv[]) {
    kiddo::initializeNoWindow(argc, argv);
    return Test::RunAll(argc, argv);
}    
