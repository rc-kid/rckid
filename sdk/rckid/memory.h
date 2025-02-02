#pragma once

#include "rckid.h"


namespace rckid {

    /** Heap allocation and management. 
     
     */
    class Heap {
    public:
        /** Allocates given amount of bytes on heap.
         
            This is under all circumstances equivalent to a standard malloc call irrespective of the default allocation mode.
         */
        static void * alloc(uint32_t bytes); 
    
        /** Allocates memory for given type. Will allways allocate on heap. 
         */
        template<typename T>
        static T * alloc() { return (T*)alloc(sizeof(T)); }

        template<typename T>
        static T* alloc(uint32_t items) { return (T*)alloc(sizeof(T) * items); }

        /** Frees the given pointer. 
         
            Expects the pointer actually belongs to heap, i.e. should only be explicitly called on pointers created with Heap::malloc explicitly. If a pointer was allocated by the default function (malloc, rckid::malloc, new) the generic deletion (free, rckid::free, delete) should be called. 
         */
        static void free(void * ptr);

        /** Returns true if given pointer belongs to the heap. 
         
            This is true if it happens to be anywhere between current heap's end and stack limit. 
         */
        static bool contains(void const * ptr);

    private:
        friend class Arena;
        friend uint32_t memoryFree();
        friend void memoryReset(); // internal

        // pointer to end of heap so that we can detect OOME during allocations
        static char * end_;

    }; // rckid::Heap

    /** Arena memory allcoation. 
     
        Static class is used so that the arena functions can be header only and thus inline to where necessary speeding up the critical paths of the system. 
     */
    class Arena {
    public:
        /** Allocates given number of bytes on the current arena. 
         */
        static void * alloc(uint32_t numBytes) {
            void * result = end_;
            end_ += numBytes;
            // check that we are not running out of memory by crossing the heap end
            ERROR_IF(error::OutOfMemory, end_ >= Heap::end_);
            return result;
        }

        /** Allocates arena for given type. 
         */
        template<typename T>
        static T * alloc() { return (T*)alloc(sizeof(T)); }

        template<typename T>
        static T* alloc(uint32_t items) { return (T*)malloc(sizeof(T) * items); }


        /** Free API for allocator. Does nothing as arena does not support per item deallocation.
         */
        static void free([[maybe_unused]] void * ptr) {}

        /** Resets the current arena by deallocating all its objects. 
         */
        static void reset() {
            end_ = start_;
        }

        /** Enters new arena. 
         
            Pushes current arena start onto the arena memmory and then sets arena start and end immediately after this pointer. This leaves old arena start right before current arena start for when we are leaving the current arena. 
         */
        static void enter() {
            char ** lastStart = alloc<char*>();
            *lastStart = start_;
            start_ = end_;
        }

        /** Leaves current arena. 
         
            Leaves the current arena loading the previous arena start from just before current arena start and then moving the end to where the previous arena start was stored. 
         */
        static void leave() {
            // TODO assert we are still in RAM actually (i.e. there is something before us)
            char ** previous = ((char**)start_) - 1;
            start_ = *previous;
            end_ = (char*)previous;
        }

        /** Returns true if given pointer belongs to any of the currently opened arenas. 
         
            This is true if it happens to be anywhere between very beginning of arena's allocatoion (after bss) the end of currently opened arena. 
         */
        static bool contains(void const * ptr);

        /** Reurns size of the currently active (top) arena. 
         */
        static uint32_t currentSize() { return static_cast<uint32_t>(end_ - start_); }

    private:
        friend class Heap;
        friend uint32_t memoryFree();
        friend void memoryReset(); // internal
        // start and end of the current arena 
        static char * start_;
        static char * end_;

    }; // rckid::Arena

    /** Arena memory allocator. 
     
        A simple C++ allocator to be used with stdlib classes that allocates on current arena.
     */
    template<typename T>
    class ArenaAllocator {
    public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using propagate_on_container_move_assignment = std::true_type;

        T * allocate(std::size_t n) { return (T *)Arena::alloc(static_cast<uint32_t>(n * sizeof(T))); }
        void deallocate([[maybe_unused]] T * ptr, [[maybe_unused]] size_t n) { }
    };    

    class ArenaGuard {
    public:
        ArenaGuard() { Arena::enter(); }
        ~ArenaGuard() { Arena::leave(); }

        ArenaGuard(ArenaGuard const &) = delete;
        ArenaGuard(ArenaGuard &&) = delete;
    
    }; // rckid::ArenaGuard

    /** Shorthand typedef for an arena allocated string. 
     */
    using astring = std::basic_string<char, std::char_traits<char>, ArenaAllocator<char>>;

    /** Returns the number of free memory, i.e. the unclaimed space between arena and heap. 
     
        This is the upper limit of what new memory can be allocated in an arena. Heap allocation might be higher because of holes in the heap that can be used for new allocations if large enough. 
     */
    inline uint32_t memoryFree() {
        // this is a safe check as the heap and arena pointers are part of the available memory and they are guaranteed to be close together
        return static_cast<uint32_t>(Heap::end_ - Arena::end_);
    }

    /** Instruments the end of stack with magic numbers so that if the app would run out of stack and overwrite the heap, it can at least be noticed by periodic calls to the memoryCheckStaticProtection function. 
     */
    void memoryInstrumentStackProtection();

    /** Verifies that the stack protection magic numbers are intact and raises a fatal error if corrupted. This function should be periodically called to be effective.
     */
    void memoryCheckStaticProtection();


} // namespace