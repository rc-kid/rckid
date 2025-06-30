#pragma once

#include "../error.h"
#include "../log.h"

namespace rckid {

    // forward declaration
    bool memoryIsImmutable(void const * ptr);

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

    private:
        uint32_t size_;
        T * front_;
        T * back_;

    }; // rckid::DoubleBuffer<T>

    template<typename T>
    class LazyBuffer {
    public:

        LazyBuffer():
            size_{0},
            capacity_{0},
            data_{nullptr} {
        }

        LazyBuffer(uint32_t capacity):
            size_{0}, 
            capacity_{capacity} {
            data_ = new T[capacity];
        }

        LazyBuffer(T const * from, uint32_t size):
            size_{size}, 
            capacity_{0},
            data_{const_cast<T*>(from)} {
            // if not immutable, we need to copy, but do not delete, or attempt to delete the source (this is mostly important for fantasy backend where immutable is not supported)
            if (!immutable()) {
                data_ = new T[size_];
                capacity_ = size_;
                memcpy(data_, from, sizeof(T) * size_);
            }
        }

        template<uint32_t SIZE>
        LazyBuffer(T const (&from)[SIZE]): LazyBuffer(from, SIZE) {}

        LazyBuffer(LazyBuffer const & from):
            size_{from.size_},
            capacity_{from.capacity_}, 
            data_{from.data_} {
            // if not immutable, we need to copy
            if (!immutable()) {
                data_ = new T[size_];
                capacity_ = size_;
                memcpy(data_, from.data_, sizeof(T) * size_);
            }
        }

        LazyBuffer(LazyBuffer && from): 
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
            if (!immutable())
                delete [] data_;
            //if (a_.contains(data_))
            //    delete [] data_;
        }

        LazyBuffer & operator = (LazyBuffer const & from) {
            if (!immutable())
                delete [] data_;
            size_ = from.size_;
            capacity_ = from.capacity_;
            data_ = from.data_;
            if (! immutable()) {
                data_ = new T[size_];
                capacity_ = size_;
                memcpy(data_, from.data_, sizeof(T) * size_);
            }
            return *this;
        }

        LazyBuffer & operator = (LazyBuffer && from) {
            if (!immutable())
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

        void setSize(uint32_t size) {
            ASSERT(size <= capacity_);
            size_ = size;
        }

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

        void reserve(uint32_t size) {
            if (size > capacity_)
                resize(size);
            else 
                size_ = size;
        }

        void shrink() {
            if (size_ < capacity_)
                resize(size_);
        }

        bool immutable() const { return memoryIsImmutable(data_); }

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
                reserve(size_ == 0 ? 16 : size_ * 2);
            data_[size_++] = value;
        }

        /** Shrinks the internal buffer and releases it, i.e. transfers the ownership to the caller. 
         
            This is a very dangerous function that should be used with extreme care. Note that it is up to caller to determine the size before calling the release function and to treat the returned pointer as immutable if part of immutable memory.
         */
        T * release() {
            shrink();
            T * result = data_;
            data_ = nullptr;
            size_ = 0;
            capacity_ = 0;
            return result;
        }

    protected:

        void resize(uint32_t newCapacity) {
            T * newData = new T[newCapacity];
            memcpy(newData, data_, size_ * sizeof(T));
            if (!immutable())
                delete [] data_;
            data_ = newData;
            capacity_ = newCapacity;
        }

    private:
        uint32_t size_;
        uint32_t capacity_;
        T * data_;
    }; 

} // namespace rckid
