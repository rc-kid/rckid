#pragma once

template<typename T, size_t BUFFER_SIZE>
class RingAvg {
public:

    void reset() {
        ready_ = false;
        i_ = 0;
    }

    void addObservation(T value) {
        sum_ = sum_ - buffer_[i_] + value;
        buffer_[i_++] = value;
        if (i_ >= BUFFER_SIZE) {
            i_ = 0;
            ready_ = true;
        }
    }

    bool ready() const { return ready_; }

    T value() const { return sum_ / BUFFER_SIZE; }

private:
    T buffer_[BUFFER_SIZE];
    unsigned i_ = 0;
    long sum_ = 0;
    bool ready_ = false;
}; // RingAvg