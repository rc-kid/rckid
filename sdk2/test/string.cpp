#include <platform/tests.h>

#include <rckid/string.h>

namespace {

    char const * foo = "foo";
    char const * bar = "bar";

} // anonymous namespace

TEST(string, fromLiteral) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    String s{foo};
    EXPECT(s.size() == 3);
    EXPECT(s.c_str() == foo);
    EXPECT(g_.usedDelta() == 0);
    EXPECT(g_.reservedDelta() == 0);
}

TEST(string, literalClone) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    String s1{foo};
    String s2 = s1;
    EXPECT(s2.size() == 3);
    EXPECT(s2.size() == 3);
    EXPECT(s1.c_str() == foo);
    EXPECT(s2.c_str() == foo);
    EXPECT(g_.usedDelta() == 0);
    EXPECT(g_.reservedDelta() == 0);
}

TEST(string, compare) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    String s1{foo};
    String s2 = s1;
    EXPECT(s1 == foo);
    EXPECT(s1 == s2);
    EXPECT(s1 <= s2);
    EXPECT(s2 >= s1);
    s2 = bar;
    EXPECT(g_.usedDelta() == 0);
    EXPECT(g_.reservedDelta() == 0);
    EXPECT(s2 == bar);
    EXPECT(s1 != s2);
    EXPECT(s1 > s2);
    EXPECT(s2 < s1);
    EXPECT(s1 >= s2);
    EXPECT(s2 <= s1);
}

TEST(string, addition) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    String s1{foo};
    String s2 = s1 + " " + bar;
    EXPECT(s2.size() == 7);
    EXPECT(std::strcmp(s2.c_str(), "foo bar") == 0);
    EXPECT(g_.usedDelta() == 16);
    EXPECT(g_.reservedDelta() == 32); // because of the intermediate allocation in the addition
}

TEST(string, empty) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    String s1;
    EXPECT(s1.size() == 0);
    EXPECT(s1.empty());
    s1 = foo;
    EXPECT(!s1.empty());
    s1 = "";
    EXPECT(s1.size() == 0);
    EXPECT(s1.empty());
}

TEST(string, startsWith) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    String s1{foo};
    EXPECT(s1.startsWith("foo"));
    EXPECT(!s1.startsWith("foobar"));
    EXPECT(!s1.startsWith("bar"));
    EXPECT(s1.startsWith(String{}));
    EXPECT(s1.startsWith(""));
}

TEST(string, reader) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    String s1{foo};
    auto r = s1.reader(1);
    EXPECT(!r.eof());
    EXPECT(r.getChar() == 'o');
    EXPECT(!r.eof());
    EXPECT(r.getChar() == 'o');
    EXPECT(r.eof());
}

TEST(string, builder) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    String s = STR("foo " << 42);
    EXPECT(s == "foo 42");
}

