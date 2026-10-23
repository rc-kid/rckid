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

TEST(tinydate, ToModifiedJulianDay) {
    EXPECT(TinyDate(17, 11, 1858).mjd() == 0);
    EXPECT(TinyDate(1, 1, 1970).mjd() == 40587);
    EXPECT(TinyDate(1, 1, 2000).mjd() == 51544);
    EXPECT(TinyDate(30, 1, 2000).mjd() == 51573);
    // leap day and the days around it
    EXPECT(TinyDate(28, 2, 2024).mjd() == 60368);
    EXPECT(TinyDate(29, 2, 2024).mjd() == 60369);
    EXPECT(TinyDate(1, 3, 2024).mjd() == 60370);
    // 1900 is not a leap year, 2000 is
    EXPECT(TinyDate(1, 3, 1900).mjd() - TinyDate(28, 2, 1900).mjd() == 1);
    EXPECT(TinyDate(1, 3, 2000).mjd() - TinyDate(28, 2, 2000).mjd() == 2);
    EXPECT(TinyDate(31, 12, 2024).mjd() == 60675);
    EXPECT(TinyDate(1, 1, 2025).mjd() == 60676);
}

TEST(tinydate, FromModifiedJulianDay) {
    TinyDate d{0};
    EXPECT(d == TinyDate(17, 11, 1858));
    d = TinyDate{51544};
    EXPECT(d == TinyDate(1, 1, 2000));
    d = TinyDate{60369};
    EXPECT(d == TinyDate(29, 2, 2024));
    d = TinyDate{60370};
    EXPECT(d == TinyDate(1, 3, 2024));
    d = TinyDate{60676};
    EXPECT(d == TinyDate(1, 1, 2025));
}

TEST(tinydate, ModifiedJulianDayRoundtrip) {
    // every day from 17/11/1858 to the end of the 12bit year range, checking mjd -> date -> mjd and that consecutive days increment the date
    uint32_t last = TinyDate(31, 12, 2200).mjd();
    TinyDate prev{0};
    for (uint32_t mjd = 0; mjd <= last; mjd += 1000) {
        TinyDate d{mjd};
        CHECK(d.mjd() == mjd);
        d.inc();
        CHECK(d.mjd() == mjd + 1);
    }
}
