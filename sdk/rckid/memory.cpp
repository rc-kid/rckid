#include "rckid.h"

// Heap is continuous space from _bss_end__ to __StackLimit, heap grows from __StackLimit down towards _bss_end, while the arena grows from __bss_end_ up. 
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
        ChunkHeader size;
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

    Chunk * freelist = nullptr;

    // heap starts at __StackLimit and grows down towards __bss_end_. This is the pointer to the end (bottom) of the heap. 
    char * heapEnd = & __StackLimit;


    PACKED(struct Arena {
        Arena * previous;
    });

    static_assert(sizeof(Arena) == sizeof(Arena*));

    // arena starts at the end of bss and grows up towards the heap. This is the end of the arena, i.e. place of next arena allocation. 
    char * arenaEnd = & __bss_end__;

    // beginning of the current arena. Anything from arenastart to arena end will reclaimed when the leaving. 
    Arena * arenaStart = nullptr;


    uint32_t mallocCalls = 0;
    uint32_t freeCalls = 0;

} // anonymous namespace for memory functions

extern "C" {
    void *__wrap_malloc(size_t numBytes) {
        ++mallocCalls;
        Chunk * freeChunk = freelist;
        Chunk * last = nullptr;
        // see if we can use a chunk from freelist
        while (freeChunk != nullptr) {
            if (freeChunk->size >= numBytes) {
                if (last == nullptr)
                    freelist = freeChunk->next;
                else 
                    last->next = freeChunk->next;
                TRACE_MEMORY("allocating " << numBytes<< " from existing chunk");
                return & freeChunk->next;
            }
            last = freeChunk;
            freeChunk = freeChunk->next;
        }
        // we haven't found anything in the freelist, use the end of the heap to create one and advance the heap end
        heapEnd -= (numBytes + sizeof(ChunkHeader));
        Chunk * result = (Chunk*) heapEnd;
        // if we are over the limit, panic
        ASSERT(heapEnd >= arenaEnd);
        // set the chunk's size and return it 
        result->size = static_cast<uint32_t>(numBytes);
        TRACE_MEMORY("allocating " << numBytes<< " from heap, free " << rckid::memoryFreeHeap());
        return &(result->next);
    }

    void __wrap_free(void * ptr) {
        // deleting nullptr is noop        
        if (ptr == nullptr)
            return;
        // check that we are freeing memory that is higher than the current arena, otherwise we are freeing from a previous arena which is wrong
        ASSERT(ptr > heapEnd); 
        ++freeCalls;
        // deal with the chunk
        Chunk * chunk = (Chunk *)((char*) ptr - 4);
        // if this is the last allocated memory chunk, simply update the heap end
        if (chunk->start() == heapEnd) {
            heapEnd = chunk->end();
            TRACE_MEMORY("deallocating last chunk, free " << rckid::memoryFreeHeap());
            return;
        }
        // get the chunk and prepend it to the freelist
        // TODO this is extremely ugly and inefficient, must be fixed in the future
        chunk->next = freelist;
        freelist = chunk;
        TRACE_MEMORY("deallocating and adding to freelist, free " << rckid::memoryFreeHeap());
    }

}

namespace rckid {

    uint32_t memoryFreeHeap() {
        ASSERT(heapEnd >= arenaEnd);
        return static_cast<uint32_t>(heapEnd - arenaEnd);
    }
    uint32_t memoryUsedHeap() {
        return static_cast<uint32_t>(heapEnd - & __bss_end__);
    }

    bool memoryIsOnHeap(void * ptr) {
        return (ptr >= heapEnd) && (ptr < & __StackLimit);
    }

    bool memoryIsInArena(void * ptr) {
        return (ptr >= & __bss_end__) && (ptr < arenaEnd);
    }

    bool memoryIsInCurrentArena(void * ptr) {
        if (arenaStart == nullptr)
            return (ptr >= & __bss_end__) && (ptr < arenaEnd);
        else 
            return (ptr >= arenaStart) && (ptr < arenaEnd);
    }

    void memoryEnterArena() {
        Arena * a = mallocArena<Arena>();
        a->previous = arenaStart;
        arenaStart = a;
        TRACE_MEMORY("Entering arena @" << (void*)(a) << " (previous @ " << (void*)a->previous <<", free heap " << memoryFreeHeap() << ")");
    }

    void memoryLeaveArena() {
        ASSERT(arenaStart != nullptr); 
        Arena * old = arenaStart;
        (void)old; // ignore the warning of unused variable when not tracing memory 
        arenaEnd = (char*) arenaStart;
        arenaStart = arenaStart->previous;
        TRACE_MEMORY("Leaving arena @" << (void*)(old) << " (previous @ " << (void*)arenaStart <<", free heap " << memoryFreeHeap() << ")");
    }

    // internal 
    void memoryResetArena() {
        arenaStart = nullptr;
        arenaEnd = & __bss_end__;
    }

    void * mallocArena(size_t numBytes) {
        void * result = arenaEnd;
        arenaEnd += numBytes;
        // if we are over the limit, panic
        ASSERT(arenaEnd <= heapEnd);
        return result;
    }


    void * malloc(size_t numBytes) { return __wrap_malloc(numBytes); }

    void free(void * ptr) { __wrap_free(ptr); }

    char * heapStart() { return & __bss_end__; }

} // namespace rckid
