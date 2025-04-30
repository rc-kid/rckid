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
            delete [] front_;
            delete [] back_;
        }

        void swap() {
            std::swap(front_, back_);
        }

        void resize(uint32_t size, Allocator & a = Heap::allocator()) {
            delete [] front_;
            delete []back_;
            size_ = size;
            front_ = a.alloc<T>(size);
            back_ = a.alloc<T>(size);
        }

        uint32_t size() const { return size_; }

        T const * front() const { return front_; }
        T const * back() const { return back_; }
        T * front() { return front_; }
        T * back() { return back_; }

    private:
        uint32_t size_;
        T * front_;
        T * back_;

    }; // rckid::DoubleBuffer<T>

    template<typename T>
    class LazyBuffer {
    public:

        LazyBuffer(Allocator & a = Heap::allocator):
            a_{a}, 
            size_{0},
            capacity_{0},
            data_{nullptr} {
        }

        LazyBuffer(uint32_t capacity, Allocator & a = Heap::allocator()):
            a_{a}, 
            size_{0}, 
            capacity_{0} {
            data_ = a_.alloc<T>(capacity);
        }

        LazyBuffer(T const * from, uint32_t size, Allocator & a = Heap::allocator()):
            a_{a}, 
            size_{size}, 
            capacity_{0},
            data_{const_cast<T*>(from)} {
            ASSERT(immutable());
        }

        template<uint32_t SIZE>
        LazyBuffer(T const (&from)[SIZE], Allocator & a = Heap::allocator()): LazyBuffer(from, SIZE, a) {}

        LazyBuffer(LazyBuffer const & from):
            a_{from.a_}, 
            size_{from.size_},
            capacity_{from.capacity_}, 
            data_{from.data_} {
            // if not immutable, we need to copy
            if (!immutable()) {
                data_ = a_.alloc<T>(size_);
                capacity_ = size_;
                memcpy(data_, from.data_, sizeof(T) * size_);
            }
        }

        LazyBuffer(LazyBuffer && from): 
            a_{from.a_},
            size_{from.size_},
            capacity_{from.capacity_},
            data_{from.data_} {
            if (! immutable()) {
                from.capacity_ = 0;
                from.size_ = 0;
                from.data_ = nullptr;
            }
        }

        ~LazyBuffer() {
            if (a_.contains(data_))
                delete [] data_;
        }

        LazyBuffer & operator = (LazyBuffer const & from) {
            if (a_.contains(data_))
                delete [] data_;
            size_ = from.size_;
            capacity_ = from.capacity_;
            data_ = from.data_;
            if (! immutable()) {
                data_ = a_.alloc<T>(size_);
                capacity_ = size_;
                memcpy(data_, from.data_, sizeof(T) * size_);
            }
            return *this;
        }

        LazyBuffer & operator = (LazyBuffer && from) {
            if (a_.contains(data_))
                delete [] data_;
            size_ = from.size_;
            capacity_ = from.capacity_;
            data_ = from.data_;
            if (! immutable()) {
                from.capacity_ = 0;
                from.size_ = 0;
                from.data_ = nullptr;
            }
            return *this;
        }

        uint32_t size() const { return size_; }
        uint32_t capacity() const { return capacity_; }

        T const * begin() const { return data_; }
        T const * end() const { return data_ + size_; }

        T * begin() {
            ASSERT(! immutable());
            return data_;
        }

        T * end() {
            ASSERT(! immutable());
            return data_ + size_;
        }

        void grow(uint32_t size) {
            if (size > capacity_)
                resize(size);
        }

        void shrink() {
            if (size_ < capacity_)
                resize(size_);
        }

        bool immutable() const { return ! a_.contains(data_); }

        void makeMutable() {
            if (immutable())
                resize(capacity_);
        }

        T const * data() const { return data_; }

        T * data() {
            ASSERT(!immutable());
            return data_;
        }

        T const & operator[](uint32_t index) const { return data_[index]; }
        T & operator[](uint32_t index) {
            ASSERT(! immutable());
            return data_[index];
        }

        void append(T const & value) {
            if (capacity_ <= size_) 
                grow(size_ == 0 ? 16 : size_ * 2);
            data_[size_] = value;
        }
        
        Allocator & allocator() const { return const_cast<Allocator &>(a_); }
    protected:

        void resize(uint32_t newCapacity) {
            T * newData = a_.alloc<T>(newCapacity);
            memcpy(newData, data_, size_ * sizeof(T));
            if (a_.contains(data_))
                delete [] data_;
            data_ = newData;
            capacity_ = newCapacity;
        }

    private:
        Allocator & a_ = Heap::allocator();
        uint32_t size_;
        uint32_t capacity_;
        T * data_;
    }; 

} // namespace rckid
