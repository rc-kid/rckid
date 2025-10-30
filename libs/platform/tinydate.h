#pragma once

#include <stdint.h>
#include "definitions.h"
#include "string_utils.h"


/** Day, month and year stored in 3 bytes. 
 */
PACKED(class TinyDate {
public:

    enum class DayOfWeek : uint8_t {
        Sunday = 0,
        Monday,
        Tuesday,
        Wednesday,
        Thursday,
        Friday,
        Saturday,
    };

    enum class Month : uint8_t {
        January = 1,
        February,
        March,
        April,
        May,
        June,
        July,
        August,
        September,
        October,
        November,
        December
    };

    TinyDate() {
        set(1, 1, 0);
    }

    TinyDate(uint8_t d, uint8_t m, uint16_t y) {
        set(d, m, y);
    }

    TinyDate & operator = (TinyDate const & other)  = default;

    bool operator == (TinyDate const & other) const {
        return (day_ == other.day_) && (month_ == other.month_) && (year_ == other.year_);
    };

    bool operator != (TinyDate const & other) const {
        return (day_ != other.day_) || (month_ != other.month_) || (year_ != other.year_);
    };

    uint8_t day() const { return day_; }
    uint8_t month() const { return month_ & 0x0f; }
    Month monthName() const { return static_cast<Month>(month()); }
    uint16_t year() const { return ((month_ & 0xf0) << 4) | year_; }

    void set(uint8_t day, uint8_t month, uint16_t year) {
        if (month == 0)
            month = 1;
        if (month > 12)
            month = 12;
        day_ = day;
        month_ = (month & 0x0f) | ((year >> 4) & 0xf0);
        year_ = year & 0xff;
        if (day_ > daysInMonth())
            day_ = daysInMonth();
    }

    bool setFromString(char const * str) {
        // expected format: DD / MM / YYYY
        int day = parseInt(str);
        if (*str != '/' || day < 1 || day > 31)
            return false;
        ++str;
        int month = parseInt(str);
        if (*str != '/' || month < 1 || month > 12)
            return false;
        ++str;
        int year = parseInt(str);
        if (year < 0 || year > 4095)
            return false;
        set(static_cast<uint8_t>(day), static_cast<uint8_t>(month), static_cast<uint16_t>(year));
        return true;
    }

    void setDay(uint8_t day) {
        day_ = day;
        if (day_ == 0)
            day_ = 1;
        if (day_ > daysInMonth())
            day_ = daysInMonth();
    }

    void setMonth(uint8_t month) {
        if (month == 0)
            month = 1;
        if (month > 12)
            month = 12;
        month_ = (month & 0x0f) | (month_ & 0xf0);
        if (day_ > daysInMonth())
            day_ = daysInMonth();
    }

    void setYear(uint16_t year) {
        month_ = (month_ & 0x0f) | ((year >> 4) & 0xf0);
        year_ = year & 0xff;
        if (day_ > daysInMonth())
            day_ = daysInMonth();
    }
    
    void setMonth(Month month) { setMonth(static_cast<uint8_t>(month)); }

    DayOfWeek dayOfWeek() const { return dayOfWeek(day_, month_, year()); }

    uint8_t daysInMonth() const { return daysInMonth(month(), year()); }

    /** Increments the date by one day.
     */
    void inc() {
        if (++day_ > daysInMonth()) {
            day_ = 1;
            uint8_t m = month() + 1;
            if (m > 12) {
                m = 1;
                uint16_t y = year() + 1;
                set(day_, m, y);
            } else {
                set(day_, m, year());
            }
        }
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
            uint32_t dim = daysInMonth(m1, y);
            result += dim - d1 + 1;
            d1 = 1;
            m1++;
            if (m1 > 12) {
                m1 = 1;
                y++;
            }
        }
        return result;
    }

    /** Returns the day of week for given date. 
     
        Only works for dates in range. Returns 0 for Monday and 6 for Sunday. 
    */
    // https://cs.uwaterloo.ca/~alopez-o/math-faq/node73.html
    static DayOfWeek dayOfWeek(uint8_t d, uint8_t m, uint16_t y) {
        y -= m<3; 
        return static_cast<DayOfWeek>((y + y / 4 - y / 100 +y / 400 + "-bed=pen+mad."[m] + d) % 7);
    }

    static bool isLeapYear(uint16_t year) {
        if (year % 4 != 0)
            return false;
        return (year % 100 != 0) || (year % 400 == 0);
    }

    static uint8_t daysInMonth(uint8_t month, uint16_t year) {
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

private:
    uint8_t day_;
    uint8_t month_;
    uint8_t year_;
}); // TinyDate

PACKED(class TinyTime {
public:

    TinyTime() : hour_{0}, minute_{0}, second_{0} {}

    TinyTime(uint8_t h, uint8_t m, uint8_t s = 0) {
        set(h, m, s);
    }

    uint8_t hour() const { return hour_; }
    uint8_t minute() const { return minute_; }
    uint8_t second() const { return second_; }

    void set(uint8_t h, uint8_t m, uint8_t s = 0) {
        hour_ = h % 24;
        minute_ = m % 60;
        second_ = s % 60;
    }

    /** Increments the time by one seconds. Returns true if the increment overflows the time, i.e. a date should change as well. 
     */
    bool inc() {
        if (++second_ == 60) {
            second_ = 0;
            if (++minute_ == 60) {
                minute_ = 0;
                if (++hour_ == 24) {
                    hour_ = 0;
                    return true;
                }
            }
        }
        return false;
    }
    
private:
    uint8_t hour_;
    uint8_t minute_;
    uint8_t second_;
}); // TinyTime

PACKED(class TinyDateTime {
public:
    TinyDate date;
    TinyTime time;

    void inc() {
        if (time.inc())
            date.inc();
    }
});

/** Simple alarm. 
 
    Hours, minutes and days of the week when the alarm is active. Since we need at least 18 bits to store this information, the alarm uses 3 bytes internally for hours, minutes and days of the week respectively. 
 */
PACKED(class TinyAlarm {
public:

    uint8_t hour() const { return raw_[0]; }
    uint8_t minute() const { return raw_[1]; }
    bool enabled() const { return raw_[2] != 0; }
    bool enabledDay(TinyDate::DayOfWeek dayOfWeek) const { return raw_[2] & (1 << static_cast<uint8_t>(dayOfWeek)); }

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

    void setEnabledDay(TinyDate::DayOfWeek day, bool value = true) {
        if (value)
            raw_[2] |= (1 << static_cast<uint8_t>(day));
        else
            raw_[2] &= ~(1 << static_cast<uint8_t>(day));
    }

    bool check(TinyDateTime const & date) {
        if (date.time.second() != 0)
            return false;
        if (date.time.minute() != raw_[1])
            return false;
        if (date.time.hour() != raw_[0])
            return false;
        return enabledDay(date.date.dayOfWeek());
    }

private:
    uint8_t raw_[3] = { 0, 0, 0};

}); 

/** Helper function for tiny date serialization to writer-like classes (writer stream, stdout, etc).
 */
template<typename WRITER>
inline WRITER & operator << (WRITER & writer, TinyDateTime const & date) {
    return writer << date.date.year() << '-' 
                  << date.date.month() << '-' 
                  << date.date.day() << ' '
                  << date.time.hour() << ':' 
                  << date.time.minute() << ':' 
                  << date.time.second();
} // operator <<

template<typename WRITER>
inline WRITER & operator << (WRITER & writer, TinyDate::DayOfWeek d) {
    switch (d) {
        case TinyDate::DayOfWeek::Monday:
            return writer << "Monday";
        case TinyDate::DayOfWeek::Tuesday:
            return writer << "Tuesday";
        case TinyDate::DayOfWeek::Wednesday:
            return writer << "Wednesday";
        case TinyDate::DayOfWeek::Thursday:
            return writer << "Thursday";
        case TinyDate::DayOfWeek::Friday:
            return writer << "Friday";
        case TinyDate::DayOfWeek::Saturday:
            return writer << "Saturday";
        case TinyDate::DayOfWeek::Sunday:
            return writer << "Sunday";
        default:
            return writer << "Unknown";
    }
} // operator << (DayOfWeek)

template<typename WRITER>
inline WRITER & operator << (WRITER & writer, TinyDate::Month m) {
    switch (m) {
        case TinyDate::Month::January:
            return writer << "January";
        case TinyDate::Month::February:
            return writer << "February";
        case TinyDate::Month::March:
            return writer << "March";
        case TinyDate::Month::April:
            return writer << "April";
        case TinyDate::Month::May:
            return writer << "May";
        case TinyDate::Month::June:
            return writer << "June";
        case TinyDate::Month::July:
            return writer << "July";
        case TinyDate::Month::August:
            return writer << "August";
        case TinyDate::Month::September:
            return writer << "September";
        case TinyDate::Month::October:
            return writer << "October";
        case TinyDate::Month::November:
            return writer << "November";
        case TinyDate::Month::December:
            return writer << "December";
        default:
            return writer << "Unknown";
    }
} // operator << (Month)
