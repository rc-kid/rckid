#include <platform/tests.h>
#include <rckid/memory.h>


uint8_t const x[] = { 1, 2, 3, 4 };
uint8_t y[] = { 1, 2, 3, 4 };

TEST(memory, immutable) {
    EXPECT(sizeof(x) == 4);
    EXPECT(sizeof(y) == 4);
    EXPECT(rckid::hal::memory::isImmutableDataPtr(& x) == true);
    EXPECT(rckid::hal::memory::isImmutableDataPtr(& y) == false);
}

TEST(memory, heapAlloc) {
    using namespace rckid;

    Heap::UseAndReserveGuard h;

    EXPECT(h.usedDelta() == 0);
    uint8_t * a = new uint8_t[10];
    EXPECT(h.usedDelta() == 16);
    EXPECT(h.reservedDelta() == 16);
    uint8_t * b = new uint8_t[20];
    EXPECT(h.usedDelta() == 16 + 24);
    EXPECT(h.reservedDelta() == 16 + 24);
    delete [] b;
    EXPECT(h.usedDelta() == 16);
    EXPECT(h.reservedDelta() == 16);
    delete [] a;
    EXPECT(h.usedDelta() == 0);
    EXPECT(h.reservedDelta() == 0);

    a = new uint8_t[10];
    b = new uint8_t[20];
    EXPECT(h.reservedDelta() == 16 + 24);
    delete [] a;
    EXPECT(h.reservedDelta() == 16 + 24);
    delete [] b;
    EXPECT(h.reservedDelta() == 0);
}

TEST(memory, heapAllocFromFreelist) {
    using namespace rckid;

    Heap::UseAndReserveGuard h;
    uint8_t * a0 = new uint8_t[10];
    uint8_t * a1 = new uint8_t[10];
    uint8_t * a2 = new uint8_t[10];
    uint8_t * a3 = new uint8_t[10];
    uint8_t * a4 = new uint8_t[10];
    EXPECT(h.reservedDelta() == 16 * 5);
    EXPECT(h.usedDelta() == 16 * 5);
    // delete the chunks in non sequential order to check freelist merging when allocating large chunk at the end
    delete [] a0;
    EXPECT(h.reservedDelta() == 16 * 5);
    EXPECT(h.usedDelta() == 16 * 4);
    delete [] a1;
    EXPECT(h.reservedDelta() == 16 * 5);
    EXPECT(h.usedDelta() == 16 * 3);
    delete [] a3;
    EXPECT(h.reservedDelta() == 16 * 5);
    EXPECT(h.usedDelta() == 16 * 2);
    delete [] a2;
    EXPECT(h.reservedDelta() == 16 * 5);
    EXPECT(h.usedDelta() == 16 * 1);
    // this would increase reservation if chunks are not merged above
    a0 = new uint8_t[40];
    EXPECT(h.reservedDelta() == 16 * 5);
    EXPECT(h.usedDelta() == 48 + 16);
    delete [] a0;
    delete [] a4;
    EXPECT(h.usedDelta() == 0);
    EXPECT(h.reservedDelta() == 0);
}

