#pragma once

#include "error.h"

#define ARENA(...) [&]() { \
    rckid::ArenaAllocationGuard __guard; \
    return __VA_ARGS__; \
}()

namespace rckid {

    /** Heap allocation and management. 
     
     */
    class Heap {
    public:

        /** Allocates given amount of bytes on heap.
         
            This is under all circumstances equivalent to a standard malloc call irrespective of the default allocation mode.
         */
        static void * allocBytes(uint32_t bytes); 
    
        /** Allocates memory for given type. Will allways allocate on heap. 
         */
        template<typename T>
        static T * alloc() { return (T*)allocBytes(sizeof(T)); }

        template<typename T>
        static T* alloc(uint32_t items) { return (T*)allocBytes(sizeof(T) * items); }

        /** Frees the given pointer. 
         
            Expects the pointer actually belongs to heap, i.e. should only be explicitly called on pointers created with Heap::malloc explicitly. If a pointer was allocated by the default function (malloc, rckid::malloc, new) the generic deletion (free, rckid::free, delete) should be called. 
         */
        static void free(void * ptr);

        /** Returns true if given pointer belongs to the heap. 
         
            This is true if it happens to be anywhere between current heap's end and stack limit. 
         */
        static bool contains(void const * ptr);

        static bool isDefaultTarget() { return default_; }

    private:
        friend class Arena;
        friend uint32_t memoryFree();
        friend void memoryReset(); // internal

        template<typename T>
        friend void memorySetDefaultTarget();
    
        // pointer to end of heap so that we can detect OOME during allocations
        static char * end_;

        // is heap default allocation target
        static inline bool default_ = true;

    }; // rckid::Heap

    /** Arena memory allcoation. 
     
        Static class is used so that the arena functions can be header only and thus inline to where necessary speeding up the critical paths of the system. 
     */
    class Arena {
    public:

        /** Allocates given number of bytes on the current arena. 
         */
        static void * allocBytes(uint32_t numBytes) {
            void * result = end_;
            end_ += numBytes;
            // check that we are not running out of memory by crossing the heap end
            ERROR_IF(error::OutOfMemory, end_ >= Heap::end_);
            return result;
        }

        static void * tryAllocBytes(uint32_t numBytes) {
            if (end_ + numBytes >= Heap::end_)
                return nullptr;
            return allocBytes(numBytes);
        }

        /** Allocates arena for given type. 
         */
        template<typename T>
        static T * alloc() { return (T*)allocBytes(sizeof(T)); }

        template<typename T>
        static T* alloc(uint32_t items) { return (T*)allocBytes(sizeof(T) * items); }

        /** Attempts to free the given arena pointer of given size. 
         
            If the pointer is the last thing allocated in the current arena, the pointer can be removed from the arena and the function returns true. Otherwise the memory cannot be deallocated as it is in the middle of the arena already and false is returned. 
         */
        static bool tryFree(void * ptr, uint32_t size) {
            if (end_ - size == ptr) {
                end_ -= size;
                return true;
            } else {
                return false;
            }
        }

        /** Shorthand for tryFree with typed pointer for automatic size detection.
         */
        template<typename T>
        static bool tryFree(T * ptr) {
            return tryFree(ptr, sizeof(T));
        }

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

        static bool isDefaultTarget() { return ! Heap::default_; }

    private:
        friend class Heap;
        friend uint32_t memoryFree();
        friend void memoryReset(); // internal
        // start and end of the current arena 
        static char * start_;
        static char * end_;

    }; // rckid::Arena

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
#if RCKID_ENABLE_STACK_PROTECTION
    void memoryCheckStackProtection();
#else
    inline void memoryCheckStackProtection() {}
#endif

    /** Returns true if the memory comes from immutable region (ROM on the device)
     */
    bool memoryIsImmutable(void const * ptr);

    template<typename T>
    void memorySetDefaultTarget();

    template<>
    inline void memorySetDefaultTarget<Heap>() {
        Heap::default_ = true;
    }

    template<>
    inline void memorySetDefaultTarget<Arena>() {
        Heap::default_ = false;
    }

    class NewArenaGuard {
        public:
            NewArenaGuard() { Arena::enter(); }
            ~NewArenaGuard() { Arena::leave(); }
    
            NewArenaGuard(NewArenaGuard const &) = delete;
            NewArenaGuard(NewArenaGuard &&) = delete;
        
    }; // rckid::NewArenaGuard
    

    /** Guards arena allocation as default method. 
     */
    class ArenaAllocationGuard {
        public:
            ArenaAllocationGuard():
                restoreHeap_{Heap::isDefaultTarget()} {
                memorySetDefaultTarget<Arena>();
            }
    
            ~ArenaAllocationGuard() {
                if (restoreHeap_)
                    memorySetDefaultTarget<Heap>();
            }
    
            ArenaAllocationGuard(ArenaAllocationGuard const &) = delete;
            ArenaAllocationGuard(ArenaAllocationGuard &&) = delete;
        private:
            bool const restoreHeap_; 
    }; // rckid::ArenaAllocationGuard

#ifdef RCKID_BACKEND_FANTASY
    class SystemMallocGuard {
    public:
        SystemMallocGuard():
            old_{systemMalloc_} { 
            systemMalloc_ = true;
        }
        ~SystemMallocGuard() { 
            systemMalloc_ = old_;
        }

        static bool isDefault() { return systemMalloc_; }

        static void enable() { systemMalloc_ = true; }
        static void disable() { systemMalloc_ = false; }

    private:
        bool old_;
        /** Start in system malloc so that any pre-main initialization does not pollute rckid's heap. 
         
            The code below replaces malloc and free functions with own versions that depending the flag either use system malloc, or rckid's heap allocator, which can be used for tracking memory footprint of applications even in the fantasy backend setting. 
        */
        static inline thread_local bool systemMalloc_ = true;
    }; 
#endif    

} // namespace