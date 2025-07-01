#include "rckid.h"

namespace rckid {

    void * RAMHeap::alloc(uint32_t numBytes) {
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
        if (bestFit != nullptr && bestFit->headerSize_ == numChunks) {
            LOG(LL_HEAP, "Alloc " << (numChunks * 8) << " from freelist " << bestFit);
            ASSERT(bestFit->isFree());
            bestFit->detachFromFreelist();
            bestFit->makeAllocated();
            return bestFit->data();
        } else {
            LOG(LL_HEAP, "Alloc " << (numChunks * 8) << " from " << heapEnd_); 
            // allocate new chunk at the end of the heap
            Chunk * result = heapEnd_;
            heapEnd_ += numChunks;
            LOG(LL_HEAP, "End of heap " << heapEnd_);
            // TODO check that we have not overrun the stack limit, this will differ for fantasy and device  
            result->initializeAllocated(numChunks, lastSize_);
            lastSize_ = numChunks;
            return result->data();
        }

    }

    void RAMHeap::free(void * ptr) {
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
                ASSERT(chunk == nullptr || RAMHeap::contains(chunk));
            }
        // if not the last chunk, simply add it to the freelist
        } else {
            LOG(LL_HEAP, "Freeing middle chunk, adding to freelist " << chunk);
            chunk->addToFreelist();
        }
    }

    uint32_t RAMHeap::freeBytes() {
        uint32_t result = 0;
        Chunk * x = freelist_;
        while (x != nullptr) {
            ASSERT(x->isFree());
            ASSERT(x->headerSize_ > 0);
            result += x->payloadSize();
            x = x->prevFree();
        }
        return result;
    }

    void RAMHeap::traceChunks() {
        LOG(LL_INFO, "Heap start: " << heapStart());
        LOG(LL_INFO, "Heap end:   " << heapEnd_);
        LOG(LL_INFO, "Heap chunks:");
        Chunk * x = heapStart();
        while (x < heapEnd_) {
            LOG(LL_INFO, "  " << (x->headerSize_) << (x->isFree() ? " f" : ""));
            x = x->nextAllocation();
        }
        LOG(LL_INFO, "Freelist:");
        x = freelist_;
        while (x != nullptr) {
            LOG(LL_INFO, "  " << (x->headerSize_) << (x->isFree() ? " f" : ""));
            x = x->prevFree();
        }
    }

    /** Internal function that resets the entire memory (heap and arena). Use with extreme caution. Pretty much the only sensible thing to do after memoryReset is to show the blue screen of death.
     */
    void memoryReset() {
        // reset the heap and arena pointers to the beginning of the memory        
        LOG(LL_HEAP, "Resetting memory");
        RAMHeap::reset();
    }

    uint32_t memoryFree() {
#ifdef RCKID_BACKEND_FANTASY
        return RCKID_MEMORY_SIZE - (RAMHeap::usedBytes() + StackProtection::currentSize());
#else
        return StackProtection::currentStack() - reinterpret_cast<char*>(RAMHeap::heapEnd_);
        StackProtection::check();
#endif
    }

} // namespace rckid