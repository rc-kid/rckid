#include <platform/tests.h>
#include <rckid/rckid.h>

TEST(memory, arenaFree) {
    uint32_t freeHeap = rckid::memoryFreeHeap();
    rckid::memoryEnterArena();
    rckid::malloc(128);
    rckid::memoryLeaveArena();
    EXPECT(freeHeap == rckid::memoryFreeHeap());
}

