#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>

/** Very simple producer / consumer unbounded size channel that supports blocking and non-blocking operations. 
 */
template<typename T>
class Channel {
public:
    bool canRead() const {
        std::lock_guard<std::mutex> g{m_};
        return ! ch_.empty();
    }

    T read() {
        std::unique_lock<std::mutex> g{m_};
        while (ch_.empty())
            cv_.wait(g);
        T result = ch_.front();
        ch_.pop_front();
        return result;
    }

    void write(T && value) {
        std::lock_guard<std::mutex> g{m_};
        ch_.push_back(std::move(value));
        cv_.notify_all();
    }

    void clar() {
        std::unique_lock<std::mutex> g{m_};
        ch_.clear();
    }

private:

    std::deque<T> ch_;
    mutable std::mutex m_;
    std::condition_variable cv_;

}; // Channel