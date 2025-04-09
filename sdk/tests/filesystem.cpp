#include <platform/tests.h>
#include <rckid/filesystem.h>

using namespace rckid;

TEST(filesystem, stem) {
    EXPECT(filesystem::stem("foo.bar") == "foo");
    EXPECT(filesystem::stem("/foo.bar") == "foo");
    EXPECT(filesystem::stem("foo/foo.bar") == "foo");
    EXPECT(filesystem::stem("foo") == "foo");
    EXPECT(filesystem::stem("/foo") == "foo");
    EXPECT(filesystem::stem("foo/foo") == "foo");
}

TEST(filesystem, extension) {
    EXPECT(filesystem::ext("foo.bar") == ".bar");
    EXPECT(filesystem::ext("foo.bar.baz") == ".baz");
    EXPECT(filesystem::ext("foobar") == "");
    EXPECT(filesystem::ext(".bar") == "");
}

TEST(filesystem, parent) {
    EXPECT(filesystem::parent("foo.bar") == "/");
    EXPECT(filesystem::parent("foo.bar.baz") == "/");
    EXPECT(filesystem::parent("/foobar") == "/");
    EXPECT(filesystem::parent("/foo/bar/baz.bar") == "/foo/bar");
}

