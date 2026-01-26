#pragma once

#include <platform.h>

#include <rckid/error.h>
#include <rckid/hal.h>
#include <rckid/log.h>

namespace rckid {

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

        /** Guard that ensures memory is not leaking. 
         
            This is the simplest of the guard that merely ensures that used heap memory at exit is not greater than used memory at the beginning. 

            For debugging purposes, the guard provides the usedDelta() function that returns currently used heap bytes *above* the used bytes at the time of guard creation. In other words the guard fails if usedDelta() is positive at the time of guard destruction.
         */
        class UseGuard {
        public:
            UseGuard() = default;
            ~UseGuard() {
                ASSERT(usedDelta() <= 0);
            }
            
            int32_t usedDelta() const {
                return static_cast<int32_t>(usedBytes()) - static_cast<int32_t>(usedAtStart_);
            }

        private:
            uint32_t usedAtStart_ = usedBytes();

        }; // Heap::UseGuard

        class ReserveGuard {
        public:
            ReserveGuard() = default;
            ~ReserveGuard() {
                ASSERT(reservedDelta() <= 0);
            }

            int32_t reservedDelta() const {
                return static_cast<int32_t>(reservedBytes()) - static_cast<int32_t>(reservedAtStart_);
            }

        private:
            uint32_t reservedAtStart_ = reservedBytes();
        }; // Heap::ReserveGuard

        class UseAndReserveGuard {
        public:
            int32_t usedDelta() const { return use_.usedDelta(); }
            int32_t reservedDelta() const { return reserve_.reservedDelta(); }
        private:
            UseGuard use_;
            ReserveGuard reserve_;
        };

    private:
        class Chunk;

        static inline Chunk * heapStart_ = reinterpret_cast<Chunk*>(hal::memory::heapStart());
        static inline Chunk * heapEnd_ = heapStart_;
        static inline Chunk * freelist_ = nullptr;
        static inline uint32_t lastSize_ = 0;
        
    }; // rckid::Heap


    /** Unique pointer.
     
        Just a wrapper around std::unique_ptr for easier future replacement and consistency together with other smart pointers below.
     */
    template<typename T>
    using unique_ptr = std::unique_ptr<T>;

    /** Immutable, unique pointer that can point to non-heap objects as well.
     
        Special version of unique pointer that can point to both heap allocated objects as well as to immutable data (i.e. data stored in flash memory or ROM). The pointer takes ownership of heap allocated objects and will free them when destroyed, but will not attempt to free pointers to immutable data.

        Unlike its mutable_ptr counterpart, immutable_ptr does not allow mutable access to the data it points to (as the data can in theory be in flash as well). This means the pointer does not need to employ lazy copy-on-write semantics.
     */
    template<typename T>
    class immutable_ptr {
    public:

        constexpr immutable_ptr() : ptr_{nullptr} {}
        constexpr immutable_ptr(nullptr_t) : ptr_{nullptr} {}


        constexpr immutable_ptr(T const * ptr): ptr_{ptr} {
            ASSERT(Heap::contains(ptr) || hal::memory::isImmutableDataPtr(ptr));
        }

        /** Immutable pointers can be created from existing values as well. 
         
            But in this case the value must belong to the immutable data pool as defined by the HW abstraction layer. This is because otherwise there could be subtle lifetime issues, such as when immutable_ptr would point to a stack variable that gets out of scope. 
         */
        constexpr immutable_ptr(T const & value): ptr_{& value } {
            ASSERT(hal::memory::isImmutableDataPtr(ptr_));
            ASSERT(! Heap::contains(ptr_));
        }

        immutable_ptr(immutable_ptr const & ) = delete;
        immutable_ptr & operator = (immutable_ptr const &) = delete;

        immutable_ptr(immutable_ptr && other) noexcept : ptr_{other.ptr_} { other.ptr_ = nullptr; }

        immutable_ptr & operator = (immutable_ptr && other) {
            if (this == & other)
                return *this;
            deletePtr();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
            return *this;
        }

        ~immutable_ptr() { deletePtr(); }

        T const & operator * () const { return * ptr_; } 
        T const * operator -> () const { return ptr_; } 
        T const * get() const { return ptr_; }
        T const & operator [] (uint32_t index) const { return ptr_[index]; }

        T const * release() {
            T const * result = ptr_;
            ptr_ = nullptr;
            return result;
        }

    private:

        void deletePtr() {
            if (Heap::contains(ptr_))
                Heap::free(const_cast<T*>(ptr_));
        }

        T const * ptr_ = nullptr;
    }; // rckid::immutable_ptr<T>

    /** Mutable unique pointer
     
        Special version of unique pointer that can, similarly to immutable_ptr, point to both heap allocated *and* immutable flash data. However, when mutable access is requested *and* the data is stored in flash, the pointer performs heap allocation and copies the data over, so that mutable access is possible. 

        This makes the mutable_ptr a bit heavier as it has to store the size of the data it points to in order to be able to perform the copy-on-write when mutable access is requested. Due to peculiarities of the new[] and delete[] operators in C++, the mutable_ptr can only be used with trivially copyable and trivially destructible types to avoid issues with array cookies and destructors not being called properly.
     */
    template<typename T>
    class mutable_ptr {
    public:

        static_assert(std::is_trivially_copy_constructible_v<T>);
        static_assert(std::is_trivially_destructible_v<T>);

        constexpr mutable_ptr(): ptr_{nullptr}, count_{0} {}

        constexpr mutable_ptr(nullptr_t) : ptr_{nullptr}, count_{0} {}

        constexpr mutable_ptr(T * ptr, uint32_t count) :
            ptr_{ptr},
            count_{count} {
            ASSERT(Heap::contains(ptr));
        }

        constexpr mutable_ptr(T const * ptr, uint32_t count) :
            ptr_{const_cast<T*>(ptr)},
            count_{count} {
            ASSERT(Heap::contains(ptr) || hal::memory::isImmutableDataPtr(ptr));
        }

        template<uint32_t SIZE>
        constexpr mutable_ptr(T const (&ptr)[SIZE]):
            ptr_{const_cast<T*>(ptr)},
            count_{SIZE} {
            ASSERT(Heap::contains(ptr) || hal::memory::isImmutableDataPtr(ptr));
        }

        template<typename U = T, typename = std::enable_if_t<std::is_same_v<U,char>>>
        constexpr mutable_ptr(char const* s) : 
            ptr_{const_cast<char*>(s)},
            count_{static_cast<uint32_t>(std::strlen(s) + 1)}
        {
            ASSERT(hal::memory::isImmutableDataPtr(s));
        }

        mutable_ptr(mutable_ptr const & ) = delete;
        mutable_ptr & operator = (mutable_ptr const &) = delete;

        template<typename U = T, typename = std::enable_if_t<std::is_same_v<U,char>>>
        mutable_ptr & operator = (char const * s) {
            deletePtr();
            ptr_ = const_cast<char*>(s);
            count_ = static_cast<uint32_t>(std::strlen(s) + 1);
            ASSERT(hal::memory::isImmutableDataPtr(s));
            return *this;
        }

        mutable_ptr(mutable_ptr && other) noexcept : 
            ptr_{other.ptr_}, 
            count_{other.count_} 
        { 
            other.ptr_ = nullptr; 
            other.count_ = 0;
        }

        mutable_ptr & operator = (mutable_ptr && other) {
            if (this == & other)
                return *this;
            deletePtr();
            ptr_ = other.ptr_;
            count_ = other.count_;
            other.ptr_ = nullptr;
            other.count_ = 0;
            return *this;
        }

        ~mutable_ptr() { deletePtr(); }

        T const * ptr() const { return ptr_; }

        T * mut() {
            if (! Heap::contains(ptr_)) {
                T * newPtr = new T[count_];
                memcpy(newPtr, ptr_, sizeof(T) * count_);
                deletePtr();
                ptr_ = newPtr;
            }
            return ptr_;
        }

        bool isMutable() const { return Heap::contains(ptr_); }

        uint32_t count() const { return count_; }

        mutable_ptr<T> clone() const {
            if (Heap::contains(ptr_)) {
                T * newPtr = new T[count_];
                memcpy(newPtr, ptr_, sizeof(T) * count_);
                return mutable_ptr<T>{newPtr, count_};
            } else {
                // use ptr() so that we get const correctness in constructor
                return mutable_ptr<T>{ptr(), count_};
            }
        }

        T const * releasePtr() {
            T * result = ptr_;
            ptr_ = nullptr;
            count_ = 0;
            return result;
        }

        T * releaseMut() {
            T * result = mut();
            ptr_ = nullptr;
            count_ = 0;
            return result;
        }

    private:

        void deletePtr() {
            if (Heap::contains(ptr_))
                // this is safe since we only work with trivially destructible types where no array cookies are stored
                Heap::free(ptr_);
        }

        T * ptr_ = nullptr;
        uint32_t count_ = 0;

    }; // rckid::mutable_ptr<T>

} // namespace rckid