#pragma once

#include <stdint.h>
#include "definitions.h"

/** A simple date-time object that fits in just 4 bytes. 
 
    Due to endiannes and associated weirdness we can't use uint32's but must use array of bytes:

    raw 0 : MMssssss
    raw 1 : MMmmmmmm
    raw 2 : YYYhhhhh
    raw 3 : YYYddddd
*/
PACKED_ALIGNED(4, class TinyDateTime {
public:

    static constexpr uint16_t YEAR_START = 2023;
    static constexpr uint16_t YEAR_END = YEAR_START + 64;

    uint8_t second() const { return raw_[0] & SECOND_MASK; }
    uint8_t minute() const { return raw_[1] & MINUTE_MASK; }
    uint8_t hour() const { return raw_[2] & HOUR_MASK; }
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

    void clear() {
        raw_[0] = 0xff;
        raw_[1] = 0xff;
        raw_[2] = 0xff;
        raw_[3] = 0xff;
    }

    void set(uint8_t day, uint8_t month, uint16_t year) {
        raw_[0] = 0x0;
        raw_[1] = 0x0;
        raw_[2] = 0x0;
        raw_[3] = 0x0;
        setDay(day);
        setMonth(month);
        setYear(year);
    }

    void set(uint8_t day, uint8_t month, uint16_t year, uint8_t h, uint8_t m, uint8_t s) {
        setDay(day);
        setMonth(month);
        setYear(year);
        setHour(h);
        setMinute(m);
        setSecond(s);
    }

    bool isValid() const { return month() < 13; }

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

    void setSecond(uint8_t value) { 
        if (value >= 60)
            value = 59;
        raw_[0] &= ~SECOND_MASK;
        raw_[0] |= value & SECOND_MASK;
    }

    void setMinute(uint8_t value) {
        if (value >= 60)
            value = 59;
        raw_[1] &= ~MINUTE_MASK;
        raw_[1] |= value & MINUTE_MASK;
    }

    void setHour(uint8_t value) {
        if (value >= 24)
            value = 23;
        raw_[2] &= ~HOUR_MASK;
        raw_[2] |= value & HOUR_MASK;
    }

    void setDay(uint8_t value) {
        unsigned dim = daysInMonth(year(), month());
        if (value > dim)
            value = dim;
        if (value == 0)
            value = 1;
        raw_[3] &= ~DAY_MASK;
        raw_[3] |= value & DAY_MASK;
    }

    void setMonth(uint8_t value) {
        if (value == 0)
            value = 1;
        if (value > 12)
            value = 12;
        uint8_t m1 = ((value >> 2) & 3) << 6;
        uint8_t m2 = (value & 3) << 6;
        raw_[0] &= ~MONTH_MASK;
        raw_[0] |= m1;
        raw_[1] &= ~MONTH_MASK;
        raw_[1] |= m2;
    }

    void setYear(uint16_t value) {
        if (month() == 2 && day() == 29)
            if (daysInMonth(value, 2) == 28)
                setDay(28);
        value -= YEAR_START;
        uint8_t y1 = ((value >> 3) & 7) << 5;
        uint8_t y2 = (value & 7) << 5;
        raw_[2] &= ~YEAR_MASK;
        raw_[2] |= y1;
        raw_[3] &= ~YEAR_MASK;
        raw_[3] |= y2;
    }

    bool incSecond() {
        uint8_t x = (second() + 1) % 60;
        setSecond(x);
        return x == 0;
    }

    bool incMinute() {
        uint8_t x = (minute() + 1) % 60;
        setMinute(x);
        return x == 0;
    }

    bool incHour() {
        uint8_t x = (hour() + 1) % 12;
        setHour(x);
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
        if (! incSecond())
            return false;
        if (! incMinute())
            return false;
        if (! incHour())
            return false;
        if (! incDay())
            return false;
        if (! incMonth())
            return false;
        return incYear();
    }

    static bool isLeapYear(uint16_t year) {
        if (year % 4 != 0)
            return false;
        return (year % 100 != 0) || (year % 400 == 0);
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
                return isLeapYear(year) ? 29 : 28;
            case 4:
            case 6:
            case 9:
            case 11:
            default: // whatever else, return something
                return 30;
        }
    }

    bool operator == (TinyDateTime const & other) const {
        return (raw_[0] == other.raw_[0]) && (raw_[1] == other.raw_[1]) && (raw_[2] == other.raw_[2]) && (raw_[3] == other.raw_[3]);
    }

    uint32_t asRaw() const {
        return (static_cast<uint32_t>(raw_[0]) << 24) 
             | (static_cast<uint32_t>(raw_[1]) << 16) 
             | (static_cast<uint32_t>(raw_[2]) << 8) 
             | static_cast<uint32_t>(raw_[3]);
    }

    static TinyDateTime fromRaw(uint32_t raw) {
        TinyDateTime result;
        result.raw_[0] = (raw >> 24) & 0xff;
        result.raw_[1] = (raw >> 16) & 0xff;
        result.raw_[2] = (raw >> 8) & 0xff;
        result.raw_[3] = raw & 0xff;
        return result;
    }

private:
    static constexpr uint8_t SECOND_MASK = 63;
    static constexpr uint8_t MINUTE_MASK = 63;
    static constexpr uint8_t HOUR_MASK = 31;
    static constexpr uint8_t DAY_MASK = 31;
    static constexpr uint8_t MONTH_MASK = 192; 
    static constexpr uint8_t YEAR_MASK = 224;

    uint8_t raw_[4] = { 0x0, 0x0, 0x0, 0x0 };

}); // TinyDateTime

static_assert(sizeof(TinyDateTime) == 4);

/** Date only, which fits into 4 bytes. Unlike TinyDateTime, which is best suited to tracking current time thanks to the small year range, TinyDate will work with years from -32768 to +32767.
 */
PACKED_ALIGNED(4, class TinyDate{
public:

    TinyDate() = default;

    TinyDate(uint8_t day, uint8_t month, int16_t year) {
        setDay(day);
        setMonth(month);
        setYear(year);
    }

    uint8_t day() const { return raw_[0]; }
    uint8_t month() const { return raw_[1]; }
    int16_t year() const { return (static_cast<int16_t>(raw_[2]) << 8) | raw_[3]; }

    void setDay(uint8_t value) {
        if (value == 0)
            value = 1;
        raw_[0] = value;
    }

    void setMonth(uint8_t value) {
        if (value == 0)
            value = 1;
        if (value > 12)
            value = 12;
        raw_[1] = value;
    }

    void setYear(int16_t value) {
        raw_[2] = (value >> 8) & 255;
        raw_[3] = value & 255;
    }

    bool operator == (TinyDate const & other) const {
        return (raw_[0] == other.raw_[0]) && (raw_[1] == other.raw_[1]) && (raw_[2] == other.raw_[2]);
    }

    uint32_t asRaw() const {
        return (static_cast<uint32_t>(raw_[0]) << 24) 
             | (static_cast<uint32_t>(raw_[1]) << 16) 
             | (static_cast<uint32_t>(raw_[2]) << 8) 
             | static_cast<uint32_t>(raw_[3]);
    }

    static TinyDate fromRaw(uint32_t raw) {
        TinyDate result;
        result.raw_[0] = (raw >> 24) & 0xff;
        result.raw_[1] = (raw >> 16) & 0xff;
        result.raw_[2] = (raw >> 8) & 0xff;
        result.raw_[3] = raw & 0xff;
        return result;
    }

    /** Returns number of days till the next date, ignoring the year, useful for birthdays, etc. 
     */
    uint32_t daysTillNextAnnual(TinyDate const & other) const {
        uint32_t result = 0;
        uint32_t y = year();
        uint32_t m1 = month();
        uint32_t d1 = day();
        uint32_t m2 = other.month();
        uint32_t d2 = other.day();
        while (true) {
            if (m1 == m2) {
                if (d1 <= d2) {
                    result += d2 - d1;
                    break;
                }
            }
            uint32_t daysInMonth = TinyDateTime::daysInMonth(y, m1);
            result += daysInMonth - d1 + 1;
            d1 = 1;
            m1++;
            if (m1 > 12) {
                m1 = 1;
                y++;
            }
        }
        return result;
    }


private:
    uint8_t raw_[4] = { 1, 1, 0, 0 };
}); // TinyDate

static_assert(sizeof(TinyDate) == 4);

/** Simple alarm. 
 
    Hours, minutes and days of the week when the alarm is active. Since we need at least 18 bits to store this information, the alarm uses 3 bytes internally for hours, minutes and days of the week respectively. 
 */
PACKED(class TinyAlarm {
public:

    uint8_t hour() const { return raw_[0]; }
    uint8_t minute() const { return raw_[1]; }
    bool enabled() const { return raw_[2] != 0; }
    bool enabledDay(uint8_t dayOfWeek) const { return raw_[2] & (1 << dayOfWeek); }

    void setHour(uint8_t value) {
        if (value >= 24)
            value = 23;
        raw_[0] = value;
    }

    void setMinute(uint8_t value) {
        if (value >= 60)
            value = 59;
        raw_[1] = value;
    }

    void setEnabled(bool value = true) {
        raw_[2] = value ? 0x7f : 0;
    }

    void setEnabledDay(uint8_t day, bool value = true) {
        if (day >= 7)
            day = 6;
        if (value)
            raw_[2] |= (1 << day);
        else
            raw_[2] &= ~(1 << day);
    }

    bool check(TinyDateTime const & date) {
        if (date.second() != 0)
            return false;
        if (date.minute() != raw_[1])
            return false;
        if (date.hour() != raw_[0])
            return false;
        return enabledDay(date.dayOfWeek());
    }

private:
    uint8_t raw_[3] = { 0, 0, 0};

}); 

static_assert(sizeof(TinyAlarm) == 3);

/** Helper function for tiny date serialization to writer-like classes (writer stream, stdout, etc).
 */
template<typename WRITER>
inline WRITER & operator << (WRITER & writer, TinyDateTime const & date) {
    return writer << date.year() << '-' 
                  << date.month() << '-' 
                  << date.day() << ' '
                  << date.hour() << ':' 
                  << date.minute() << ':' 
                  << date.second();
} // operator <<
