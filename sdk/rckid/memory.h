#pragma once

#include <platform.h>

#include "errors.h"

namespace rckid {
    
    void memoryInstrumentStackProtection();

    void memoryCheckStackProtection();

    /** Returns free memory available on the system. */
    uint32_t memoryFree(); 

    /** RAII object that redirects all default allocations to arena. 
     */
    class ArenaScope {
    public:
        ArenaScope();
        ~ArenaScope();

        template<typename T>
        T && operator()(T && value) { return std::move(value); }
    };

    /** RAII object that overrides all default allocations to heap, even when called from inside arena scope. 
     */
    class HeapScope {
    public:
        HeapScope();
        ~HeapScope();

        template<typename T>
        T && operator()(T && value) { return std::move(value); }

    private:
        uint32_t const backup_;
    };

    /** RAII object that enters new arena, redirects all default allocations to it and leaves the arena when going out of scope itself. 
     */
    class NewArenaScope {
    public:
        NewArenaScope();
        ~NewArenaScope();
    };

    class Heap {
    public:

        /** Allocates given amount of bytes on heap.
         
            This is under all circumstances equivalent to a standard malloc call irrespective of the default allocation mode.
         */
        static void * malloc(uint32_t bytes);

        /** Allocates memory for given type. Will allways allocate on heap. 
         */
        template<typename T>
        static T * malloc() { return (T*)malloc(sizeof(T)); }

        /** Frees the given pointer. 
         
            Expects the pointer actually belongs to heap, i.e. should only be explicitly called on pointers created with Heap::malloc explicitly. If a pointer was allocated by the default function (malloc, rckid::malloc, new) the generic deletion (free, rckid::free, delete) should be called. 
         */
        static void free(void * ptr);

        /** Returns true if heap is the preferred mode of allocation at the point of calling the method. 
         */
        static bool isPreferred() { return preferredArena_ == 0; }

        /** Returns true if given pointer belongs to the heap. 
         
            This is true if it happens to be anywhere between current heap's end and stack limit. 
         */
        static bool contains(void * ptr);

        /** Returns the size of the contiguous heap including any freed objects.
         */
        static uint32_t used(); 

        /** Returns the total allocated heap memory.
         
            This is everything in memory used minus all chunks in the freelist. 
         */
        static uint32_t allocated();

    private:
        friend class Arena;
        friend class ArenaScope;
        friend class HeapScope;
        friend class NewArenaScope;
        friend uint32_t memoryFree();
        friend void memoryReset();

        using ChunkHeader = uint32_t;

        /** Heap allocated chunk.
         
            Chunk consists of header and body. Chunk header is 4 bytes long and is the chunk size (including the header). Since chunk sizes must be in multiples of 4, ths LSB (2 of them in fact) is free and is used to determine whether the chunk is allocated (0), or part of the freelist (1). When the chunk is allocated, its body is the user data, but when the chunk becomes part of the freelist, the body consists of a pointer to the previous and next chunks in the freelist.  
         */
        PACKED(struct Chunk {
            ChunkHeader header;
            Chunk * next;
            Chunk * prev;

            Chunk(uint32_t size): header{size} {}

            char * start() { return (char*)this; }

            /** Returns the pointer past the end of the chunk. which is the address of the chunk + the chunk header + chunk size. 
             */
            char * end() { return start() + size(); }

            size_t size() { return header & ~3; }

            /** Sets size of the chunk and marks it as allocated.
             */
            void setSize(uint32_t bytes) { header = bytes; }

            bool isFree() { return header & CHUNK_IS_FREE; }

            void markAsAllocated() { header &= ~CHUNK_IS_FREE; }

            void markAsFree() { header |= CHUNK_IS_FREE; }

        private:

            static constexpr uint32_t CHUNK_IS_FREE = 1;

        }); // Heap::Chunk

        /** Detaches given chunk from the freelist. This is useful when the last chunk on the heap is freed and the chunk above it is part of the freelist, in which case we can remove that chunk as well to fully reclaim the memory.
         */
        static void detachChunk(Chunk * chunk);

        static char * end_;

        static inline Chunk * freelist_ = nullptr;

        static inline uint32_t preferredArena_ = 0;

    }; // rckid::Heap

    class Arena {
    public:

        /** Allocates given number of bytes on the current arena. 
         */
        static void * malloc(uint32_t numBytes) {
            TRACE_ARENA("Allocating " << numBytes << " on arena");        
            void * result = end_;
            end_ += numBytes;
            // check that we are not running out of memory by crossing the heap end
            ASSERT(end_ < Heap::end_);
            return result;
        }

        /** Allocates arena for given type. 
         */
        template<typename T>
        static T * malloc() { return (T*)malloc(sizeof(T)); }

        /** Resets the current arena by deallocating all its objects. 
         */
        static void reset() {
            TRACE_ARENA("Resetting arena (clearing " <<  (uint32_t)(end_ - start_) << " bytes)");
            end_ = start_;
        }

        /** Enters new arena. 
         
            Pushes current arena start onto the arena memmory and then sets arena start and end immediately after this pointer. This leaves old arena start right before current arena start for when we are leaving the current arena. 
         */
        static void enter() {
            TRACE_ARENA("Entering arena");
            char ** lastStart = malloc<char*>();
            *lastStart = start_;
            start_ = end_;
        }

        /** Leaves current arena. 
         
            Leaves the current arena loading the previous arena start from just before current arena start and then moving the end to where the previous arena start was stored. 
         */
        static void leave() {
            TRACE_ARENA("Leaving arena");
            // TODO assert we are still in RAM actually (i.e. there is something before us)
            char ** previous = ((char**)start_) - 1;
            start_ = *previous;
            end_ = (char*)previous;
            TRACE_ARENA("Free memory: " << memoryFree());
        }

        /** Returns true if given pointer belongs to any of the currently opened arenas. 
         
            This is true if it happens to be anywhere between very beginning of arena's allocatoion (after bss) the end of currently opened arena. 
         */
        static bool contains(void * ptr);

        /** Returns true if arena is the preferred mode of allocation at the point of calling the method. 
         */
        static bool isPreferred() { return Heap::preferredArena_ > 0; }

        /** Returns the total memory used by the arenas (i.e. the sum of all entered arenas). 
         */
        static uint32_t used(); 

        /** Returns the memory used by the current arena. 
         */
        static uint32_t usedCurrent() { return end_ - start_; }

    private:
        friend class Heap;
        friend uint32_t memoryFree();
        friend void memoryReset();

        // start of the topmost arena
        static char * start_;
        // end of the topmost arena
        static char * end_;

    }; // rckid::Arena


    /** Generic malloc. 
     
        Depending on the preferred mode of allocation, this will either allocate on the heap, or if arena is preferred on the current arena. This function is also what system's malloc points to. 
     */
    inline void * malloc(uint32_t numBytes) {
        return Arena::isPreferred() ? Arena::malloc(numBytes) : Heap::malloc(numBytes);
    }

    /** Generic malloc shorthand for particular type. 
     */
    template<typename T>
    inline T* malloc() { return (T*) malloc(sizeof(T)); }

    /** Generic free. 
     
        If the given pointer belongs to heap, calls heap's free on it. Otherwise (arena & other parts of memory) does nothing. 
      */
    inline void free(void * ptr) { if (Heap::contains(ptr)) Heap::free(ptr); }

    #define HEAP(...) rckid::HeapScope{}(__VA_ARGS__)

    #define ARENA(...) rckid::ArenaScope{}(__VA_ARGS__)
    


    inline ArenaScope::ArenaScope() { ++Heap::preferredArena_; }

    inline ArenaScope::~ArenaScope() { 
        ASSERT(Heap::preferredArena_ > 0);
        --Heap::preferredArena_; 
    }

    inline HeapScope::HeapScope():backup_{Heap::preferredArena_} { Heap::preferredArena_ = 0; }
    inline HeapScope::~HeapScope() { Heap::preferredArena_ = backup_; }

    inline NewArenaScope::NewArenaScope() {
        Arena::enter();
        ++Heap::preferredArena_;
    }

    inline NewArenaScope::~NewArenaScope() {
        ASSERT(Heap::preferredArena_ > 0);
        Arena::leave();
        --Heap::preferredArena_; 
    }

} // namespace rckid