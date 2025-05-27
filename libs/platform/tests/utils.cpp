#include "../tests.h"
#include "../utils.h"

TEST(platform, utils_64_shifts) {
    uint64_t x = 0xffff_u64 << 20;
    EXPECT(x == 0xffff00000_u64);
    x = 0xff00_u64 << 20;
    EXPECT(x == 0xff0000000_u64);
    x = 0xff_u64 << 28;
    EXPECT(x == 0xff0000000_u64);
}

TEST(platform, utils_64_reverse) {
    EXPECT(platform::reverseByte(0x80) == 0x01);
    EXPECT(platform::reverseByte(0x40) == 0x02);
    EXPECT(platform::reverseByte(0x20) == 0x04);
    EXPECT(platform::reverseByte(0x10) == 0x08);

    EXPECT(platform::reverse2Bytes(0x1234) == 0x2c48);
}