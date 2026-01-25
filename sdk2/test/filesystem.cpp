#include <platform/tests.h>

#include <rckid/filesystem.h>

TEST(filesystem, pathJoin) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    String p1 = fs::join("folder", "file.txt");
    EXPECT(p1 == "folder/file.txt");
    String p2 = fs::join("folder/", "file.txt");
    EXPECT(p2 == "folder/file.txt");
}

TEST(filesystem, pathStem) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    EXPECT(fs::stem("foo.bar") == "foo");
    EXPECT(fs::stem("/foo.bar") == "foo");
    EXPECT(fs::stem("foo/foo.bar") == "foo");
    EXPECT(fs::stem("foo") == "foo");
    EXPECT(fs::stem("/foo") == "foo");
    EXPECT(fs::stem("foo/foo") == "foo");
}

TEST(filesystem, pathExt) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    EXPECT(fs::ext("foo.bar") == ".bar");
    EXPECT(fs::ext("foo.bar.baz") == ".baz");
    EXPECT(fs::ext("foobar") == "");
    EXPECT(fs::ext(".bar") == "");
}

TEST(filesystem, pathParent) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    EXPECT(fs::parent("foo.bar") == "/");
    EXPECT(fs::parent("foo.bar.baz") == "/");
    EXPECT(fs::parent("/foobar") == "/");
    EXPECT(fs::parent("/foo/bar/baz.bar") == "/foo/bar");
}

TEST(filesystem, pathRoot) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    EXPECT(fs::root("foo/bar/baz") == "foo");
    EXPECT(fs::root("/foo/bar/baz") == "/foo");
    EXPECT(fs::root("foo") == "foo");
    EXPECT(fs::root("/foo") == "/foo");
    EXPECT(fs::root("") == "");
}
