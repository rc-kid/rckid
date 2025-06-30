#include <platform/tests.h>
#include <rckid/rckid.h>

TEST(memory, heapAlloc) {
    using namespace rckid;

    RAMHeap::reset();
    EXPECT(RAMHeap::usedBytes() == 0);
    uint8_t * a = new uint8_t[10];
    EXPECT(RAMHeap::usedBytes() == 16);
    uint8_t * b = new uint8_t[20];
    EXPECT(RAMHeap::usedBytes() == 16 + 24);

    delete [] b;
    EXPECT(RAMHeap::usedBytes() == 16);
    delete [] a;
    EXPECT(RAMHeap::usedBytes() == 0);

    a = new uint8_t[10];
    b = new uint8_t[20];
    EXPECT(RAMHeap::usedBytes() == 16 + 24);
    delete [] a;
    EXPECT(RAMHeap::usedBytes() == 16 + 24);
    delete [] b;
    EXPECT(RAMHeap::usedBytes() == 0);
}

TEST(memory, heapAllocFromFreelist) {
    using namespace rckid;

    RAMHeap::reset();
    EXPECT(RAMHeap::usedBytes() == 0);
    uint8_t * a0 = new uint8_t[10];
    uint8_t * a1 = new uint8_t[10];
    uint8_t * a2 = new uint8_t[10];
    uint8_t * a3 = new uint8_t[10];
    uint8_t * a4 = new uint8_t[10];
    EXPECT(RAMHeap::freeBytes() == 0);
    EXPECT(RAMHeap::usedBytes() == 16 * 5);
    delete [] a0;
    EXPECT(RAMHeap::freeBytes() == 12);
    delete [] a1;
    EXPECT(RAMHeap::freeBytes() == 12 * 2);
    delete [] a3;
    EXPECT(RAMHeap::freeBytes() == 12 * 3);
    EXPECT(RAMHeap::usedBytes() == 16 * 5);
    delete [] a2;
    delete [] a4;
    EXPECT(RAMHeap::usedBytes() == 0);
    EXPECT(RAMHeap::freeBytes() == 0);

}

/*

TEST(memory, heapAllocation) {
    using namespace rckid;
    uint8_t * a = (uint8_t *)Heap::allocBytes(128);
    uint8_t * b = (uint8_t *)Heap::allocBytes(128);
    EXPECT(a + 128 != b);
    EXPECT(Heap::contains(a));
    EXPECT(Heap::contains(b));
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
    free(a);
    delete [] b;
    rckid::SystemMallocGuard::enable();
}

*/

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

