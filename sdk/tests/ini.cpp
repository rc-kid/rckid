#include <platform/tests.h>
#include <rckid/utils/ini.h>

using namespace rckid;

TEST(ini, sections) {
    MemoryReadStream s{""
        "[section1]\n"
        "key1=value1\n"
        "key2=value2\n"
        "\n"
        "[section2]\n"
        "keyA=valueA\n"
        "keyB=valueB\n"
    };
    ini::Reader p{std::move(s)};
    EXPECT(p.nextSection().value(), "section1");
    auto ov = p.nextValue();
    EXPECT(ov.has_value());
    auto v = ov.value();
    EXPECT(v.first, "key1");
    EXPECT(v.second, "value1");
    v = p.nextValue().value();
    EXPECT(v.first, "key2");
    EXPECT(v.second, "value2");
    v = p.nextValue().value();
    EXPECT(v.first, "");
    EXPECT(v.second, "");
    EXPECT(p.nextSection().value(), "section2");
    v = p.nextValue().value();
    EXPECT(v.first, "keyA");
    EXPECT(v.second, "valueA");
    v = p.nextValue().value();
    EXPECT(v.first, "keyB");
    EXPECT(v.second, "valueB");
    ov = p.nextValue();
    EXPECT(!ov.has_value());
    EXPECT(p.eof());
    EXPECT(p.nextSection().has_value() == false);
    EXPECT(p.eof());
}