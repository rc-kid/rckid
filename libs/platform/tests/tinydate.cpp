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

    d.setYear(64);
    EXPECT(d.year() == 64);
    
    d.setYear(3000);
    EXPECT(d.year() == 3000);

    TinyDate d2 = d;
    EXPECT(d2.day() == d.day());
    EXPECT(d2.month() == d.month());
    EXPECT(d2.year() == d.year());
    EXPECT(d2 == d);
}

TEST(tinydate, daysTillAnnual) {
    TinyDate d1{1, 1, 2024};
    TinyDate d2{1, 2, 2024};
    EXPECT(d1.daysTillNextAnnual(d2) == 31);
    d2 = TinyDate{1, 3, 2024};
    EXPECT(d1.daysTillNextAnnual(d2) == 60);
    d2 = TinyDate{10, 3, 2024};
    EXPECT(d1.daysTillNextAnnual(d2) == 69);
    d1 = TinyDate{6, 7, 2024};
    EXPECT(d1.daysTillNextAnnual(d2) == 247);
}

TEST(tinydate, ModifiedJulianDay) {
    TinyDate d1{30, 1, 2000};
    TinyDate d2{51573};
    EXPECT(d1.day() == d2.day());
    EXPECT(d1.month() == d2.month());
    EXPECT(d1.year() == d2.year());
}