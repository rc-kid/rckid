#pragma once

#include <platform.h>

namespace rckid {

    /** Basic buffer. 
     
        Buffer is simply an array of given type and a size indicating how much of the allocated space is currently used. 
     */
    template<typename T>
    class alignas(4) Buffer {
    public:

        uint32_t used() const { return used_; }

        void setUsed(uint32_t used) { used_ = used; }

        T * data() { return data_; }

        T const * data() const { return data_.get(); }

    private:

        template<typename V>
        friend class DoubleBuffer;

        template<typename V>
        friend class MultiBuffer;

        static Buffer * create(uint32_t size) {
            Buffer * result = reinterpret_cast<Buffer*>(new uint8_t[sizeof(Buffer) + sizeof(T) * size]);
            result->used_ = 0;
            return result;
        }

        uint32_t used_ = 0;
        T data_[];
    } __attribute__((packed)); // rckid::Buffer<T>

    
    /** Very simple double buffer. 
     
        Double buffer consists of two buffers of the same size and type, front and back, and the ability to swap them. This is particularly useful for things like rendering where we want to prepare the next frame in the back buffer while the front buffer is being displayed, and then swap them when the next frame is ready.
     */
    template<typename T>
    class DoubleBuffer {
    public:
        DoubleBuffer(uint32_t size): 
            size_{size},
            front_{Buffer<T>::create(size)}, 
            back_{Buffer<T>::create(size)} {
        }

        ~DoubleBuffer() {
            delete [] reinterpret_cast<uint8_t*>(front_);
            delete [] reinterpret_cast<uint8_t*>(back_);
        }

        uint32_t size() const { return size_; }

        void swap() {
            std::swap(front_, back_);
        }

        Buffer<T> const & front() const { return *front_; }
        Buffer<T> const & back() const { return *back_; }
        Buffer<T> & front() { return *front_; }
        Buffer<T> & back() { return *back_; }

    private:
        uint32_t size_;
        Buffer<T> * front_;
        Buffer<T> * back_;
    }; // rckid::DoubleBuffer<T>

    template<typename T>
    class MultiBuffer {
    public:
        MultiBuffer(uint32_t bufferSize, uint32_t numBuffers):
            bufferSize_{bufferSize},
            numBuffers_{numBuffers},
            buffers_{new BufferInfo*[numBuffers_]}
        {
            // initialize the buffers to correct size and add them all into a free list
            for (uint32_t i = 0; i < numBuffers_; ++i) {
                buffers_[i] = BufferInfo::create(bufferSize_);
                buffers_[i]->next = free_;
                free_ = buffers_[i];
            }
        }

        ~MultiBuffer() {
            for (uint32_t i = 0; i < numBuffers_; ++i)
                delete [] reinterpret_cast<uint8_t*>(buffers_[i]);
            delete [] buffers_;
        }

        uint32_t size() const { return bufferSize_; }
        uint32_t numBuffers() const { return numBuffers_; }

        /** Returns next free buffer that can be refilled with data. 
         
            NOTE that the returned buffer is *removed* from the freelist and *must* be marked as ready when filled via the markReady function, or returned to the free list again via the markFree fuction. 

            If all buffers are in use, returns nullptr. 
         */
        Buffer<T> * nextFree() {
            if (free_ == nullptr)
                return nullptr;
            Buffer<T> * result = & free_->buffer;
            free_ = free_->next;
            return result;
        }

        /** Returns next ready buffer, i.e. buffer that can be consumed. 
          
            Buffers are consumed in the order they were marked as ready. Returns nullptr if no ready buffer is available. The returned buffer is removed from the ready list and must be returned th to the free list explicitly when no longer used by the consumer via the markFree() function.
         */
        Buffer<T> * nextReady() {
            if (ready_ == nullptr)
                return nullptr;
            Buffer<T> * result = & ready_->buffer;
            ready_ = ready_->next;
            if (ready_ == nullptr)
                readyEnd_ = nullptr;
            return result;
        }

        /** Marks the buffer as free. 
         */
        void markFree(Buffer<T> * buffer) { markFree(buffer->data()); }

        /** Marks the raw buffer as free. 
         
            The buffer is identified by its data pointer
         */
        void markFree(T * data) {
            BufferInfo * buf = BufferInfo::fromData(data);
            buf->buffer.used_ = 0;
            buf->next = free_;
            free_ = buf;
        }

        /** Marks the bufer as ready to be consumed. Buffers are consumed in the order they were marked ready.
         */
        void markReady(Buffer<T> * buffer) {
            BufferInfo * buf = BufferInfo::fromData(buffer->data());
            buf->next = nullptr;
            if (readyEnd_ == nullptr) {
                ready_ = buf;
                readyEnd_ = buf;
            } else {
                readyEnd_->next = buf;
                readyEnd_ = buf;
            }
        }

    private:

        struct alignas(4) BufferInfo {
            BufferInfo * next;
            Buffer<T> buffer;

            static BufferInfo * create(uint32_t size) {
                BufferInfo * result = reinterpret_cast<BufferInfo*>(new uint8_t[sizeof(BufferInfo*) + sizeof(T) * size]);
                result->next = 0;
                result->buffer.used_ = 0;
                return result;
            }

            static BufferInfo * fromData(T * data) {
                return reinterpret_cast<BufferInfo*>(reinterpret_cast<uint8_t*>(data) - sizeof(BufferInfo*) - sizeof(Buffer<T>));
            }

        } __attribute__((packed));

        static_assert(sizeof(BufferInfo) == sizeof(BufferInfo*) + sizeof(Buffer<T>));

        uint32_t bufferSize_;
        uint32_t numBuffers_;
        BufferInfo ** buffers_ = nullptr;
        BufferInfo * free_ = nullptr;
        BufferInfo * ready_ = nullptr;
        BufferInfo * readyEnd_ = nullptr;


    }; // rckid::MultiBuffer<T>



} // namespace rckid