#include "../tests.h"
#include "../tinydate.h"

TEST(tinydate, TinyDate) {
    TinyDate d;
    EXPECT(d.day() == 1);
    EXPECT(d.month() == 1);
    EXPECT(d.year() == 0);

    d.setDay(31);
    EXPECT(d.day() == 31);
    d.setDay(0);
    EXPECT(d.day() == 1);

    d.setMonth(12);
    EXPECT(d.month() == 12);
    d.setMonth(0);
    EXPECT(d.month() == 1);
    d.setMonth(13);
    EXPECT(d.month() == 12);

    d.setYear(1982);
    EXPECT(d.year() == 1982);

    d.setYear(10000);
    EXPECT(d.year() == 10000);

    d.setYear(64);
    EXPECT(d.year() == 64);
    
    d.setYear(-3000);
    EXPECT(d.year() == -3000);

    uint32_t r = d.asRaw();
    TinyDate d2 = TinyDate::fromRaw(r);
    EXPECT(d2.day() == d.day());
    EXPECT(d2.month() == d.month());
    EXPECT(d2.year() == d.year());
    EXPECT(d2 == d);
}