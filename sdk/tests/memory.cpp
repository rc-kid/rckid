#include <platform/tests.h>
#include <rckid/rckid.h>

TEST(memory, arenaAllocation) {
    using namespace rckid;
    Arena::enter();
    uint8_t * a = (uint8_t*)Arena::allocBytes(128);
    uint8_t * b = (uint8_t*)Arena::allocBytes(128);
    EXPECT(a < b);
    EXPECT(b == a + 128);
    EXPECT(Arena::contains(a));
    EXPECT(Arena::contains(b));
    EXPECT(!Heap::contains(a));
    EXPECT(!Heap::contains(b));
    Arena::leave();
}

TEST(memory, arenaReset) {
    using namespace rckid;
    Arena::enter();
    uint8_t * a = (uint8_t*)Arena::allocBytes(128);
    uint8_t * b = (uint8_t*)Arena::allocBytes(128);
    Arena::reset();
    uint8_t * c = (uint8_t*)Arena::allocBytes(128);
    uint8_t * d = (uint8_t*)Arena::allocBytes(128);
    EXPECT(a == c);
    EXPECT(b == d);
    Arena::leave();
}

TEST(memory, arenaLeave) {
    using namespace rckid;
    Arena::enter();
    uint8_t * a = (uint8_t*)Arena::allocBytes(128);
    uint8_t * b = (uint8_t*)Arena::allocBytes(128);
    Arena::leave();
    Arena::enter();
    uint8_t * c = (uint8_t*)Arena::allocBytes(128);
    uint8_t * d = (uint8_t*)Arena::allocBytes(128);
    EXPECT(a == c);
    EXPECT(b == d);
    Arena::leave();
}

TEST(memory, arenaLeaveMemoryFootprint) {
    using namespace rckid;
    uint32_t freeHeap = memoryFree();
    Arena::enter();
    uint32_t afterArena = memoryFree();
    // entering arena allocated new arena header, total free memory should be lower by that
    EXPECT(freeHeap - sizeof(void*) == afterArena);
    Arena::allocBytes(128);
    uint32_t afterAlloc = memoryFree();
    // the 128bytes, no header
    EXPECT(afterArena - 128 == afterAlloc);
    Arena::leave();
    // everything gets deleted when done
    EXPECT(freeHeap == memoryFree());
}

TEST(memory, heapAllocation) {
    using namespace rckid;
    uint8_t * a = (uint8_t *)Heap::allocBytes(128);
    uint8_t * b = (uint8_t *)Heap::allocBytes(128);
    EXPECT(a + 128 != b);
    EXPECT(Heap::contains(a));
    EXPECT(Heap::contains(b));
    EXPECT(! Arena::contains(a));
    EXPECT(! Arena::contains(b));
    Heap::free(a);
    Heap::free(b);
}

TEST(memory, atLeast12Bytes) {
    using namespace rckid;
    uint32_t freeHeap = rckid::memoryFree();
    void * ptr = Heap::allocBytes(4);
    EXPECT(rckid::memoryFree() + 12 == freeHeap);
    Heap::free(ptr);
    EXPECT(rckid::memoryFree() == freeHeap);
}

TEST(memory, immediateDeleteDeallocates) {
    using namespace rckid;
    uint32_t freeHeap = rckid::memoryFree();
    void * ptr = Heap::allocBytes(128);
    EXPECT(rckid::memoryFree() + 128 + 4 == freeHeap);
    Heap::free(ptr);
    EXPECT(rckid::memoryFree() == freeHeap);
}

TEST(memory, nonImmediateDeleteDeallocates) {
    using namespace rckid;
    uint32_t freeHeap = rckid::memoryFree();
    void * ptr1 = Heap::allocBytes(128);
    void * ptr2 = Heap::allocBytes(128);
    EXPECT(rckid::memoryFree() + (128 + 4) * 2 == freeHeap);
    Heap::free(ptr1);
    Heap::free(ptr2);
    EXPECT(rckid::memoryFree() == freeHeap);
}


TEST(memory, ownHeapManagementEnabled) {
#ifdef RCKID_BACKEND_FANTASY
    rckid::SystemMallocGuard::disable();
#endif
    using namespace rckid;
    uint8_t * a = (uint8_t *)malloc(128);
    uint8_t * b = new uint8_t[128];
    EXPECT(a + 128 != b);
    EXPECT(Heap::contains(a));
    EXPECT(Heap::contains(b));
    EXPECT(! Arena::contains(a));
    EXPECT(! Arena::contains(b));
    free(a);
    delete [] b;
    rckid::SystemMallocGuard::enable();
}

/*
TEST(memory, astring) {
    using namespace rckid;
    Arena::reset();
    EXPECT(Arena::currentSize() == 0);
    char const * str = "foobar and some friends walked to a bar. This has to be long because of small string allocation optimization";
    size_t strl = strlen(str);
    {
        astring foo{str};
        EXPECT(Arena::currentSize() >= strl);
        EXPECT(Arena::contains(foo.c_str()));
    }
    EXPECT(Arena::currentSize() >= strl);
    Arena::reset();
}
    */

/*
TEST(memory, defaultAllocationIsHeap) {
    using namespace rckid;
    void * a = rckid::malloc(128);
    EXPECT(Heap::contains(a));
    EXPECT(! Arena::contains(a));
}

TEST(memory, arenaScope) {
    using namespace rckid;
    void * a = rckid::malloc(128);
    EXPECT(Heap::contains(a));
    EXPECT(! Arena::contains(a));
    {
        ArenaScope _{};
        void *b = rckid::malloc(128);
        EXPECT(! Heap::contains(b));
        EXPECT(Arena::contains(b));
    }
    void * c = rckid::malloc(128);
    EXPECT(Heap::contains(c));
    EXPECT(! Arena::contains(c));
    Arena::reset();
}

TEST(memory, explicitHeapInArenaScope) {
    using namespace rckid;
    ArenaScope _{};
    void * a = rckid::malloc(128);
    EXPECT(! Heap::contains(a));
    EXPECT(Arena::contains(a));
    void * b = Heap::malloc(128);
    EXPECT(Heap::contains(b));
    EXPECT(! Arena::contains(b));
    void * c = rckid::malloc(128);
    EXPECT(! Heap::contains(c));
    EXPECT(Arena::contains(c));
    Arena::reset();
}

TEST(memory, nestedArenaScope) {
    using namespace rckid;
    void * a = rckid::malloc(128);
    EXPECT(Heap::contains(a));
    EXPECT(! Arena::contains(a));
    {
        ArenaScope _{};
        void *b = rckid::malloc(128);
        EXPECT(! Heap::contains(b));
        EXPECT(Arena::contains(b));
        {
            ArenaScope __{};
            void *d = rckid::malloc(128);
            EXPECT(! Heap::contains(d));
            EXPECT(Arena::contains(d));
        }
        void *e = rckid::malloc(128);
        EXPECT(! Heap::contains(e));
        EXPECT(Arena::contains(e));
    }
    void * c = rckid::malloc(128);
    EXPECT(Heap::contains(c));
    EXPECT(! Arena::contains(c));
    Arena::reset();
}

TEST(memory, heapScopeInArenaScope) {
    using namespace rckid;
    void * a = rckid::malloc(128);
    EXPECT(Heap::contains(a));
    EXPECT(! Arena::contains(a));
    {
        ArenaScope _{};
        void *b = rckid::malloc(128);
        EXPECT(! Heap::contains(b));
        EXPECT(Arena::contains(b));
        {
            HeapScope __{};
            void *d = rckid::malloc(128);
            EXPECT(Heap::contains(d));
            EXPECT(! Arena::contains(d));
        }
        void *e = rckid::malloc(128);
        EXPECT(! Heap::contains(e));
        EXPECT(Arena::contains(e));
    }
    void * c = rckid::malloc(128);
    EXPECT(Heap::contains(c));
    EXPECT(! Arena::contains(c));
    Arena::reset();
}

TEST(memory, arenaMacro) {
    uint8_t * a = ARENA((uint8_t*)rckid::malloc(128));
    EXPECT(rckid::Arena::contains(a));
    rckid::Arena::reset();
}

TEST(memory, newArenaScope) {
    using namespace rckid;
    uint32_t before = memoryFree();
    {
        NewArenaScope _{};
        Arena::malloc(128);
        EXPECT(memoryFree() == before - 128 - sizeof(void*));
    }
    EXPECT(memoryFree() == before);
}
*/

