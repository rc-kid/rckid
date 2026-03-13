#include <new>
#include <rckid/rckid.h>
#include <rckid/memory.h>

namespace rckid::internal::memory {
    extern uint32_t useSystemMalloc;
}

extern "C" {

    extern uint8_t __rodata_start; 
    extern uint8_t __rodata_end;    
    
    extern void *__libc_malloc(size_t);
    extern void __libc_free(void *);

    //depending on whether we are in system malloc, or not use libc malloc, or RCKid's heap
    void * malloc(size_t numBytes) {
        if (rckid::internal::memory::useSystemMalloc > 0)
            return __libc_malloc(numBytes);
        else
            return rckid::Heap::alloc(numBytes);
    }

    // if the pointer to be freed belongs to RCKId's heap, we should use own heap free, otherwise use normal free (and assert it does not belong to fantasy heap in general as that would be weird)
    void free(void * ptr) {
        if (rckid::Heap::contains(ptr)) {
            rckid::Heap::free(ptr);
        // otherwise this is a libc pointer and should be deleted accordingly
        } else {
            __libc_free(ptr);
        }
   }
} // extern C

void* operator new(std::size_t numBytes) {
    return malloc(numBytes); 
}
void* operator new[](std::size_t numBytes) {
    return malloc(numBytes); 
}
void* operator new(std::size_t numBytes, std::align_val_t align) {
    ASSERT(static_cast<size_t>(align) <= 4);
    return malloc(numBytes); 
}
void* operator new[](std::size_t numBytes, std::align_val_t align) {
    ASSERT(static_cast<size_t>(align) <= 4);
    return malloc(numBytes); 
}

void operator delete(void * ptr) noexcept {
    return free(ptr);
}

void operator delete[](void* ptr) noexcept {
    return free(ptr);
}

void operator delete(void * ptr, std::size_t) noexcept {
    return free(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept {
    return free(ptr);
}

void operator delete(void* ptr, std::align_val_t) noexcept {
    return free(ptr);
}

void operator delete[](void* ptr, std::align_val_t) noexcept {
    return free(ptr);
}

void operator delete(void* ptr, std::size_t, std::align_val_t) noexcept {
    return free(ptr);
}

void operator delete[](void* ptr, std::size_t, std::align_val_t) noexcept {
    return free(ptr);
}
