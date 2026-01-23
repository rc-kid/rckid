#include <platform/tests.h>

#include <rckid/stream.h>

namespace {
    uint8_t const foo[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

} // namespace rckid

TEST(stream, memoryStream) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    MemoryStream s{foo, sizeof(foo)};
    EXPECT(g_.usedDelta() == 0);
    EXPECT(g_.reservedDelta() == 0);
    EXPECT(s.size() == sizeof(foo));
    EXPECT(s.peek() == 1);
    EXPECT(s.peek() == 1);
    EXPECT(s.peek() == 1);
    EXPECT(s.readByte() == 1);
    EXPECT(s.peek() == 2);
    EXPECT(s.readByte() == 2);
    EXPECT(s.readByte() == 3);
    EXPECT(s.readByte() == 4);
    EXPECT(s.readByte() == 5);
    EXPECT(s.readByte() == 6);
    EXPECT(s.readByte() == 7);
    EXPECT(s.readByte() == 8);
    EXPECT(s.readByte() == 9);
    EXPECT(s.readByte() == 10);
    EXPECT(!s.readByte());
    s.seek(1);
    EXPECT(s.peek() == 2);
    EXPECT(s.readByte() == 2);
    s.writeByte(33);
    EXPECT(g_.usedDelta() == 16);
    EXPECT(g_.reservedDelta() == 16);
    s.seek(0);
    EXPECT(s.readByte() == 1);
    EXPECT(s.readByte() == 2);
    EXPECT(s.readByte() == 33);
}   
