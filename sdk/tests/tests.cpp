#include "platform/tests.h"


TEST(tests, AtLeastOneTestWorks) {
    EXPECT(true);
}

int main(int argc, char  * argv[]) {
    return Test::RunAll(argc, argv);
}    
