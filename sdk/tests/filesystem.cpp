#include <platform/tests.h>
#include <rckid/filesystem.h>

using namespace rckid;

TEST(filesystem, stem) {
    EXPECT(fs::stem("foo.bar") == "foo");
    EXPECT(fs::stem("/foo.bar") == "foo");
    EXPECT(fs::stem("foo/foo.bar") == "foo");
    EXPECT(fs::stem("foo") == "foo");
    EXPECT(fs::stem("/foo") == "foo");
    EXPECT(fs::stem("foo/foo") == "foo");
}

TEST(filesystem, extension) {
    EXPECT(fs::ext("foo.bar") == ".bar");
    EXPECT(fs::ext("foo.bar.baz") == ".baz");
    EXPECT(fs::ext("foobar") == "");
    EXPECT(fs::ext(".bar") == "");
}

TEST(filesystem, parent) {
    EXPECT(fs::parent("foo.bar") == "/");
    EXPECT(fs::parent("foo.bar.baz") == "/");
    EXPECT(fs::parent("/foobar") == "/");
    EXPECT(fs::parent("/foo/bar/baz.bar") == "/foo/bar");
}

TEST(filesystem, root) {
    EXPECT(fs::root("foo/bar/baz") == "foo");
    EXPECT(fs::root("/foo/bar/baz") == "/foo");
    EXPECT(fs::root("foo") == "foo");
    EXPECT(fs::root("/foo") == "/foo");
    EXPECT(fs::root("") == "");
}

