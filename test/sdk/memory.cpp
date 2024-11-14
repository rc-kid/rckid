#include <platform/tests.h>
#include <rckid/rckid.h>


TEST(memory, arenaLeave) {
    using namespace rckid;
    uint32_t freeHeap = memoryFree();
    Arena::enter();
    uint32_t afterArena = memoryFree();
    // entering arena allocated new arena header, total free memory should be lower by that
    EXPECT(freeHeap - sizeof(void*) == afterArena);
    Arena::malloc(128);
    uint32_t afterAlloc = memoryFree();
    // the 128bytes, no header
    EXPECT(afterArena - 128 == afterAlloc);
    Arena::leave();
    // everything gets deleted when done
    EXPECT(freeHeap == memoryFree());
}

TEST(memory, immediateDeleteDeallocates) {
    uint32_t freeHeap = rckid::memoryFree();
    void * ptr = rckid::malloc(128);
    EXPECT(rckid::memoryFree() + 128 + 4 == freeHeap);
    rckid::free(ptr);
    EXPECT(rckid::memoryFree() == freeHeap);
}

TEST(memory, nonImmediateDeleteDoesNotDeallocate) {
    uint32_t freeHeap = rckid::memoryFree();
    void * ptr1 = rckid::malloc(128);
    void * ptr2 = rckid::malloc(128);
    EXPECT(rckid::memoryFree() + (128 + 4) * 2 == freeHeap);
    rckid::free(ptr1);
    rckid::free(ptr2);
    EXPECT(rckid::memoryFree() != freeHeap);
}
