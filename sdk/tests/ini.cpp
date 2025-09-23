#include <platform/tests.h>
#include <rckid/utils/ini.h>

using namespace rckid;

TEST(ini, sections) {
    auto s = std::make_unique<MemoryReadStream>(""
        "[section1]\n"
        "key1=value1\n"
        "key2=value2\n"
        "\n"
        "[section2]\n"
        "keyA=valueA\n"
        "keyB=valueB\n"
    );
    ini::Reader p{std::move(s)};
    EXPECT(p.nextSection(), "section1");
    auto v = p.nextValue();
    EXPECT(v.first, "key1");
    EXPECT(v.second, "value1");
    v = p.nextValue();
    EXPECT(v.first, "key2");
    EXPECT(v.second, "value2");
    v = p.nextValue();
    EXPECT(v.first, "");
    EXPECT(v.second, "");
    EXPECT(p.nextSection(), "section2");
    v = p.nextValue();
    EXPECT(v.first, "keyA");
    EXPECT(v.second, "valueA");
    v = p.nextValue();
    EXPECT(v.first, "keyB");
    EXPECT(v.second, "valueB");
    v = p.nextValue();
    EXPECT(v.first, "");
    EXPECT(v.second, "");
    EXPECT(p.eof());
    EXPECT(p.nextSection(), "");
    EXPECT(p.eof());
}