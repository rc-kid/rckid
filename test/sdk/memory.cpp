#include <platform/tests.h>
#include <rckid/rckid.h>

TEST(memory, arenaLeave) {
    uint32_t freeHeap = rckid::memoryFreeHeap();
    rckid::memoryEnterArena();
    uint32_t afterArena = rckid::memoryFreeHeap();
    // entering arena allocated new arena header, total free memory should be lower by that
    EXPECT(freeHeap - sizeof(void*) == afterArena);
    rckid::mallocArena(128);
    uint32_t afterAlloc = rckid::memoryFreeHeap();
    // the 128bytes, no header
    EXPECT(afterArena - 128 == afterAlloc);
    rckid::memoryLeaveArena();
    // everything gets deleted when done
    EXPECT(freeHeap == rckid::memoryFreeHeap());
}

TEST(memory, immediateDeleteDeallocates) {
    uint32_t freeHeap = rckid::memoryFreeHeap();
    void * ptr = rckid::malloc(128);
    EXPECT(rckid::memoryFreeHeap() + 128 + 4 == freeHeap);
    rckid::free(ptr);
    EXPECT(rckid::memoryFreeHeap() == freeHeap);
}

TEST(memory, nonImmediateDeleteDoesNotDeallocate) {
    uint32_t freeHeap = rckid::memoryFreeHeap();
    void * ptr1 = rckid::malloc(128);
    void * ptr2 = rckid::malloc(128);
    EXPECT(rckid::memoryFreeHeap() + (128 + 4) * 2 == freeHeap);
    rckid::free(ptr1);
    rckid::free(ptr2);
    EXPECT(rckid::memoryFreeHeap() != freeHeap);
}

TEST(memory, freeAndUsedCountersMatch) {
    uint32_t totalMem = rckid::memoryFreeHeap() + rckid::memoryUsedHeap();
    EXPECT(totalMem = rckid::memoryFreeHeap() + rckid::memoryUsedHeap());
    void * ptr = rckid::malloc(128);
    EXPECT(totalMem = rckid::memoryFreeHeap() + rckid::memoryUsedHeap());
    rckid::free(ptr);
    EXPECT(totalMem = rckid::memoryFreeHeap() + rckid::memoryUsedHeap());
    EXPECT(totalMem = rckid::memoryFreeHeap() + rckid::memoryUsedHeap());
}

