#pragma once

#include <platform.h>
namespace rckid {

    /** Unique pointer.
     
        Just a wrapper around std::unique_ptr for easier future replacement and consistency together with other smart pointers below.
     */
    template<typename T>
    using unique_ptr = std::unique_ptr<T>;



    /** Heap Manager
     
        As heap is a scarce resource on embedded devices, RCKid provides its own heap manager implementation that optimizes the total memory used for the heap as the remainder of RAM can be used for stack. This is done via aggresive de-reservation of latest allocated chunks when freed and chunk merging & splitting during free & malloc respectively to curb unnecessary heap growth at the expense of potential fragmentation. 

        Of lesser importance is the allocator speed as using dynamic memory in hot loops where performance matters is not the best practice in general. 

        For detailed information about the memory allocator, see the Heap::Chunk implementation in memory.cpp.

        To enable a very detailed heap logging, enable the LL_HEAP log level.
     */
    class Heap {
    public:
        /** Allocates the given number of bytes. 
         
            Smallest chunk that is large enough is found in the freelist and split. If no such chunk is found, attempots to grow the heap. Can return nullptr *if* out of memory (or cannot allocate large enough continous chunk due to fragmentation). 

            This is "advanced" function that should only be used explicitly if the application has its own strategies to mitigate the out-of-memory sitiations.

            At most 262132 (262kB) continuous bytes can be allocated by a single call. 
         */
        static void * tryAlloc(uint32_t numBytes);

        /** Allocates the given number of bytes. 
         
            If the request cannot be satisfied (OOME), raises fatal error. This is the default allocator function for rckid, ensuring predictable memory performance.

            At most 262132 (262kB) continuous bytes can be allocated by a single call. 
         */
        static void * alloc(uint32_t numBytes) {
            void * result = tryAlloc(numBytes);
            if (result == nullptr)
                FATAL_ERROR("OOME", numBytes);
            return result;
        }

        /** Frees given pointer. 
         
            The pointer *must* have been previously allocated via alloc() or tryAlloc(). nullptr ir out of heap pointers are *not* allowed and will assert. 
          */
        static void free(void * ptr);

        /** Returns true if the given pointer is within the currently active heap range. 

            This is determined solely on the address range, not pointer properties (i.e. whether it is actually valid & pointing to allocated memory, or to a freelist gap).
         */
        static bool contains(void const * ptr) { return ptr >= heapStart_ && ptr < heapEnd_; }

        /** Returns the number of bytes reserved for the heap. 
         
            This is the chunk of device's RAM that is occupied by the heap, both allocated and free and cannot be used for other purposes. The only way to decrease this number is to free the last allocated chunk so that the heap can shrink.
         */
        static uint32_t reservedBytes() { return reinterpret_cast<uint8_t*>(heapEnd_) - reinterpret_cast<uint8_t*>(heapStart_); }

        /** Returns the actually used bytes in the heap (including the chunk headers). 
         
            Internally this takes the reservedBytes and then subtracts from it all the chunks found in the freelist. As such its complexity is linear wrt the number of free chunks. 
         */
        static uint32_t usedBytes();

    private:
        class Chunk;

        static inline Chunk * heapStart_ = reinterpret_cast<Chunk*>(hal::memory::heapStart());
        static inline Chunk * heapEnd_ = heapStart_;
        static inline Chunk * freelist_ = nullptr;
        static inline uint32_t lastSize_ = 0;
        
    }; // rckid::heap

} // namespace rckid