#include <hardware/timer.h>

#include <platform.h>

/** Simple wrap-around timepoint with 32bit arithmetics. 
 
    Super useful for the uint32_t steady time in us.
 */
class TimePoint {
public:
    TimePoint() : t_(0) {}

    explicit TimePoint(uint32_t now) : t_(now) {}

    static TimePoint now() { return TimePoint{timer_hw->timerawl}; }

    TimePoint & operator = (uint32_t value) {
        t_ = value;
        return *this;
    }

    TimePoint operator + (uint32_t uS) const {
        return TimePoint{t_ + uS};
    }

    bool operator == (uint32_t value) const { return t_ == value; }
    bool operator != (uint32_t value) const { return t_ != value; }

    bool operator >= (TimePoint now) const {
        return static_cast<int32_t>(now.t_ - t_) <= 0;
    }

    uint32_t raw() const { return t_; }

private:
    uint32_t t_;
};