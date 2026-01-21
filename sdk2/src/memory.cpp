#include <rckid/error.h>
#include <rckid/util/log.h>
#include <rckid/memory.h>


namespace rckid {

    /** Heap Chunk
     
        Heap chunks are tailored towards very little memory waste. Each chunk consists of a header that is only 4 bytes and contains current chunk's size in Chunks (8 bytes) as well as previous chunk's size in chunks. As the MSB of previous chunk's size is occupied by a free bit, we can allocate at most 32767 chunks (262136 bytes) in a single allocation, of which 4 bytes are used by the header itself, leaving 262132 bytes for user data.

        Minimal allocation physical allocation is 8 bytes, so that when the chunk is free, its data can hold 2 pointers for the free list in the data portion. Minimal user allocation is 4 bytes with increments by 8 (chunk header + min 4 bytes of payload, then incremented by ectra "chunks"). 

        
     */
    class Heap::Chunk {
    public:
        static constexpr uint32_t HEADER_SIZE = 4;

        uint32_t payloadSize() const { return headerSize_ * sizeof(Chunk) - 4; }

        bool isFree() const { return (headerPrevSize_ & FREE_BIT) != 0; }

        Chunk * nextFree() { return offsetToPtr(nextFree_); }
        Chunk * prevFree() { return offsetToPtr(prevFree_); }

        Chunk * nextAllocation() { return this + headerSize_; }

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
            headerPrevSize_ |= FREE_BIT;
            prevFree_ = ptrToOffset(Heap::freelist_);
            if (Heap::freelist_ != nullptr)
                Heap::freelist_->nextFree_ = ptrToOffset(this);
            Heap::freelist_ = this;
        }

        void makeAllocated() {
            ASSERT(isFree());
            headerPrevSize_ &= ~FREE_BIT;
        }

        /** Takes a chunk and enlarges it by given size. 
            
            The size must be equivalent to the sum of one or more chunks immediately after it as the new chunk cannot be created in the middle of existing allocations. Furthermore the new chunk size must be smaller than the max chunk size (0x7fff chunks) so that we can still keep the prev chunk size in the header prev size field for quick coalescing. 
            */
        void enlargeBy(uint16_t by) {
            ASSERT((headerSize_ + by) < FREE_BIT); // otherwise we would create too large a chunk to store in the prev size field together with the free bit
            uint32_t ownSize = headerSize_;
            headerSize_ += by;
            Chunk * next = nextAllocation();
            ASSERT(next < Heap::heapEnd_); // we do not expect to enlarge the last chunk
            next->headerPrevSize_ += ownSize;
        }

        /** Splits the chunk into two. 
         
            Resizes itself to the given size and returns the newly created chunk that follows it with the remainder of the size. 
            */
        Chunk * splitBy(uint16_t newSize) {
            ASSERT(newSize < headerSize_);
            ASSERT(! isFree()); // the chunk is not expected to be in freelist when split
            Chunk * next = this + newSize;
            next->headerSize_ = headerSize_ - newSize;
            next->headerPrevSize_ = newSize;
            headerSize_ = newSize;
            Chunk * nextNext = next->nextAllocation();
            if (nextNext < Heap::heapEnd_)
                nextNext->headerPrevSize_ = next->headerSize_;
            return next;
        }

        void * data() { return reinterpret_cast<uint8_t*>(this) + 4; }

    private:

        friend class Heap;

        uint16_t headerSize_;
        uint16_t headerPrevSize_;
        uint16_t prevFree_;
        uint16_t nextFree_;

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
            if (x->headerSize_ == numChunks) {
                bestFit = x;
                break;
            }
            if (x->headerSize_ > numChunks) {
                if (bestFit == nullptr)
                    bestFit = x;
                else if (x->headerSize_ < bestFit->headerSize_) {
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
            if (bestFit->headerSize_ > numChunks) {
                Chunk * next = bestFit->splitBy(numChunks);
                next->addToFreelist();
            }
            return bestFit->data();
        // if we can't fit the chunk into the current heap, allocate new, if possible.
        } else {
            LOG(LL_HEAP, "Alloc " << (numChunks * 8) << " from " << heapEnd_); 
            // allocate new chunk at the end of the heap
            Chunk * result = heapEnd_;
            if (reinterpret_cast<uint8_t*>(result) > hal::memory::heapEnd())
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
        ASSERT(! chunk->isFree()); // double free
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
        // if not the last chunk, simply add it to the freelist, but join with adjacent chunks, if they are free. Note that we only have to do this for the single previous and single next chunks as there cannot be any longer sequeces as long as every chunk does this check when freed.
        // also note that sice last chunk can never be free itself (this is handled above), we do not have to update the last allocation size either 
        } else {
            LOG(LL_HEAP, "Freeing middle chunk, adding to freelist " << chunk);
            Chunk * x = chunk->nextAllocation();
            ASSERT(x < heapEnd_);
            if ((x < heapEnd_) && x->isFree()) {
                if (chunk->headerSize_ + x->headerSize_ < 0x8000) {
                    LOG(LL_HEAP, "Joining with next free chunk " << x);
                    x->detachFromFreelist();
                    chunk->enlargeBy(x->headerSize_);
                }

            }
            x = chunk->prevAllocation();
            if ((x != nullptr) && x->isFree() && ((chunk->headerSize_ + x->headerSize_) < 0x8000)) {
                LOG(LL_HEAP, "Joining with previous free chunk " << x);
                x->enlargeBy(chunk->headerSize_);
                return; // don't add the chunk into the freelist as we have simply enlarged the previous chunk which already was in the freelist
            } 
            chunk->addToFreelist();
        }
    }

}

