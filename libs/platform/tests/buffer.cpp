#include "../tests.h"
#include "../buffer.h"

TEST(platform, doubleBuffer_swap) {
    DoubleBuffer<uint8_t> b(1024);
    EXPECT(b.size() == 1024);
    uint8_t * front = b.front();
    uint8_t * back = b.back();
    EXPECT(back == b.back());
    EXPECT(front == b.front());
    b.swap();
    EXPECT(front == b.back());
    EXPECT(back == b.front());
    b.swap();
    EXPECT(back == b.back());
    EXPECT(front == b.front());
}
