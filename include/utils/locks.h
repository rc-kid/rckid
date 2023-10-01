#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "utils.h"

namespace utils {

    /** Conditionally locking lock guard that can be used for already acquired locks where the std::adopt_lock is not acceptable (we know at runtime)
    */
    template<typename T>
    class cond_lock_guard {
    public:
        explicit cond_lock_guard(T & t, bool already_locked = false):
            l_{already_locked ? nullptr : & t} {
            if (l_)
                l_->lock();
        }

        ~cond_lock_guard() {
            if (l_)
                l_->unlock();
        }
    private:
        T * l_;  
    }; // cond_lock_guard    

    /** A simple spinlock. 
     
        These are useful in ISRs where a normal blocking mutex cannot be used. 
    */
    class SpinLock {
    public:
        void lock() {
            while (f_.test_and_set(std::memory_order_acquire)) {
                // spin
            }
        }

        void unlock() {
            f_.clear(std::memory_order_release);
        }

    private:

        std::atomic_flag f_ = ATOMIC_FLAG_INIT;
    }; // utils::SpinLock

    /** A simple lock that allows locking in normal and priority modes, guaranteeing that a priority lock request will be serviced before any waiting normal locks. 
     */
    class PriorityLock {
    public:

        PriorityLock():
            priorityRequests_(0) {
        }

        /** Grabs the lock in non-priority mode.
         */
        PriorityLock & lock() {
            std::unique_lock<std::mutex> g(m_);
            while (priorityRequests_ > 0)
                cv_.wait(g);
            g.release();
#ifndef NDEBUG
            locked_ = std::this_thread::get_id();
#endif
            return *this;
        }

        /** Grabs the lock in priority mode.
         */
        PriorityLock & priorityLock() {
            ++priorityRequests_;
            m_.lock();
            --priorityRequests_;
#ifndef NDEBUG
            locked_ = std::this_thread::get_id();
#endif
            return *this;
        }

        /** Releases the lock. 
         */
        void unlock() {
            cv_.notify_all();
#ifndef NDEBUG
            locked_ = std::thread::id{};
#endif
            m_.unlock();
        }

#ifndef NDEBUG
        bool locked() const {
            return locked_ == std::this_thread::get_id();
        }
#endif

    private:
        std::atomic<unsigned> priorityRequests_;
        std::mutex m_;
        std::condition_variable cv_;
#ifndef NDEBUG
        std::atomic<std::thread::id> locked_;
#endif          
    }; // utils::ReentrantLock

    /** Reentrant variant of the priority lock. 
     */
    class ReentrantPriorityLock {
    public:

        ReentrantPriorityLock():
            depth_{0},
            priorityRequests_(0) {
        }

        /** Grabs the lock in non-priority mode.
         */
        ReentrantPriorityLock & lock() {
            if (owner_ != std::this_thread::get_id()) {
                std::unique_lock<std::mutex> g(m_);
                while (priorityRequests_ > 0)
                    cv_.wait(g);
                g.release();
                owner_ = std::this_thread::get_id();
            }
            ++depth_;
            return *this;
        }

        /** Grabs the lock in priority mode.
         */
        ReentrantPriorityLock & priorityLock() {
            ++priorityRequests_;
            if (owner_ != std::this_thread::get_id()) {
                m_.lock();
                owner_ = std::this_thread::get_id();
            }
            --priorityRequests_;
            ++depth_;
            return *this;
        }

        /** Releases the lock. 
         */
        void unlock() {
            ASSERT(owner_ == std::this_thread::get_id());
            ASSERT(depth_ > 0);
            if (--depth_ == 0) {
                owner_ = std::thread::id{};
                cv_.notify_all();
                m_.unlock();
            }        
        }

    private:
        std::thread::id owner_;
        unsigned depth_;
        std::atomic<unsigned> priorityRequests_;
        std::mutex m_;
        std::condition_variable cv_;

    }; // utils::ReentrantPriorityLock

} // namespace utils
