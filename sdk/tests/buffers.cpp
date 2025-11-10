#include <platform/tests.h>
#include <rckid/utils/buffers.h>

TEST(platform, doubleBuffer_swap) {
    using namespace rckid;
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

TEST(platform, multibuffer) {
    using namespace rckid;
    MultiBuffer<uint16_t> mb(256, 8);
    EXPECT(mb.size() == 256);
    EXPECT(mb.numBuffers() == 8);
    EXPECT(mb.nextReady() == nullptr);
    // test single buffer  
    uint16_t * x = mb.nextFree();
    EXPECT(x != nullptr);
    mb.markReady(x);
    uint16_t * y = mb.nextReady();
    EXPECT(y == x);
    EXPECT(mb.nextReady() == nullptr);
    // return to free buffers
    mb.markFree(y);
    // get all free buffers
    uint16_t * buffers[8];
    for (uint32_t i = 0; i < 8; ++i) {
        buffers[i] = mb.nextFree();
        EXPECT(buffers[i] != nullptr);
    }
    EXPECT(mb.nextFree() == nullptr);
    // mark all ready
    for (uint32_t i = 0; i < 8; ++i)
        mb.markReady(buffers[i]);
    // verify
    for (uint32_t i = 0; i < 8; ++i) {
        uint16_t * b = mb.nextReady();
        EXPECT(b == buffers[i]);
    }
    EXPECT(mb.nextReady() == nullptr);
    EXPECT(mb.nextFree() == nullptr);
}
