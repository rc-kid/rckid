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

    bool ptrInsideFantasyHeap(void * ptr) {
        return ptr >= & __bss_end__ && ptr < & __StackLimit;
    }

    void memoryInstrumentStackProtection() {
        char * x = & __StackLimit;
        x[0] = 'R';
        x[1] = 'C';
        x[2] = 'k';
        x[3] = 'i';
        x[4] = 'd';
    }

    void memoryCheckStackProtection() {
        char * x = & __StackLimit;
        if (x[0] != 'R' || x[1] != 'C' || x[2] != 'k' || x[3] != 'i' || x[4] != 'd')
            rckid::fatalError(Error::StackProtectionFailure, __LINE__, __FILE__);
    }

    char * Heap::end_ = & __StackLimit;

    char * Arena::start_ = & __bss_end__;
    char * Arena::end_ = & __bss_end__;

    void * Heap::malloc(uint32_t numBytes) {
        // we only allow total number of bytes reserved (including the header) to be divisible by 4, bump the number of bytes here accordingly
        numBytes = numBytes + sizeof(ChunkHeader);
        if ((numBytes & 3) != 0)
            numBytes = (numBytes & ~3) + 4;
        if (numBytes < 12)
            numBytes = 12;
        Chunk * freeChunk = freelist_;
        // see if we can use a chunk from freelist
        while (freeChunk != nullptr) {
            if (freeChunk->size() >= numBytes) {
                TRACE_HEAP("allocating " << numBytes<< " from existing chunk (size " << freeChunk->size << ", addr " << (uintptr_t)(& freeChunk->next) << ")");
                ASSERT(freeChunk->isFree());
                freeChunk->markAsAllocated();
                detachChunk(freeChunk);
                return & freeChunk->next;
            }
            freeChunk = freeChunk->next;
        }
        // we haven't found anything in the freelist, use the end of the heap to create one and advance the heap end
        end_ -= numBytes;
        Chunk * result = (Chunk*) end_;
        // if we are over the limit, panic
        ASSERT(end_ >= Arena::end_);
        // set the chunk's size and return it 
        result->setSize(numBytes);
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
        ASSERT(!chunk->isFree());
        // if this is the last allocated memory chunk, simply update the heap end instead of putting the chunk to a freelist. This is a transitive operation, i.e. if the chunk after the current one is free chunk, reclaim its memory too
        if (chunk->start() == end_) {
            TRACE_HEAP("deallocating last chunk (addr " << (uintptr_t)(ptr) << ")");
            while (true) {
                end_ = chunk->end();
                chunk = (Chunk*) chunk->end();
                if (!chunk->isFree())
                    break;
                detachChunk(chunk);
                TRACE_HEAP("deallocating transitive last chunk (addr " << (uintptr_t)(ptr) << ")");
            }
            return;
        }
        // get the chunk and prepend it to the freelist
        // TODO this is extremely ugly and inefficient, must be fixed in the future
        chunk->markAsFree();
        chunk->prev = nullptr;
        chunk->next = freelist_;
        if (freelist_ != nullptr)
            freelist_->prev = chunk;
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
            result -= c->size();
            c = c ->next;
        }
        return result;
    }

    void Heap::detachChunk(Chunk * chunk) {
        if (chunk == freelist_) {
            freelist_ = freelist_->next;
            if (freelist_ != nullptr) {
                ASSERT(freelist_->prev == chunk);
                freelist_->prev = nullptr;
            }
        } else {
            chunk->prev->next = chunk->next;
            if (chunk->next)
                chunk->next->prev = chunk->prev;
        }
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

    void *__wrap_calloc(size_t numBytes) {
        void * result = rckid::malloc(numBytes);
        memset(result, 0, numBytes);
        return result;
    }
}
