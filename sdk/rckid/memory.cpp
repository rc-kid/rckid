#include "rckid.h"

#include <backend_config.h>

// this includes the memory boundaries - __StackLimit and __bss_end__, whose values are really backend dependent
RCKID_MEMORY_INITIALIZATION

namespace rckid {

    using ChunkHeader = uint32_t;
    struct HeapChunk;

    char * Heap::end_ = & __StackLimit;
    char * Arena::start_ = & __bss_end__;
    char * Arena::end_ = & __bss_end__;

    HeapChunk * freelist_ = nullptr;

    /** Heap allocated chunk.
     
        Chunk consists of header and body. Chunk header is 4 bytes long and is the chunk size (including the header). Since chunk sizes must be in multiples of 4, ths LSB (2 of them in fact) is free and is used to determine whether the chunk is allocated (0), or part of the freelist (1). When the chunk is allocated, its body is the user data, but when the chunk becomes part of the freelist, the body consists of a pointer to the previous and next chunks in the freelist.  

        NOTE for the fantasy backend: the chunk pointers are stored as offsets from the __bss_end__ symbol, which is the start of the fantasy heap. This is to ensure that the pointer sizes in the heap chunks fit in 4 bytes each even on 64bit systems. On physical devices, which are expected to be 32bit this is streamlined to actual pointers. 
     */
    struct HeapChunk {
        HeapChunk(uint32_t size): header_{size} {}

        char * start() { return (char*)this; }

        /** Returns the pointer past the end of the chunk. which is the address of the chunk + the chunk header + chunk size. 
         */
        char * end() { return start() + size(); }

        size_t size() { return header_ & ~3; }

        /** Sets size of the chunk and marks it as allocated.
         */
        void setSize(uint32_t bytes) { header_ = bytes; }

        bool isFree() { return header_ & CHUNK_IS_FREE; }

        void markAsAllocated() { header_ &= ~CHUNK_IS_FREE; }

        void markAsFree() { header_ |= CHUNK_IS_FREE; }

        void * ptr() {
            return & next_;
        }

        HeapChunk * next() {
#ifdef RCKID_BACKEND_FANTASY
            return (next_ == CHUNK_NULLPTR) ? nullptr : reinterpret_cast<HeapChunk*>((& __bss_end__) + next_);
#else
            return next_;
#endif
        }

        HeapChunk * prev() {
#ifdef RCKID_BACKEND_FANTASY
            return (prev_ == CHUNK_NULLPTR) ? nullptr : reinterpret_cast<HeapChunk*>((& __bss_end__) + prev_);
#else
            return prev_;
#endif
        }

        void setNext(HeapChunk * next) {
#ifdef RCKID_BACKEND_FANTASY
            next_ = (next == nullptr) ? CHUNK_NULLPTR : reinterpret_cast<char*>(next) - & __bss_end__;
#else
            next_ = next;
#endif
        }

        void setPrev(HeapChunk * prev) {
#ifdef RCKID_BACKEND_FANTASY
            prev_ = (prev == nullptr) ? CHUNK_NULLPTR : reinterpret_cast<char*>(prev) - & __bss_end__;
#else
            prev_ = prev;
#endif
        }

    private:
        ChunkHeader header_;
        // to ensure the chunk header is always 12 bytes even on 64bit systems (fantasy backend), we use relative offsets from the heap start instead of pointers for the next and prev chunk pointers. 
#ifdef RCKID_BACKEND_FANTASY
        uint32_t next_ = CHUNK_NULLPTR;
        uint32_t prev_ = CHUNK_NULLPTR;

        static constexpr uint32_t CHUNK_NULLPTR = 0xffffffff;
#else
        HeapChunk * next_ = nullptr;
        HeapChunk * prev_ = nullptr;
#endif

        static constexpr uint32_t CHUNK_IS_FREE = 1;

    }; // HeapChunk


    static_assert(sizeof(HeapChunk) == 12);

    /** Detaches given chunk from the freelist. This is useful when the last chunk on the heap is freed and the chunk above it is part of the freelist, in which case we can remove that chunk as well to fully reclaim the memory.
     */
    void detachChunk(HeapChunk * chunk) {
        if (chunk == freelist_) {
            freelist_ = freelist_->next();
            if (freelist_ != nullptr) {
                ASSERT(freelist_->prev() == chunk);
                freelist_->setPrev(nullptr);
            }
        } else {
            chunk->prev()->setNext(chunk->next());
            if (chunk->next())
                chunk->next()->setPrev(chunk->prev());
        }
    }

    /** Internal function that resets the entire memory (heap and arena). Use with extreme caution. Pretty much the only sensible thing to do after memoryReset is to show the blue screen of death.
     */
    void memoryReset() {
        freelist_ = nullptr;
        Heap::end_ = & __StackLimit;
        Arena::start_ = & __bss_end__;
        Arena::end_ = & __bss_end__;
    }

    void * Heap::allocBytes(uint32_t numBytes) {
        // we only allow total number of bytes reserved (including the header) to be divisible by 4, bump the number of bytes here accordingly
        numBytes = numBytes + sizeof(ChunkHeader);
        if ((numBytes & 3) != 0)
            numBytes = (numBytes & ~3) + 4;
        if (numBytes < 12)
            numBytes = 12;
        HeapChunk * freeChunk = freelist_;
        // see if we can use a chunk from freelist
        while (freeChunk != nullptr) {
            if (freeChunk->size() >= numBytes) {
                ASSERT(freeChunk->isFree());
                freeChunk->markAsAllocated();
                detachChunk(freeChunk);
                return freeChunk->next();
            }
            freeChunk = freeChunk->next();
        }
        // we haven't found anything in the freelist, use the end of the heap to create one and advance the heap end
        end_ -= numBytes;
        HeapChunk * result = (HeapChunk*) end_;
        // if we are over the limit, panic
        ASSERT(end_ >= Arena::end_);
        // set the chunk's size and return it 
        result->setSize(numBytes);
        LOG(LL_HEAP, "Allocating " << numBytes << " bytes from " << (result->ptr()));
        return result->ptr();
    }

    void Heap::free(void * ptr) {
        // deleting nullptr is noop        
        if (ptr == nullptr)
            return;
        ASSERT(Heap::contains(ptr)); 
        // deal with the chunk
        HeapChunk * chunk = (HeapChunk *)((char*) ptr - 4);
        ASSERT(!chunk->isFree());
        // if this is the last allocated memory chunk, simply update the heap end instead of putting the chunk to a freelist. This is a transitive operation, i.e. if the chunk after the current one is free chunk, reclaim its memory too
        if (chunk->start() == end_) {
            while (true) {
                end_ = chunk->end();
                chunk = (HeapChunk*) chunk->end();
                if (!chunk->isFree())
                    break;
                detachChunk(chunk);
            }
            LOG(LL_HEAP, "Freeing joined chunk " << ptr << " (size " << chunk->size() << ")");
            return;
        }
        // get the chunk and prepend it to the freelist
        // TODO this is extremely ugly and inefficient, must be fixed in the future
        LOG(LL_HEAP, "Freeing standalone chunk " << ptr << " (size " << chunk->size() << ")");
        chunk->markAsFree();
        chunk->setPrev(nullptr);
        chunk->setNext(freelist_);
        if (freelist_ != nullptr)
            freelist_->setPrev(chunk);
        freelist_ = chunk;
    }

    void Heap::tryFree(void * ptr) {
        if (contains(ptr))
            Heap::free(ptr);
    }

    bool Heap::contains(void const * ptr) {
        return ptr >= Heap::end_ && ptr <= & __StackLimit;

    }

    bool Arena::contains(void const * ptr) {
        return ptr >= & __bss_end__ && ptr < Arena::end_;
    }

    void memoryInstrumentStackProtection() {
        char * x = & __StackLimit;
#ifdef __GNUC__        
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"        
#pragma GCC diagnostic ignored "-Wstringop-overflow"        
#endif
        x[0] = 'R';
        x[1] = 'C';
        x[2] = 'k';
        x[3] = 'i';
        x[4] = 'd';
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    }

    void memoryCheckStackProtection() {
        char * x = & __StackLimit;
#ifdef __GNUC__        
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
        ERROR_IF(error::StackProtectionFailure, (x[0] != 'R' || x[1] != 'C' || x[2] != 'k' || x[3] != 'i' || x[4] != 'd'));
#ifdef __GNUC__        
#pragma GCC diagnostic pop
#endif
    }

} // namespace rckid