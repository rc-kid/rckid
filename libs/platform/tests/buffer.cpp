#include "../tests.h"
#include "../buffer.h"

TEST(platform, doubleBuffer_swap) {
    DoubleBuffer b(1024);
    EXPECT(b.size() == 1024);
    uint8_t * front = b.getFrontBuffer();
    uint8_t * back = b.getBackBuffer();
    EXPECT(front + 1024 == back);
    EXPECT(back == b.getBackBuffer());
    EXPECT(front == b.getFrontBuffer());
    b.swap();
    EXPECT(front == b.getBackBuffer());
    EXPECT(back == b.getFrontBuffer());
    b.swap();
    EXPECT(back == b.getBackBuffer());
    EXPECT(front == b.getFrontBuffer());
}

TEST(platform, doubleBuffer_cb) {
    bool swapped = false;
    DoubleBuffer b(1024, [&](DoubleBuffer &) mutable { swapped = true; });
    EXPECT(swapped == false);
    b.swap();
    EXPECT(swapped == true);
}
