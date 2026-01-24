#include <platform/tests.h>

#include <rckid/string.h>
#include <rckid/serialization.h>

TEST(serialiation, textSerializer) {
    using namespace rckid;
    Heap::UseAndReserveGuard g_;
    MemoryStream ms = MemoryStream::withCapacity(1024);
    TextSerializer serializer{ms};
    serializer << "Hello " << 125; 
    ms.seek(0);
    EXPECT(ms.readByte() == 'H');
    EXPECT(ms.readByte() == 'e');
    EXPECT(ms.readByte() == 'l');
    EXPECT(ms.readByte() == 'l');
    EXPECT(ms.readByte() == 'o');
    EXPECT(ms.readByte() == ' ');
    EXPECT(ms.readByte() == '1');
    EXPECT(ms.readByte() == '2');
    EXPECT(ms.readByte() == '5');
}
