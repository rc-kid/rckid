#pragma once

#include "error.h"

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
        friend uint32_t memoryFree();
        friend void memoryReset(); // internal

        template<typename T>
        friend void memorySetDefaultTarget();
    
        // pointer to end of heap so that we can detect OOME during allocations
        static char * end_;

        // is heap default allocation target
        static inline bool default_ = true;

    }; // rckid::Heap

    /** Returns the number of free memory, i.e. the unclaimed space between heap and stack
     
        This is the upper limit of what new memory can be allocated. The actual free memory can be larger because of free holes in the heap space that this method does not track. 
     */
    uint32_t memoryFree();

    /** Instruments the end of stack with magic numbers so that if the app would run out of stack and overwrite the heap, it can at least be noticed by periodic calls to the memoryCheckStaticProtection function. 
     */
    void memoryInstrumentStackProtection();

    /** Verifies that the stack protection magic numbers are intact and raises a fatal error if corrupted. This function should be periodically called to be effective.
     */
#if RCKID_ENABLE_STACK_PROTECTION
    void memoryCheckStackProtection();
    uint32_t memoryMaxStackSize();
    void memoryResetMaxStackSize();
#else
    inline void memoryCheckStackProtection() {}
#endif

    /** Returns true if the memory comes from immutable region (ROM on the device)
     */
    bool memoryIsImmutable(void const * ptr);

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