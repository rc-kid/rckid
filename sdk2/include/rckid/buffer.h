#pragma once

#include <platform.h>

namespace rckid {

    template<typename T>
    class Buffer {
    public:
    
    private:
        uint32_t size_ = 0;
        uint32_t capacity_ = 0;
        mutable_ptr<T> data_ = nullptr;
    }; // rckid::Buffer<T>

    /** Very simple double buffer. 
     */
    template<typename T>
    class DoubleBuffer {
    public:
        DoubleBuffer(uint32_t size): 
            size_{size},
            front_{new T[size]}, 
            back_{new T[size]} {
        }

        ~DoubleBuffer() {
            delete [] front_;
            delete [] back_;
        }

        void swap() {
            T * tmp = front_;
            front_ = back_;
            back_ = tmp;
        }

        void resize(uint32_t size) {
            delete [] front_;
            delete [] back_;
            size_ = size;
            front_ = new T[size];
            back_ = new T[size];
        }

        uint32_t size() const { return size_; }

        T const * front() const { return front_; }
        T const * back() const { return back_; }
        T * front() { return front_; }
        T * back() { return back_; }

        /** Returns pointer to the front buffer and swaps immediately, putting the returned buffer to back.
         */
        T * frontAndSwap() {
            swap();
            return back_;
        }

    private:
        uint32_t size_;
        T * front_;
        T * back_;

    }; // rckid::DoubleBuffer<T>

    // TODO CountedBuffer

    // TODO MultiBuffer


} // namespace rckid