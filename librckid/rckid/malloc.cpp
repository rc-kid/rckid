#include <new>
#include <cstdlib>
#include "rckid.h"
#include "stats.h"

#if (defined ARCH_RP2040)

extern char __bss_end__; 
extern char __StackLimit;

#else

char __StackLimit;
char __bss_end__;

#endif
namespace {


    using ChunkHeader = uint32_t;

    struct Chunk {
        uint32_t size;
        Chunk * next;

        Chunk(size_t size): size{size} {}

        char * start() {
            return reinterpret_cast<char*>(this);
        }

        /** Returns the pointer past the end of the chunk. which is the address of the chunk + the chunk header + chunk size. 
         */
        char * end() {
            return start() + sizeof(ChunkHeader) + allocatedSize();
        }

        size_t allocatedSize() {
            return size;
        }

    };

    /** Arena information, which contains a pointer to the previous arena and its own freelist. so that when the arena is removed, previous freelist can be restored. 
     */
    struct Arena {
        Chunk * freelist = nullptr;
        Arena * previous;

        Arena(Arena * prev): previous{prev} {}
    }; 

    //Chunk * freelist = nullptr;
    //char * heapEnd = & __bss_end__;
    char * heapEnd = & __bss_end__ + sizeof(Arena);

    unsigned allocated = 0;
    unsigned mallocCalls = 0;
    unsigned freeCalls = 0;

    Arena * arena = new (__builtin_assume_aligned(&__bss_end__, 8)) Arena{nullptr};
}

extern "C" {

    void resetHeap() {
        heapEnd = & __bss_end__ + sizeof(Arena);
        arena = new (__builtin_assume_aligned(&__bss_end__, 8)) Arena{nullptr};
    }

    void * __wrap_malloc(size_t numBytes) {
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
                allocated += freeChunk->size;
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
        allocated += numBytes + 4;
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
        allocated -= chunk->size; // the 4 bytes in the chunk header are still allocated
        chunk->next = arena->freelist;
        arena->freelist = chunk;
    }

} 

namespace rckid {

    size_t getFreeHeap() { return & __StackLimit - & __bss_end__ - allocated; }

    size_t getUsedHeap() { return allocated; }

    size_t getMallocCalls() { return mallocCalls; }

    size_t getFreeCalls() { return freeCalls; }

    /** Simply allocate new arena, which also creates new empty freelist to be used in it. 
     */
    void enterHeapArena() {
        arena = new (heapEnd) Arena{arena};
        heapEnd += sizeof (Arena);
    }

    /** Leaving the arena means resetting the heap end to the current arena pointer and then moving current arena to the previous one, which effectively reclaims all memory allocated in the arena we are leaving now. 
     */
    void leaveHeapArena() {
        // TODO BSOD in debug mode, do nothing in production
        if (arena->previous == nullptr)
            return;
        heapEnd = reinterpret_cast<char*>(arena);
        arena = arena->previous;
    }
}

