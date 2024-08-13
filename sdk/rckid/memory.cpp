#include "rckid.h"

#if (defined ARCH_FANTASY)
    char fantasyHeap[520 * 1024];
    char & __bss_end__ = *fantasyHeap;
    char & __StackLimit = *(fantasyHeap + sizeof(fantasyHeap));
#else
    // beginning of the heap
    extern char __bss_end__;
    // end of the heap 
    extern char __StackLimit;
#endif

namespace {

    using ChunkHeader = uint32_t;

    /** Memory chunk. 
     
        Each chunk contains its size, followed by the actual data of the chunk (the user available allocated memory )
     */
    PACKED(struct Chunk {
        uint32_t size;
        Chunk * next;

        Chunk(uint32_t size): size{size} {}

        char * start() {
            return reinterpret_cast<char*>(this);
        }

        /** Returns the pointer past the end of the chunk. which is the address of the chunk + the chunk header + chunk size. 
         */
        char * end() {
            return start() + sizeof(ChunkHeader) + size;
        }

        size_t allocatedSize() {
            return size;
        }

    });

    static_assert(sizeof(Chunk) == sizeof(ChunkHeader) + sizeof(Chunk*));

    /** Memory Arena
     
        Contains a pointer to the previous arena and its own freelist. so that when the arena is removed, previous freelist can be restored.
     */
    struct Arena {
        Chunk * freelist = nullptr;
        Arena * previous;

        explicit Arena(Arena * prev): previous{prev} {}
    }; 

    static_assert(sizeof(Arena) == sizeof(void*) * 2);

    Arena * arena = new (__builtin_assume_aligned(&__bss_end__, 8)) Arena{nullptr};
    char * heapEnd = & __bss_end__ + sizeof(Arena);

    uint32_t mallocCalls = 0;
    uint32_t freeCalls = 0;

} // anonymous namespace for memory functions

extern "C" {
    void *__wrap_malloc(size_t numBytes) {
        ++mallocCalls;
        Chunk * freeChunk = arena->freelist;
        Chunk * last = nullptr;
        // see if we can use a chunk from freelist
        while (freeChunk != nullptr) {
            if (freeChunk->size >= numBytes) {
                if (last == nullptr)
                    arena->freelist = freeChunk->next;
                else 
                    last->next = freeChunk->next;
                return & freeChunk->next;
            }
            last = freeChunk;
            freeChunk = freeChunk->next;
        }
        // we haven't found anything in the freelist, use the end of the heap to create one and advance the heap end
        Chunk * result = (Chunk*) heapEnd;
        heapEnd += numBytes + sizeof(ChunkHeader); 
        // if we are over the limit, panic
        ASSERT(heapEnd <= & __StackLimit);
        // set the chunk's size and return it 
        result->size = numBytes;
        return &(result->next);
    }

    void __wrap_free(void * ptr) {
        // check that we are freeing memory that is higher than the current arena, otherwise we are freeing from a previous arena which is wrong
        ASSERT(ptr > & arena); 
        ++freeCalls;
        // deal with the chunk
        Chunk * chunk = (Chunk *)((char*) ptr - 4);
        // if this is the last allocated memory chunk, simply update the heap end
        if (chunk->end() == heapEnd) {
            heapEnd = chunk->start();
            return;
        }

        // get the chunk and prepend it to the freelist
        // TODO this is extremely ugly and inefficient, must be fixed in the future
        chunk->next = arena->freelist;
        arena->freelist = chunk;
    }


}




namespace rckid {

    uint32_t memoryFreeHeap() {
        return & __StackLimit - heapEnd;
    }
    uint32_t memoryUsedHeap() {
        return heapEnd - & __bss_end__;
    }

    bool memoryIsOnHeap(void * ptr) {
        return (ptr >= & __bss_end__) && (ptr < & __StackLimit);
    }

    bool memoryIsInCurrentArena(void * ptr) {
        return (ptr >= arena) && (ptr < & __StackLimit);
    }

    void memoryEnterArena() {
        arena = new (heapEnd) Arena{arena};
        heapEnd += sizeof (Arena);
    }

    void memoryLeaveArena() {
        // TODO BSOD in debug mode, do nothing in production
        if (arena->previous == nullptr)
            return;
        heapEnd = reinterpret_cast<char*>(arena);
        arena = arena->previous;
    }

    void * malloc(size_t numBytes) { return __wrap_malloc(numBytes); }

    void free(void * ptr) { __wrap_free(ptr); }

} // namespace rckid
