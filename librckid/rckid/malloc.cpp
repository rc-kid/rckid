#include <cstdlib>
#include "rckid.h"
#include "stats.h"

extern char __bss_end__; 
extern char __StackLimit;

namespace {
    struct Chunk {
        uint32_t size;
        Chunk * next;

        Chunk(size_t size): size{size} {}
    };

    Chunk * freelist = nullptr;
    char * heapEnd = & __bss_end__;

    unsigned allocated = 0;
    unsigned mallocCalls = 0;
    unsigned freeCalls = 0;
}

extern "C" {

    void * __wrap_malloc(size_t numBytes) {
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
                allocated += freeChunk->size;
                return & freeChunk->next;
            }
            last = freeChunk;
            freeChunk = freeChunk->next;
        }
        // we haven't found anything in the freelist, use the end of the heap to create one and advance the heap end
        Chunk * result = (Chunk*) heapEnd;
        heapEnd += numBytes + 4; 
        // TODO if we are over the limit, panic
        allocated += numBytes + 4;
        // set the chunk's size and return it 
        result->size = numBytes;
        return &(result->next);
    }

    void __wrap_free(void * ptr) {
        ++freeCalls;
        // get the chunk and prepend it to the freelist
        // TODO this is extremely ugly and inefficient, must be fixed in the future
        Chunk * chunk = (Chunk *)((char*) ptr - 4);
        allocated -= chunk->size; // the 4 bytes in the chunk header are still allocated
        chunk->next = freelist;
        freelist = chunk;
    }

} 

namespace rckid {

    size_t getFreeHeap() { return & __StackLimit - & __bss_end__ - allocated; }

    size_t getUsedHeap() { return allocated; }

    size_t getMallocCalls() { return mallocCalls; }

    size_t getFreeCalls() { return freeCalls; }

    void enterHeapArena() {

    }

    void leaveHeapArena() {

    }
}