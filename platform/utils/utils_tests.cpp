#include "utils.h"
#include "tests.h"
#include "json.h"
#include "locks.h"
#include "process.h"

#ifdef TESTS

TEST(utils, trimLeft) {
    std::string s{"foo"};
    str::trimLeft(s);
    EXPECT_EQ(s, "foo");    
    s = "  foo";
    str::trimLeft(s);
    EXPECT_EQ(s, "foo");    
    s = "  \n\r\tfoo";
    str::trimLeft(s);
    EXPECT_EQ(s, "foo");    
    s = "  \n\r\tf o o  ";
    str::trimLeft(s);
    EXPECT_EQ(s, "f o o  ");    
}

TEST(utils, trimRight) {
    std::string s{"foo"};
    str::trimRight(s);
    EXPECT_EQ(s, "foo");    
    s = "foo ";
    str::trimRight(s);
    EXPECT_EQ(s, "foo");    
    s = "foo \n\r ";
    str::trimRight(s);
    EXPECT_EQ(s, "foo");    
    s = "  f o o \n\r ";
    str::trimRight(s);
    EXPECT_EQ(s, "  f o o");    
}

TEST(utils, trim) {
    std::string s{"foo"};
    str::trim(s);
    EXPECT_EQ(s, "foo");    
    s = "\n\r\t f o o \n\r\t";
    str::trim(s);
    EXPECT_EQ(s, "f o o");    
}

TEST(utils, escape) {
    EXPECT_EQ(str::escape("foo"), "foo");
    EXPECT_EQ(str::escape("foo\n"), "foo\\n");
    EXPECT_EQ(str::escape("foo\r"), "foo\\r");
    EXPECT_EQ(str::escape("foo\t"), "foo\\t");
    EXPECT_EQ(str::escape("foo\\"), "foo\\\\");
}

#endif



RUN_TESTS
