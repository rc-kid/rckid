#include "rckid.h"

// Heap is continuous space from _bss_end__ to __StackLimit, heap grows from __StackLimit down towards _bss_end, while the arena grows from __bss_end_ up. 
#if (defined ARCH_FANTASY)
// use 256 kb for RP2040 and 520 kb for RP2350
    char fantasyHeap[240 * 1024];
    char & __bss_end__ = *fantasyHeap;
    char & __StackLimit = *(fantasyHeap + sizeof(fantasyHeap));
#else
    // beginning of the heap
    extern char __bss_end__;
    // end of the heap 
    extern char __StackLimit;
#endif

namespace rckid {

    char * Heap::end_ = & __StackLimit;

    char * Arena::start_ = & __bss_end__;
    char * Arena::end_ = & __bss_end__;

    void * Heap::malloc(uint32_t numBytes) {
        Chunk * freeChunk = freelist_;
        Chunk * last = nullptr;
        // see if we can use a chunk from freelist
        while (freeChunk != nullptr) {
            if (freeChunk->size >= numBytes) {
                if (last == nullptr)
                    freelist_ = freeChunk->next;
                else 
                    last->next = freeChunk->next;
                TRACE_HEAP("allocating " << numBytes<< " from existing chunk (size " << freeChunk->size << ", addr " << (uintptr_t)(& freeChunk->next) << ")");
                return & freeChunk->next;
            }
            last = freeChunk;
            freeChunk = freeChunk->next;
        }
        // we haven't found anything in the freelist, use the end of the heap to create one and advance the heap end
        end_ -= (numBytes + sizeof(ChunkHeader));
        Chunk * result = (Chunk*) end_;
        // if we are over the limit, panic
        ASSERT(end_ >= Arena::end_);
        // set the chunk's size and return it 
        result->size = static_cast<uint32_t>(numBytes);
        TRACE_HEAP("allocating " << numBytes<< " from heap, address " << (uintptr_t)(& result->next));
        return &(result->next);
    }

    void Heap::free(void * ptr) {
        // deleting nullptr is noop        
        if (ptr == nullptr)
            return;
        ASSERT(Heap::contains(ptr)); 
        // deal with the chunk
        Chunk * chunk = (Chunk *)((char*) ptr - 4);
        // if this is the last allocated memory chunk, simply update the heap end
        if (chunk->start() == end_) {
            end_ = chunk->end();
            TRACE_HEAP("deallocating last chunk (addr " << (uintptr_t)(ptr) << ")");
            return;
        }
        // get the chunk and prepend it to the freelist
        // TODO this is extremely ugly and inefficient, must be fixed in the future
        chunk->next = freelist_;
        freelist_ = chunk;
        TRACE_HEAP("deallocating (addr " << (uintptr_t)(ptr) << ") and adding to freelist");
    }

    bool Heap::contains(void * ptr) {
        return ptr >= end_ && ptr < & __StackLimit;
    }

    uint32_t Heap::used() {
        return (& __StackLimit - end_);
    }

    uint32_t Heap::allocated() {
        uint32_t result = used();
        Chunk * c = freelist_;
        while (c != nullptr) {
            result -= c->size + sizeof(ChunkHeader);
            c = c ->next;
        }
        return result;
    }

    bool Arena::contains(void * ptr) {
        return ptr >= & __bss_end__ && ptr < end_;
    }

    uint32_t Arena::used() {
        return end_ - & __bss_end__;
    }

    uint32_t memoryFree() {
        uint32_t result = Heap::end_ - Arena::end_;
        TRACE_MEMORY("Free memory:        " << result);
        TRACE_MEMORY("Head used:          " << Heap::used());
        TRACE_MEMORY("Arena used:         " << Arena::used());
        TRACE_MEMORY("Current Arena used: " << Arena::usedCurrent());
        TRACE_MEMORY("Arena memory start: " << (uintptr_t) & __bss_end__);
        TRACE_MEMORY("Heap start:         " << (uintptr_t) & __StackLimit);
        return result;
    }

    void memoryReset() {
        Heap::freelist_ = nullptr;
        Heap::end_ = & __StackLimit;
        Arena::start_ = & __bss_end__;
        Arena::end_ = & __bss_end__;
        TRACE_MEMORY("Resetting memory");
        memoryFree();
    }

}

extern "C" {
    void *__wrap_malloc(size_t numBytes) { return rckid::malloc(numBytes); }
    void __wrap_free(void * ptr) { rckid::free(ptr); }
}
