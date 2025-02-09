#pragma once

#include "../memory.h"

namespace rckid {

    /** Very simple double buffer. 
     */
    template<typename T>
    class DoubleBuffer {
    public:
        DoubleBuffer(uint32_t size, Allocator & a = Heap::allocator()): 
            size_{size},
            front_{a.alloc<T>(size)}, 
            back_{a.alloc<T>(size)} {
        }

        ~DoubleBuffer() {
            Heap::tryFree(front_);
            Heap::tryFree(back_);
        }

        void swap() {
            std::swap(front_, back_);
        }

        void resize(uint32_t size, Allocator & a = Heap::allocator()) {
            Heap::tryFree(front_);
            Heap::tryFree(back_);
            size_ = size;
            front_ = a.alloc<T>(size);
            back_ = a.alloc<T>(size);
        }

        size_t size() const { return size_; }

        T const * front() const { return front_; }
        T const * back() const { return back_; }
        T * front() { return front_; }
        T * back() { return back_; }

    private:
        size_t size_;
        T * front_;
        T * back_;

    }; // rckid::DoubleBuffer<T>

} // namespace rckid
