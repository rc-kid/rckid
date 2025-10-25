#include <platform/tests.h>
#include <rckid/rckid.h>
#include <rckid/utils/string.h>

TEST(string, fromLiteral) {
    using namespace rckid;
    char const * x = "Hello";
    String s{x};
#if defined(RCKID_BACKEND_FANTASY)
    EXPECT(s.size() == 5);
    EXPECT(s.capacity() == 5);
    EXPECT(s.immutable() == false);
#else
    EXPECT(s.size() == 5);
    EXPECT(s.capacity() == 0);
    EXPECT(s.immutable() == true);
    EXPECT(s.c_str() == x);
#endif
    EXPECT(s[0] == 'H');
    EXPECT(s[1] == 'e');
    EXPECT(s[2] == 'l');
    EXPECT(s[3] == 'l');
    EXPECT(s[4] == 'o');
    EXPECT(s[5] == '\0');

    // growing the string drops the immutability
    String s2 = String::withCapacity(x, 10);
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
    String s = String::withCapacity("Hello", 20);
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

TEST(string, insertErase) {
    using namespace rckid;
    String s = "hello";
    s.insert(0, 'H');
    EXPECT(s == "Hhello");
    s.erase(1,1);
    EXPECT(s == "Hello");
}

TEST(string, insertInEmpty) {
    using namespace rckid;
    String s = "";
    s.insert(0, 'H');
    EXPECT(s == "H");
    s.insert(1, 'e');
    EXPECT(s == "He");
    s.insert(2, 'l');
    EXPECT(s == "Hel");
    s.insert(3, 'l');
    EXPECT(s == "Hell");
    s.insert(4, 'o');
    EXPECT(s == "Hello");
}

TEST(string, insertInEmptyFromStart) {
    using namespace rckid;
    String s = "";
    s.insert(0, 'o');
    EXPECT(s == "o");
    s.insert(0, 'l');
    EXPECT(s == "lo");
    s.insert(0, 'l');
    EXPECT(s == "llo");
    s.insert(0, 'e');
    EXPECT(s == "ello");
    s.insert(0, 'H');
    EXPECT(s == "Hello");
    s.erase(2, 3);
    EXPECT(s == "He");
    s = "Hello";
    EXPECT(s == "Hello");
    s.erase(1,3);
    EXPECT(s == "Ho");
}

TEST(string, assign) {
    using namespace rckid;
    String s1;
    String s2;
    s1 = "Hello";
    s2 = "World";
    s2 = s1;
    s1 = s2;
}

TEST(string, charAssign) {
    using namespace rckid;
    String s{' ', 8};
    s[0] = 'H';
    s[1] = 'e';
    EXPECT(s[0] == 'H');
    EXPECT(s[1] == 'e');
}