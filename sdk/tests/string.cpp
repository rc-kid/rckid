#include <platform/tests.h>
#include <rckid/rckid.h>
#include <rckid/utils/string.h>

TEST(string, fromLiteral) {
    using namespace rckid;
    char const * x = "Hello";
    String s{x};
    EXPECT(s.size() == 5);
    EXPECT(s.capacity() == 0);
    EXPECT(s.immutable() == true);
    EXPECT(s.c_str() == x);
    EXPECT(s[0] == 'H');
    EXPECT(s[1] == 'e');
    EXPECT(s[2] == 'l');
    EXPECT(s[3] == 'l');
    EXPECT(s[4] == 'o');
    EXPECT(s[5] == '\0');

    // growing the string drops the immutability
    String s2{x, 10};
    EXPECT(s2.size() == 5);
    EXPECT(s2.capacity() == 10);
    EXPECT(s2.immutable() == false);
    EXPECT(s2.c_str() != x);
    EXPECT(s2[0] == 'H');
    EXPECT(s2[1] == 'e');
    EXPECT(s2[2] == 'l');
    EXPECT(s2[3] == 'l');
    EXPECT(s2[4] == 'o');
    EXPECT(s2[5] == '\0');

}

TEST(string, shrink) {
    using namespace rckid;
    String s{"Hello", 20};
    EXPECT(s.size() == 5);
    EXPECT(s.capacity() == 20);
    EXPECT(s.immutable() == false);
    s.shrink();
    EXPECT(s.size() == 5);
    EXPECT(s.capacity() == 5);
    EXPECT(s.immutable() == false);
    EXPECT(s[0] == 'H');
    EXPECT(s[1] == 'e');
    EXPECT(s[2] == 'l');
    EXPECT(s[3] == 'l');
    EXPECT(s[4] == 'o');
    EXPECT(s[5] == '\0');

}