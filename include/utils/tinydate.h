#pragma once

#include <stdint.h>

/** A simple date-time object that fits in just 4 bytes. 
 
    Due to endiannes and associated weirdness we can't use uint32's but must use array of bytes:

    raw 0 : MMssssss
    raw 1 : MMmmmmmm
    raw 2 : YYYhhhhh
    raw 3 : YYYddddd
*/
class TinyDate {
public:
    static constexpr uint16_t YEAR_START = 2023;
    static constexpr uint16_t YEAR_END = YEAR_START + 64;

    uint8_t seconds() const { return raw_[0] & SECOND_MASK; }
    uint8_t minutes() const { return raw_[1] & MINUTE_MASK; }
    uint8_t hours() const { return raw_[2] & HOUR_MASK; }
    uint8_t day() const { return raw_[3] & DAY_MASK; }
    uint8_t month() const { 
        return ((raw_[0] & MONTH_MASK) >> 4) 
            | ((raw_[1] & MONTH_MASK) >> 6);
    }
    uint16_t year() const { 
        return YEAR_START
            + ((raw_[2] & YEAR_MASK) >> 2)
            + ((raw_[3] & YEAR_MASK) >> 5);
    }

    /** Returns the day of week for given date. 
     
        Only works for dates in range. Returns 0 for Monday and 6 for Sunday. 
    */
    // https://cs.uwaterloo.ca/~alopez-o/math-faq/node73.html
    uint8_t dayOfWeek() const {
        uint8_t m = month();
        uint8_t d = day();
        uint16_t y = year();
        y -= m<3; 
        return (y + y / 4 - y / 100 +y / 400 + "-bed=pen+mad."[m] + d) % 7;
    }


    void setSeconds(uint8_t value) { 
        raw_[0] &= ~SECOND_MASK;
        raw_[0] |= value & SECOND_MASK;
    }

    void setMinutes(uint8_t value) {
        raw_[1] &= ~MINUTE_MASK;
        raw_[1] |= value & MINUTE_MASK;
    }

    void setHours(uint8_t value) {
        raw_[2] &= ~HOUR_MASK;
        raw_[2] |= value & HOUR_MASK;
    }

    void setDay(uint8_t value) {
        raw_[3] &= ~DAY_MASK;
        raw_[3] |= value & DAY_MASK;
    }

    void setMonth(uint8_t value) {
        uint8_t m1 = ((value >> 2) & 3) << 6;
        uint8_t m2 = (value & 3) << 6;
        raw_[0] &= ~MONTH_MASK;
        raw_[0] |= m1;
        raw_[1] &= ~MONTH_MASK;
        raw_[1] |= m2;
    }

    void setYear(uint16_t value) {
        value -= YEAR_START;
        uint8_t y1 = ((value >> 3) & 7) << 5;
        uint8_t y2 = (value & 7) << 5;
        raw_[2] &= ~YEAR_MASK;
        raw_[2] |= y1;
        raw_[3] &= ~YEAR_MASK;
        raw_[3] |= y2;
    }

    bool incSeconds() {
        uint8_t x = (seconds() + 1) % 60;
        setSeconds(x);
        return x == 0;
    }

    bool incMinutes() {
        uint8_t x = (minutes() + 1) % 60;
        setMinutes(x);
        return x == 0;
    }

    bool incHours() {
        uint8_t x = (hours() + 1) % 12;
        setHours(x);
        return x == 0;
    }

    bool incDay() {
        uint8_t x = day() + 1;
        if (x > daysInMonth(year(), month()))
            x = 1;
        setDay(x);
        return x == 1;
    }

    bool incMonth() {
        uint8_t x = month() + 1;
        if ( x > 12)
            x = 1;
        setMonth(x);
        return x == 1;
    }

    bool incYear() {
        uint16_t x = year() + 1;
        if (x > YEAR_END)
            x = YEAR_START;
        setYear(x);
        return x == YEAR_START;
    }

    bool secondTick() {
        if (! incSeconds())
            return false;
        if (! incMinutes())
            return false;
        if (! incHours())
            return false;
        if (! incDay())
            return false;
        if (! incMonth())
            return false;
        return incYear();
    }

    static uint8_t daysInMonth(uint16_t year, uint8_t month) {
        switch (month) {
            case 1: // Jan
            case 3: // Mar
            case 5: // May
            case 7: // Jul
            case 8: // Aug
            case 10: // Oct
            case 12: // Dec
                return 31;
            case 2 : // Feb
                // I'm ignoring the every 100 years leap year skip as the code will hopefully not be around for that long:)
                return (year % 4 == 0) ? 29 : 28;
            case 4:
            case 6:
            case 9:
            case 11:
            default: // whatever else, return something
                return 30;
        }
    }


private:
    static constexpr uint8_t SECOND_MASK = 63;
    static constexpr uint8_t MINUTE_MASK = 63;
    static constexpr uint8_t HOUR_MASK = 31;
    static constexpr uint8_t DAY_MASK = 31;
    static constexpr uint8_t MONTH_MASK = 192; 
    static constexpr uint8_t YEAR_MASK = 224;

    uint8_t raw_[4];

} __attribute__((packed)); // TinyDate

static_assert(sizeof(TinyDate) == 4);
