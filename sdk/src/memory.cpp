#include <kiddo/error.h>
#include <kiddo/log.h>
#include <kiddo/memory.h>
#include <kiddo/hal.h>


namespace kiddo {

    /** Heap Chunk
     
        Heap chunks are tailored towards very little memory waste. Each chunk consists of a header that is only 4 bytes and contains current chunk's size in Chunks (8 bytes) as well as previous chunk's size in chunks. As the MSB of previous chunk's size is occupied by a free bit, we can allocate at most 32767 chunks (262136 bytes) in a single allocation, of which 4 bytes are used by the header itself, leaving 262132 bytes for user data.

        Minimal allocation physical allocation is 8 bytes, so that when the chunk is free, its data can hold 2 pointers for the free list in the data portion. Minimal user allocation is 4 bytes with increments by 8 (chunk header + min 4 bytes of payload, then incremented by extra "chunks"). 
     */
    class Heap::Chunk {
    public:
        /** State of the heap chunk */
        enum class State {
            Manual = 0b00,
            GC = 0b01,
            GCMarked = 0b10,
            Free = 0b11,
        }; 

        static constexpr uint32_t HEADER_SIZE = 4;

        uint32_t payloadSize() const { return (headerSize_ & ~FREE_BIT) * sizeof(Chunk) - 4; }

        State state() const {
            return static_cast<State>(((headerSize_ & FREE_BIT) >> 14) | ((headerPrevSize_ & FREE_BIT) >> 15));
        }

        void setState(State s) {
            uint16_t state = static_cast<uint16_t>(s);
            headerSize_ = (headerSize_ & ~FREE_BIT) | ((state & 0b10) << 14);
            headerPrevSize_ = (headerPrevSize_ & ~FREE_BIT) | ((state & 0b01) << 15);
        }

        bool isFree() const { return state() == State::Free; }

        //bool isFree() const { return (headerPrevSize_ & FREE_BIT) != 0; }

        Chunk * nextFree() { return offsetToPtr(nextFree_); }
        Chunk * prevFree() { return offsetToPtr(prevFree_); }

        Chunk * nextAllocation() { return this + headerSize(); }

        Chunk * prevAllocation() { 
            uint32_t prevSize = headerPrevSize_ & ~FREE_BIT;
            return prevSize == 0 ? nullptr : this - prevSize;
        }

        uint16_t prevAllocationSize() const { return headerPrevSize_ & ~FREE_BIT; }

        void initializeAllocated(uint16_t size, uint16_t prevSize) {
            ASSERT(size > 0);
            headerSize_ = size;
            headerPrevSize_ = prevSize;
        }

        void detachFromFreelist() {
            ASSERT(isFree());
            if (Heap::freelist_ == this) {
                Heap::freelist_ = prevFree();
            } else {
                if (prevFree_ != OFFSET_NULL)
                    prevFree()->nextFree_ = nextFree_;
                if (nextFree_ != OFFSET_NULL)
                    nextFree()->prevFree_ = prevFree_;
            }
        }

        void addToFreelist() {
            ASSERT(!isFree());
            setState(State::Free);
            //headerPrevSize_ |= FREE_BIT;
            prevFree_ = ptrToOffset(Heap::freelist_);
            if (Heap::freelist_ != nullptr)
                Heap::freelist_->nextFree_ = ptrToOffset(this);
            Heap::freelist_ = this;
        }

        void makeAllocated() {
            ASSERT(isFree());
            setState(State::Manual);
            //headerPrevSize_ &= ~FREE_BIT;
        }

        /** Takes a chunk and enlarges it by given size. 
            
            The size must be equivalent to the sum of one or more chunks immediately after it as the new chunk cannot be created in the middle of existing allocations. Furthermore the new chunk size must be smaller than the max chunk size (0x7fff chunks) so that we can still keep the prev chunk size in the header prev size field for quick coalescing. 
            */
        void enlargeBy(uint16_t by) {
            ASSERT((headerSize() + by) < FREE_BIT); // otherwise we would create too large a chunk to store in the prev size field together with the free bit
            uint32_t ownSize = headerSize();
            setHeaderSize(ownSize + by);
            Chunk * next = nextAllocation();
            ASSERT(next < Heap::heapEnd_); // we do not expect to enlarge the last chunk
            next->setHeaderPrevSize(headerSize());
            //next->headerPrevSize_ += ownSize;
        }

        /** Splits the chunk into two. 
         
            Resizes itself to the given size and returns the newly created chunk that follows it with the remainder of the size. 
            */
        Chunk * splitBy(uint16_t newSize) {
            ASSERT(newSize < headerSize());
            ASSERT(! isFree()); // the chunk is not expected to be in freelist when split
            Chunk * next = this + newSize;
            // new header in the middle of the chunk contains garbage, so it must be fully initialized (Manual state) instead of preserving the state bits
            next->initializeAllocated(headerSize() - newSize, newSize);
            setHeaderSize(newSize);
            Chunk * nextNext = next->nextAllocation();
            if (nextNext < Heap::heapEnd_)
                nextNext->setHeaderPrevSize(next->headerSize());
            return next;
        }

        void * data() { return reinterpret_cast<uint8_t*>(this) + 4; }

    private:

        friend class Heap;

        uint16_t headerSize() const { return headerSize_ & ~FREE_BIT; }
        void setHeaderSize(uint16_t size) { headerSize_ = (headerSize_ & FREE_BIT) | size; }
        void setHeaderPrevSize(uint16_t size) { headerPrevSize_ = (headerPrevSize_ & FREE_BIT) | size; }

        uint16_t headerSize_;
        uint16_t headerPrevSize_;
        uint16_t prevFree_;
        uint16_t nextFree_;

        /** MSB of header sizes (prev and next) is used to indicate the state of the chunk.
         */
        static constexpr uint16_t FREE_BIT = 0x8000;
        static constexpr uint16_t OFFSET_NULL = 0xffff;

        static Chunk * offsetToPtr(uint16_t offset) {
            if (offset == OFFSET_NULL)
                return nullptr;
            else 
                return Heap::heapStart_ + offset;
        }

        static uint16_t ptrToOffset(Chunk * ptr) {
            if (ptr == nullptr)
                return OFFSET_NULL;
            return ptr - Heap::heapStart_;
        }

    }; // Heap::Chunk

    void * Heap::tryAlloc(uint32_t numBytes) {
        static_assert(sizeof(Chunk) == 8);        
        ASSERT(heapEnd_ >= heapStart_);
        // first determine how many bytes we actually need by adding the extra header size (4 bytes) and then convert bytes to chunk sizes granularity
        numBytes += 4;
        uint32_t numChunks = (numBytes % 8 == 0) ? (numBytes >> 3) : ((numBytes >> 3) + 1);
        ASSERT(numChunks < 32768);
        // walk the freelist to see if we can reuse some free chunk from the middle of the heap
        Chunk * bestFit = nullptr;
        Chunk * x = freelist_;
        while (x != nullptr) {
            ASSERT(x->isFree());
            if (x->headerSize() == numChunks) {
                bestFit = x;
                break;
            }
            if (x->headerSize() > numChunks) {
                if (bestFit == nullptr)
                    bestFit = x;
                else if (x->headerSize() < bestFit->headerSize()) {
                    bestFit = x;
                }
            }
            x = x->prevFree();
        }
        // if we have found a chunk we can put the value in, do so. If the chunk is too large, siply split it and add the remainder back to the freelist. 
        if (bestFit != nullptr) {
            LOG(LL_HEAP, "Alloc " << (numChunks * 8) << " from freelist " << bestFit);
            ASSERT(bestFit->isFree());
            bestFit->detachFromFreelist();
            bestFit->makeAllocated();
            if (bestFit->headerSize() > numChunks) {
                Chunk * next = bestFit->splitBy(numChunks);
                next->addToFreelist();
            }
            return bestFit->data();
        // if we can't fit the chunk into the current heap, allocate new, if possible.
        } else {
            LOG(LL_HEAP, "Alloc " << (numChunks * 8) << " from " << heapEnd_); 
            // allocate new chunk at the end of the heap
            Chunk * result = heapEnd_;
            if (reinterpret_cast<uint8_t*>(result + numChunks) > hal::memory::heapEnd())
                return nullptr;
            heapEnd_ += numChunks;
            // verify that we have not overrun the stack
            // TODO actually return nullptr? 
            LOG(LL_HEAP, "End of heap " << heapEnd_);
            result->initializeAllocated(numChunks, lastSize_);
            lastSize_ = numChunks;
            return result->data();
        }

    }

    void Heap::free(void * ptr) {
        static_assert(sizeof(Chunk) == 8);

        LOG(LL_HEAP, "Freeing " << ptr);
        ASSERT(contains(ptr));
        // get the actual chunk pointer
        Chunk * chunk = reinterpret_cast<Chunk*>(reinterpret_cast<uint8_t*>(ptr) - 4);
        // ensure the chunk we are deleting has been manually allocated
        ASSERT(chunk->state() == Chunk::State::Manual);
        freeChunk(chunk);
    }

    Heap::Chunk * Heap::freeChunk(Chunk * chunk) {
        ASSERT(chunk->state() != Chunk::State::Free);
        // now that we know we are going to free, lets see if this is the last chunk in the heap, in which case we shrink the heap. When shrinking the heap we must also update the lastsize accordingly.
        if (chunk->nextAllocation() == heapEnd_) {
            LOG(LL_HEAP, "Freeing last chunk, shrinking heap to  " << chunk);
            heapEnd_ = chunk;
            lastSize_ = chunk->prevAllocationSize();
            chunk = heapEnd_->prevAllocation();
            // then while the last chunk is also free, we should shrink the heap further, but now we have to detach the chunk from the freelist first
            while (chunk != nullptr && chunk->isFree()) {
                LOG(LL_HEAP, "Previous chunk free, shrinking heap to " << chunk);
                chunk->detachFromFreelist();
                heapEnd_ = chunk;
                lastSize_ = chunk->prevAllocationSize();
                chunk = chunk->prevAllocation();
                ASSERT(chunk == nullptr || contains(chunk));
            }
            // the freed chunk is no longer part of the heap
            return nullptr;
        // if not the last chunk, simply add it to the freelist, but join with adjacent chunks, if they are free. Note that we only have to do this for the single previous and single next chunks as there cannot be any longer sequeces as long as every chunk does this check when freed.
        // also note that sice last chunk can never be free itself (this is handled above), we do not have to update the last allocation size either 
        } else {
            LOG(LL_HEAP, "Freeing middle chunk, adding to freelist " << chunk);
            Chunk * x = chunk->nextAllocation();
            ASSERT(x < heapEnd_);
            if ((x < heapEnd_) && x->isFree()) {
                if (chunk->headerSize() + x->headerSize() < 0x8000) {
                    LOG(LL_HEAP, "Joining with next free chunk " << x);
                    x->detachFromFreelist();
                    chunk->enlargeBy(x->headerSize());
                }

            }
            x = chunk->prevAllocation();
            if ((x != nullptr) && x->isFree() && ((chunk->headerSize() + x->headerSize()) < 0x8000)) {
                LOG(LL_HEAP, "Joining with previous free chunk " << x);
                x->enlargeBy(chunk->headerSize());
                return x; // don't add the chunk into the freelist as we have simply enlarged the previous chunk which already was in the freelist
            }
            chunk->addToFreelist();
            return chunk;
        }
    }

    uint32_t Heap::usedBytes() {
        uint32_t used = 0;
        Chunk * x = freelist_;
        while (x != nullptr) {
            ASSERT(x->isFree());
            used += x->headerSize() * sizeof(Chunk);
            x = x->prevFree();
        }
        return reservedBytes() - used;
    }

    void Heap::traceChunks() {
        LOG(LL_HEAP, "Heap chunks:");
        Chunk * x = heapStart_;
        while (x < heapEnd_) {
            LOG(LL_INFO, "@" << hex((x - heapStart_) * sizeof(Chunk)) << " size " << x->headerSize() * sizeof(Chunk) << " free: " << x->isFree());
            x = x->nextAllocation();
        }
    }

    void Heap::markAsGC(void * ptr) {
        ASSERT(contains(ptr));
        Chunk * chunk = reinterpret_cast<Chunk*>(reinterpret_cast<uint8_t*>(ptr) - 4);
        ASSERT(chunk->state() == Chunk::State::Manual);
        chunk->setState(Chunk::State::GC);
    }

    void GC::mark(Object const * obj) {
        ASSERT(Heap::contains(obj));
        Heap::Chunk * chunk = reinterpret_cast<Heap::Chunk*>(const_cast<uint8_t *>(reinterpret_cast<uint8_t const *>(obj) - 4));
        switch (chunk->state()) {
            case Heap::Chunk::State::GCMarked:
                // already marked, nothing to do
                return;
            case Heap::Chunk::State::GC:
                chunk->setState(Heap::Chunk::State::GCMarked);
                obj->gcTrace();
                break;
            default:
                UNREACHABLE;
        }
    }

    /** Walks the heap, clears the marks of surviving GC objects and frees the unmarked ones.

        The walk relies on two facts:

        - while chunk x is allocated, x->nextAllocation() is always a valid chunk, even if the destructor of x frees other chunks (those can only join with each other, never with the allocated x)
        - after x is freed, the walk continues from the free chunk that now covers x (x itself, or the previous free chunk x was joined with), whose header is valid. If the heap shrank below x, there is nothing left to walk

        Destructors of the GC objects must not allocate, nor free or access other GC objects.
     */
    void GC::sweep() {
        LOG(LL_HEAP, "GC sweep");
        Heap::Chunk * x = Heap::heapStart_;
        while (x < Heap::heapEnd_) {
            switch (x->state()) {
                // marked objects survive, clear the mark for the next collection
                case Heap::Chunk::State::GCMarked:
                    x->setState(Heap::Chunk::State::GC);
                    x = x->nextAllocation();
                    break;
                // unmarked objects are garbage, destroy & free them
                case Heap::Chunk::State::GC: {
                    LOG(LL_HEAP, "GC freeing " << x);
                    reinterpret_cast<GC::Object *>(x->data())->~Object();
                    // TODO the pointer is to GC object and we must call its destructor here, i.e. static_cast<GCObject *>(x->data())->~GCObject();
                    Heap::Chunk * f = Heap::freeChunk(x);
                    // heap shrank below x, so there are no more chunks to walk
                    if (f == nullptr)
                        return;
                    x = f->nextAllocation();
                    break;
                }
                // manual allocations and free chunks are skipped
                case Heap::Chunk::State::Manual:
                case Heap::Chunk::State::Free:
                    x = x->nextAllocation();
                    break;
            }
        }
    }

}

