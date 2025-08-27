#pragma once

#include "backend_config.h"
#include "error.h"
#include "log.h"

#ifndef RCKID_BACKEND_FANTASY
extern char __bss_end__;
extern char __StackTop;
#endif

namespace rckid {

    uint32_t memoryFree();

    /** RAM Heap Manager
     */
    class RAMHeap {
    public:
        static void * alloc(uint32_t numBytes);
        static void free(void * ptr);

        static bool contains(void const * ptr) { return ptr >= heapStart() && ptr < heapEnd_; }

        /** Returns the total number of bytes used by the heap. This includes the chunk headers and all chunks (and their headers) in the freelist. */
        static uint32_t usedBytes() { return (heapEnd_ - heapStart()) * sizeof(Chunk);}

        /** Returns the number of bytes in the freelist. 
         
            Note that this number *includes* the chunk headers as well - this means that the number is slightly higher than the actual sum of user-available free bytes in the freelist, and is extremely likely to be larger than the largest freelist chunk.
         */
        static uint32_t freeBytes();

        static void reset() {
            heapEnd_ = heapStart();
            lastSize_ = 0;
            freelist_ = nullptr;
        }

        static void traceChunks();

    private:

        friend class StackProtection;
        friend uint32_t memoryFree();
        friend bool memoryIsImmutable(void const * ptr);

        class Chunk {
        public:
            static constexpr uint32_t HEADER_SIZE = 4;

            uint32_t payloadSize() const { return headerSize_ * sizeof(Chunk) - 4; }

            bool isFree() const { return (headerPrevSize_ & 0x8000) != 0; }

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
                if (freelist_ == this) {
                    freelist_ = prevFree();
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
                prevFree_ = ptrToOffset(freelist_);
                if (freelist_ != nullptr)
                    freelist_->nextFree_ = ptrToOffset(this);
                freelist_ = this;
            }

            void makeAllocated() {
                ASSERT(isFree());
                headerPrevSize_ &= ~FREE_BIT;
            }

            void * data() { return reinterpret_cast<uint8_t*>(this) + 4; }

        private:

            friend class RAMHeap;

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
                    return heapStart() + offset;
            }

            static uint16_t ptrToOffset(Chunk * ptr) {
                if (ptr == nullptr)
                    return OFFSET_NULL;
                return ptr - heapStart();
            }

        } __attribute__((packed)); // RAMHeap::Chunk

        static_assert(sizeof(Chunk) == 8);

        /** Returns the address of the start of the heap.
         */
        static Chunk * heapStart() {
#ifdef RCKID_BACKEND_FANTASY
            static char fantasyHeap[RCKID_MEMORY_SIZE];
            return reinterpret_cast<Chunk*>(& fantasyHeap);
#else
            return reinterpret_cast<Chunk*>(& __bss_end__);
#endif
        }

        static inline Chunk * heapEnd_ = heapStart();
        static inline Chunk * freelist_ = nullptr;
        static inline uint16_t lastSize_ = 0;

    }; 

    class MemoryLeakGuard {
    public:
        MemoryLeakGuard(): freeMem_{memoryFree()} {}

        ~MemoryLeakGuard() {
            uint32_t newFree = memoryFree();
            if (newFree < freeMem_)
                LOG(LL_ERROR, "Memory leak: " << (freeMem_ - newFree) << " bytes lost");
        }

    private:
        uint32_t const freeMem_;

    }; // rckid::MemoryLeakGuard

    class StackProtection {
    public:
        static uint32_t currentSize() {
            return stackStart() - currentStack();
        }

        static uint32_t maxSize() { return maxSize_; }

        static void resetMaxSize() { maxSize_ = 0;}

        static void check() {
#ifdef RCKID_ENABLE_STACK_PROTECTION
            uint32_t cur = currentSize();
            if (cur > maxSize_)
                maxSize_ = cur;
    #ifdef RCKID_BACKEND_FANTASY
            ERROR_IF(error::StackProtectionFailure, cur + RAMHeap::usedBytes() >= RCKID_MEMORY_SIZE);
    #else
            ERROR_IF(error::StackProtectionFailure, currentStack() < reinterpret_cast<char*>(RAMHeap::heapEnd_));
    #endif
#endif
        }

    private:

        friend uint32_t memoryFree();

        static char * stackStart() {
#ifdef RCKID_BACKEND_FANTASY
            ASSERT(stackTop_ != nullptr);
            return stackTop_;
#else
            return & __StackTop;
#endif
        }

        static char * currentStack() {
            uintptr_t sp;
#ifdef RCKID_BACKEND_FANTASY
    #if defined(__x86_64__) || defined(_M_X64)
            asm volatile("mov %%rsp, %0" : "=r"(sp));
    #elif defined(__aarch64__) || defined(_M_ARM64)
            asm volatile("mov %0, sp" : "=r"(sp));
    #elif RCKID_ENABLE_STACK_PROTECTION
            #error "Only aarch64 and x86_64 are supported for RCKid stack protection in fantasy backend"
    #else
            UNIMPLEMENTED;
    #endif
#else 
            // Cortex-M 
            asm volatile("mov %0, sp" : "=r"(sp));
#endif
            return (char*)sp;
        }   

        static inline uint32_t maxSize_ = 0;
#ifdef RCKID_BACKEND_FANTASY
        static inline thread_local char * stackTop_ = currentStack(); 
#endif        
    };

#ifdef RCKID_BACKEND_FANTASY
    /** Default system malloc switch for fantasy backend. 
     
        On fantasy backend, various extra-sdk features such as audio, video playback, host filesystem, etc. require the use of normal heap as their memory consumption should not be counted towards RCKid limited RAM. This guard simply ensures that when active all malloc (and hence new) calls will go through original system malloc. 

        When the memory is freed, the fantasy implementation will use RAMHeap if the pointer comes from RAM heap, or default implementation otherwise.
     */
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