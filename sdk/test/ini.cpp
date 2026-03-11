#include <platform/tests.h>

#include <rckid/ini.h>

TEST(ini, basicWriter) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    MemoryStream ms = MemoryStream::withCapacity(1024);
    {
        ini::Writer writer{ms};
        writer 
            << ini::Section("general")
                << ini::Field("version", 42)
                << ini::Field("name", "test")
            << ini::Section("settings")
                << ini::Field("enabled", true)
                << ini::Field("threshold", 314);
    }
    ms.writeByte(0);
    ms.seek(0);
    String result{ms.readString()};
    String expected =
        "[general]\n"
        "version=42\n"
        "name=test\n\n"
        "[settings]\n"
        "enabled=T\n"
        "threshold=314\n";
    EXPECT(result == expected);
}

TEST(ini, basicReader) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    MemoryStream ms = MemoryStream::withCapacity(1024);
    String iniData =
        "[general]\n"
        "version=42\n"
        "name=test\n\n"
        "[settings]\n"
        "enabled=T\n"
        "threshold=314\n";
    ini::Reader reader{iniData.reader()};

    uint32_t version = 0;
    String name;
    bool enabled = false;
    uint32_t threshold = 0;

    reader 
        >> ini::Section("general")
            >> ini::Field("version", version)
            >> ini::Field("name", name)
        >> ini::Section("settings")
            >> ini::Field("enabled", enabled)
            >> ini::Field("threshold", threshold);

    EXPECT(version == 42);
    EXPECT(name == "test");
    EXPECT(enabled == true);
    EXPECT(threshold == 314);
}