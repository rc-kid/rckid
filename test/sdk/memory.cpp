#include <platform/tests.h>
#include <rckid/rckid.h>

TEST(memory,  insideArena) {
    EXPECT(rckid::memoryInsideArena() == false);
    rckid::memoryEnterArena();
    EXPECT(rckid::memoryInsideArena() == true);
    rckid::memoryLeaveArena();
    EXPECT(rckid::memoryInsideArena() == false);
}

TEST(memory, arenaFree) {
    uint32_t freeHeap = rckid::memoryFreeHeap();
    rckid::memoryEnterArena();
    uint32_t afterArena = rckid::memoryFreeHeap();
    EXPECT(freeHeap - sizeof(void*) *2 == afterArena);
    rckid::malloc(128);
    uint32_t afterAlloc = rckid::memoryFreeHeap();
    // the 128bytes + 4 bytes chunk header 
    EXPECT(afterArena - 128 - 4 == afterAlloc);
    rckid::memoryLeaveArena();
    // everything gets deleted when done
    EXPECT(freeHeap == rckid::memoryFreeHeap());
}

TEST(memory, immediateDeleteDeallocates) {
    rckid::memoryEnterArena();
    uint32_t freeHeap = rckid::memoryFreeHeap();
    void * ptr = rckid::malloc(128);
    EXPECT(rckid::memoryFreeHeap() + 128 + 4 == freeHeap);
    rckid::free(ptr);
    EXPECT(rckid::memoryFreeHeap() == freeHeap);
    rckid::memoryLeaveArena();
}

TEST(memory, nonImmediateDeleteDoesNotDeallocate) {
    rckid::memoryEnterArena();
    uint32_t freeHeap = rckid::memoryFreeHeap();
    void * ptr1 = rckid::malloc(128);
    void * ptr2 = rckid::malloc(128);
    EXPECT(rckid::memoryFreeHeap() + (128 + 4) * 2 == freeHeap);
    rckid::free(ptr1);
    rckid::free(ptr2);
    EXPECT(rckid::memoryFreeHeap() != freeHeap);
    rckid::memoryLeaveArena();
}

TEST(memory, freeAndUsedCountersMatch) {
    uint32_t totalMem = rckid::memoryFreeHeap() + rckid::memoryUsedHeap();
    rckid::memoryEnterArena();
    EXPECT(totalMem = rckid::memoryFreeHeap() + rckid::memoryUsedHeap());
    void * ptr = rckid::malloc(128);
    EXPECT(totalMem = rckid::memoryFreeHeap() + rckid::memoryUsedHeap());
    rckid::free(ptr);
    EXPECT(totalMem = rckid::memoryFreeHeap() + rckid::memoryUsedHeap());
    rckid::memoryLeaveArena();
    EXPECT(totalMem = rckid::memoryFreeHeap() + rckid::memoryUsedHeap());
}

